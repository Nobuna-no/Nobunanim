// Fill out your copyright notice in the Description page of Project Settings.


#include "ProceduralGaitAnimInstance.h"


void UProceduralGaitAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	//DeltaTime = DeltaSeconds;
	
	LerpValue = bUseDeltaSecond ? DeltaSeconds * LerpSpeed : LerpSpeed;
}


void UProceduralGaitAnimInstance::UpdateEffectorTranslation_Implementation(const FName& TargetBone, FVector Translation)
{
	const FVector* Vec = EffectorsTranslation.Find(TargetBone);
	if (Vec)
	{
		EffectorsTranslation[TargetBone] = FMath::Lerp(*Vec, Translation, LerpValue);
	}
	else
	{
		EffectorsTranslation.Add(TargetBone, Translation);
	}
}

void UProceduralGaitAnimInstance::UpdateEffectorRotation_Implementation(const FName& TargetBone, FRotator Rotation)
{
	const FRotator* Vec = BonesRotation.Find(TargetBone);
	if (Vec)
	{
		BonesRotation[TargetBone] = FMath::Lerp(*Vec, Rotation, LerpValue);
	}
	else
	{
		BonesRotation.Add(TargetBone, Rotation);
	}
}

void UProceduralGaitAnimInstance::SetProceduralGaitEnable_Implementation(bool bEnable)
{
	GaitActive = bEnable ? 0.f : 1.f;
}
