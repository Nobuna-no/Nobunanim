// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ProceduralGaitInterface.generated.h"

class UCurveVector;


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

	/** Swing curve.*/
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data", EditAnywhere, BlueprintReadWrite)
	UCurveVector* SwingTranslationCurve;

	/** Factor to apply on translation swing.*/
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data", EditAnywhere, BlueprintReadWrite)
	FVector TranslationFactor;

	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data", EditAnywhere, BlueprintReadWrite)
	FVector Offset;

	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data", EditAnywhere, BlueprintReadWrite)
	UCurveVector* SwingRotationCurve;
	
	/** Factor to apply on rotation swing.*/
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data", EditAnywhere, BlueprintReadWrite)
	FVector RotationFactor;

	/** May the swing be adjust if too far from ideal effector. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait SwingData", EditAnywhere, BlueprintReadWrite)
	bool bAutoAdjustWithIdealEffector = true;

	/** Distance treshold to adjust (if enabled) if too far from ideal effector. */
	UPROPERTY(Category = "[NOBUNANIM]|Gait Swing Data", EditAnywhere, BlueprintReadWrite)
	float DistanceTresholdToAdjust = 100.f;
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

};
