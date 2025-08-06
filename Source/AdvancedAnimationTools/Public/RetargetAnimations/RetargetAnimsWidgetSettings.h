// Copyright DRICODYSS. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RetargetAnimsWidgetSettings.generated.h"

class UIKRetargeter;

UCLASS(Blueprintable, Category = "BatchRetargetSettings")
class ADVANCEDANIMATIONTOOLS_API URetargetAnimsWidgetSettings : public UObject
{
	GENERATED_BODY()

	static URetargetAnimsWidgetSettings* SingletonInstance;

public:
	static URetargetAnimsWidgetSettings* GetInstance();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Retargeter")
	TObjectPtr<UIKRetargeter> RetargetAsset;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dependencies")
	// bool bBindDependenciesToNewAssets = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path")
	FName Path = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path")
	FName RootFolderName = "Animations";
};
