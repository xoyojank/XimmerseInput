// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "IXimmerseInputPlugin.h"
#include "IMotionController.h"

/** Total number of controllers in a set */
#define CONTROLLERS_PER_PLAYER	2

class FXimmerseInput : public IInputDevice, public IMotionController, public IHapticDevice
{
public:
	/** Total number of motion controllers we'll support */
	static const int32 MaxControllers = 2;

	/**
	* Buttons on the SteamVR controller
	*/
	struct EXimmerseInputButton
	{
		enum Type
		{
			System,
			ApplicationMenu,
			TouchPadPress,
			TouchPadTouch,
			TriggerPress,
			Grip,
			TouchPadUp,
			TouchPadDown,
			TouchPadLeft,
			TouchPadRight,

			/** Max number of controller buttons.  Must be < 256 */
			TotalButtonCount
		};
	};

	FXimmerseInput(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler);
	virtual ~FXimmerseInput();

	virtual void Tick(float DeltaTime) override	{}

	virtual void SendControllerEvents() override;

	void SetChannelValue(int32 UnrealControllerId, FForceFeedbackChannelType ChannelType, float Value) override;

	void SetChannelValues(int32 UnrealControllerId, const FForceFeedbackValues& Values) override;

	virtual IHapticDevice* GetHapticDevice() override
	{
		return this;
	}

	virtual void SetHapticFeedbackValues(int32 UnrealControllerId, int32 Hand, const FHapticFeedbackValues& Values) override;

	virtual void GetHapticFrequencyRange(float& MinFrequency, float& MaxFrequency) const override
	{
		MinFrequency = 0.0f;
		MaxFrequency = 1.0f;
	}

	virtual float GetHapticAmplitudeScale() const override
	{
		return 1.0f;
	}

	virtual void SetMessageHandler(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override
	{
		MessageHandler = InMessageHandler;
	}

	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override
	{
		return false;
	}

	virtual bool GetControllerOrientationAndPosition(const int32 ControllerIndex, const EControllerHand DeviceHand, FRotator& OutOrientation, FVector& OutPosition) const;

	virtual ETrackingStatus GetControllerTrackingStatus(const int32 ControllerIndex, const EControllerHand DeviceHand) const;

#if XIMMERSE_INPUT_SUPPORTED_PLATFORMS

	int32 UnrealControllerIdToControllerIndex(const int32 UnrealControllerId, const EControllerHand Hand) const;
	void UpdateVibration(const int32 ControllerIndex);
	virtual bool IsGamepadAttached() const override;

private:

	struct FControllerState
	{
		/** Which hand this controller is representing */
		EControllerHand Hand;

		/** If packet num matches that on your prior call, then the controller state hasn't been changed since
		* your last call and there is no need to process it. */
		uint32 PacketNum;

		/** touchpad analog values */
		float TouchPadXAnalog;
		float TouchPadYAnalog;

		/** trigger analog value */
		float TriggerAnalog;

		/** Last frame's button states, so we only send events on edges */
		bool ButtonStates[EXimmerseInputButton::TotalButtonCount];

		/** Next time a repeat event should be generated for each button */
		double NextRepeatTime[EXimmerseInputButton::TotalButtonCount];

		/** Value for force feedback on this controller hand */
		float ForceFeedbackValue;
	};

	/** Mappings between tracked devices and 0 indexed controllers */
	int32 NumControllersMapped;
	int32 ControllerToDeviceMap[MaxControllers];
	int32 DeviceToControllerMap[MaxControllers];

	/** Controller states */
	FControllerState ControllerStates[MaxControllers];

	/** Delay before sending a repeat message after a button was first pressed */
	float InitialButtonRepeatDelay;

	/** Delay before sending a repeat message after a button has been pressed for a while */
	float ButtonRepeatDelay;

	/** Mapping of controller buttons */
	FGamepadKeyNames::Type Buttons[CONTROLLERS_PER_PLAYER][EXimmerseInputButton::TotalButtonCount];

	friend class FXimmerseInputPlugin;
	int32 DeviceHandle[MaxControllers + 1];

#endif // XIMMERSE_INPUT_SUPPORTED_PLATFORMS

	/** handler to send all messages to */
	TSharedRef<FGenericApplicationMessageHandler> MessageHandler;
};
