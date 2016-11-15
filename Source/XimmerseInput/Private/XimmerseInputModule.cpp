// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "XimmerseInput.h"
#include "IXimmerseInputPlugin.h"

#if XIMMERSE_INPUT_SUPPORTED_PLATFORMS

#define LOCTEXT_NAMESPACE "XimmerseInput"

class FXimmerseInputModule : public IXimmerseInputPlugin
{
	// IInputDeviceModule overrides
	virtual TSharedPtr< class IInputDevice > CreateInputDevice(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override
	{
		return TSharedPtr< class IInputDevice >(new FXimmerseInput(InMessageHandler));
	}
};

#else	//	XIMMERSE_INPUT_SUPPORTED_PLATFORMS

class FXimmerseInputModule : public FDefaultModuleImpl
{
};

#undef LOCTEXT_NAMESPACE
#endif	// XIMMERSE_INPUT_SUPPORTED_PLATFORMS

IMPLEMENT_MODULE(FXimmerseInputModule, XimmerseInput)
