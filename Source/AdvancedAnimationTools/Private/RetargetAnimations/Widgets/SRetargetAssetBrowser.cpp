// Copyright DRICODYSS. All Rights Reserved.

#include "RetargetAnimations/Widgets/SRetargetrAssetBrowser.h"

#include "ContentBrowserDataSource.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Animation/AimOffsetBlendSpace.h"
#include "Animation/AimOffsetBlendSpace1D.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/PoseAsset.h"
#include "RetargetAnimations/RetargetAnimsWidgetSettings.h"
#include "Retargeter/IKRetargeter.h"
#include "UObject/AssetRegistryTagsContext.h"

void SRetargetAssetBrowser::Construct(const FArguments& InArgs, const TSharedRef<SRetargetAnimsWidget> InRetargetWindow)
{
	RetargetWindow = InRetargetWindow;
	
	ChildSlot
	[
		SAssignNew(AssetBrowserBox, SBox)
	];

	RefreshView();
}

void SRetargetAssetBrowser::RefreshView()
{
	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter.ClassPaths.Add(UAnimBlueprint::StaticClass()->GetClassPathName());
	AssetPickerConfig.Filter.ClassPaths.Add(UAnimSequence::StaticClass()->GetClassPathName());
	AssetPickerConfig.Filter.ClassPaths.Add(UAnimMontage::StaticClass()->GetClassPathName());
	AssetPickerConfig.Filter.ClassPaths.Add(UPoseAsset::StaticClass()->GetClassPathName());
	AssetPickerConfig.Filter.ClassPaths.Add(UBlendSpace::StaticClass()->GetClassPathName());
	AssetPickerConfig.Filter.ClassPaths.Add(UBlendSpace1D::StaticClass()->GetClassPathName());
	AssetPickerConfig.Filter.ClassPaths.Add(UAimOffsetBlendSpace::StaticClass()->GetClassPathName());
	AssetPickerConfig.Filter.ClassPaths.Add(UAimOffsetBlendSpace1D::StaticClass()->GetClassPathName());
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Column;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.DefaultFilterMenuExpansion = EAssetTypeCategories::Animation;
	AssetPickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateSP(this, &SRetargetAssetBrowser::OnShouldFilterAsset);
	AssetPickerConfig.OnAssetDoubleClicked = FOnAssetSelected::CreateSP(this, &SRetargetAssetBrowser::OnAssetDoubleClicked);
	AssetPickerConfig.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.bFocusSearchBoxWhenOpened = false;
	AssetPickerConfig.bShowPathInColumnView = false;
	AssetPickerConfig.bShowTypeInColumnView = false;
	AssetPickerConfig.HiddenColumnNames.Add(ContentBrowserItemAttributes::ItemDiskSize.ToString());
	AssetPickerConfig.HiddenColumnNames.Add(ContentBrowserItemAttributes::VirtualizedData.ToString());
	AssetPickerConfig.HiddenColumnNames.Add(TEXT("Path"));
	AssetPickerConfig.HiddenColumnNames.Add(TEXT("Class"));
	AssetPickerConfig.HiddenColumnNames.Add(TEXT("RevisionControl"));

	UObject* AnimSequenceDefaultObject = UAnimSequence::StaticClass()->GetDefaultObject();
	FAssetRegistryTagsContextData TagsContext(AnimSequenceDefaultObject, EAssetRegistryTagsCaller::Uncategorized);
	AnimSequenceDefaultObject->GetAssetRegistryTags(TagsContext);
	for (const TPair<FName, UObject::FAssetRegistryTag>& TagPair : TagsContext.Tags)
	{
		AssetPickerConfig.HiddenColumnNames.Add(TagPair.Key.ToString());
	}

	AssetPickerConfig.HiddenColumnNames.Add(TEXT("Class"));

	const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	AssetBrowserBox->SetContent(ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig));
}

void SRetargetAssetBrowser::GetSelectedAssets(TArray<FAssetData>& OutSelectedAssets) const
{
	OutSelectedAssets = GetCurrentSelectionDelegate.Execute();
}

bool SRetargetAssetBrowser::AreAnyAssetsSelected() const
{
	return !GetCurrentSelectionDelegate.Execute().IsEmpty();
}

void SRetargetAssetBrowser::OnAssetDoubleClicked(const FAssetData& AssetData)
{
	if (!AssetData.GetAsset())
	{
		return;
	}

	UAnimationAsset* NewAnimationAsset = Cast<UAnimationAsset>(AssetData.GetAsset());
	if (!NewAnimationAsset)
	{
		return;
	}
}

bool SRetargetAssetBrowser::OnShouldFilterAsset(const FAssetData& AssetData)
{
	const bool bIsAnimAsset = AssetData.IsInstanceOf(UAnimationAsset::StaticClass());
	const bool bIsAnimBlueprint = AssetData.IsInstanceOf(UAnimBlueprint::StaticClass());
	if (!(bIsAnimAsset || bIsAnimBlueprint))
	{
		return true;
	}
	
	const TObjectPtr<URetargetAnimsWidgetSettings> BatchRetargetSettings = URetargetAnimsWidgetSettings::GetInstance();
	if (!ensure(BatchRetargetSettings) || !BatchRetargetSettings->RetargetAsset)
	{
		return true;
	}
	
	auto SourceMesh = BatchRetargetSettings->RetargetAsset->GetIKRig(ERetargetSourceOrTarget::Source)->GetPreviewMesh();
	if (!SourceMesh)
	{
		return true;
	}

	const USkeleton* DesiredSkeleton = SourceMesh->GetSkeleton();
	if (!DesiredSkeleton)
	{
		return true;
	}

	if (bIsAnimBlueprint)
	{
		const FAssetDataTagMapSharedView::FFindTagResult Result = AssetData.TagsAndValues.FindTag("TargetSkeleton");
		if (!Result.IsSet())
		{
			return true;
		}

		return !DesiredSkeleton->IsCompatibleForEditor(Result.GetValue());
	}

	return !DesiredSkeleton->IsCompatibleForEditor(AssetData);
}
