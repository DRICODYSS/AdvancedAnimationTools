// Copyright DRICODYSS. All Rights Reserved.

#include "RetargetAnimations/RetargetAnimsWidgetSettings.h"

URetargetAnimsWidgetSettings* URetargetAnimsWidgetSettings::SingletonInstance = nullptr;

URetargetAnimsWidgetSettings* URetargetAnimsWidgetSettings::GetInstance()
{
	{
		if (!SingletonInstance)
		{
			SingletonInstance = NewObject<URetargetAnimsWidgetSettings>(GetTransientPackage(), URetargetAnimsWidgetSettings::StaticClass());
			SingletonInstance->AddToRoot();
		}
		return SingletonInstance;
	}
}
