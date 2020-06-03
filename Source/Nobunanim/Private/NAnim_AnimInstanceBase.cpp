// Fill out your copyright notice in the Description page of Project Settings.


#include "NAnim_AnimInstanceBase.h"

#include "Nobunanim/Public/ProceduralAnimator.h"


void UNAnim_AnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	animator = NewObject<UProceduralAnimator>();
	if (animator)
	{
		animator->Initialize(this->GetOwningComponent());
		animator->SetActive(true);
	}
}