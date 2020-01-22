// Fill out your copyright notice in the Description page of Project Settings.

#include "DebugPlayerController.h"

#include "GameFramework/Pawn.h"

#include "ProceduralGaitControllerComponent.h"


bool ADebugPlayerController::ProcessConsoleExec(const TCHAR* _Cmd, FOutputDevice& _Ar, UObject* _Executor)
{
	bool handled = Super::ProcessConsoleExec(_Cmd, _Ar, _Executor);

	if (!handled)
	{
		TArray<UProceduralGaitControllerComponent*> Children;
		GetPawn()->GetComponents<UProceduralGaitControllerComponent>(Children);
		if (Children.Num())
		{
			UProceduralGaitControllerComponent* GaitComponent = Children[0];
			handled &= GaitComponent->ProcessConsoleExec(_Cmd, _Ar, _Executor);
		}
	}

	return handled;
}
