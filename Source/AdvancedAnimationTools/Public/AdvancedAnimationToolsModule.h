// Copyright DRICODYSS. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FAdvancedAnimationToolsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();
	void FillSubMenu(UToolMenu* Menu);
};
