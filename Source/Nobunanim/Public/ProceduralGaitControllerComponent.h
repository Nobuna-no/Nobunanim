#pragma once

#include <Runtime/Core/Public/Containers/Map.h>
#include <Engine/Classes/Components/ActorComponent.h>

#include "ProceduralGaitControllerComponent.generated.h"

class UCurveFloat;
class UGaitDataAsset;
class USkeletalMeshComponent;
class IProceduralGaitInterface;
class UProceduralGaitAnimInstance;

struct FGaitDebugData;
struct FGaitCorrectionData;

DECLARE_DYNAMIC_DELEGATE(FSwingEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEffectorCollision, FName, EffectorName, FVector, ImpactLocation);

USTRUCT(BlueprintType)
struct FGaitEffectorData
{
	GENERATED_BODY()

	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Effector Data", EditAnywhere, BlueprintReadWrite)
	FVector CurrentEffectorLocation = FVector::ZeroVector;
	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Effector Data", EditAnywhere, BlueprintReadWrite)
	FVector IdealEffectorLocation = FVector::ZeroVector;

	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Effector Data", EditAnywhere, BlueprintReadWrite)
	FVector GroundLocation = FVector::ZeroVector;

	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Effector Data", EditAnywhere, BlueprintReadWrite)
	bool bForceSwing = false;

	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Effector Data", EditAnywhere, BlueprintReadWrite)
	bool bCorrectionIK = false;

	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Effector Data", EditAnywhere, BlueprintReadWrite)
	float BeginForceSwingInterval = 0;
	/** @to do: Documentation. */
	UPROPERTY(Category = "[NOBUNANIM]|Effector Data", EditAnywhere, BlueprintReadWrite)
	float EndForceSwingInterval = 0;

	//FSwingEvent OnBeginSwing;
	//FSwingEvent OnEndSwing;

	//FSwingEvent OnBeginForceSwing;
	//FSwingEvent OnEndForceSwing;

	/** @to do: Documentation. */
	float CurrentBlendValue = 0.f;
	/** @to do: Documentation. */
	FName CurrentGait;
	/** @to do: Documentation. */
	float BlockTime = -1.f;
};


// RENAME AS UProceduralProceduralGaitControllerComponentComponentCOMPONENT
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class NOBUNANIM_API UProceduralGaitControllerComponent : public UActorComponent
{
	GENERATED_BODY()

	private:
		/** Current gait monde name.*/
		FName CurrentGaitMode;
		/** Pending gait mode name. Use to switch gaot with blend time. */
		FName PendingGaitMode = "";
		/** Current time of the cycle (in absolute time [0,1]).*/
		float CurrentTime = 0;
		/** Current time buffer used to compute current time.*/
		float TimeBuffer = 0;
		/** Owned anim instance. */
		UProceduralGaitAnimInstance* AnimInstanceRef = nullptr;
		/** Current LOD.*/
		int32 CurrentLOD = 0;
		/** @to do: document. */
		bool bBlendIn = true;



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

	protected:
		/** Map of Gaits data. Keys are the name of the Gait. */
		TMap<FName, FGaitEffectorData> Effectors;
	
		/** @to do: documentation. */
		FVector LastVelocity;
		
		/** */
		bool bLastFrameWasDisable = true;


	public:	
		// Sets default values for this component's properties
		UProceduralGaitControllerComponent();

	protected:
		// Called when the game starts
		virtual void BeginPlay() override;

	public:	
		// Called every frame
		virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		UFUNCTION(Category = "[NOBUNANIM]|Gait Controller", BlueprintNativeEvent, BlueprintCallable)
		void UpdateGaitMode(const FName& NewGaitName);
		
		/** Called each time than an effector that must raise the event enter in collision. */
		UPROPERTY(Category = "[NOBUNANIM]|Gait Controller|Debug", BlueprintAssignable)
		FOnEffectorCollision OnCollisionEvent;


#if WITH_EDITOR
		/** [NOBUNANIM] Toggle procedural gait debug for this actor. */
		UFUNCTION(Exec, Category = "[NOBUNANIM]|Gait Controller")
		void ShowGaitDebug();

		/** [NOBUNANIM] Toggle procedural gait LOD debug for this actor. */
		UFUNCTION(Exec, Category = "[NOBUNANIM]|Gait Controller")
		void ShowGaitLOD();
#endif


	private:
		/** .*/
		void ComputeCollisionCorrection(const FGaitCorrectionData* CorrectionData, FGaitEffectorData& Effector);

		/** Update effectors data.*/
		void UpdateEffectors(const UGaitDataAsset& CurrentAsset);

		/** AARJHALJKDHFLKJDAHL(some kind of dying scream). */
		bool IsInRange(float Value, float Min, float Max, float& OutRangeMin, float& OutRangeMax);

		void DrawGaitDebug(FVector Position, FVector EffectorLocation, FVector CurrentLocation, float Treshold, bool bAutoAdjustWithIdealEffector, bool bForceSwing, const FGaitDebugData* DebugData);

		void UpdateLOD(bool bForceUpdate = false);

		bool TraceRay(UWorld* World, TArray<FHitResult>& HitResults, FVector Origin, FVector Dest, TEnumAsByte<ECollisionChannel> TraceChannel, float SphereCastRadius);

		FHitResult& GetBestHitResult(TArray<FHitResult>& HitResults, FVector IdealLocation);
		
		//FVector RotateToVelocity(FVector Input);
};
