// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "XimmerseInputPrivatePCH.h"
#include "XimmerseInput.h"
#include "IXimmerseInputPlugin.h"

#if XIMMERSE_INPUT_SUPPORTED_PLATFORMS

#define LOCTEXT_NAMESPACE "XimmerseInput"

class FXimmerseInputModule : public IXimmerseInputPlugin
{
	virtual TSharedPtr< class IInputDevice > CreateInputDevice(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override
	{
		FXimmerseInput* XimmerseInput = new FXimmerseInput(InMessageHandler);
		XimmerseInput->DeviceHandle[0] = XDeviceGetInputDeviceHandle("XCobra-0");
		XimmerseInput->DeviceHandle[1] = XDeviceGetInputDeviceHandle("XCobra-1");
		XimmerseInput->DeviceHandle[2] = XDeviceGetInputDeviceHandle("XHawk-0");
		return TSharedPtr< class IInputDevice >(XimmerseInput);
	}

	virtual void StartupModule() override
	{
		IXimmerseInputPlugin::StartupModule();

		XDeviceInit();
	}

	virtual void ShutdownModule() override
	{
		IXimmerseInputPlugin::ShutdownModule();

		XDeviceExit();
	}
};

#else	//	XIMMERSE_INPUT_SUPPORTED_PLATFORMS

class FXimmerseInputModule : public FDefaultModuleImpl
{
};

#undef LOCTEXT_NAMESPACE
#endif	// XIMMERSE_INPUT_SUPPORTED_PLATFORMS

IMPLEMENT_MODULE(FXimmerseInputModule, XimmerseInput)
