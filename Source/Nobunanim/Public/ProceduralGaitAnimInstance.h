// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Map.h"
#include "ProceduralGaitInterface.h"

#include "Animation/AnimInstance.h"
#include "ProceduralGaitAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class NOBUNANIM_API UProceduralGaitAnimInstance : public UAnimInstance, public IProceduralGaitInterface
{
	GENERATED_BODY()

	public:
		///** Lerp speed of effectors. */
		//UPROPERTY(Category = "[NOBUNANIM]|Procedural Gait Anim Instance", EditAnywhere, BlueprintReadWrite)
		//float LerpSpeed = 0.5f;
		///** May use the deltasecond to calculate the lerp speed. */
		//UPROPERTY(Category = "[NOBUNANIM]|Procedural Gait Anim Instance", EditAnywhere, BlueprintReadWrite)
		//bool bUseDeltaSecond = false;
		/** @todo: documentation. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Gait Anim Instance", VisibleAnywhere, BlueprintReadOnly)
		TMap<FName, FVector> EffectorsTranslation;
		/** @todo: documentation. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Gait Anim Instance", VisibleAnywhere, BlueprintReadOnly)
		TMap<FName, FRotator> BonesRotation;

		/** Lerp speed of effectors. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Gait Anim Instance", EditAnywhere, BlueprintReadWrite)
		float GaitActive = 0.f;


	private:
		float LerpValue;
		float DeltaTime = 0.f;

	public:
		// Native update override point. It is usually a good idea to simply gather data in this step and 
		// for the bulk of the work to be done in NativeUpdateAnimation.
		virtual void NativeUpdateAnimation(float DeltaSeconds) override;


	public:
		void UpdateEffectorTranslation_Implementation(const FName& TargetBone, FVector Translation, float LerpSpeed) override;
		void UpdateEffectorRotation_Implementation(const FName& TargetBone, FRotator Rotation, float LerpSpeed) override;
		void SetProceduralGaitEnable_Implementation(bool bEnable) override;
};
