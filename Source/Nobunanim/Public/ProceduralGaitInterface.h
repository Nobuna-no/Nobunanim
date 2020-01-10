// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ProceduralGaitInterface.generated.h"

class UCurveVector;

/**
*/
USTRUCT(BlueprintType)
struct FGaitTranslationData
{
	GENERATED_BODY()

	/** Swing curve.*/
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Translation", EditAnywhere, BlueprintReadWrite)
	UCurveVector* SwingTranslationCurve;

	/** Scale of the swing curve. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Translation", EditAnywhere, BlueprintReadWrite)
	float TranslationSwingScale = 1.f;

	/** Factor to apply on translation swing.*/
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Translation", EditAnywhere, BlueprintReadWrite)
	FVector TranslationFactor = FVector(1.f, 1.f, 1.f);

	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Translation", EditAnywhere, BlueprintReadWrite)
	FVector Offset;

	/** Should the translation be oriented in the direction of the speed or the front of the actor? */UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data|New|Translation", EditAnywhere, BlueprintReadWrite)
	bool bOrientToVelocity = true;
};


/**
*/
USTRUCT(BlueprintType)
struct FGaitRotationData
{
	GENERATED_BODY()

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
		void UpdateEffectorTranslation(const FName& TargetBone, FVector Translation);
		UFUNCTION(Category = "[NOBUNANIM]|Procedural Gait Interface", BlueprintNativeEvent, BlueprintCallable)
		void UpdateEffectorRotation(const FName& TargetBone, FRotator Rotation);

		UFUNCTION(Category = "[NOBUNANIM]|Procedural Gait Interface", BlueprintNativeEvent, BlueprintCallable)
		void SetProceduralGaitEnable(bool bEnable);
};
