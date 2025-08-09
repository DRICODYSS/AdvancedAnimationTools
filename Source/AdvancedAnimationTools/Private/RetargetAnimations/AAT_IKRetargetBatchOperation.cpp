// Copyright DRICODYSS. All Rights Reserved.

#include "RetargetAnimations/AAT_IKRetargetBatchOperation.h"

#include "AnimPose.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "EditorReimportHandler.h"
#include "FileHelpers.h"
#include "SSkeletonWidget.h"
#include "EditorFramework/AssetImportData.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Retargeter/IKRetargetProcessor.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Retargeter/RetargetOps/CurveRemapOp.h"
#include "ObjectTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Retargeter/RetargetOps/SpeedPlantingOp.h"

#define LOCTEXT_NAMESPACE "AAT_IKRetargetBatchOperation"

int32 UAAT_IKRetargetBatchOperation::GenerateAssetLists(const FAAT_IKRetargetBatchOperationContext& Context)
{
	AnimationAssetsToRetarget.Reset();
	AnimBlueprintsToRetarget.Reset();

	for (TWeakObjectPtr<UObject> AssetPtr : Context.AssetsToRetarget)
	{
		UObject* Asset = AssetPtr.Get();
		if (UAnimationAsset* AnimAsset = Cast<UAnimationAsset>(Asset))
		{
			AnimationAssetsToRetarget.AddUnique(AnimAsset);

			if (UAnimMontage* AnimMontage = Cast<UAnimMontage>(AnimAsset))
			{
				for (const FSlotAnimationTrack& Track: AnimMontage->SlotAnimTracks)
				{
					for (const FAnimSegment& Segment: Track.AnimTrack.AnimSegments)
					{
						if (Segment.IsValid() && Segment.GetAnimReference())
						{
							AnimationAssetsToRetarget.AddUnique(Segment.GetAnimReference());
						}
					}
				}

				if (AnimMontage->PreviewBasePose)
				{
					AnimationAssetsToRetarget.AddUnique(AnimMontage->PreviewBasePose);
				}
			}
		}
		else if (UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>(Asset))
		{
			UAnimBlueprint* ParentBP = Cast<UAnimBlueprint>(AnimBlueprint->ParentClass->ClassGeneratedBy);
			while (ParentBP)
			{
				AnimBlueprintsToRetarget.AddUnique(ParentBP);
				ParentBP = Cast<UAnimBlueprint>(ParentBP->ParentClass->ClassGeneratedBy);
			}
				
			AnimBlueprintsToRetarget.AddUnique(AnimBlueprint);				
		}
	}

	if (Context.bIncludeReferencedAssets)
	{
		for (UAnimBlueprint* AnimBlueprint : AnimBlueprintsToRetarget)
		{
			GetAllAnimationSequencesReferredInBlueprint(AnimBlueprint, AnimationAssetsToRetarget);
		}

		int32 AssetIndex = 0;
		while (AssetIndex < AnimationAssetsToRetarget.Num())
		{
			UAnimationAsset* AnimAsset = AnimationAssetsToRetarget[AssetIndex++];
			AnimAsset->HandleAnimReferenceCollection(AnimationAssetsToRetarget, true);
		}
	}

	return AnimationAssetsToRetarget.Num() + AnimBlueprintsToRetarget.Num();
}

void UAAT_IKRetargetBatchOperation::DuplicateRetargetAssets(
	const FAAT_IKRetargetBatchOperationContext& Context,
	FScopedSlowTask& Progress)
{
	Progress.EnterProgressFrame(1.f, FText(LOCTEXT("DuplicatingBatchRetarget", "Duplicating animation assets...")));
	
	UPackage* DestinationPackage = Context.TargetMesh->GetOutermost();

	TArray<UAnimationAsset*> AnimationAssetsToDuplicate = AnimationAssetsToRetarget;
	TArray<UAnimBlueprint*> AnimBlueprintsToDuplicate = AnimBlueprintsToRetarget;
	for (TPair<UAnimationAsset*, UAnimationAsset*>& Pair : RemappedAnimAssets)
	{
		AnimationAssetsToDuplicate.Remove(Pair.Key);
	}

	for (UAnimationAsset* Asset : AnimationAssetsToDuplicate)
	{
		if (Progress.ShouldCancel())
		{
			return;
		}

		FString AssetName = Asset->GetName();
		Progress.EnterProgressFrame(1.f, FText::Format(LOCTEXT("DuplicatingAnimation", "Duplicating animation: {0}"), FText::FromString(AssetName)));

		FNameDuplicationRule NameRule = Context.NameRule;
		if (Context.bUseCustomPath)
		{
			FString AssetPath = Asset->GetPathName();
			FString AssetFolderPath = FPackageName::GetLongPackagePath(AssetPath);

			FString ReferenceFolder = Context.RootFolderName;

			int32 FolderIndex = AssetFolderPath.Find(ReferenceFolder, ESearchCase::IgnoreCase, ESearchDir::FromStart);
			FString SubFolderPath;

			if (FolderIndex != INDEX_NONE)
			{
				int32 StartIndex = FolderIndex + ReferenceFolder.Len();
				if (AssetFolderPath.Mid(StartIndex, 1) == TEXT("/"))
				{
					StartIndex++;
				}
				SubFolderPath = AssetFolderPath.Mid(StartIndex);
			}
			else
			{
				SubFolderPath = TEXT("");
			}

			FString BaseTargetPath = Context.Path;
			FString TargetFolderPath = FPaths::Combine(BaseTargetPath, SubFolderPath);
			NameRule.FolderPath = TargetFolderPath;
		}
		else
		{
			NameRule.FolderPath = FPackageName::GetLongPackagePath(Asset->GetPathName()) / TEXT("");
		}
		
		TMap<UAnimationAsset*, UAnimationAsset*> DuplicateMap = DuplicateAssets<UAnimationAsset>({Asset}, DestinationPackage, &NameRule);
		DuplicatedAnimAssets.Append(DuplicateMap);
	}
	// for (UAnimBlueprint* Asset : AnimBlueprintsToDuplicate)
	// {
	// 	if (Progress.ShouldCancel())
	// 	{
	// 		return;
	// 	}
	//
	// 	FString AssetName = Asset->GetName();
	// 	Progress.EnterProgressFrame(1.f, FText::Format(LOCTEXT("DuplicatingBlueprint", "Duplicating blueprint: {0}"), FText::FromString(AssetName)));
	//
	// 	FNameDuplicationRule NameRule = Context.NameRule;
	// 	if (Context.bUseSourcePath)
	// 	{
	// 		NameRule.FolderPath = FPackageName::GetLongPackagePath(Asset->GetPathName()) / TEXT("");
	// 	}
	// 	
	// 	TMap<UAnimBlueprint*, UAnimBlueprint*> DuplicateMap = DuplicateAssets<UAnimBlueprint>({Asset}, DestinationPackage, &NameRule);
	// 	DuplicatedBlueprints.Append(DuplicateMap);
	// }

	if (!Context.NameRule.FolderPath.IsEmpty())
	{
		for (TPair<UAnimationAsset*, UAnimationAsset*>& Pair : DuplicatedAnimAssets)
		{
			UAnimSequence* SourceSequence = Cast<UAnimSequence>(Pair.Key);
			UAnimSequence* DestinationSequence = Cast<UAnimSequence>(Pair.Value);
			if (!(SourceSequence && DestinationSequence))
			{
				continue;
			}
			
			for (int index = 0; index < SourceSequence->AssetImportData->SourceData.SourceFiles.Num(); index++)
			{
				const FString& RelativeFilename = SourceSequence->AssetImportData->SourceData.SourceFiles[index].RelativeFilename;
				const FString OldPackagePath = FPackageName::GetLongPackagePath(SourceSequence->GetPathName()) / TEXT("");
				const FString NewPackagePath = FPackageName::GetLongPackagePath(DestinationSequence->GetPathName()) / TEXT("");
				const FString AbsoluteSrcPath = FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(OldPackagePath));
				const FString SrcFile = AbsoluteSrcPath / RelativeFilename;
				const bool bSrcFileExists = FPlatformFileManager::Get().GetPlatformFile().FileExists(*SrcFile);
				if (!bSrcFileExists || (NewPackagePath == OldPackagePath))
				{
					continue;
				}

				FString BasePath = FPackageName::LongPackageNameToFilename(OldPackagePath);
				FString OldSourceFilePath = FPaths::ConvertRelativePathToFull(BasePath, RelativeFilename);
				TArray<FString> Paths;
				Paths.Add(OldSourceFilePath);
			
				FReimportManager::Instance()->UpdateReimportPaths(DestinationSequence, Paths);
			}
		}
	}

	RemappedAnimAssets.Append(DuplicatedAnimAssets);

	DuplicatedAnimAssets.GenerateValueArray(AnimationAssetsToRetarget);
	DuplicatedBlueprints.GenerateValueArray(AnimBlueprintsToRetarget);
}

void UAAT_IKRetargetBatchOperation::RetargetAssets(
	const FAAT_IKRetargetBatchOperationContext& Context,
	FScopedSlowTask& Progress)
{
	USkeleton* OldSkeleton = Context.SourceMesh->GetSkeleton();
	USkeleton* NewSkeleton = Context.TargetMesh->GetSkeleton();

	TArray<FAAT_AdditiveRetargetSettings> SettingsToRestoreAfterRetarget;
	
	for (UAnimationAsset* AssetToRetarget : AnimationAssetsToRetarget)
	{
		if (Progress.ShouldCancel())
		{
			return;
		}

		if (UAnimSequence* AnimSequenceToRetarget = Cast<UAnimSequence>(AssetToRetarget))
		{
			FString AssetName = AnimSequenceToRetarget->GetName();
			Progress.EnterProgressFrame(1.f, FText::Format(LOCTEXT("PreparingAsset", "Preparing asset: {0}"), FText::FromString(AssetName)));

			UAnimationBlueprintLibrary::CopyAnimationCurveNamesToSkeleton(OldSkeleton, NewSkeleton, AnimSequenceToRetarget, ERawCurveTrackTypes::RCT_Float);	

			IAnimationDataController& Controller = AnimSequenceToRetarget->GetController();
			constexpr bool bShouldTransact = false;
			Controller.OpenBracket(FText::FromString("Preparing for retargeted animation."), bShouldTransact);
			Controller.RemoveAllCurvesOfType(ERawCurveTrackTypes::RCT_Transform, bShouldTransact);

			FAAT_AdditiveRetargetSettings SequenceSettings;
			SequenceSettings.PrepareForRetarget(AnimSequenceToRetarget);
			SettingsToRestoreAfterRetarget.Add(SequenceSettings);

			AnimSequenceToRetarget->RetargetSource = NAME_None;
			AnimSequenceToRetarget->SetRetargetSourceAsset(Context.TargetMesh);

			Controller.UpdateWithSkeleton(NewSkeleton, bShouldTransact);
			Controller.CloseBracket(bShouldTransact);
		}

		AssetToRetarget->ReplaceReferredAnimations(RemappedAnimAssets);
		AssetToRetarget->SetSkeleton(NewSkeleton);
		AssetToRetarget->SetPreviewMesh(Context.TargetMesh);
	}

	for (UAnimationAsset* AssetToRetarget : AnimationAssetsToRetarget)
	{
		if (Progress.ShouldCancel())
		{
			return;
		}

		if (UAnimSequence* AnimSequenceToRetarget = Cast<UAnimSequence>(AssetToRetarget))
		{
			AnimSequenceToRetarget->UpdateRetargetSourceAssetData();
		}

		AssetToRetarget->PostEditChange();
		AssetToRetarget->MarkPackageDirty();
	}

	ConvertAnimation(Context,Progress);
	if (Context.bRetainAdditiveFlags)
	{
		for (FAAT_AdditiveRetargetSettings SettingsToRestore : SettingsToRestoreAfterRetarget)
		{
			SettingsToRestore.RestoreOnAsset();
		}
	}

	for (UAnimBlueprint* AnimBlueprint : AnimBlueprintsToRetarget)
	{
		if (Progress.ShouldCancel())
		{
			return;
		}

		AnimBlueprint->TargetSkeleton = NewSkeleton;
		AnimBlueprint->SetPreviewMesh(Context.TargetMesh);

		UAnimBlueprint* CurrentParentBP = Cast<UAnimBlueprint>(AnimBlueprint->ParentClass->ClassGeneratedBy);
		if (CurrentParentBP)
		{
			UAnimBlueprint* const * ParentBP = DuplicatedBlueprints.Find(CurrentParentBP);
			if (ParentBP)
			{
				AnimBlueprint->ParentClass = (*ParentBP)->GeneratedClass;
			}
		}

		if(RemappedAnimAssets.Num() > 0)
		{
			ReplaceReferredAnimationsInBlueprint(AnimBlueprint, RemappedAnimAssets);
		}

		FBlueprintEditorUtils::RefreshAllNodes(AnimBlueprint);
		FKismetEditorUtilities::CompileBlueprint(AnimBlueprint, EBlueprintCompileOptions::SkipGarbageCollection);
		AnimBlueprint->PostEditChange();
		AnimBlueprint->MarkPackageDirty();
	}

	RemapCurves(Context, Progress);
}

void UAAT_IKRetargetBatchOperation::ConvertAnimation(
	const FAAT_IKRetargetBatchOperationContext& Context,
	FScopedSlowTask& Progress)
{
	FIKRetargetProcessor Processor;
	FRetargetProfile RetargetProfile;
	RetargetProfile.FillProfileWithAssetSettings(Context.IKRetargetAsset);
	Processor.Initialize(Context.SourceMesh, Context.TargetMesh, Context.IKRetargetAsset, RetargetProfile);
	if (!Processor.IsInitialized())
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to initialize the IK Retargeter. Newly created animations were not retargeted!"));
		return;
	}

	const FRetargetSkeleton& TargetSkeleton = Processor.GetSkeleton(ERetargetSourceOrTarget::Target);
	const TArray<FName>& TargetBoneNames = TargetSkeleton.BoneNames;
	const int32 NumTargetBones = TargetBoneNames.Num();

	TArray<FRawAnimSequenceTrack> BoneTracks;
	BoneTracks.SetNumZeroed(NumTargetBones);

	const FRetargetSkeleton& SourceSkeleton = Processor.GetSkeleton(ERetargetSourceOrTarget::Source);
	const TArray<FName>& SourceBoneNames = SourceSkeleton.BoneNames;
	const int32 NumSourceBones = SourceBoneNames.Num();

	TArray<FTransform> SourceComponentPose;
	SourceComponentPose.SetNum(NumSourceBones);

	TArray<FName> SpeedCurveNames;
	const TArray<FIKRetargetSpeedPlantingOp*> SpeedPlantingOps = Context.IKRetargetAsset->GetAllRetargetOpsOfType<FIKRetargetSpeedPlantingOp>();
	for (FIKRetargetSpeedPlantingOp* SpeedPlantingOp : SpeedPlantingOps)
	{
		SpeedCurveNames.Append(SpeedPlantingOp->GetRequiredSpeedCurves());
	}

	TMap<FString, TSet<FName>> MissingSpeedCurves;
	for (TPair<UAnimationAsset*, UAnimationAsset*>& Pair : DuplicatedAnimAssets)
	{
		if (Progress.ShouldCancel())
		{
			return;
		}
		
		UAnimSequence* SourceSequence = Cast<UAnimSequence>(Pair.Key);
		UAnimSequence* TargetSequence = Cast<UAnimSequence>(Pair.Value);
		if (!(SourceSequence && TargetSequence))
		{
			continue;
		}

		FString AssetName = TargetSequence->GetName();
		Progress.EnterProgressFrame(1.f, FText::Format(LOCTEXT("RunningBatchRetarget", "Retargeting animation asset: {0}"), FText::FromString(AssetName)));

		IAnimationDataController& TargetSeqController = TargetSequence->GetController();
		constexpr bool bShouldTransact = false;
		TargetSeqController.OpenBracket(FText::FromString("Generating Retargeted Animation Data"), bShouldTransact);
		TargetSeqController.NotifyPopulated();
		TargetSeqController.UpdateWithSkeleton(const_cast<USkeleton*>(TargetSkeleton.SkeletalMesh->GetSkeleton()), bShouldTransact);

		const int32 NumFrames = SourceSequence->GetNumberOfSampledKeys();
		for (int32 TargetBoneIndex=0; TargetBoneIndex<NumTargetBones; ++TargetBoneIndex)
		{
			BoneTracks[TargetBoneIndex].PosKeys.SetNum(NumFrames);
			BoneTracks[TargetBoneIndex].RotKeys.SetNum(NumFrames);
			BoneTracks[TargetBoneIndex].ScaleKeys.SetNum(NumFrames);
		}

		FAnimPoseEvaluationOptions EvaluationOptions = FAnimPoseEvaluationOptions();
		EvaluationOptions.OptionalSkeletalMesh = const_cast<USkeletalMesh*>(SourceSkeleton.SkeletalMesh);
		EvaluationOptions.bExtractRootMotion = false;
		EvaluationOptions.bIncorporateRootMotionIntoPose = true;

		Processor.OnPlaybackReset();
		for (int32 FrameIndex=0; FrameIndex<NumFrames; ++FrameIndex)
		{
			if (Progress.ShouldCancel())
			{
				TargetSeqController.CloseBracket(bShouldTransact);
				return;
			}

			FAnimPose SourcePoseAtFrame;
			UAnimPoseExtensions::GetAnimPoseAtFrame(SourceSequence, FrameIndex, EvaluationOptions, SourcePoseAtFrame);
			for (int32 BoneIndex = 0; BoneIndex < NumSourceBones; BoneIndex++)
			{
				const FName& BoneName = SourceBoneNames[BoneIndex];
				SourceComponentPose[BoneIndex] = UAnimPoseExtensions::GetBonePose(SourcePoseAtFrame, BoneName, EAnimPoseSpaces::World);
			}

			for (FTransform& Transform : SourceComponentPose)
			{
				Transform.SetScale3D(FVector::OneVector);
			}

			const float TimeAtCurrentFrame = SourceSequence->GetTimeAtFrame(FrameIndex);
			float DeltaTime = TimeAtCurrentFrame;
			if (FrameIndex > 0)
			{
				const float TimeAtPrevFrame = SourceSequence->GetTimeAtFrame(FrameIndex-1);
				DeltaTime = TimeAtCurrentFrame - TimeAtPrevFrame;
			}

			TMap<FName, float> SpeedCurveValues;
			constexpr bool bForceUseRawData = false;
			for (const FName& SpeedCurveName : SpeedCurveNames)
			{
				if (!SourceSequence->HasCurveData(SpeedCurveName, bForceUseRawData))
				{
					TSet<FName>& MissingCurves = MissingSpeedCurves.FindOrAdd(SourceSequence->GetName());
					MissingCurves.Add(SpeedCurveName);
					continue;
				}

				SpeedCurveValues.Add(SpeedCurveName, SourceSequence->EvaluateCurveData(SpeedCurveName, FAnimExtractContext(static_cast<double>(TimeAtCurrentFrame))));
			}

			FRetargetProfile SettingsProfile;
			SettingsProfile.FillProfileWithAssetSettings(Context.IKRetargetAsset);
			Processor.ScaleSourcePose(SourceComponentPose);
			const TArray<FTransform>& TargetComponentPose = Processor.RunRetargeter(SourceComponentPose, SettingsProfile, DeltaTime);

			TArray<FTransform> TargetLocalPose = TargetComponentPose;
			TargetSkeleton.UpdateLocalTransformsBelowBone(0,TargetLocalPose, TargetComponentPose);

			for (int32 TargetBoneIndex=0; TargetBoneIndex<NumTargetBones; ++TargetBoneIndex)
			{
				const FTransform& LocalPose = TargetLocalPose[TargetBoneIndex];
				
				FRawAnimSequenceTrack& BoneTrack = BoneTracks[TargetBoneIndex];
				
				BoneTrack.PosKeys[FrameIndex] = FVector3f(LocalPose.GetLocation());
				BoneTrack.RotKeys[FrameIndex] = FQuat4f(LocalPose.GetRotation().GetNormalized());
				BoneTrack.ScaleKeys[FrameIndex] = FVector3f(LocalPose.GetScale3D());
			}
		}

		for (int32 TargetBoneIndex=0; TargetBoneIndex<NumTargetBones; ++TargetBoneIndex)
		{
			const FName& TargetBoneName = TargetBoneNames[TargetBoneIndex];

			const FRawAnimSequenceTrack& RawTrack = BoneTracks[TargetBoneIndex];
			TargetSeqController.AddBoneCurve(TargetBoneName, bShouldTransact);
			TargetSeqController.SetBoneTrackKeys(TargetBoneName, RawTrack.PosKeys, RawTrack.RotKeys, RawTrack.ScaleKeys, bShouldTransact);
		}

		TargetSeqController.CloseBracket(bShouldTransact);
	}

	for (const TPair<FString, TSet<FName>>& Pair : MissingSpeedCurves)
	{
		for (const FName& MissingCurve : Pair.Value)
		{
			UE_LOG(LogTemp, Warning, TEXT("IK Retarget Batch Operation: Missing speed curve, %s on sequence %s."), *MissingCurve.ToString(), *Pair.Key);	
		}
	}
}

void UAAT_IKRetargetBatchOperation::RemapCurves(const FAAT_IKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress)
{
	USkeleton* SourceSkeleton = Context.SourceMesh->GetSkeleton();
	USkeleton* TargetSkeleton = Context.TargetMesh->GetSkeleton();

	bool bCopyAllSourceCurves = true;
	TMap<FAnimationCurveIdentifier, FAnimationCurveIdentifier> CurvesToRemap;
	const TArray<FInstancedStruct>& OpStack = Context.IKRetargetAsset->GetRetargetOps();
	for (const FInstancedStruct& OpStruct : OpStack)
	{
		if (OpStruct.GetScriptStruct() != FIKRetargetCurveRemapOp::StaticStruct())
		{
			continue;
		}
		
		const FIKRetargetCurveRemapOp* CurveRemapOp = OpStruct.GetPtr<FIKRetargetCurveRemapOp>();
		if (!CurveRemapOp)
		{
			continue;
		}

		if (!CurveRemapOp->IsEnabled())
		{
			continue;
		}

		for (const FCurveRemapPair& CurveToRemap : CurveRemapOp->Settings.CurvesToRemap)
		{
			const FAnimationCurveIdentifier SourceCurveId = UAnimationCurveIdentifierExtensions::FindCurveIdentifier(SourceSkeleton, CurveToRemap.SourceCurve, ERawCurveTrackTypes::RCT_Float);
			const FAnimationCurveIdentifier TargetCurveId = UAnimationCurveIdentifierExtensions::FindCurveIdentifier(TargetSkeleton, CurveToRemap.TargetCurve, ERawCurveTrackTypes::RCT_Float);
			CurvesToRemap.Add(SourceCurveId, TargetCurveId);
		}

		bCopyAllSourceCurves &= CurveRemapOp->Settings.bCopyAllSourceCurves;
	}

	Progress.EnterProgressFrame(1.f, FText::Format(LOCTEXT("RemappingCurves", "Remapping {0} curves on animation assets..."), FText::AsNumber(CurvesToRemap.Num())));

	for (TPair<UAnimationAsset*, UAnimationAsset*>& Pair : DuplicatedAnimAssets)
	{
		UAnimSequence* SourceSequence = Cast<UAnimSequence>(Pair.Key);
		UAnimSequence* TargetSequence = Cast<UAnimSequence>(Pair.Value);
		if (!(SourceSequence && TargetSequence))
		{
			continue;
		}

		if (Progress.ShouldCancel())
		{
			return;
		}

		IAnimationDataController& TargetSeqController = TargetSequence->GetController();
		constexpr bool bShouldTransact = false;
		TargetSeqController.OpenBracket(FText::FromString("Remapping Curve Data"), bShouldTransact);

		const IAnimationDataModel* SourceDataModel = SourceSequence->GetDataModel();
		
		for (const TTuple<FAnimationCurveIdentifier, FAnimationCurveIdentifier>& CurveToRemap : CurvesToRemap)
		{
			const FFloatCurve* SourceCurve =  SourceDataModel->FindFloatCurve(CurveToRemap.Key);
			if (!SourceCurve)
			{
				continue;
			}

			FAnimationCurveIdentifier TargetCurveID = CurveToRemap.Value;
			if (!TargetCurveID.IsValid())
			{
				continue;
			}
			TargetSeqController.AddCurve(TargetCurveID, SourceCurve->GetCurveTypeFlags(), bShouldTransact);

			TargetSeqController.SetCurveKeys(TargetCurveID, SourceCurve->FloatCurve.GetConstRefOfKeys(), bShouldTransact);
			TargetSeqController.SetCurveColor(TargetCurveID, SourceCurve->GetColor(), bShouldTransact);
		}

		if (!bCopyAllSourceCurves)
		{
			TArray<FAnimationCurveIdentifier> TargetCurvesToKeep;
			CurvesToRemap.GenerateValueArray(TargetCurvesToKeep);

			const TArray<FFloatCurve>& AllTargetCurves = TargetSeqController.GetModel()->GetFloatCurves();
			TArray<FAnimationCurveIdentifier> CurvesToRemove;
			for (const FFloatCurve& TargetCurve : AllTargetCurves)
			{
				const FAnimationCurveIdentifier TargetCurveId = UAnimationCurveIdentifierExtensions::FindCurveIdentifier(TargetSkeleton, TargetCurve.GetName(), ERawCurveTrackTypes::RCT_Float);
				if (TargetCurvesToKeep.Contains(TargetCurveId))
				{
					continue;
				}
				CurvesToRemove.Add(TargetCurveId);
			}

			for (const FAnimationCurveIdentifier& CurveToRemove : CurvesToRemove)
			{
				TargetSeqController.RemoveCurve(CurveToRemove, bShouldTransact);
			}
		}

		TargetSeqController.CloseBracket(bShouldTransact);
	}
}

void UAAT_IKRetargetBatchOperation::OverwriteExistingAssets(const FAAT_IKRetargetBatchOperationContext& Context, FScopedSlowTask& Progress)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

	for (TPair<UAnimationAsset*, UAnimationAsset*>& Pair : DuplicatedAnimAssets)
	{
		UAnimationAsset* OldAsset = Pair.Key;
		UAnimationAsset* NewAsset = Pair.Value;

		if (!OldAsset || !NewAsset) continue;

		FString DesiredObjectName = Context.NameRule.Rename(OldAsset);
		FString PathName = FPackageName::GetLongPackagePath(NewAsset->GetPathName());
		FString DesiredPackagePath = PathName;
		FString DesiredObjectPath = DesiredPackagePath + TEXT(".") + DesiredObjectName;

		if (Context.bReplaceReferences)
		{
			TArray<UObject*> ReplaceThese = { OldAsset };
			UE_LOG(LogTemp, Log, TEXT("[OverwriteExistingAssets] Replacing references to %s with %s"), *OldAsset->GetPathName(), *NewAsset->GetPathName());
			ObjectTools::ForceReplaceReferences(NewAsset, ReplaceThese);
		}

		FAssetData AssetDataAtDesired = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(DesiredObjectPath));
		if (AssetDataAtDesired.IsValid())
		{
			UObject* AssetAtDesired = AssetDataAtDesired.GetAsset();
			if (AssetAtDesired && AssetAtDesired != NewAsset)
			{
				UE_LOG(LogTemp, Log, TEXT("[OverwriteExistingAssets] Deleting duplicate asset at desired path: %s"), *AssetAtDesired->GetPathName());
				ObjectTools::ForceDeleteObjects({ AssetAtDesired }, false);
			}
		}

		if (NewAsset->GetName() != DesiredObjectName || FPackageName::GetLongPackagePath(NewAsset->GetPathName()) != DesiredPackagePath)
		{
			UE_LOG(LogTemp, Log, TEXT("[OverwriteExistingAssets] Renaming %s -> %s/%s"), *NewAsset->GetPathName(), *DesiredPackagePath, *DesiredObjectName);
			TArray<FAssetRenameData> AssetsToRename;
			AssetsToRename.Emplace(NewAsset, DesiredPackagePath, DesiredObjectName);
			AssetToolsModule.Get().RenameAssets(AssetsToRename);
		}

		UPackage* NewPkg = NewAsset->GetOutermost();
		if (NewPkg)
		{
			NewPkg->MarkPackageDirty();
			UEditorLoadingAndSavingUtils::SavePackages({ NewPkg }, false);
		}
	}
}

void UAAT_IKRetargetBatchOperation::NotifyUserOfResults(
	const FAAT_IKRetargetBatchOperationContext& Context,
	FScopedSlowTask& Progress) const
{
	TArray<UObject*> NewAssets;
	GetNewAssets(NewAssets);

	TArray<FAssetData> CurrentSelection;
	for(UObject* NewObject : NewAssets)
	{
		CurrentSelection.Add(FAssetData(NewObject));
	}
	const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets(CurrentSelection);

	constexpr float NotificationDuration = 5.0f;
	if (Progress.ShouldCancel())
	{
		Progress.EnterProgressFrame(1.f, FText(LOCTEXT("CancelledBatchRetarget", "Cancelled.")));

		FNotificationInfo Notification(FText::GetEmpty());
		Notification.ExpireDuration = NotificationDuration;
		Notification.Text = FText(LOCTEXT("BatchRetargetCancelled", "Batch retarget cancelled."));
		FSlateNotificationManager::Get().AddNotification(Notification);
	}
	else
	{
		Progress.EnterProgressFrame(1.f, FText(LOCTEXT("DoneBatchRetarget", "Duplicate and retarget complete!")));

		for (const UObject* NewAsset : NewAssets)
		{
			UE_LOG(LogTemp, Display, TEXT("Duplicate and Retarget - New Asset Created: %s"), *NewAsset->GetName());
		}

		FNotificationInfo Notification(FText::GetEmpty());
		Notification.ExpireDuration = NotificationDuration;
		Notification.Text = FText::Format(
			LOCTEXT("MultiNonDuplicatedAsset", "{0} assets were retargeted to new skeleton {1}. See Output for details."),
			FText::AsNumber(NewAssets.Num()),
			FText::FromString(Context.TargetMesh->GetName()));
		FSlateNotificationManager::Get().AddNotification(Notification);
	}
}

void UAAT_IKRetargetBatchOperation::GetNewAssets(TArray<UObject*>& NewAssets) const
{
	TArray<UAnimationAsset*> NewAnims;
	DuplicatedAnimAssets.GenerateValueArray(NewAnims);
	for (UAnimationAsset* NewAnim : NewAnims)
	{
		NewAssets.Add(Cast<UObject>(NewAnim));
	}

	TArray<UAnimBlueprint*> NewBlueprints;
	DuplicatedBlueprints.GenerateValueArray(NewBlueprints);
	for (UAnimBlueprint* NewBlueprint : NewBlueprints)
	{
		NewAssets.Add(Cast<UObject>(NewBlueprint));
	}
}

void UAAT_IKRetargetBatchOperation::CleanupIfCancelled(const FScopedSlowTask& Progress) const
{
	if (!Progress.ShouldCancel())
	{
		return;
	}

	TArray<UObject*> NewAssets;
	GetNewAssets(NewAssets);

	constexpr bool bShowConfirmation = true;
	ObjectTools::DeleteObjects(NewAssets, bShowConfirmation);
}

TArray<FAssetData> UAAT_IKRetargetBatchOperation::DuplicateAndRetarget(
	const TArray<FAssetData>& AssetsToRetarget,
	USkeletalMesh* SourceMesh,
	USkeletalMesh* TargetMesh,
	UIKRetargeter* IKRetargetAsset,
	const FString& Search,
	const FString& Replace,
	const FString& Prefix,
	const FString& Suffix,
	const bool bIncludeReferencedAssets)
{
	FAAT_IKRetargetBatchOperationContext Context;
	for (const FAssetData& Asset : AssetsToRetarget)
	{
		if (UObject* Object = Cast<UObject>(Asset.GetAsset()))
		{
			Context.AssetsToRetarget.Add(Object);
		}
	}
	Context.SourceMesh = SourceMesh;
	Context.TargetMesh = TargetMesh;
	Context.IKRetargetAsset = IKRetargetAsset;
	Context.NameRule.Prefix = Prefix;
	Context.NameRule.Suffix = Suffix;
	Context.NameRule.ReplaceFrom = Search;
	Context.NameRule.ReplaceTo = Replace;
	Context.bIncludeReferencedAssets = bIncludeReferencedAssets;

	UAAT_IKRetargetBatchOperation* BatchOperation = NewObject<UAAT_IKRetargetBatchOperation>();
	BatchOperation->AddToRoot();
	BatchOperation->RunRetarget(Context);

	TArray<FAssetData> Results;
	for (const UAnimationAsset* RetargetedAsset : BatchOperation->AnimationAssetsToRetarget)
	{
		Results.Add(FAssetData(RetargetedAsset));
	}
	
	BatchOperation->RemoveFromRoot();
	return Results;
}

void UAAT_IKRetargetBatchOperation::RunRetarget(FAAT_IKRetargetBatchOperationContext& Context)
{
	Reset();

	const int32 NumAssets = GenerateAssetLists(Context);
	if (NumAssets == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Batch retarget aborted. No animation assets were specified."));
		return;
	}

	if (!Context.IKRetargetAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("Batch retarget aborted. No IK Retargeter asset was specified."));
		return;
	}

	const UIKRigDefinition* SrcIKRig = Context.IKRetargetAsset->GetIKRig(ERetargetSourceOrTarget::Source);
	if (!SrcIKRig)
	{
		UE_LOG(LogTemp, Warning, TEXT("Batch retarget aborted. Specified IK Retargeter does not reference a source IK Rig."));
		return;
	}

	const UIKRigDefinition* TgtIKRig = Context.IKRetargetAsset->GetIKRig(ERetargetSourceOrTarget::Target);
	if (!TgtIKRig)
	{
		UE_LOG(LogTemp, Warning, TEXT("Batch retarget aborted. Specified IK Retargeter does not reference a target IK Rig."));
		return;
	}

	if (!Context.SourceMesh)
	{
		Context.SourceMesh = SrcIKRig->GetPreviewMesh();
		if (!Context.SourceMesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("Batch retarget aborted. No source mesh was specified and the source IK Rig did not have one. "));
			return;	
		}
	}

	if (!Context.TargetMesh)
	{
		Context.TargetMesh = TgtIKRig->GetPreviewMesh();
		if (!Context.TargetMesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("Batch retarget aborted. No target mesh was specified and the target IK Rig did not have one. "));
			return;
		}
	}

	constexpr int32 NumAdditionalProgressFrames = 4;
	constexpr int32 NumPassesOverAssets = 3;
	const int32 NumProgressSteps = (NumAssets * NumPassesOverAssets) + NumAdditionalProgressFrames;
	FScopedSlowTask Progress(static_cast<float>(NumProgressSteps), LOCTEXT("GatheringBatchRetarget", "Gathering animation assets..."));
	constexpr bool bShowCancelButton = true;
	Progress.MakeDialog(bShowCancelButton);
	
	DuplicateRetargetAssets(Context, Progress);
	RetargetAssets(Context, Progress);
	OverwriteExistingAssets(Context, Progress);
	NotifyUserOfResults(Context, Progress);
	CleanupIfCancelled(Progress);
}

void UAAT_IKRetargetBatchOperation::Reset()
{
	AnimationAssetsToRetarget.Reset();
	AnimBlueprintsToRetarget.Reset();
	DuplicatedAnimAssets.Reset();
	DuplicatedBlueprints.Reset();
	RemappedAnimAssets.Reset();
}

void FAAT_AdditiveRetargetSettings::PrepareForRetarget(UAnimSequence* InSequenceAsset)
{
	if (!ensure(InSequenceAsset))
	{
		return;
	}

	SequenceAsset = InSequenceAsset;

	AdditiveAnimType = SequenceAsset->AdditiveAnimType;
	RefPoseType = SequenceAsset->RefPoseType;
	RefFrameIndex = SequenceAsset->RefFrameIndex;
	RefPoseSeq = SequenceAsset->RefPoseSeq;

	SequenceAsset->AdditiveAnimType = EAdditiveAnimationType::AAT_None;
	SequenceAsset->RefPoseType = EAdditiveBasePoseType::ABPT_None;
	SequenceAsset->RefFrameIndex = 0;
	SequenceAsset->RefPoseSeq = nullptr;
}

void FAAT_AdditiveRetargetSettings::RestoreOnAsset() const
{
	if (!ensure(SequenceAsset))
	{
		return;
	}
		
	SequenceAsset->AdditiveAnimType = AdditiveAnimType;
	SequenceAsset->RefPoseType = RefPoseType;
	SequenceAsset->RefFrameIndex = RefFrameIndex;
	SequenceAsset->RefPoseSeq = RefPoseSeq;
}

#undef LOCTEXT_NAMESPACE
