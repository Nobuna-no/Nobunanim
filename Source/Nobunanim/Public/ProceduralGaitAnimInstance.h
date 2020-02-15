// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Map.h"
#include "ProceduralGaitInterface.h"

#include "Animation/AnimInstance.h"
#include "ProceduralGaitAnimInstance.generated.h"

/**
*/
USTRUCT(BlueprintType)
struct FGroundReflectionSocketData
{
	GENERATED_BODY()

	/** @to do: documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Data|Ground reflection", EditAnywhere, BlueprintReadWrite)
	FName FrontSocket;

	/** @to do: documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Data|Ground reflection", EditAnywhere, BlueprintReadWrite)
	FName BackSocket;

	/** @to do: documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Data|Ground reflection", EditAnywhere, BlueprintReadWrite)
	FName RightSocket;

	/** @to do: documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Data|Ground reflection", EditAnywhere, BlueprintReadWrite)
	FName LeftSocket;

	/** bShowDebug. */
	UPROPERTY(Category = "[NOBUNANIM]|Procedural Gait Anim Instance", EditAnywhere, BlueprintReadWrite)
	bool  bShowDebugPlanes = true;

	/** Should use half of the plane's vectors. */
	UPROPERTY(Category = "[NOBUNANIM]|Procedural Gait Anim Instance", EditAnywhere, BlueprintReadWrite)
	bool  bUseHalfVector = false;

	/** Lerp speed of effectors. */
	UPROPERTY(Category = "[NOBUNANIM]|Procedural Gait Anim Instance", EditAnywhere, BlueprintReadWrite)
	float GroundReflectionLerpSpeed = 10.f;
};


/**
 * Only manage 
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

		UPROPERTY(Category = "[NOBUNANIM]|Procedural Gait Anim Instance|Ground Reflection", EditAnywhere, BlueprintReadOnly)
		FGroundReflectionSocketData GroundReflection;

		/** .*/
		UPROPERTY(Category = "[NOBUNANIM]|Gait Data|Ground reflection", EditAnywhere, BlueprintReadOnly)
		FRotator GroundReflectionRotation;

		UPROPERTY(Category = "[NOBUNANIM]|Gait Data|Ground reflection", EditAnywhere, BlueprintReadOnly)
		FVector RayVector = FVector(0,0, -500.f);

		/** Lerp speed of effectors. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Gait Anim Instance", EditAnywhere, BlueprintReadWrite)
		float GaitActive = 0.f;

		/** Lerp speed of effectors. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Gait Anim Instance", EditAnywhere, BlueprintReadWrite)
		float GroundReflectionLerpSpeed = 10.f;

	

	private:
		float DeltaTime = 0.f;
		USkeletalMeshComponent* OwnedMesh;
		/** Current LOD.*/
		int32 CurrentLOD = 0;

	public:
		// Native update override point. It is usually a good idea to simply gather data in this step and 
		// for the bulk of the work to be done in NativeUpdateAnimation.
		virtual void NativeUpdateAnimation(float DeltaSeconds) override;


	public:
		void UpdateEffectorTranslation_Implementation(const FName& TargetBone, FVector Translation, bool bLerp, float LerpSpeed) override;
		void UpdateEffectorRotation_Implementation(const FName& TargetBone, FRotator Rotation, float LerpSpeed) override;
		void SetProceduralGaitEnable_Implementation(bool bEnable) override;


	private:
		FRotator GetPlaneRotation(FVector A, FVector B, FVector C, FVector RightVector, FRotator& OutRotation, bool bComputeHalf, bool bShowDebug);
		FVector TraceRaycast(UWorld* World, FVector Origin, FVector Dest);


		FRotator ComputeGroundReflection_LOD0();
		FRotator ComputeGroundReflection_LOD1();
};
