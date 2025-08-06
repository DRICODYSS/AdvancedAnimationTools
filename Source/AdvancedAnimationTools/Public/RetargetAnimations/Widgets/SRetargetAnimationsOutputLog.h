// Copyright DRICODYSS. All Rights Reserved.

#pragma once

#include "Widgets/Layout/SBox.h"

struct FIKRigLogger;
class IMessageLogListing;
class FIKRetargetEditorController;

class SRetargetAnimationsOutputLog : public SBox
{
	TSharedPtr<SWidget> OutputLogWidget;
	TSharedPtr<IMessageLogListing> MessageLogListing;

public:
	
	SLATE_BEGIN_ARGS(SRetargetAnimationsOutputLog) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const FName& InLogName);

	void ClearLog() const;
};
