// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Engine/EngineTypes.h>
#include <Engine/DataAsset.h>
#include <Engine/EngineTypes.h>

#include "Curves/CurveFloat.h"

#include "ProceduralAnimAsset.generated.h"

class UCurveFloat;
class UCurveVector;

USTRUCT(BlueprintType)
struct FProceduralAnimData_CollisionAdjustment
{
	GENERATED_BODY()

	public:
		/** Should this effector compute IK collisions? */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bComputeCollision;

		/** Collision channel to trace. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TEnumAsByte<ECollisionChannel> TraceChannel = ECollisionChannel::ECC_Visibility;

		/** The name of the socket or bone to use as origin of the sweep. "None" will result to the use of the effector location. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName OriginCollisionSocketName;
	
		/** Raycast direction. If value == FVector::ZeroVector, take the velocity of the effector as direction. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector RayDirection = FVector(0.f, 0.f, 0.f);
};

USTRUCT(BlueprintType)
struct FProceduralAnimData_Adjustment
{
	GENERATED_BODY()

	public:
		/** Velocity factor to apply on the normalized velocity. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector VelocityFactor = FVector(1.f, 1.f, 1.f);

		/** May use a simulated velocity? */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bUseSimulatedVelocity = false;

		/** The velocity to use if @bUseSimulatedVelocity is set to true. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector SimulatedVelocity = FVector::ZeroVector;
		
		/** May the swing be adjust if too far from ideal effector. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bForceSwingIfTooFar = true;

		/** Factor to calibrate the adjustement per axis. An axis value == 0 means no correction from this axis. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector FactorPerAxis = FVector(1.f,1.f,0.f);

		/** Distance treshold to adjust (if enabled) if too far from ideal effector. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float DistanceTreshold = 10.f;

		/** Collision adjustment data. */
		//UPROPERTY(EditAnywhere, BlueprintReadWrite)
		//FProceduralAnimData_CollisionAdjustment CollisionData;
};

USTRUCT(BlueprintType)
struct FProceduralAnimData_AdditiveMovement
{
	GENERATED_BODY()

	public:
		/** Should this effector be evaluated? Usefull for non destructive operation. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayPriority = "1"))
		bool bEvaluate = false;

		/** Lerp speed of the additive. If value <= 0, then no lerp. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayPriority = "2"))
		float LerpSpeed = -1.f;

		/** Additive curve. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayPriority = "3"))
		UCurveVector* AdditiveCurve;

		/** Scale of the additive curve. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayPriority = "4"))
		float Scale = 1.f;

		/** Factor to apply on additive curve. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayPriority = "5"))
		FVector Factor = FVector(1.f, 1.f, 1.f);
};

USTRUCT(BlueprintType)
struct FProceduralAnimData_AdditiveTranslation : public FProceduralAnimData_AdditiveMovement
{
	GENERATED_BODY()

	public:
		/** Additional global offset. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayPriority = "6"))
		FVector AdditiveOffset = FVector::ZeroVector;

		/** Should the additive curve and offset be oriented in the direction of the speed or the front of the actor? */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayPriority = "7"))
		bool bOrientToVelocity = true;

		/** Transform space to compute additive data. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayPriority = "8"))
		TEnumAsByte<ERelativeTransformSpace> TransformSpace = ERelativeTransformSpace::RTS_World;
};

USTRUCT(BlueprintType)
struct FProceduralAnimData_AdditiveRotation : public FProceduralAnimData_AdditiveMovement
{
	GENERATED_BODY()

	public:
		/** Additional global offset. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayPriority = "6"))
		FRotator AdditiveOffset = FRotator::ZeroRotator;

		///** Should the additive curve be scaled in the direction of the velocity? */
		//UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayPriority = "7"))
		//bool bScaleByVelocity = false;

		/** Should the additive data be oriented to direction in local space? */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayPriority = "8"))
		bool bOrientToDirection = false;
};

USTRUCT(BlueprintType)
struct FProceduralAnimData_SwingAcceleration
{
	GENERATED_BODY()
	
	public:
		/** Describe the acceleration of the swing until it reach his destination. A nullptr value will lead to a linear acceleration.
			* Think about it as:
			* - if currentCurveValue == 0, then the location of the effector is the origin.
			* - if currentCurveValue == 1, then the location of the effector is the ideal effector location (aka Socket location).
			* So if you want that the effector to follow closely to the ideal effector (i.e. when you want only additive rotation),
			* you can create a constant curve (t(0)=1, t(1)=1) and play with the lerp speed. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FRuntimeFloatCurve AccelerationCurveX;
		/** Describe the acceleration of the swing until it reach his destination. A nullptr value will lead to a linear acceleration.
			* Think about it as:
			* - if currentCurveValue == 0, then the location of the effector is the origin.
			* - if currentCurveValue == 1, then the location of the effector is the ideal effector location (aka Socket location).
			* So if you want that the effector to follow closely to the ideal effector (i.e. when you want only additive rotation),
			* you can create a constant curve (t(0)=1, t(1)=1) and play with the lerp speed. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FRuntimeFloatCurve AccelerationCurveY;
		/** Describe the acceleration of the swing until it reach his destination. A nullptr value will lead to a linear acceleration.
			* Think about it as:
			* - if currentCurveValue == 0, then the location of the effector is the origin.
			* - if currentCurveValue == 1, then the location of the effector is the ideal effector location (aka Socket location).
			* So if you want that the effector to follow closely to the ideal effector (i.e. when you want only additive rotation),
			* you can create a constant curve (t(0)=1, t(1)=1) and play with the lerp speed. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FRuntimeFloatCurve AccelerationCurveZ;
};

USTRUCT(BlueprintType)
struct FProceduralAnimData_Movement
{
	GENERATED_BODY()

	public:
		/** Describe the acceleration of the swing until it reach his destination. A nullptr value will lead to a linear acceleration. 
		* Think about it as:
		* - if currentCurveValue == 0, then the location of the effector is the origin.
		* - if currentCurveValue == 1, then the location of the effector is the ideal effector location (aka Socket location).
		* So if you want that the effector to follow closely to the ideal effector (i.e. when you want only additive rotation),
		* you can create a constant curve (t(0)=1, t(1)=1) and play with the lerp speed. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FRuntimeFloatCurve AccelerationCurve;
		
		/** Describe the acceleration of the swing until it reach his destination. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FProceduralAnimData_SwingAcceleration AccelerationData;
		
		/** Lerp speed of the effector moving to it's ideal location. If value <= 0, then no there is no lerp and it's instantaneous. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float LerpSpeed = -1.f;

		/** Additive data that will be added to the swing path. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FProceduralAnimData_AdditiveTranslation AdditiveTranslationData;
		
		/** Additive rotation data that will be apply on the swing. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FProceduralAnimData_AdditiveRotation AdditiveRotationData;
};

USTRUCT(BlueprintType)
struct FProceduralAnimData_Time
{
	GENERATED_BODY()

	public:
		/** The name of the effector to link to. If valid, this effector will swing at the same time as the parent. Begin swing and End swing will be considered as offset (i.e. EndSwing = 0.1 will end at parent EndSwing + 0.1)*/
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName ParentEffector;

		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bBindToParentBeginSwing = false;
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bBindToParentEndSwing = false;

		
		/** Begin of the swing in absolute time (0-1). If have a parent, this value will be added to the parent one.
		An effector beeing in 'Swing' describe the moment where it is active, else it is known as being in 'Stance'. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1", SliderMin = "0", SliderMax = "1"))
		float BeginSwing = 0.f;
		/** End of the swing in absolute time (0-1). If have a parent, this value will be added to the parent one.
		An effector beeing in 'Swing' describe the moment where it is active, else it is known as being in 'Stance'. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1", SliderMin = "0", SliderMax = "1"))
		float EndSwing = 1.f;
};

USTRUCT(BlueprintType)
struct FProceduralAnimData
{
	GENERATED_BODY()

	public:
		/** Should this effector be evaluated? Usefull for non destructive operation. */
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bEvaluate = true;

		/** Procedural animation time infos. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Anim Data", EditAnywhere, BlueprintReadWrite)
		FProceduralAnimData_Time TimeData;

		/** Procedural animation movement infos. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Anim Data", EditAnywhere, BlueprintReadWrite)
		FProceduralAnimData_Movement MovementData;

		/** Procedural animation adjustment infos. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Anim Data", EditAnywhere, BlueprintReadWrite)
		FProceduralAnimData_Adjustment AdjustmentData;
};

UCLASS(BlueprintType)
class NOBUNANIM_API UProceduralAnimAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
		/** Procedural anim data per effector. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Anim Data", EditAnywhere, BlueprintReadWrite)
		TMap<FName, FProceduralAnimData> Effectors;

		/** Weight of this procedural animation. 0 means this animation will be overrided by any animation with greater weight. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Anim Data", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1", SliderMin = "0", SliderMax = "1"))
		float Weight = 0.0f;
		/** Animation frame number. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Anim Data", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
		int AnimationFrameCount = 60;
		/** Animation refresh rate (or target FPS). */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Anim Data", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "15", ClampMax = "60", SliderMin = "15", SliderMax = "60"))
		int AnimationFrameRate = 60;
		
		/** AnimationPlayRate. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Anim Data", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.01", ClampMax = "5", SliderMin = "0.01", SliderMax = "5"))
		float PlayRate = 1.0f;

		/** Events to trigger. Key is the name of the event, Value is the time to trigger in absolute time (between 0 and 1).  */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Anim Data|Events", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.01", ClampMax = "5", SliderMin = "0.01", SliderMax = "5"))
		TMap<FName, float> EventsToTrigger;

	public:
	/**
	*/
		/** */
		UFUNCTION(Category = "[NOBUNANIM]|Procedural Anim Data", BlueprintCallable)
		float GetFrameRatio() const;
};