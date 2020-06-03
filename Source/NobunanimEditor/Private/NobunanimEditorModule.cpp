// Copyright 2017 Google Inc.

#include "NobunanimEditorModule.h"
#include "NobunanimEditor.h"
#include "Textures/SlateIcon.h"
#include "SafeCCDIKEditMode.h"

DEFINE_LOG_CATEGORY(logNobunanimEditor)

#define LOCTEXT_NAMESPACE "FNobunanimEditorModule"

void FNobunanimEditorModule::StartupModule()
{
	FEditorModeRegistry::Get().RegisterMode<FSafeCCDIKEditMode>("AnimGraph.SkeletalControl.SafeCCDIK", LOCTEXT("FSafeCCDIKEditMode", "Safe CCDIK"), FSlateIcon(), false);
}

void FNobunanimEditorModule::ShutdownModule()
{
	FEditorModeRegistry::Get().UnregisterMode("AnimGraph.SkeletalControl.SafeCCDIK");
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNobunanimEditorModule, NobunanimEditor)
