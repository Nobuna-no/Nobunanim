// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Map.h"
#include "ProceduralGaitInterface.h"

#include "Nobunanim/Public/ProceduralGaitControllerComponent.h"
#include "Animation/AnimInstance.h"
#include "ProceduralGaitAnimInstance.generated.h"


class UCurveFloat;
class UGaitDataAsset;


//USTRUCT(BlueprintType)
//struct FGaitEffectorData
//{
//	GENERATED_BODY()
//
//	/** @to do: Documentation. */
//	UPROPERTY(Category = "[NOBUNANIM]|Effector Data", EditAnywhere, BlueprintReadWrite)
//	FVector CurrentEffectorLocation = FVector::ZeroVector;
//	/** @to do: Documentation. */
//	UPROPERTY(Category = "[NOBUNANIM]|Effector Data", EditAnywhere, BlueprintReadWrite)
//	FVector IdealEffectorLocation = FVector::ZeroVector;
//
//	/** @to do: Documentation. */
//	UPROPERTY(Category = "[NOBUNANIM]|Effector Data", EditAnywhere, BlueprintReadWrite)
//	FVector GroundLocation = FVector::ZeroVector;
//
//	/** @to do: Documentation. */
//	UPROPERTY(Category = "[NOBUNANIM]|Effector Data", EditAnywhere, BlueprintReadWrite)
//	bool bForceSwing = false;
//
//	/** @to do: Documentation. */
//	UPROPERTY(Category = "[NOBUNANIM]|Effector Data", EditAnywhere, BlueprintReadWrite)
//	bool bCorrectionIK = false;
//
//	/** @to do: Documentation. */
//	UPROPERTY(Category = "[NOBUNANIM]|Effector Data", EditAnywhere, BlueprintReadWrite)
//	float BeginForceSwingInterval = 0;
//	/** @to do: Documentation. */
//	UPROPERTY(Category = "[NOBUNANIM]|Effector Data", EditAnywhere, BlueprintReadWrite)
//	float EndForceSwingInterval = 0;
//
//	//FSwingEvent OnBeginSwing;
//	//FSwingEvent OnEndSwing;
//
//	//FSwingEvent OnBeginForceSwing;
//	//FSwingEvent OnEndForceSwing;
//
//	/** @to do: Documentation. */
//	float CurrentBlendValue = 0.f;
//	/** @to do: Documentation. */
//	FName CurrentGait;
//	/** @to do: Documentation. */
//	float BlockTime = -1.f;
//};


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

	
	protected:
		/** Current gait monde name.*/
		FName CurrentGaitMode;
		/** Pending gait mode name. Use to switch gaot with blend time. */
		FName PendingGaitMode = "";
		/** Current time of the cycle (in absolute time [0,1]).*/
		float CurrentTime = 0;
		/** Current time buffer used to compute current time.*/
		float TimeBuffer = 0;
		/** Current LOD.*/
		int32 CurrentLOD = 0;

		/** Map of Gaits data. Keys are the name of the Gait. */
		TMap<FName, FGaitEffectorData> Effectors;
		/** @to do: documentation. */
		FVector LastVelocity;
		/** */
		bool bLastFrameWasDisable = true;


	protected:
		/** Owned mesh. */
		UPROPERTY(Category = "[NOBUNANIM]|Gait Controller", VisibleAnywhere, BlueprintReadOnly)
		USkeletalMeshComponent* OwnedMesh = nullptr;

		/** Map of Gaits data. Keys are the name of the Gait. */
		UPROPERTY(Category = "[NOBUNANIM]|Gait Controller", EditAnywhere, BlueprintReadOnly)
		TMap<FName, UGaitDataAsset*> GaitsData;


		/** Current playrate of the cycle.*/
		UPROPERTY(Category = "[NOBUNANIM]|Gait Controller", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.001", ClampMax = "10", SliderMin = "0.001", SliderMax = "10.f"))
		float PlayRate = 1.f;

		/** Current playrate of the cycle.*/
		UPROPERTY(Category = "[NOBUNANIM]|Gait Controller", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.001", ClampMax = "10", SliderMin = "0.001", SliderMax = "10.f"))
		bool bGaitActive = true;

		/** Show effector debug. */
		UPROPERTY(Category = "[NOBUNANIM]|Gait Controller|Debug", EditAnywhere, BlueprintReadWrite)
		bool bShowDebug = false;


	#if WITH_EDITORONLY_DATA
		/** Show effector debug. */
		UPROPERTY(Category = "[NOBUNANIM]|Gait Controller|Debug", EditAnywhere, BlueprintReadWrite)
		bool bShowLOD = false;
	#endif


	public:
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

		/** Called each time than an effector that must raise the event enter in collision. */
		UPROPERTY(Category = "[NOBUNANIM]|Gait Controller|Debug", BlueprintAssignable)
		FOnEffectorCollision OnCollisionEvent;


	private:
		float DeltaTime = 0.f;
		//USkeletalMeshComponent* OwnedMesh;
		/** Current LOD.*/
		//int32 CurrentLOD = 0;


	public:
	/** UNREAL METHODS
	*/
		// Native update override point. It is usually a good idea to simply gather data in this step and 
		// for the bulk of the work to be done in NativeUpdateAnimation.
		virtual void NativeUpdateAnimation(float DeltaSeconds) override;


	public:
	/** PROCEDURAL GAIT INTERFACE
	*/
		void UpdateEffectorTranslation_Implementation(const FName& TargetBone, FVector Translation, bool bLerp, float LerpSpeed) override;
		void UpdateEffectorRotation_Implementation(const FName& TargetBone, FRotator Rotation, float LerpSpeed) override;
		void SetProceduralGaitEnable_Implementation(bool bEnable) override;
		
	protected:
		/** Update of procedural gait. */
		void virtual ProceduralGaitUpdadte(float DeltaSeconds);
		
		UFUNCTION(Category = "[NOBUNANIM]|Gait Controller", BlueprintNativeEvent, BlueprintCallable)
		void UpdateGaitMode(const FName& NewGaitName);

	private:
	/** TERRAIN PREDICTION UTILITIES
	*/
		FRotator GetPlaneRotation(FVector A, FVector B, FVector C, FVector RightVector, FRotator& OutRotation, bool bComputeHalf, bool bShowDebug);
		FVector TraceGroundRaycast(UWorld* World, FVector Origin, FVector Dest);


		FRotator ComputeGroundReflection_LOD0();
		FRotator ComputeGroundReflection_LOD1();


	private:
	/** PROCEDURAL GAIT UTILITIES
	*/
		/** Trace complexe ray... */
		bool TraceRay(UWorld* World, TArray<FHitResult>& HitResults, FVector Origin, FVector Dest, TEnumAsByte<ECollisionChannel> TraceChannel, float SphereCastRadius);

		/** Get best hit result according to the ideal location.*/
		FHitResult& GetBestHitResult(TArray<FHitResult>& HitResults, FVector IdealLocation);

		/** .*/
		void ComputeCollisionCorrection(const FGaitCorrectionData* CorrectionData, FGaitEffectorData& Effector);

		/** Update effectors data.*/
		void UpdateEffectors(const UGaitDataAsset& CurrentAsset);

		/** AARJHALJKDHFLKJDAHL(some kind of dying scream). */
		bool IsInRange(float Value, float Min, float Max, float& OutRangeMin, float& OutRangeMax);

		void DrawGaitDebug(FVector Position, FVector EffectorLocation, FVector CurrentLocation, float Treshold, bool bAutoAdjustWithIdealEffector, bool bForceSwing, const FGaitDebugData* DebugData);

		void UpdateLOD(bool bForceUpdate = false);
};
