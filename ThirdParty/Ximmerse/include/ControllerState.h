#ifndef __XIM_CONTROLLER_STATE_H__
#define __XIM_CONTROLLER_STATE_H__

// Reference : https://msdn.microsoft.com/en-us/library/windows/apps/microsoft.directx_sdk.reference.xinput_gamepad
enum ControllerAxis
{
	CONTROLLER_AXIS_LEFT_TRIGGER,
	CONTROLLER_AXIS_RIGHT_TRIGGER,
	CONTROLLER_AXIS_LEFT_THUMB_X,
	CONTROLLER_AXIS_LEFT_THUMB_Y,
	CONTROLLER_AXIS_RIGHT_THUMB_X,
	CONTROLLER_AXIS_RIGHT_THUMB_Y,
	CONTROLLER_AXIS_MAX,
};

// Reference : https://msdn.microsoft.com/en-us/library/windows/apps/microsoft.directx_sdk.reference.xinput_gamepad
enum ControllerButton
{
	CONTROLLER_BUTTON_HOME			= 0x0010,
	CONTROLLER_BUTTON_APP			= 0x0020,
	CONTROLLER_BUTTON_CLICK			= 0x0040,
	CONTROLLER_BUTTON_LEFT_GRIP		= 0x0100,
	CONTROLLER_BUTTON_RIGHT_GRIP	= 0x0200,
	CONTROLLER_BUTTON_TRIGGER		= 0x0400,
	CONTROLLER_BUTTON_TOUCH			= 0x10000,
	//
	CONTROLLER_BUTTON_NONE			= 0x0,
	CONTROLLER_BUTTON_ANY			= ~CONTROLLER_BUTTON_NONE, ,
};

typedef struct tagControllerState
{
	// Common
	int handle;
	int timestamp;
	int state_mask;
	// Gamepad
	float axes[6];
	unsigned int buttons;
	// Motion
	float position[3];
	float rotation[4];
	float accelerometer[3];
	float gyroscope[3];
} ControllerState;

#endif