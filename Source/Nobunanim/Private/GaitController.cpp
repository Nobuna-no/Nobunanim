// Fill out your copyright notice in the Description page of Project Settings.


#include "GaitController.h"

#include "Nobunanim/Private/Nobunanim.h"
#include "Nobunanim/Public/GaitDataAsset.h"
#include "Nobunanim/Public/ProceduralGaitAnimInstance.h"

#include <Engine/Classes/Curves/CurveVector.h>
#include <Engine/Classes/Animation/AnimInstance.h>
#include <Engine/Classes/GameFramework/Character.h>

// Sets default values for this component's properties
UGaitController::UGaitController()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGaitController::BeginPlay()
{
	Super::BeginPlay();

	// INIT REFERENCES
	{
		if (!AnimInstanceRef)
		{
			AActor* Owner = GetOwner();
			if (Owner)
			{
				ACharacter* OwnerCharacter = Cast<ACharacter>(Owner);
				OwnedMesh = OwnerCharacter->GetMesh();

				if (OwnedMesh)
				{
					UAnimInstance* AnimInstance = OwnedMesh->GetAnimInstance();

					if (AnimInstance)
					{
						AnimInstanceRef = Cast<UProceduralGaitAnimInstance>(AnimInstance);
						if (!AnimInstanceRef)
						{
							DEBUG_LOG_FORMAT(Warning, "Invalid anim instance own by actor %s.", *Owner->GetName());
						}
					}
					else
					{
						DEBUG_LOG_FORMAT(Warning, "Invalid anim instance own by actor %s.", *Owner->GetName());
					}
				}
			}
		}
	}

	{
		SetComponentTickEnabled(AnimInstanceRef);
	}
}



// Called every frame
void UGaitController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateEffectors();
	
	FVector NewCurrentLocation;
	FVector IdealEffectorLocation;
	FVector CurrentEffectorLocation;
	float Treshold;
	bool bForceSwing = false;
	
	LastVelocity = GetOwner()->GetVelocity();

	// Update Gaits Data.
	if (GaitsData.Contains(CurrentGaitMode) && LastVelocity.SizeSquared2D() != 0.f)
	{
		AnimInstanceRef->Execute_SetProceduralGaitEnable(AnimInstanceRef, true);

		UGaitDataAsset& CurrentAsset = *GaitsData[CurrentGaitMode];

		// Step 1: Timers.
		TimeBuffer += (DeltaTime * CurrentAsset.GetFrameRatio() * PlayRate);
		CurrentTime = FMath::Fmod(TimeBuffer, 1.f);

		TArray<FName> SwingValuesKeys;
		CurrentAsset.GaitSwingValues.GetKeys(SwingValuesKeys);
		
		// Step 2: Foreach swing values, we will check if we are in 'Swing' or 'Stance' according to @CurrentTime.
		for (int j = 0, m = SwingValuesKeys.Num(); j < m; ++j)
		{
			FName Key = SwingValuesKeys[j];
			FGaitSwingData& CurrentData = CurrentAsset.GaitSwingValues[Key];

			if (Effectors.Contains(Key))
			{
				FGaitEffectorData& Effector = Effectors[Key];

				// Step 2.1: Check if valid swing data.
				if ((!CurrentData.TranslationData.SwingTranslationCurve || CurrentData.TranslationData.TranslationFactor == FVector::ZeroVector) 
					&& (!CurrentData.RotationData.SwingRotationCurve || CurrentData.RotationData.RotationFactor == FVector::ZeroVector))
				{
					continue;
				}

				float MinRange, MaxRange;
				bool InRange = IsInRange(CurrentTime,
					Effector.bForceSwing ? Effector.BeginForceSwingInterval : CurrentData.BeginSwing,
					Effector.bForceSwing ? Effector.EndForceSwingInterval : CurrentData.EndSwing,
					MinRange, MaxRange);

				// Step 2.2: If the effector is in 'Swing'.
				if (InRange)
				{
					float CurrentCurvePosition = FMath::GetMappedRangeValueClamped(FVector2D(MinRange, MaxRange), FVector2D(0.f, 1.f), CurrentTime);
					
					// Step 2.2.1: Apply 'Swing' rotation (for bones).
					if (CurrentData.RotationData.SwingRotationCurve)
					{
						FVector CurrentCurveValue = CurrentData.RotationData.RotationFactor * CurrentData.RotationData.SwingRotationCurve->GetVectorValue(CurrentCurvePosition);
						
						AnimInstanceRef->Execute_UpdateEffectorRotation(AnimInstanceRef, Key, OwnedMesh->GetComponentRotation().RotateVector(CurrentCurveValue).Rotation());
					}
						
					// Step 2.2.1: Apply 'Swing' translation (for effectors IK(socket)).
					if (CurrentData.TranslationData.SwingTranslationCurve)
					{
						FVector CurrentCurveValue = CurrentData.TranslationData.TranslationFactor * CurrentData.TranslationData.TranslationSwingScale * CurrentData.TranslationData.SwingTranslationCurve->GetVectorValue(CurrentCurvePosition);
						CurrentCurveValue = CurrentData.TranslationData.bOrientToVelocity ? 
							LastVelocity.Rotation().RotateVector(CurrentCurveValue) : OwnedMesh->GetComponentRotation().RotateVector(CurrentCurveValue);
						FVector Offset = CurrentData.TranslationData.Offset * CurrentData.TranslationData.TranslationFactor;
						Offset = CurrentData.TranslationData.bOrientToVelocity ? 
							LastVelocity.Rotation().RotateVector(Offset) : OwnedMesh->GetComponentRotation().RotateVector(Offset);

						CurrentEffectorLocation = Effector.CurrentEffectorLocation;
						IdealEffectorLocation = Effector.IdealEffectorLocation;
						NewCurrentLocation = Effector.IdealEffectorLocation + Offset + CurrentCurveValue;
						Treshold = CurrentData.CorrectionData.DistanceTresholdToAdjust;
						bForceSwing = Effector.bForceSwing;

						AnimInstanceRef->Execute_UpdateEffectorTranslation(AnimInstanceRef, Key, NewCurrentLocation);
					
						Effector.CurrentEffectorLocation = NewCurrentLocation;
					}
				}
				// Step 2.3: If the effector is in 'Stance'.
				else
				{
					bForceSwing = Effector.bForceSwing = false;
					//Effector.BeginForceSwingInterval = CurrentAsset.GaitSwingValues[Key].BeginSwing;
					//Effector.EndForceSwingInterval = CurrentAsset.GaitSwingValues[Key].EndSwing;

					CurrentEffectorLocation = Effector.CurrentEffectorLocation;
					IdealEffectorLocation = Effector.IdealEffectorLocation;
					NewCurrentLocation = CurrentEffectorLocation;// Effector.IdealEffectorLocation + TranslationData.Offset + CurrentCurveValue;
					Treshold = CurrentData.CorrectionData.DistanceTresholdToAdjust;

					// Step 2.3.1: Check if the effector need to be adjusted
					if (CurrentAsset.GaitSwingValues[Key].CorrectionData.bAutoAdjustWithIdealEffector)
					{
						float VectorLength = (Effector.CurrentEffectorLocation - Effector.IdealEffectorLocation).Size();

						// Is the distance breaking treshold?
						if (VectorLength >= CurrentAsset.GaitSwingValues[Key].CorrectionData.DistanceTresholdToAdjust)
						{
							// Here we compute the new interval of swing.
							float Begin = CurrentAsset.GaitSwingValues[Key].BeginSwing;
							float End = CurrentAsset.GaitSwingValues[Key].EndSwing;
							
							float Alpha = 0.f;
							if (Begin > End)
							{
								Alpha = (1.f - Begin) + End;
							}
							else
							{
								Alpha = End - Begin;
							}

							float Beta = CurrentTime + Alpha;
							// if the end swing is > 1 (absolute time) we just consider that the swing will end the next cycle.
							if (Beta > 1.f)
							{
								Beta -= 1.f;
							}

							Effector.BeginForceSwingInterval = CurrentTime;
							Effector.EndForceSwingInterval = Beta;
							bForceSwing = Effector.bForceSwing = true;
						}
					}
				}

#if WITH_EDITOR
				// Step 3: Draw Debug.
				{
					DrawGaitDebug(NewCurrentLocation, IdealEffectorLocation, CurrentEffectorLocation, Treshold, 
						CurrentAsset.GaitSwingValues[Key].CorrectionData.bAutoAdjustWithIdealEffector, bForceSwing, 
						&CurrentAsset.GaitSwingValues[Key].DebugData);
				}
#endif
			}
		}
	}
	else
	{
		CurrentTime = 0;
		AnimInstanceRef->Execute_SetProceduralGaitEnable(AnimInstanceRef, false);
	}
}

void UGaitController::UpdateGaitMode_Implementation(const FName& NewGaitName)
{
	if (GaitsData.Contains(NewGaitName))
	{
		CurrentGaitMode = NewGaitName;
	}
	else
	{
		//DEBUG_LOG_FORMAT(Warning, "Invalid NewGaitName %s. There is no gait data corresponding. Ignored.", NewGaitName);
	}
}

void UGaitController::UpdateEffectors()
{
	TArray<FName> Keys;
	GaitsData.GetKeys(Keys);

	for (int i = 0, n = Keys.Num(); i < n; ++i)
	{
		TArray<FName> SwingValuesKeys;
		TMap<FName, FGaitSwingData>& SwingValues = GaitsData[Keys[i]]->GaitSwingValues;
		SwingValues.GetKeys(SwingValuesKeys);

		for (int j = 0, m = SwingValuesKeys.Num(); j < m; ++j)
		{
			FVector EffectorLocation = OwnedMesh->GetSocketLocation(SwingValuesKeys[j]);

			if (Effectors.Contains(SwingValuesKeys[j]))
			{
				Effectors[SwingValuesKeys[j]].IdealEffectorLocation = EffectorLocation;
			}
			else
			{
				FGaitEffectorData Effector;
				Effector.IdealEffectorLocation = EffectorLocation;
				Effectors.Add(SwingValuesKeys[j], Effector);
			}
		}
	}
}

bool UGaitController::IsInRange(float Value, float Min, float Max, float& OutRangeMin, float& OutRangeMax)
{
	bool A = Value >= Min && Value <= Max;
	bool APrime = Value >= Max;

	// Gave me headache ._.
	OutRangeMin = A ? Min : (APrime ? Min : 1.f - Min);
	OutRangeMax = A ? Max : (APrime ? 1.f /*- Min + Min*/ + Max : Max);

	if (A)
	{
		return true;
	}
	else
	{
		bool B1 = Value <= Min && Value <= Max;
		bool B2 = Value >= Min && Value >= Max;
		bool B3 = B1 || B2;
		bool C = Min >= Max && B3;

		if (C)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

void UGaitController::DrawGaitDebug(FVector Position, FVector EffectorLocation, FVector CurrentLocation, float Treshold, bool bAutoAdjustWithIdealEffector, bool bForceSwing, const FGaitDebugData* DebugData)
{
	if (bShowDebug && DebugData->bDrawDebug)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			DrawDebugPoint(World, Position, 2, FColor::Yellow, false, 1.f);
			DrawDebugSphere(World, Position, 2, 12, FColor(255.f, 130.f, 0.f), false, 0.f);
			DrawDebugSphere(World, EffectorLocation, 2, 12, FColor(255.f, 0.f, 0.f), false, 0.f);

			DrawDebugLine(World, EffectorLocation, Position, FColor(255.f, 255.f, 255.f), false, 0.f, 0, 0.5f);
			//DrawDebugLine(World, CurrentLocation, EffectorLocation, FColor(255.f, 0.f, 0.f), false, 0.f, 0, 1.f);

			FRotator Rot = GetOwner()->GetVelocity().Rotation();
			DrawDebugDirectionalArrow(World, EffectorLocation, EffectorLocation + (Rot.RotateVector(FVector::ForwardVector) * 10.f), 1, DebugData->VelocityColor, false, 0, 0, 1.f);
			
			if (bAutoAdjustWithIdealEffector)
			{
				DrawDebugCircle(World, FVector(CurrentLocation.X, CurrentLocation.Y, EffectorLocation.Z), Treshold, 20, bForceSwing ? DebugData->ForceSwingColor : DebugData->CorrectionCircleColor, false, 0.f, 0, bForceSwing ? 1.f : 0.75f, FVector(0,1,0), FVector(1,0,0), false);
			}
		}
	}
}


void UGaitController::ShowGaitDebug()
{
	bShowDebug = !bShowDebug;
}
