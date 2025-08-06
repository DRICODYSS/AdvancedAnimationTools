// Copyright DRICODYSS. All Rights Reserved.

#include "RetargetAnimations/Widgets//SRetargetAnimationsOutputLog.h"

#include "IMessageLogListing.h"
#include "MessageLogModule.h"

void SRetargetAnimationsOutputLog::Construct(
	const FArguments& InArgs,
	const FName& InLogName
)
{
	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	ensure(MessageLogModule.IsRegisteredLogListing(InLogName));

	MessageLogListing = MessageLogModule.GetLogListing(InLogName);
	OutputLogWidget = MessageLogModule.CreateLogListingWidget( MessageLogListing.ToSharedRef() );
	
	ChildSlot
	[
		SNew(SBox)
		[
			OutputLogWidget.ToSharedRef()
		]
	];
}

void SRetargetAnimationsOutputLog::ClearLog() const
{
	MessageLogListing.Get()->ClearMessages();
}
