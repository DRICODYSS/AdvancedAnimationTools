// Copyright DRICODYSS. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IKRigLogger.h"
#include "PreviewScene.h"
#include "RetargetAnimations/AAT_IKRetargetBatchOperation.h"
#include "Widgets/SCompoundWidget.h"

class SRetargetAnimationsOutputLog;
class SRetargetAssetBrowser;
class SRetargetPoseViewport;
class URetargetAnimsWidgetSettings;

enum class ERetargetAnimsWidgetState : uint8
{
	MANUAL_RETARGET_INVALID,
	NO_ANIMATIONS_SELECTED,
	READY_TO_EXPORT
};

class SRetargetAnimsWidget :
	public SCompoundWidget,
	public FGCObject
{
	TObjectPtr<URetargetAnimsWidgetSettings> Settings;

	static TSharedPtr<SWindow> Window;
	TSharedPtr<SRetargetAssetBrowser> AssetBrowser;

	FAAT_IKRetargetBatchOperationContext BatchContext;
	FIKRigLogger Log;

public:
	SLATE_BEGIN_ARGS(SRetargetAnimsWidget) {}
	SLATE_END_ARGS()

	SRetargetAnimsWidget();
	void Construct(const FArguments& InArgs);
	static void ShowWindow();

	TObjectPtr<URetargetAnimsWidgetSettings> GetSettings() { return Settings; }

	// FGCObject

	virtual FString GetReferencerName() const override	{ return TEXT("SRetargetAnimAssetsWindow"); }
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

private:
	void SetAssets(
		UIKRetargeter* Retargeter,
		bool bReplaceReferences,
		bool bUseCustomPath,
		FName Path,
		FName RootFolderName
	);
	void ShowAssetWarnings();

	void OnFinishedChangingSelectionProperties(const FPropertyChangedEvent& PropertyChangedEvent);

	bool CanExportAnimations() const;
	FReply OnExportAnimations();

	ERetargetAnimsWidgetState GetCurrentState() const;
	EVisibility GetWarningVisibility() const;
	FText GetWarningText() const;
};
