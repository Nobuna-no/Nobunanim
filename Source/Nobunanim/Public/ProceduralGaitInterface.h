// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Engine/EngineTypes.h>

#include "UObject/Interface.h"
#include "ProceduralGaitInterface.generated.h"

class UCurveFloat;
class UCurveVector;

/**
*/
USTRUCT(BlueprintType)
struct FGaitBlendData
{
	GENERATED_BODY()

	/** Blend in time in second. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|Blend", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "5", SliderMin = "0", SliderMax = "5"))
	float BlendInTime = 0.f;

	/** Blend out time in second. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|Blend", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "5", SliderMin = "0", SliderMax = "5"))
	float BlendOutTime = 0.f;

	/** @LerpSpeed acceleration during blend in. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|Blend", EditAnywhere, BlueprintReadWrite)
	UCurveFloat* BlendInAcceleration;
	
	/** @LerpSpeed acceleration during blend out. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|Blend", EditAnywhere, BlueprintReadWrite)
	UCurveFloat* BlendOutAcceleration;
};

USTRUCT(BlueprintType)
struct FGaitEventData
{
	GENERATED_BODY()

	/** Should this bone raise the event @OnEffectorCollision of the ProceduralGaitComponent. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|Evet", EditAnywhere, BlueprintReadWrite)
	bool bRaiseOnCollisionEvent;
};

/**
*/
USTRUCT(BlueprintType)
struct FGaitTranslationData
{
	GENERATED_BODY()

	/** Should the translation affect the effector? Usefull for non destructive operation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Translation", EditAnywhere, BlueprintReadWrite)
	bool bAffectEffector = true;

	/** Swing curve. This value is FVector(1,1,1) if the curve is not set. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Translation", EditAnywhere, BlueprintReadWrite)
	UCurveVector* SwingTranslationCurve;

	/** Scale of the swing curve. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Translation", EditAnywhere, BlueprintReadWrite)
	float TranslationSwingScale = 1.f;

	/** Lerp speed of the translation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Translation", EditAnywhere, BlueprintReadWrite)
	float LerpSpeed = 10.f;

	/** Factor to apply on translation swing. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Translation", EditAnywhere, BlueprintReadWrite)
	FVector TranslationFactor = FVector(1.f, 1.f, 1.f);

	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Translation", EditAnywhere, BlueprintReadWrite)
	FVector Offset;

	/** Should the translation be oriented in the direction of the speed or the front of the actor? */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Translation", EditAnywhere, BlueprintReadWrite)
	bool bOrientToVelocity = true;

	/* @to do: documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Translation", EditAnywhere, BlueprintReadWrite)
	bool bAdaptToGroundLevel = true;
};


/**
*/
USTRUCT(BlueprintType)
struct FGaitRotationData
{
	GENERATED_BODY()

	/** Should the translation affect the effector? Usefull for non destructive operation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Translation", EditAnywhere, BlueprintReadWrite)
	bool bAffectEffector = false;

	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Rotation", EditAnywhere, BlueprintReadWrite)
	UCurveVector* SwingRotationCurve;

	/** Factor to apply on rotation swing.*/
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Rotation", EditAnywhere, BlueprintReadWrite)
	FVector RotationFactor = FVector(1.f, 1.f, 1.f);
};


/**
*/
USTRUCT(BlueprintType)
struct FGaitCorrectionData
{
	GENERATED_BODY()

	/** May the swing be adjust if too far from ideal effector. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Correction", EditAnywhere, BlueprintReadWrite)
	bool bAutoAdjustWithIdealEffector = true;

	/** Distance treshold to adjust (if enabled) if too far from ideal effector. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Correction", EditAnywhere, BlueprintReadWrite)
	float DistanceTresholdToAdjust = 100.f;

	/** Swing curve.  This value is FVector(1,1,1) if the curve is not set. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Translation", EditAnywhere, BlueprintReadWrite)
	UCurveVector* CorrectionSwingTranslationCurve;

	/** Should this effector take collisions into account? */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Correction", EditAnywhere, BlueprintReadWrite)
	bool bComputeCollision;

	/** Should this effector take collisions into account? */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Correction", EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ECollisionChannel> TraceChannel;	
	
	/** @to do: documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Correction", EditAnywhere, BlueprintReadWrite)
	bool bUseCurrentEffector = false;

	/** The name of the socket or bone to use as origin of the sweep. "None" will result to the use of the effector location. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Correction", EditAnywhere, BlueprintReadWrite)
	FName OriginCollisionSocketName;

	/** @to do: documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Correction", EditAnywhere, BlueprintReadWrite)
	FVector CollisionSnapOffset;


	/** @to do: documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Correction", EditAnywhere, BlueprintReadWrite)
	FVector AbsoluteDirection = FVector(0,0,-10.f);


	/** @to do: documentation. 1: rotation, 2: velocity, 3: absolute direction. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Correction", EditAnywhere, BlueprintReadWrite)
	bool bOrientToVelocity = false;
};


/**
*/
USTRUCT(BlueprintType)
struct FGaitDebugData
{
	GENERATED_BODY()

	/** May this gait render debug. (require to call cmd: ShowGaitDebug) */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|Debug", EditAnywhere, BlueprintReadWrite)
	bool bDrawDebug = true;

	/** Color of the correction circle. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|Debug", EditAnywhere, BlueprintReadWrite)
	FColor CorrectionCircleColor = FColor::Orange;

	/** Color of the correction circle. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|Debug", EditAnywhere, BlueprintReadWrite)
	FColor ForceSwingColor = FColor::Red;

	/** Color of the correction circle. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|Debug", EditAnywhere, BlueprintReadWrite)
	FColor VelocityColor = FColor::Purple;
};


/**																
*/
USTRUCT(BlueprintType)
struct FGaitSwingData
{
	GENERATED_BODY()

	/** Begin of the swing in absolute time (0-1).*/
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1", SliderMin = "0", SliderMax = "1"))
	float BeginSwing = 0.f;
	/** End of the swing in absolute time (0-1).*/
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1", SliderMin = "0", SliderMax = "1"))
	float EndSwing = 1.f;

	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data", EditAnywhere, BlueprintReadWrite)
	FGaitBlendData BlendData;

	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data", EditAnywhere, BlueprintReadWrite)
	FGaitEventData EventData;

	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data", EditAnywhere, BlueprintReadWrite)
	FGaitTranslationData TranslationData;

	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data", EditAnywhere, BlueprintReadWrite)
	FGaitRotationData RotationData;

	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data", EditAnywhere, BlueprintReadWrite)
	FGaitCorrectionData CorrectionData;

	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data", EditAnywhere, BlueprintReadWrite)
	FGaitDebugData DebugData;

};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UProceduralGaitInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class NOBUNANIM_API IProceduralGaitInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
	public:
		UFUNCTION(Category = "[NOBUNANIM]|Procedural Gait Interface", BlueprintNativeEvent, BlueprintCallable)
		void UpdateEffectorTranslation(const FName& TargetBone, FVector Translation, bool bLerp = true, float LerpSpeed = 10.f);
		UFUNCTION(Category = "[NOBUNANIM]|Procedural Gait Interface", BlueprintNativeEvent, BlueprintCallable)
		void UpdateEffectorRotation(const FName& TargetBone, FRotator Rotation, float LerpSpeed = 10.f);

		UFUNCTION(Category = "[NOBUNANIM]|Procedural Gait Interface", BlueprintNativeEvent, BlueprintCallable)
		void SetProceduralGaitEnable(bool bEnable);
};
