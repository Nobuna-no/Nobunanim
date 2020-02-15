// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Engine/DataAsset.h>
#include <Runtime/Core/Public/Containers/Map.h>

#include "Nobunanim/Public/ProceduralGaitInterface.h"

#include "GaitDataAsset.generated.h"


/**
*/
USTRUCT(BlueprintType)
struct FGaitGroundReflectionPlaneData
{
	GENERATED_BODY()

public:
	/** First effector use to get the of the ground level. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Data|Ground reflection", EditAnywhere, BlueprintReadWrite)
	FName EffectorName1;

	/** Second effector use to get the of the ground level. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Data|Ground reflection", EditAnywhere, BlueprintReadWrite)
	FName EffectorName2;

	/** Third effector use to get the of the ground level. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Data|Ground reflection", EditAnywhere, BlueprintReadWrite)
	FName EffectorName3;

	/** Third effector use to get the of the ground level. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Data|Ground reflection", EditAnywhere, BlueprintReadWrite)
	FColor PlaneDebugColor = FColor::Red;
};

/**
*/
USTRUCT(BlueprintType)
struct FGaitGroundReflectionData
{
	GENERATED_BODY()

	/** Array of planes defining the ground reflection. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Data|Debug", EditAnywhere, BlueprintReadWrite)
	TArray<FGaitGroundReflectionPlaneData> Planes;
};

/**
 * 
 */
UCLASS()
class NOBUNANIM_API UGaitDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
		/** The key value is the name of the effector or bone that should apply the gait data. */
		UPROPERTY(Category = "[NOBUNANIM]|Gait Data", EditAnywhere, BlueprintReadOnly)
		TMap<FName, FGaitSwingData> GaitSwingValues;

		///** @to do: documentation. */
		//UPROPERTY(Category = "[NOBUNANIM]|Gait Data", EditAnywhere, BlueprintReadOnly)
		//TMap<FName, FGaitGroundReflectionData> GroundReflectionPerEffector;

		/** Animation frame number. 60 frame = 1 second. */
		UPROPERTY(Category = "[NOBUNANIM]|Gait Data", EditAnywhere, BlueprintReadOnly)
		int AnimationFrameCount = 60;
		
		/** Should this gait be played only if the velocity isn't zero. */
		UPROPERTY(Category = "[NOBUNANIM]|Gait Data", EditAnywhere, BlueprintReadOnly)
		bool bComputeWithVelocityOnly = true;

	public:
		/** Return the ratio according to the animation framecount. (1 sec = 60frames). */
		UFUNCTION(Category = "[NOBUNANIM]|Gait Data", BlueprintCallable)
		float GetFrameRatio() const;
};
