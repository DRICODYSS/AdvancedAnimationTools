// Copyright DRICODYSS. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorAnimUtils.h"

#include "AAT_IKRetargetBatchOperation.generated.h"

class UIKRetargeter;

struct FAAT_AdditiveRetargetSettings
{
	TObjectPtr<UAnimSequence> SequenceAsset;
	TEnumAsByte<EAdditiveAnimationType> AdditiveAnimType;
	TEnumAsByte<EAdditiveBasePoseType> RefPoseType;
	int32 RefFrameIndex;
	TObjectPtr<UAnimSequence> RefPoseSeq;
	
	void PrepareForRetarget(UAnimSequence* InSequenceAsset);
	void RestoreOnAsset() const;
};

struct ADVANCEDANIMATIONTOOLS_API FAAT_IKRetargetBatchOperationContext
{
	TArray<TWeakObjectPtr<UObject>> AssetsToRetarget;
	USkeletalMesh* SourceMesh = nullptr;
	USkeletalMesh* TargetMesh = nullptr;
	UIKRetargeter* IKRetargetAsset = nullptr;
	EditorAnimUtils::FNameDuplicationRule NameRule;
	bool bUseSourcePath = true;
	bool bOverwriteExistingFiles = false;
	bool bIncludeReferencedAssets = true;
	bool bExportOnlyAnimatedBones = true;
	bool bRetainAdditiveFlags = true;
	bool bBindDependenciesToNewAssets = false;

	FString Path = "";
	FString RootFolderName = "Animations";

	void Reset()
	{
		SourceMesh = nullptr;
		TargetMesh = nullptr;
		IKRetargetAsset = nullptr;
		bIncludeReferencedAssets = true;
		NameRule.Prefix = "";
		NameRule.Suffix = "";
		NameRule.ReplaceFrom = "";
		NameRule.ReplaceTo = "";
	}

	bool IsValid() const
	{
		return SourceMesh && TargetMesh && IKRetargetAsset && (SourceMesh != TargetMesh);
	}
};

UCLASS()
class ADVANCEDANIMATIONTOOLS_API UAAT_IKRetargetBatchOperation : public UObject
{
	GENERATED_BODY()

	TArray<UAnimationAsset*> AnimationAssetsToRetarget;
	TArray<UAnimBlueprint*>	AnimBlueprintsToRetarget;
	TMap<UAnimationAsset*, UAnimationAsset*> DuplicatedAnimAssets;
	TMap<UAnimBlueprint*, UAnimBlueprint*> DuplicatedBlueprints;
	TMap<UAnimationAsset*, UAnimationAsset*> RemappedAnimAssets;

public:
	UFUNCTION(BlueprintCallable, Category=IKBatchRetarget)
	static TArray<FAssetData> DuplicateAndRetarget(
		const TArray<FAssetData>& AssetsToRetarget,
		USkeletalMesh* SourceMesh,
		USkeletalMesh* TargetMesh,
		UIKRetargeter* IKRetargetAsset,
		const FString& Search = "",
		const FString& Replace = "",
		const FString& Prefix = "",
		const FString& Suffix = "",
		const bool bIncludeReferencedAssets=true
	);

	void RunRetarget(FAAT_IKRetargetBatchOperationContext& Context);

private:

	void Reset();

	int32 GenerateAssetLists(const FAAT_IKRetargetBatchOperationContext& Context);
	void DuplicateRetargetAssets(const FAAT_IKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress);
	void RetargetAssets(const FAAT_IKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress);
	void ConvertAnimation(const FAAT_IKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress);
	void RemapCurves(const FAAT_IKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress);
	void OverwriteExistingAssets(const FAAT_IKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress);
	void NotifyUserOfResults(const FAAT_IKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress) const;
	void GetNewAssets(TArray<UObject*>& NewAssets) const;
	void CleanupIfCancelled(const FScopedSlowTask& Progress) const;
};
