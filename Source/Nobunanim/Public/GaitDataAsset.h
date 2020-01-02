// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Engine/DataAsset.h>
#include <Runtime/Core/Public/Containers/Map.h>

#include "Nobunanim/Public/ProceduralGaitInterface.h"

#include "GaitDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class NOBUNANIM_API UGaitDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
		UPROPERTY(Category = "[NOBUNANIM]|Gait Data", EditAnywhere, BlueprintReadOnly)
		TMap<FName, FGaitSwingData> GaitSwingValues;

		UPROPERTY(Category = "[NOBUNANIM]|Gait Data", EditAnywhere, BlueprintReadOnly)
		int TargetFramePerSecond = 60;

		UPROPERTY(Category = "[NOBUNANIM]|Gait Data", EditAnywhere, BlueprintReadOnly)
		int AnimationFrameCount = 60;


	public:
		UFUNCTION(Category = "[NOBUNANIM]|Gait Data", BlueprintCallable)
		float GetFrameRatio()
		{
			return TargetFramePerSecond / AnimationFrameCount;
		}
};
