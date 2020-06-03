// Copyright 2017 Google Inc.

#include "NobunanimModule.h"
#include "Nobunanim.h"

DEFINE_LOG_CATEGORY(logNobunanim)

#define LOCTEXT_NAMESPACE "FNobunanimModule"

void FNobunanimModule::StartupModule()
{
}

void FNobunanimModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNobunanimModule, Nobunanim)
