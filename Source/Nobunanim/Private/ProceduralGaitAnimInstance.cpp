// Fill out your copyright notice in the Description page of Project Settings.


#include "ProceduralGaitAnimInstance.h"

#include "Nobunanim/Private/Nobunanim.h"

#include <Engine/Classes/Kismet/KismetSystemLibrary.h>


FVector UProceduralGaitAnimInstance::TraceRaycast(UWorld* World, FVector Origin, FVector Dest)
{
	FCollisionObjectQueryParams ObjectQuery(ECollisionChannel::ECC_WorldStatic);
	FCollisionQueryParams SweepParam();
	FHitResult Hit;
	if (!World->LineTraceSingleByObjectType
	(
		Hit,
		Origin,
		Dest,
		ObjectQuery,
		SweepParam
	))
	{
		Hit.ImpactPoint = Dest;
	}

	if (GroundReflection.bShowDebugPlanes)
	{
		DrawDebugLine(World, Origin, Dest, FColor::Cyan, false, 0.f, 0, 0.5f);
		DrawDebugPoint(World, Hit.ImpactPoint, 10, FColor::Cyan, false, 0.f);
	}
	return Hit.ImpactPoint;
}

void UProceduralGaitAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	NOBUNANIM_SCOPE_COUNTER(ProceduralGaitAnimInstance_Update);

	DeltaTime = DeltaSeconds;
	
	if (!OwnedMesh)
	{
		OwnedMesh = GetOwningComponent();
	}

	int32 CurrentLOD = OwnedMesh->PredictedLODLevel;
	
	FRotator Rotation(0,0,0);
	
	if (CurrentLOD == 0)
	{
		Rotation = ComputeGroundReflection_LOD0();
	}
	else if (CurrentLOD == 1)
	{
		Rotation = ComputeGroundReflection_LOD1();
	}
	else
	{

	}

	GroundReflectionRotation = FMath::Lerp(GroundReflectionRotation, Rotation.GetInverse(), DeltaSeconds * GroundReflectionLerpSpeed);
}

FVector GetAverage(const TArray<FVector>& Vectors)
{
	FVector Sum(0.f);
	FVector Average(0.f);

	if (Vectors.Num() > 0)
	{
		for (int32 VecIdx = 0; VecIdx < Vectors.Num(); VecIdx++)
		{
			Sum += Vectors[VecIdx];
		}

		Average = Sum / ((float)Vectors.Num());
	}

	return Average;
}

FRotator UProceduralGaitAnimInstance::ComputeGroundReflection_LOD0()
{
	// Get Socket location
	FVector F = OwnedMesh->GetSocketLocation(GroundReflection.FrontSocket);
	FVector B = OwnedMesh->GetSocketLocation(GroundReflection.BackSocket);
	FVector R = OwnedMesh->GetSocketLocation(GroundReflection.RightSocket);
	FVector L = OwnedMesh->GetSocketLocation(GroundReflection.LeftSocket);

	// Compute centroid
	TArray<FVector> Average;
	Average.Add(F);
	Average.Add(B);
	Average.Add(R);
	Average.Add(L);
	FVector C = GetAverage(Average);

	// Trace for ground
	F = TraceRaycast(GetWorld(), F, F + RayVector);
	B = TraceRaycast(GetWorld(), B, B + RayVector);
	R = TraceRaycast(GetWorld(), R, R + RayVector);
	L = TraceRaycast(GetWorld(), L, L + RayVector);
	C = TraceRaycast(GetWorld(), C, C + RayVector);

	// Compute ground reflection
	FRotator Rotation(0, 0, 0);
	FVector RightVec = OwnedMesh->GetRightVector();
	GetPlaneRotation(F, R, C, RightVec, Rotation, GroundReflection.bUseHalfVector, GroundReflection.bShowDebugPlanes);
	GetPlaneRotation(C, R, B, RightVec, Rotation, GroundReflection.bUseHalfVector, GroundReflection.bShowDebugPlanes);
	GetPlaneRotation(C, B, L, RightVec, Rotation, GroundReflection.bUseHalfVector, GroundReflection.bShowDebugPlanes);
	GetPlaneRotation(F, C, L, RightVec, Rotation, GroundReflection.bUseHalfVector, GroundReflection.bShowDebugPlanes);

	return Rotation;
}

FRotator UProceduralGaitAnimInstance::ComputeGroundReflection_LOD1()
{
	FVector Front = OwnedMesh->GetSocketLocation(GroundReflection.FrontSocket);
	Front = TraceRaycast(GetWorld(), Front, Front + RayVector);
	FVector Back = OwnedMesh->GetSocketLocation(GroundReflection.BackSocket);
	Back = TraceRaycast(GetWorld(), Back, Back + RayVector);
	FVector Right = OwnedMesh->GetSocketLocation(GroundReflection.RightSocket);
	Right = TraceRaycast(GetWorld(), Right, Right + RayVector);
	FVector Left = OwnedMesh->GetSocketLocation(GroundReflection.LeftSocket);
	Left = TraceRaycast(GetWorld(), Left, Left + RayVector);

	FVector RightVec = OwnedMesh->GetRightVector();

	FRotator Rotation(0, 0, 0);
	GetPlaneRotation(Front, Right, Back, RightVec, Rotation, GroundReflection.bUseHalfVector, GroundReflection.bShowDebugPlanes);
	GetPlaneRotation(Front, Back, Left, RightVec, Rotation, GroundReflection.bUseHalfVector, GroundReflection.bShowDebugPlanes);
	GetPlaneRotation(Front, Right, Left, RightVec, Rotation, GroundReflection.bUseHalfVector, GroundReflection.bShowDebugPlanes);
	GetPlaneRotation(Right, Back, Left, RightVec, Rotation, GroundReflection.bUseHalfVector, GroundReflection.bShowDebugPlanes);

	return Rotation;
}


FRotator UProceduralGaitAnimInstance::GetPlaneRotation(FVector A, FVector B, FVector C, FVector RightVector, FRotator& OutRotation, bool bComputeHalf, bool bShowDebug)
{
	FRotator Rotation;

	FVector AP = bComputeHalf ? (A + B) * 0.5f : A;
	FVector BP = bComputeHalf ? (B + C) * 0.5f : B;
	FVector CP = bComputeHalf ? (C + A) * 0.5f : C;

	FVector AB = BP - AP;
	FVector AC = CP - AP;
	FVector CrossABC = AB ^ AC;
	CrossABC.Normalize();

	const FVector FloorZAxis = CrossABC;
	const FVector FloorXAxis = RightVector ^ FloorZAxis;
	const FVector FloorYAxis = FloorZAxis ^ FloorXAxis;

	float OutSlopePitchDegreeAngle = 90.f - FMath::RadiansToDegrees(FMath::Acos(FloorXAxis | FVector(0, 0, 1.f)));
	float OutSlopeRollDegreeAngle = 90.f - FMath::RadiansToDegrees(FMath::Acos(FloorYAxis | FVector(0, 0, 1.f)));

	OutRotation.Pitch = 0;
	OutRotation.Roll += OutSlopeRollDegreeAngle * 0.25f;
	OutRotation.Yaw += OutSlopePitchDegreeAngle * 0.25f;
	
#if WITH_EDITOR
	if (bShowDebug)
	{
	//DEBUG_LOG_FORMAT(Log, "\t=> Rotation: %s", *OutRotation.ToString());
		DrawDebugLine(GetWorld(), AP, BP, FColor(255.f, 255.f, 255.f), false, 0.f, 0, 0.5f);
		DrawDebugLine(GetWorld(), BP, CP, FColor(255.f, 255.f, 255.f), false, 0.f, 0, 0.5f);
		DrawDebugLine(GetWorld(), CP, AP, FColor(255.f, 255.f, 255.f), false, 0.f, 0, 0.5f);
	}
#endif

	return Rotation;
}

void UProceduralGaitAnimInstance::UpdateEffectorTranslation_Implementation(const FName& TargetBone, FVector Translation, bool bLerp, float LerpSpeed)
{
	const FVector* Vec = EffectorsTranslation.Find(TargetBone);
	if (!Vec)
	{
		EffectorsTranslation.Add(TargetBone, Translation);
	}
	else
	{
		EffectorsTranslation[TargetBone] = bLerp ? FMath::Lerp(*Vec, Translation, LerpSpeed * DeltaTime) : Translation;
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
	GaitActive = bEnable ? 1.f : 0.f;
}
