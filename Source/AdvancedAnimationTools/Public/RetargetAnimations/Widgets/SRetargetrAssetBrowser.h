// Copyright DRICODYSS. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "Widgets/SCompoundWidget.h"

class SRetargetAnimsWidget;

class SRetargetAssetBrowser : public SBox
{
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;
	TSharedPtr<SRetargetAnimsWidget> RetargetWindow;
	TSharedPtr<SBox> AssetBrowserBox;

public:
	SLATE_BEGIN_ARGS(SRetargetAssetBrowser) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<SRetargetAnimsWidget> RetargetWindow);
	void RefreshView();
	void GetSelectedAssets(TArray<FAssetData>& OutSelectedAssets) const;
	bool AreAnyAssetsSelected() const;
	
private:
	
	void OnAssetDoubleClicked(const FAssetData& AssetData);
	bool OnShouldFilterAsset(const struct FAssetData& AssetData);
};
