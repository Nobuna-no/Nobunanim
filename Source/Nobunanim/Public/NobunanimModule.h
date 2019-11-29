// Copyright 2017 Google Inc.

#pragma once

#include "ModuleManager.h"

class FNobunanimModule : public IModuleInterface
{
	public:
		virtual void StartupModule() override;
		virtual void ShutdownModule() override;
};
