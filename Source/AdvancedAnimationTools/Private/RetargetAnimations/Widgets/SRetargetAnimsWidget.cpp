// Copyright DRICODYSS. All Rights Reserved.

#include "RetargetAnimations/Widgets//SRetargetAnimsWidget.h"

#include "SWarningOrErrorBox.h"
#include "RetargetAnimations/RetargetAnimsWidgetSettings.h"
#include "RetargetAnimations/Widgets/SRetargetrAssetBrowser.h"
#include "Retargeter/IKRetargeter.h"
#include "Widgets/Layout/SUniformGridPanel.h"

SRetargetAnimsWidget::SRetargetAnimsWidget()
{
	Settings = URetargetAnimsWidgetSettings::GetInstance();
	//Log.SetLogTarget(nullptr);
}

TSharedPtr<SWindow> SRetargetAnimsWidget::Window;

void SRetargetAnimsWidget::Construct(const FArguments& InArgs)
{
	FDetailsViewArgs GridDetailsViewArgs;
	GridDetailsViewArgs.bAllowSearch = false;
	GridDetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	GridDetailsViewArgs.bHideSelectionTip = true;
	GridDetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Automatic;
	GridDetailsViewArgs.bShowOptions = false;
	GridDetailsViewArgs.bAllowMultipleTopLevelObjects = false;
	
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(GridDetailsViewArgs);
	DetailsView->SetObject(Settings);
	DetailsView->OnFinishedChangingProperties().AddSP(this, &SRetargetAnimsWidget::OnFinishedChangingSelectionProperties);
	
	ChildSlot
	[
		SNew (SHorizontalBox)
		
		+SHorizontalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.FillWidth(1.f)
		[
		
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)
			// + SSplitter::Slot()
			// .Value(0.6f)
			// [
			// 	SNew(SSplitter)
			// 	.Orientation(Orient_Vertical)
			// 	+ SSplitter::Slot()
			// 	.Value(0.2f)
			// 	[
			// 		SAssignNew(LogView, SRetargetAnimationsOutputLog, Log.GetLogTarget())
			// 	]
			// ]
			+ SSplitter::Slot()
			.Value(0.4f)
			[
			
				SNew(SSplitter)
				.Orientation(Orient_Vertical)
				+ SSplitter::Slot()
				.Value(0.4f)
				[
					DetailsView
				]
				+ SSplitter::Slot()
				.Value(0.6f)
				[
					SNew(SVerticalBox)
					
					+SVerticalBox::Slot()
					[
						SAssignNew(AssetBrowser, SRetargetAssetBrowser, SharedThis(this))
					]
			
					+SVerticalBox::Slot()
					.Padding(2)
					.AutoHeight()
					[
						SNew(SWarningOrErrorBox)
						.Visibility_Lambda([this]{return GetWarningVisibility(); })
						.MessageStyle(EMessageStyle::Warning)
						.Message_Lambda([this] { return GetWarningText(); })
					]
					
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2)
					[
						SNew(SUniformGridPanel)
						.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
						.MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
						.MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
						// +SUniformGridPanel::Slot(0, 0)
						// [
						// 	SNew(SButton).HAlign(HAlign_Center)
						// 	.Text(FText::FromString("Export Retarget Assets"))
						// 	.IsEnabled(this, &SRetargetAnimsWidget::CanExportRetargetAssets)
						// 	.OnClicked(this, &SRetargetAnimsWidget::OnExportRetargetAssets)
						// 	.HAlign(HAlign_Center)
						// 	.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
						// ]
						+SUniformGridPanel::Slot(1, 0)
						[
							SNew(SButton).HAlign(HAlign_Center)
							.Text(FText::FromString("Export Animations"))
							.IsEnabled(this, &SRetargetAnimsWidget::CanExportAnimations)
							.OnClicked(this, &SRetargetAnimsWidget::OnExportAnimations)
							.HAlign(HAlign_Center)
							.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
						]	
					]
				]
			]
		]
	];

	//LogView.Get()->ClearLog();
}

void SRetargetAnimsWidget::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Settings);
}

void SRetargetAnimsWidget::OnFinishedChangingSelectionProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (!PropertyChangedEvent.Property)
	{
		return;
	}

	const bool ChangedRetargeter = PropertyChangedEvent.Property->GetName() == "RetargetAsset";
	if (ChangedRetargeter)
	{
		SetAssets(Settings->RetargetAsset, Settings->Path, Settings->RootFolderName, false);
	}

	const bool ChangedPath = PropertyChangedEvent.Property->GetName() == "Path";
	if (ChangedPath)
	{
		SetAssets(Settings->RetargetAsset, Settings->Path, Settings->RootFolderName, false);
	}

	const bool ChangedRootFolderName = PropertyChangedEvent.Property->GetName() == "RootFolderName";
	if (ChangedRootFolderName)
	{
		SetAssets(Settings->RetargetAsset, Settings->Path, Settings->RootFolderName, false);
	}

	// const bool ChangedBindDependenciesToNewAssets = PropertyChangedEvent.Property->GetName() == "bBindDependenciesToNewAssets";
	// if (ChangedBindDependenciesToNewAssets)
	// {
	// 	SetAssets(Settings->RetargetAsset, Settings->Path, Settings->RootFolderName, false);
	// }
}

bool SRetargetAnimsWidget::CanExportAnimations() const
{
	return GetCurrentState() == ERetargetAnimsWidgetState::READY_TO_EXPORT;
}

FReply SRetargetAnimsWidget::OnExportAnimations()
{
	TArray<FAssetData> SelectedAssets;
	AssetBrowser->GetSelectedAssets(SelectedAssets);
	BatchContext.AssetsToRetarget.Reset();
	for (const FAssetData& Asset : SelectedAssets)
	{
		if (UObject* AssetObject = Cast<UObject>(Asset.GetAsset()))
		{
			BatchContext.AssetsToRetarget.Add(AssetObject);	
		}
	}

	const TStrongObjectPtr<UAAT_IKRetargetBatchOperation> BatchOperation(NewObject<UAAT_IKRetargetBatchOperation>());
	BatchOperation->RunRetarget(BatchContext);
	
	return FReply::Handled();
}

ERetargetAnimsWidgetState SRetargetAnimsWidget::GetCurrentState() const
{
	if (!AssetBrowser->AreAnyAssetsSelected())
	{
		return ERetargetAnimsWidgetState::NO_ANIMATIONS_SELECTED;
	}

	return ERetargetAnimsWidgetState::READY_TO_EXPORT;
}

EVisibility SRetargetAnimsWidget::GetWarningVisibility() const
{
	return GetCurrentState() == ERetargetAnimsWidgetState::READY_TO_EXPORT ? EVisibility::Collapsed : EVisibility::Visible;
}

FText SRetargetAnimsWidget::GetWarningText() const
{
	ERetargetAnimsWidgetState CurrentState = GetCurrentState();
	switch (CurrentState)
	{
	case ERetargetAnimsWidgetState::MANUAL_RETARGET_INVALID:
		return FText::FromString("User supplied retargeter was invalid. See output for details.");
	case ERetargetAnimsWidgetState::NO_ANIMATIONS_SELECTED:
		return FText::FromString("Ready to export! Select animations to export.");
	case ERetargetAnimsWidgetState::READY_TO_EXPORT:
		return FText::GetEmpty(); // message hidden when warnings are all dealt with
	default:
		checkNoEntry();
	};

	return FText::GetEmpty();
}

void SRetargetAnimsWidget::ShowWindow()
{	
	if(Window.IsValid())
	{
		FSlateApplication::Get().DestroyWindowImmediately(Window.ToSharedRef());
	}
	
	Window = SNew(SWindow)
		.Title(FText::FromString("Retarget Animations"))
		.SupportsMinimize(true)
		.SupportsMaximize(true)
		.HasCloseButton(true)
		.IsTopmostWindow(false)
		.ClientSize(FVector2D(1280, 720))
		.SizingRule(ESizingRule::UserSized);
	
	TSharedPtr<SRetargetAnimsWidget> DialogWidget;
	TSharedPtr<SBorder> DialogWrapper =
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(4.0f)
		[
			SAssignNew(DialogWidget, SRetargetAnimsWidget)
		];
	Window->SetOnWindowClosed(FOnWindowClosed::CreateLambda([](const TSharedRef<SWindow>&){Window = nullptr;}));
	Window->SetContent(DialogWrapper.ToSharedRef());

	FSlateApplication::Get().AddWindow(Window.ToSharedRef());
}

void SRetargetAnimsWidget::SetAssets(UIKRetargeter* Retargeter, FName Path, FName RootFolderName, bool bBindDependenciesToNewAssets)
{
	//LogView->ClearLog();

	auto SourceMesh = Retargeter && Retargeter->GetIKRig(ERetargetSourceOrTarget::Source) ? Retargeter->GetIKRig(ERetargetSourceOrTarget::Source)->GetPreviewMesh() : nullptr;
	auto TargetMesh = Retargeter && Retargeter->GetIKRig(ERetargetSourceOrTarget::Target) ? Retargeter->GetIKRig(ERetargetSourceOrTarget::Target)->GetPreviewMesh() : nullptr;
	const bool ReplacedSourceMesh = BatchContext.SourceMesh != SourceMesh;

	Settings->RetargetAsset = Retargeter;
	
	BatchContext.SourceMesh = SourceMesh;
	BatchContext.TargetMesh = TargetMesh;
	BatchContext.IKRetargetAsset = Retargeter;

	BatchContext.TargetMesh = TargetMesh;

	BatchContext.Path = Path.ToString();
	BatchContext.RootFolderName = RootFolderName.ToString();
	BatchContext.bBindDependenciesToNewAssets = bBindDependenciesToNewAssets;

	// Viewport->SetSkeletalMesh(SourceMesh, ERetargetSourceOrTarget::Source);
	// Viewport->SetSkeletalMesh(TargetMesh, ERetargetSourceOrTarget::Target);
	// Viewport->SetRetargetAsset(BatchContext.IKRetargetAsset);

	ShowAssetWarnings();
	
	if (ReplacedSourceMesh)
	{
		AssetBrowser.Get()->RefreshView();
	}
}

void SRetargetAnimsWidget::ShowAssetWarnings()
{
	if (!Settings->RetargetAsset)
	{
		Log.LogError(FText::FromString("Not using auto-generated retargeter and no IK Retargeter asset was provided."));
	}
}
