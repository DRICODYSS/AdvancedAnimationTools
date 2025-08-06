// Copyright DRICODYSS. All Rights Reserved.

#include "AdvancedAnimationToolsModule.h"

#include "RetargetAnimations/Widgets/SRetargetAnimsWidget.h"

#define LOCTEXT_NAMESPACE "AdvancedAnimationToolsModule"

void FAdvancedAnimationToolsModule::StartupModule()
{
	RegisterMenus();
}

void FAdvancedAnimationToolsModule::ShutdownModule()
{
	UToolMenus::UnregisterOwner(this);
}

void FAdvancedAnimationToolsModule::RegisterMenus()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	if (!Menu) return;

	Menu->AddSection("AdvancedAnimationToolsSection", LOCTEXT("AdvancedAnimationToolsSectionLabel", "Advanced Animation Tools"))
		.AddSubMenu(
			"OpenAdvancedAnimationToolsSubMenu",
			LOCTEXT("AdvancedAnimationToolsSubMenuLabel", "Advanced Animation Tools"),
			LOCTEXT("AdvancedAnimationToolsSubMenuTooltip", "Open Advanced Animation Tools"),
			FNewToolMenuDelegate::CreateRaw(this, &FAdvancedAnimationToolsModule::FillSubMenu),
			false,
			FSlateIcon()
		);
}

void FAdvancedAnimationToolsModule::FillSubMenu(UToolMenu* Menu)
{
	FToolMenuEntry RetargetEntry = FToolMenuEntry::InitMenuEntry(
		"RetargetAnimations",
		LOCTEXT("RetargetAnimationsLabel", "Retarget Animations"),
		LOCTEXT("RetargetAnimationsTooltip", "Open Retarget Animations tool"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&SRetargetAnimsWidget::ShowWindow))
	);
	Menu->AddMenuEntry("AdvancedAnimationToolsSection", RetargetEntry);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAdvancedAnimationToolsModule, AdvancedAnimationTools)
