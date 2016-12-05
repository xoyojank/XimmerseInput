// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "XimmerseInputPrivatePCH.h"
#include "XimmerseInput.h"
#include <ControllerState.h>

DEFINE_LOG_CATEGORY_STATIC(LogXimmerseInput, Log, All);

#define DOT_45DEG		0.7071f

//
// Gamepad thresholds
//
#define TOUCHPAD_DEADZONE  0.0f

#define LOCTEXT_NAMESPACE "XimmerseInput"

// Controls whether or not we need to swap the input routing for the hands, for debugging
static TAutoConsoleVariable<int32> CVarSwapHands(
    TEXT("vr.SwapMotionControllerInput"),
    0,
    TEXT("This command allows you to swap the button / axis input handedness for the input controller, for debugging purposes.\n")
    TEXT(" 0: don't swap (default)\n")
    TEXT(" 1: swap left and right buttons"),
    ECVF_Cheat);

namespace XimmerseControllerKeyNames
{
const FGamepadKeyNames::Type Touch0("Ximmerse_Touch_0");
const FGamepadKeyNames::Type Touch1("Ximmerse_Touch_1");
}


FXimmerseInput::FXimmerseInput(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler)
	: MessageHandler(InMessageHandler)
{
#if XIMMERSE_INPUT_SUPPORTED_PLATFORMS
	FMemory::Memzero(ControllerStates, sizeof(ControllerStates));

	for (int32 i = 0; i < MaxControllers; ++i)
	{
		DeviceToControllerMap[i] = INDEX_NONE;
	}

	for (int32 i = 0; i < MaxControllers; ++i)
	{
		ControllerToDeviceMap[i] = INDEX_NONE;
	}

	NumControllersMapped = 0;

	InitialButtonRepeatDelay = 0.2f;
	ButtonRepeatDelay = 0.1f;

	EKeys::AddKey(FKeyDetails(FKey(XimmerseControllerKeyNames::Touch0), LOCTEXT("Ximmerse_Touch_0", "MotionController (L) Touchpad"), FKeyDetails::GamepadKey | FKeyDetails::FloatAxis));
	EKeys::AddKey(FKeyDetails(FKey(XimmerseControllerKeyNames::Touch1), LOCTEXT("Ximmerse_Touch_1", "MotionController (R) Touchpad"), FKeyDetails::GamepadKey | FKeyDetails::FloatAxis));

	Buttons[(int32)EControllerHand::Left][EXimmerseInputButton::System] = FGamepadKeyNames::SpecialLeft;
	Buttons[(int32)EControllerHand::Left][EXimmerseInputButton::ApplicationMenu] = FGamepadKeyNames::MotionController_Left_Shoulder;
	Buttons[(int32)EControllerHand::Left][EXimmerseInputButton::TouchPadPress] = FGamepadKeyNames::MotionController_Left_Thumbstick;
	Buttons[(int32)EControllerHand::Left][EXimmerseInputButton::TouchPadTouch] = XimmerseControllerKeyNames::Touch0;
	Buttons[(int32)EControllerHand::Left][EXimmerseInputButton::TriggerPress] = FGamepadKeyNames::MotionController_Left_Trigger;
	Buttons[(int32)EControllerHand::Left][EXimmerseInputButton::Grip] = FGamepadKeyNames::MotionController_Left_Grip1;
	Buttons[(int32)EControllerHand::Left][EXimmerseInputButton::TouchPadUp] = FGamepadKeyNames::MotionController_Left_FaceButton1;
	Buttons[(int32)EControllerHand::Left][EXimmerseInputButton::TouchPadDown] = FGamepadKeyNames::MotionController_Left_FaceButton3;
	Buttons[(int32)EControllerHand::Left][EXimmerseInputButton::TouchPadLeft] = FGamepadKeyNames::MotionController_Left_FaceButton4;
	Buttons[(int32)EControllerHand::Left][EXimmerseInputButton::TouchPadRight] = FGamepadKeyNames::MotionController_Left_FaceButton2;

	Buttons[(int32)EControllerHand::Right][EXimmerseInputButton::System] = FGamepadKeyNames::SpecialRight;
	Buttons[(int32)EControllerHand::Right][EXimmerseInputButton::ApplicationMenu] = FGamepadKeyNames::MotionController_Right_Shoulder;
	Buttons[(int32)EControllerHand::Right][EXimmerseInputButton::TouchPadPress] = FGamepadKeyNames::MotionController_Right_Thumbstick;
	Buttons[(int32)EControllerHand::Right][EXimmerseInputButton::TouchPadTouch] = XimmerseControllerKeyNames::Touch1;
	Buttons[(int32)EControllerHand::Right][EXimmerseInputButton::TriggerPress] = FGamepadKeyNames::MotionController_Right_Trigger;
	Buttons[(int32)EControllerHand::Right][EXimmerseInputButton::Grip] = FGamepadKeyNames::MotionController_Right_Grip1;
	Buttons[(int32)EControllerHand::Right][EXimmerseInputButton::TouchPadUp] = FGamepadKeyNames::MotionController_Right_FaceButton1;
	Buttons[(int32)EControllerHand::Right][EXimmerseInputButton::TouchPadDown] = FGamepadKeyNames::MotionController_Right_FaceButton3;
	Buttons[(int32)EControllerHand::Right][EXimmerseInputButton::TouchPadLeft] = FGamepadKeyNames::MotionController_Right_FaceButton4;
	Buttons[(int32)EControllerHand::Right][EXimmerseInputButton::TouchPadRight] = FGamepadKeyNames::MotionController_Right_FaceButton2;

	IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
#endif // XIMMERSE_INPUT_SUPPORTED_PLATFORMS
}

FXimmerseInput::~FXimmerseInput()
{
#if XIMMERSE_INPUT_SUPPORTED_PLATFORMS
	IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
#endif // XIMMERSE_INPUT_SUPPORTED_PLATFORMS
}

void FXimmerseInput::SendControllerEvents()
{
#if XIMMERSE_INPUT_SUPPORTED_PLATFORMS
	ControllerState XControllerState;

	const double CurrentTime = FPlatformTime::Seconds();

	for (int32 DeviceIndex = 0; DeviceIndex < MaxControllers; ++DeviceIndex)
	{
		// update the mappings if this is a new device
		if (DeviceToControllerMap[DeviceIndex] == INDEX_NONE)
		{
			// don't map too many controllers
			if (NumControllersMapped >= MaxControllers)
			{
				continue;
			}

			DeviceToControllerMap[DeviceIndex] = FMath::FloorToInt(NumControllersMapped / CONTROLLERS_PER_PLAYER);
			ControllerToDeviceMap[NumControllersMapped] = DeviceIndex;
			ControllerStates[DeviceIndex].Hand = (EControllerHand)(NumControllersMapped % CONTROLLERS_PER_PLAYER);
			++NumControllersMapped;
		}

		// get the controller index for this device
		int32 ControllerIndex = DeviceToControllerMap[DeviceIndex];
		FControllerState& ControllerState = ControllerStates[DeviceIndex];
		EControllerHand HandToUse = ControllerState.Hand;

		// check to see if we need to swap input hands for debugging
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("vr.SwapMotionControllerInput"));
		bool bSwapHandInput = (CVar->GetValueOnGameThread() != 0) ? true : false;
		if (bSwapHandInput)
		{
			HandToUse = (HandToUse == EControllerHand::Left) ? EControllerHand::Right : EControllerHand::Left;
		}

		if (XDeviceGetInputState(DeviceHandle[DeviceIndex], &XControllerState) >= 0)
		{
			if (XControllerState.timestamp != ControllerState.Timestamp)
			{
				bool CurrentStates[EXimmerseInputButton::TotalButtonCount] = { 0 };

				// Get the current state of all buttons
				CurrentStates[EXimmerseInputButton::System] = !!(XControllerState.buttons & CONTROLLER_BUTTON_HOME);
				CurrentStates[EXimmerseInputButton::ApplicationMenu] = !!(XControllerState.buttons & CONTROLLER_BUTTON_APP);
				CurrentStates[EXimmerseInputButton::TouchPadPress] = !!(XControllerState.buttons & CONTROLLER_BUTTON_CLICK);
				CurrentStates[EXimmerseInputButton::TouchPadTouch] = !!(XControllerState.buttons & CONTROLLER_BUTTON_TOUCH);
				CurrentStates[EXimmerseInputButton::Grip] = !!(XControllerState.buttons & (CONTROLLER_BUTTON_LEFT_GRIP | CONTROLLER_BUTTON_RIGHT_GRIP));

				// If the touchpad isn't currently pressed or touched, zero put both of the axes
				if (!CurrentStates[EXimmerseInputButton::TouchPadTouch])
				{
					XControllerState.axes[CONTROLLER_AXIS_PRIMARY_THUMB_X] = 0.0f;
					XControllerState.axes[CONTROLLER_AXIS_PRIMARY_THUMB_Y] = 0.0f;
				}

				// D-pad emulation
				const FVector2D TouchDir = FVector2D(XControllerState.axes[CONTROLLER_AXIS_PRIMARY_THUMB_X], XControllerState.axes[CONTROLLER_AXIS_PRIMARY_THUMB_Y]).GetSafeNormal();
				const FVector2D UpDir(0.f, 1.f);
				const FVector2D RightDir(1.f, 0.f);

				const float VerticalDot = TouchDir | UpDir;
				const float RightDot = TouchDir | RightDir;

				const bool bPressed = !TouchDir.IsNearlyZero() && CurrentStates[EXimmerseInputButton::TouchPadPress];

				CurrentStates[EXimmerseInputButton::TouchPadUp] = bPressed && (VerticalDot >= DOT_45DEG);
				CurrentStates[EXimmerseInputButton::TouchPadDown] = bPressed && (VerticalDot <= -DOT_45DEG);
				CurrentStates[EXimmerseInputButton::TouchPadLeft] = bPressed && (RightDot <= -DOT_45DEG);
				CurrentStates[EXimmerseInputButton::TouchPadRight] = bPressed && (RightDot >= DOT_45DEG);

				if (ControllerState.TouchPadXAnalog != XControllerState.axes[CONTROLLER_AXIS_PRIMARY_THUMB_X])
				{
					const FGamepadKeyNames::Type AxisButton = (HandToUse == EControllerHand::Left) ? FGamepadKeyNames::MotionController_Left_Thumbstick_X : FGamepadKeyNames::MotionController_Right_Thumbstick_X;
					MessageHandler->OnControllerAnalog(AxisButton, ControllerIndex, XControllerState.axes[CONTROLLER_AXIS_PRIMARY_THUMB_X]);
					ControllerState.TouchPadXAnalog = XControllerState.axes[CONTROLLER_AXIS_PRIMARY_THUMB_X];
				}

				if (ControllerState.TouchPadYAnalog != XControllerState.axes[CONTROLLER_AXIS_PRIMARY_THUMB_Y])
				{
					const FGamepadKeyNames::Type AxisButton = (HandToUse == EControllerHand::Left) ? FGamepadKeyNames::MotionController_Left_Thumbstick_Y : FGamepadKeyNames::MotionController_Right_Thumbstick_Y;
					// Invert the y to match UE4 convention
					const float Value = -XControllerState.axes[CONTROLLER_AXIS_PRIMARY_THUMB_Y];
					MessageHandler->OnControllerAnalog(AxisButton, ControllerIndex, Value);
					ControllerState.TouchPadYAnalog = Value;
				}

				if (ControllerState.TriggerAnalog != XControllerState.axes[CONTROLLER_AXIS_PRIMARY_TRIGGER])
				{
					const FGamepadKeyNames::Type AxisButton = (HandToUse == EControllerHand::Left) ? FGamepadKeyNames::MotionController_Left_TriggerAxis : FGamepadKeyNames::MotionController_Right_TriggerAxis;
					MessageHandler->OnControllerAnalog(AxisButton, ControllerIndex, XControllerState.axes[CONTROLLER_AXIS_PRIMARY_TRIGGER]);
					ControllerState.TriggerAnalog = XControllerState.axes[CONTROLLER_AXIS_PRIMARY_TRIGGER];

					// emulate trigger button state
					CurrentStates[EXimmerseInputButton::TriggerPress] = ControllerState.TriggerAnalog > 0.5f;
				}

				// For each button check against the previous state and send the correct message if any
				for (int32 ButtonIndex = 0; ButtonIndex < EXimmerseInputButton::TotalButtonCount; ++ButtonIndex)
				{
					if (CurrentStates[ButtonIndex] != ControllerState.ButtonStates[ButtonIndex])
					{
						if (CurrentStates[ButtonIndex])
						{
							MessageHandler->OnControllerButtonPressed(Buttons[(int32)HandToUse][ButtonIndex], ControllerIndex, false);
						}
						else
						{
							MessageHandler->OnControllerButtonReleased(Buttons[(int32)HandToUse][ButtonIndex], ControllerIndex, false);
						}

						if (CurrentStates[ButtonIndex] != 0)
						{
							// this button was pressed - set the button's NextRepeatTime to the InitialButtonRepeatDelay
							ControllerState.NextRepeatTime[ButtonIndex] = CurrentTime + InitialButtonRepeatDelay;
						}
					}

					// Update the state for next time
					ControllerState.ButtonStates[ButtonIndex] = CurrentStates[ButtonIndex];
				}

				ControllerState.Timestamp = XControllerState.timestamp;
			}
		}

		for (int32 ButtonIndex = 0; ButtonIndex < EXimmerseInputButton::TotalButtonCount; ++ButtonIndex)
		{
			if (ControllerState.ButtonStates[ButtonIndex] != 0 && ControllerState.NextRepeatTime[ButtonIndex] <= CurrentTime)
			{
				MessageHandler->OnControllerButtonPressed(Buttons[(int32)HandToUse][ButtonIndex], ControllerIndex, true);

				// set the button's NextRepeatTime to the ButtonRepeatDelay
				ControllerState.NextRepeatTime[ButtonIndex] = CurrentTime + ButtonRepeatDelay;
			}
		}
	}
#endif // XIMMERSE_INPUT_SUPPORTED_PLATFORMS
}
void FXimmerseInput::SetChannelValue(int32 UnrealControllerId, FForceFeedbackChannelType ChannelType, float Value)
{
#if XIMMERSE_INPUT_SUPPORTED_PLATFORMS && XIMMERSE_INPUT_VIBRATION_ENABLED
	// Skip unless this is the left or right large channel, which we consider to be the only XimmerseInput feedback channel
	if (ChannelType != FForceFeedbackChannelType::LEFT_LARGE && ChannelType != FForceFeedbackChannelType::RIGHT_LARGE)
	{
		return;
	}

	const EControllerHand Hand = (ChannelType == FForceFeedbackChannelType::LEFT_LARGE) ? EControllerHand::Left : EControllerHand::Right;
	const int32 ControllerIndex = UnrealControllerIdToControllerIndex(UnrealControllerId, Hand);

	if ((ControllerIndex >= 0) && (ControllerIndex < MaxControllers))
	{
		FControllerState& ControllerState = ControllerStates[ControllerIndex];

		ControllerState.ForceFeedbackValue = Value;

		UpdateVibration(ControllerIndex);
	}
#endif // XIMMERSE_INPUT_SUPPORTED_PLATFORMS
}


void FXimmerseInput::SetChannelValues(int32 UnrealControllerId, const FForceFeedbackValues& Values)
{
#if XIMMERSE_INPUT_SUPPORTED_PLATFORMS && XIMMERSE_INPUT_VIBRATION_ENABLED
	const int32 LeftControllerIndex = UnrealControllerIdToControllerIndex(UnrealControllerId, EControllerHand::Left);
	if ((LeftControllerIndex >= 0) && (LeftControllerIndex < MaxControllers))
	{
		FControllerState& ControllerState = ControllerStates[LeftControllerIndex];
		ControllerState.ForceFeedbackValue = Values.LeftLarge;

		UpdateVibration(LeftControllerIndex);
	}

	const int32 RightControllerIndex = UnrealControllerIdToControllerIndex(UnrealControllerId, EControllerHand::Right);
	if ((RightControllerIndex >= 0) && (RightControllerIndex < MaxControllers))
	{
		FControllerState& ControllerState = ControllerStates[RightControllerIndex];
		ControllerState.ForceFeedbackValue = Values.RightLarge;

		UpdateVibration(RightControllerIndex);
	}
#endif // XIMMERSE_INPUT_SUPPORTED_PLATFORMS
}

void FXimmerseInput::SetHapticFeedbackValues(int32 UnrealControllerId, int32 Hand, const FHapticFeedbackValues& Values)
{
#if XIMMERSE_INPUT_SUPPORTED_PLATFORMS && XIMMERSE_INPUT_VIBRATION_ENABLED
	if (Hand != (int32)EControllerHand::Left && Hand != (int32)EControllerHand::Right)
	{
		return;
	}

	const int32 ControllerIndex = UnrealControllerIdToControllerIndex(UnrealControllerId, (EControllerHand)Hand);
	if (ControllerIndex >= 0 && ControllerIndex < MaxControllers)
	{
		FControllerState& ControllerState = ControllerStates[ControllerIndex];
		ControllerState.ForceFeedbackValue = (Values.Frequency > 0.0f) ? Values.Amplitude : 0.0f;

		UpdateVibration(ControllerIndex);
	}
#endif
}

bool FXimmerseInput::GetControllerOrientationAndPosition(const int32 UnrealControllerId, const EControllerHand DeviceHand, FRotator& OutOrientation, FVector& OutPosition) const
{
	bool RetVal = false;

#if XIMMERSE_INPUT_SUPPORTED_PLATFORMS
	const int32 ControllerIndex = UnrealControllerIdToControllerIndex(UnrealControllerId, DeviceHand);
	int32 DeviceIndex = ControllerToDeviceMap[ControllerIndex];

	ControllerState XControllerState;
	if (XDeviceGetInputState(DeviceHandle[DeviceIndex], &XControllerState) >= 0)
	{
		RetVal = true;

		OutPosition.X = -XControllerState.position[2] * 100;
		OutPosition.Y = XControllerState.position[0] * 100;
		OutPosition.Z = XControllerState.position[1] * 100;

		FQuat Rotation;
		Rotation.X = XControllerState.rotation[2];
		Rotation.Y = -XControllerState.rotation[0];
		Rotation.Z = -XControllerState.rotation[1];
		Rotation.W = XControllerState.rotation[3];
		OutOrientation = Rotation.Rotator();

		UE_LOG(LogXimmerseInput, Log, TEXT("Position(%f, %f, %f), Rotation(%f, %f, %f)"),
		       OutPosition.X, OutPosition.Y, OutPosition.Z, OutOrientation.Pitch, OutOrientation.Yaw, OutOrientation.Roll);
	}
#endif // XIMMERSE_INPUT_SUPPORTED_PLATFORMS

	return RetVal;
}

ETrackingStatus FXimmerseInput::GetControllerTrackingStatus(const int32 UnrealControllerId, const EControllerHand DeviceHand) const
{
	ETrackingStatus TrackingStatus = ETrackingStatus::NotTracked;

#if XIMMERSE_INPUT_SUPPORTED_PLATFORMS
	const int32 ControllerIndex = UnrealControllerIdToControllerIndex(UnrealControllerId, DeviceHand);
	int32 DeviceIndex = ControllerToDeviceMap[ControllerIndex];
	TrackingResult Result = (TrackingResult)XDeviceGetInt(DeviceHandle[DeviceIndex], FieldID::kField_TrackingResult, 0);
	if (Result != TrackingResult::kTrackingResult_NotTracked)
	{
		TrackingStatus = ETrackingStatus::Tracked;
	}
#endif // XIMMERSE_INPUT_SUPPORTED_PLATFORMS

	return TrackingStatus;
}

#if XIMMERSE_INPUT_SUPPORTED_PLATFORMS
int32 FXimmerseInput::UnrealControllerIdToControllerIndex(const int32 UnrealControllerId, const EControllerHand Hand) const
{
	return UnrealControllerId * CONTROLLERS_PER_PLAYER + (int32)Hand;
}

void FXimmerseInput::UpdateVibration(const int32 ControllerIndex)
{
#if XIMMERSE_INPUT_VIBRATION_ENABLED
	// make sure there is a valid device for this controller
	int32 DeviceIndex = ControllerToDeviceMap[ControllerIndex];
	if (DeviceIndex < 0)
	{
		return;
	}

	const FControllerState& ControllerState = ControllerStates[ControllerIndex];
	vr::IVRSystem* VRSystem = GetVRSystem();

	if (VRSystem == nullptr)
	{
		return;
	}

	// Map the float values from [0,1] to be more reasonable values for the SteamController.  The docs say that [100,2000] are reasonable values
	const float LeftIntensity = FMath::Clamp(ControllerState.ForceFeedbackValue * 2000.f, 0.f, 2000.f);
	if (LeftIntensity > 0.f)
	{
		VRSystem->TriggerHapticPulse(DeviceIndex, TOUCHPAD_AXIS, LeftIntensity);
	}
#endif
}

bool FXimmerseInput::IsGamepadAttached() const
{
	// Check if at least one motion controller is tracked
	// Only need to check for at least one player (player index 0)
	int32 PlayerIndex = 0;
	ETrackingStatus LeftHandTrackingStatus = GetControllerTrackingStatus(PlayerIndex, EControllerHand::Left);
	ETrackingStatus RightHandTrackingStatus = GetControllerTrackingStatus(PlayerIndex, EControllerHand::Right);

	return LeftHandTrackingStatus == ETrackingStatus::Tracked || RightHandTrackingStatus == ETrackingStatus::Tracked;
}

#undef LOCTEXT_NAMESPACE
#endif // XIMMERSE_INPUT_SUPPORTED_PLATFORMS
