// Fill out your copyright notice in the Description page of Project Settings.

#include "DebugPlayerController.h"

#include "GameFramework/Pawn.h"

#include "GaitController.h"


bool ADebugPlayerController::ProcessConsoleExec(const TCHAR* _Cmd, FOutputDevice& _Ar, UObject* _Executor)
{
	bool handled = Super::ProcessConsoleExec(_Cmd, _Ar, _Executor);

	if (!handled)
	{
		TArray<UGaitController*> Children;
		GetPawn()->GetComponents<UGaitController>(Children);
		if (Children.Num())
		{
			UGaitController* GaitComponent = Children[0];
			handled &= GaitComponent->ProcessConsoleExec(_Cmd, _Ar, _Executor);
		}
	}

	return handled;
}
