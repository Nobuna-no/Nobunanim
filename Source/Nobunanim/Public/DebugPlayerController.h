// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DebugPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class NOBUNANIM_API ADebugPlayerController : public APlayerController
{
	GENERATED_BODY()

	public:
		virtual bool ProcessConsoleExec(const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor) override;

};
