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

		/** Animation frame number. 60 frame = 1 second. */
		UPROPERTY(Category = "[NOBUNANIM]|Gait Data", EditAnywhere, BlueprintReadOnly)
		int AnimationFrameCount = 60;

		/** Animation target frame per second. It's the refresh rate of the animation. 
		*i.e:
		* - AnimationFrameCount(60) &&  TargetFPS(60) = 1 update per frame.
		* - AnimationFrameCount(60) &&  TargetFPS(30) = 1 update every 2 frames. */
		//UPROPERTY(Category = "[NOBUNANIM]|Gait Data", EditAnywhere, BlueprintReadOnly)
		//int TargetFramePerSecond = 60;
		
		/** Should this gait be played only if the velocity isn't zero. */
		UPROPERTY(Category = "[NOBUNANIM]|Gait Data", EditAnywhere, BlueprintReadOnly)
		bool bComputeWithVelocityOnly = true;

	public:
		/** Return the ratio according to the animation framecount. (1 sec = 60frames). */
		UFUNCTION(Category = "[NOBUNANIM]|Gait Data", BlueprintCallable)
		float GetFrameRatio()
		{
			return 60.f / (float)AnimationFrameCount /*/ (float)TargetFramePerSecond*/;
		}
};
