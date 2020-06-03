// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Animation/AnimInstance.h>
#include "NAnim_AnimInstanceBase.generated.h"

/**
 * 
 */
UCLASS()
class NOBUNANIM_API UNAnim_AnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()
	

	protected:
	/** PARAMETERS
	*/
		UPROPERTY(Category = "Nobunanim", BlueprintReadOnly)
		class UProceduralAnimator* animator;

	public:
	/** NATIVE ANIM INSTANCE OVERRIDE
	*/
		/** Native initialization override point. */
		virtual void NativeInitializeAnimation() override;
};
