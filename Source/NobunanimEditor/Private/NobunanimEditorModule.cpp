// Copyright 2017 Google Inc.

#include "NobunanimEditorModule.h"
#include "Private/NobunanimEditor.h"

DEFINE_LOG_CATEGORY(logNobunanimEditor)

#define LOCTEXT_NAMESPACE "FNobunanimEditorModule"

void FNobunanimEditorModule::StartupModule()
{
}

void FNobunanimEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNobunanimEditorModule, NobunanimEditor)
