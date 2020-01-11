#pragma once

#include <Runtime/Core/Public/Containers/Map.h>
#include <Engine/Classes/Components/ActorComponent.h>

#include "GaitController.generated.h"

class UCurveFloat;
class UGaitDataAsset;
class USkeletalMeshComponent;
class IProceduralGaitInterface;
class UProceduralGaitAnimInstance;
struct FGaitDebugData;
class UCurveLinearColor;

DECLARE_DYNAMIC_DELEGATE(FSwingEvent);

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
	bool bForceSwing = false;

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
	float BlockTime = -1.f;
	bool bFlipFlop = false;
};


// RENAME AS UGAITCONTROLLERCOMPONENT
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class NOBUNANIM_API UGaitController : public UActorComponent
{
	GENERATED_BODY()

	private:
		/** Current gait monde name.*/
		FName CurrentGaitMode = "None";
		/** Current time of the cycle (in absolute time [0,1]).*/
		float CurrentTime = 0;
		/** Current time buffer used to compute current time.*/
		float TimeBuffer = 0;
		/** Owned anim instance. */
		UProceduralGaitAnimInstance* AnimInstanceRef = nullptr;

	protected:
		/** Owned mesh. */
		UPROPERTY(Category = "[NOBUNANIM]|Gait Controller", VisibleAnywhere, BlueprintReadOnly)
		USkeletalMeshComponent* OwnedMesh = nullptr;

		/** Map of Gaits data. Keys are the name of the Gait. */
		UPROPERTY(Category = "[NOBUNANIM]|Gait Controller", EditAnywhere, BlueprintReadOnly)
		TMap<FName, UGaitDataAsset*> GaitsData;

		/** Map of Gaits data. Keys are the name of the Gait. */
		//UPROPERTY(Category = "[NOBUNANIM]|Gait Controller", EditAnywhere, BlueprintReadOnly)
		TMap<FName, FGaitEffectorData> Effectors;

		/** Current playrate of the cycle.*/
		UPROPERTY(Category = "[NOBUNANIM]|Gait Controller", EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.001", ClampMax = "10", SliderMin = "0.001", SliderMax = "10.f"))
		float PlayRate = 1.f;
		
		/** Target FPS for LOD. */
		UPROPERTY(Category = "[NOBUNANIM]|Gait Controller|Optimization", EditAnywhere, BlueprintReadOnly)
		TMap<int32, int32> LODTargetFPS;

		/** Show effector debug. */
		UPROPERTY(Category = "[NOBUNANIM]|Gait Controller|Debug", EditAnywhere, BlueprintReadWrite)
		bool bShowDebug = false;

		/** Show effector debug. */
		UPROPERTY(Category = "[NOBUNANIM]|Gait Controller|Debug", EditAnywhere, BlueprintReadWrite)
		bool bShowLOD = false;

		/** @to do: documentation. */
		UPROPERTY(Category = "[NOBUNANIM]|Gait Controller|Debug", EditAnywhere, BlueprintReadWrite)
		UCurveLinearColor* LODsColorGradient = nullptr;

		/** */
		FVector LastVelocity;
		
		/** */
		int32 CurrentLOD = 0;


	public:	
		// Sets default values for this component's properties
		UGaitController();

	protected:
		// Called when the game starts
		virtual void BeginPlay() override;

	public:	
		// Called every frame
		virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		UFUNCTION(Category = "[NOBUNANIM]|Gait Controller", BlueprintNativeEvent, BlueprintCallable)
		void UpdateGaitMode(const FName& NewGaitName);

#if WITH_EDITOR
		/** Toggle NOBUNANIM gait controller debug. */
		UFUNCTION(Exec, Category = "[NOBUNANIM]|Gait Controller")
		void ShowGaitDebug();

		/** Toggle NOBUNANIM gait controller debug. */
		UFUNCTION(Exec, Category = "[NOBUNANIM]|Gait Controller")
		void ShowGaitLOD();
#endif
		
	private:
		/** Update effectors data.*/
		void UpdateEffectors();

		/** AARJHALJKDHFLKJDAHL(some kind of dying scream). */
		bool IsInRange(float Value, float Min, float Max, float& OutRangeMin, float& OutRangeMax);

		void DrawGaitDebug(FVector Position, FVector EffectorLocation, FVector CurrentLocation, float Treshold, bool bAutoAdjustWithIdealEffector, bool bForceSwing, const FGaitDebugData* DebugData);

		void UpdateLOD();
};
