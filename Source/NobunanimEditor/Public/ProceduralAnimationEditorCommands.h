// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "ProceduralAnimationEditorStyle.h"

class FProceduralAnimationEditorCommands : public TCommands<FProceduralAnimationEditorCommands>
{
public:

	FProceduralAnimationEditorCommands()
		: TCommands<FProceduralAnimationEditorCommands>(TEXT("ProceduralAnimationEditor"), NSLOCTEXT("Contexts", "ProceduralAnimationEditor", "ProceduralAnimationEditor Plugin"), NAME_None, FProceduralAnimationEditorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};