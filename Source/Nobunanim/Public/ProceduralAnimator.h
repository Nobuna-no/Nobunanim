// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <UObject/Object.h>
#include <Engine/EngineTypes.h>
#include <Engine/DataAsset.h>
#include <Engine/EngineTypes.h>

#include "Nobunanim/Public/ProceduralGaitInterface.h"
#include "Nobunanim/Public/ProceduralAnimAsset.h"

#include "ProceduralAnimator.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FProceduralAnimAssetEvent, FName, _EventName);

//class ULocomotionComponent;
class USkeletalMeshComponent;
class UCurveFloat;
class UCurveVector;


USTRUCT(BlueprintType)
struct FEffectorAnimData
{
	GENERATED_BODY()

	public:
		/** */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", BlueprintReadOnly)
		FVector IdLocation;

		/** */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", BlueprintReadOnly)
		FVector CurrentDestination;

		bool bStartSwing = false;

		/** */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", BlueprintReadOnly)
		FVector CurrentLocation;

		/** */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", BlueprintReadOnly)
		bool bForceSwing;
		/** */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", BlueprintReadOnly)
		float currentEndSwing;
		/** */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", BlueprintReadOnly)
		float currentBeginSwing;
};

// This class does not need to be modified.
UCLASS()
class NOBUNANIM_API UProceduralAnimator : public UObject, public IProceduralGaitInterface
{
	GENERATED_BODY()

	private:
	/** REFERENCES
	*/
		/** World. */
		UWorld* world;
		/** Skeletal Mesh reference. */
		USkeletalMeshComponent* mesh;
		/** Locomotion component reference. */
		//ULocomotionComponent* locomotion;

		/** Deltatime evaluated each Evaluate_Internal(). */
		float deltaTime = 0.f;
		/** Buffer to calculate deltatime. */
		float lastTime = 0.f;
		/** Current direction based on locomotion component. Range is [-180;180] in degree rad. */
		int direction = 0.f;
		/** Cached last velocity. */
		FVector lastVelocity = FVector::ZeroVector;

		/** Procedural anim asset raised event buffer. */
		TMap<FName, TArray<FName>> PAAEventRaised;


	protected:
		/** Current procedural animation to evaluate. */
		TMap<FName, UProceduralAnimAsset*> currentAnimSet;
		/** Map to manage adding/removing anim. */
		TMap<FName, FTimerHandle> timerHandles;
		
		/** Effectors. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", BlueprintReadOnly)
		TMap<FName, FEffectorAnimData> effectors;
		/** Map to manage anim asset time buffers. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", BlueprintReadOnly)
		TMap<FName, float> timeBuffers;
		/** Map to manage anim asset frame refresh rate. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", BlueprintReadOnly)
		TMap<FName, float> refreshRateBuffers;

		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", VisibleAnywhere, BlueprintReadOnly)
		TMap<FName, FVector> effectorsLocation;
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", VisibleAnywhere, BlueprintReadOnly)
		TMap<FName, FRotator> effectorsRotation;

		int updateCount;

		/** TimerHandle for internal update. */
		FTimerHandle updateTimerHandle;

		/** Animations total weight. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", VisibleAnywhere, BlueprintReadOnly)
		float globalWeight = 0.0f ;
		/** Current LOD. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", VisibleAnywhere, BlueprintReadOnly)
		int32 currentLOD;
		/** Use to init check. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", VisibleAnywhere, BlueprintReadOnly)
		bool bIsInitialized = false;
		/** Is procedural animator enabled? */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", BlueprintReadOnly)
		bool bEnable;
		/** Is procedural animator running evaluations? */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", BlueprintReadOnly)
		bool bEvaluationActive;
		/** Global animation play rate of Procedural Animator. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", VisibleAnywhere, BlueprintReadOnly)
		float playRate = 1.f;

		/** May the belocity be conserved instead of being set to Zero. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", EditAnywhere, BlueprintReadWrite)
		bool bConserveVelocityIfZero = true;

		/** Swing if to far time treshold. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", EditAnywhere, BlueprintReadWrite)
		float swingTimeTreshold = 0.095f;

	public:
		/** Multicast raised from procedural anim assets. */
		UPROPERTY(Category = "[NOBUNANIM]|Procedural Animator", BlueprintAssignable)
		FProceduralAnimAssetEvent OnProceduralAnimAssetEvent;


	public:
		/** Init the animator. */
		UFUNCTION(Category = "[NOBUNANIM]|Procedural Animator", BlueprintCallable)
		void Initialize(USkeletalMeshComponent* _Mesh /*,ULocomotionComponent* _Locomotion = nullptr*/);
		/** Inject procedural animation for a duration. */
		UFUNCTION(Category = "[NOBUNANIM]|Procedural Animator", BlueprintCallable)
		void Add(UProceduralAnimAsset* _Anim, int _FrameDuration, bool bCached = true);
		/** Remove the given procedural animation asset from the active anim set. */
		UFUNCTION(Category = "[NOBUNANIM]|Procedural Animator", BlueprintCallable)
		void Remove(UProceduralAnimAsset* _Anim);
		/** Remove the given key from the active anim set. */
		UFUNCTION(Category = "[NOBUNANIM]|Procedural Animator", BlueprintCallable)
		void RemoveFromName(FName _Key);

		UFUNCTION(Category = "[NOBUNANIM]|Procedural Animator", BlueprintCallable)
		void SetActive(bool _bEnable);

	protected:
		/** .*/
		virtual void UpdateEffectorTranslation_Implementation(const FName& _Socket, FVector _Translation, bool _bLerp, float _LerpSpeed) override;
		virtual void UpdateEffectorRotation_Implementation(const FName& _Socket, FRotator _Rotation, float _LerpSpeed) override;
		virtual void SetProceduralGaitEnable_Implementation(bool _bEnable) override;


	private:
		/** Global update. */
		void Evaluate_Internal();
		/** Per anim asset evaluation. */
		void EvaluateAnimSet();
		/** Update effector ideal location. */
		void UpdateEffectors();
		/** Per effector evaluation. */
		void EvaluateEffector(const UProceduralAnimAsset* _AnimAsset, const FProceduralAnimData& _AnimData, const FName& _EffectorName, float _CurrentTime);

		/** Update global weight based on all active procedural anim weight. */
		void UpdateGlobalWeight();

		/** Return if a value is in the range [0;1] and output adjusted ranges. */
		bool IsInSwingRange(float _Value, float _Min, float _Max, float& out_RangeMin, float& out_RangeMax);
};
