// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ProceduralAnimationEditorCommands.h"

#define LOCTEXT_NAMESPACE "FProceduralAnimationEditorModule"

void FProceduralAnimationEditorCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "Procedural Animation Editor", "Bring up Procedural Animation editor window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
