// Fill out your copyright notice in the Description page of Project Settings.


#include "ProceduralGaitAnimInstance.h"


void UProceduralGaitAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	DeltaTime = DeltaSeconds;
	
	//LerpValue = bUseDeltaSecond ? DeltaSeconds * LerpSpeed : LerpSpeed;
}


void UProceduralGaitAnimInstance::UpdateEffectorTranslation_Implementation(const FName& TargetBone, FVector Translation, float LerpSpeed)
{
	const FVector* Vec = EffectorsTranslation.Find(TargetBone);
	if (!Vec)
	{
		EffectorsTranslation.Add(TargetBone, Translation);
	}
	else
	{
		EffectorsTranslation[TargetBone] = FMath::Lerp(*Vec, Translation, LerpSpeed * DeltaTime);
	}
}

void UProceduralGaitAnimInstance::UpdateEffectorRotation_Implementation(const FName& TargetBone, FRotator Rotation, float LerpSpeed)
{
	const FRotator* Vec = BonesRotation.Find(TargetBone);
	if (!Vec)
	{
		BonesRotation.Add(TargetBone, Rotation);
	}
	else
	{
		BonesRotation[TargetBone] = FMath::Lerp(*Vec, Rotation, LerpSpeed * DeltaTime);
	}
}

void UProceduralGaitAnimInstance::SetProceduralGaitEnable_Implementation(bool bEnable)
{
	GaitActive = bEnable ? 0.f : 1.f;
}
