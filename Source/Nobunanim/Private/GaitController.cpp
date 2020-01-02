// Fill out your copyright notice in the Description page of Project Settings.


#include "GaitController.h"

#include "Nobunanim/Private/Nobunanim.h"
#include "Nobunanim/Public/GaitDataAsset.h"
#include "Nobunanim/Public/ProceduralGaitInterface.h"

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
						AnimInstanceRef = Cast<IProceduralGaitInterface>(AnimInstanceRef);
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

	// Update Gaits Data.
	if (GaitsData.Contains(CurrentGaitMode))
	{
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
				if ((!CurrentData.SwingTranslationCurve || CurrentData.TranslationFactor == FVector::ZeroVector) 
					&& (!CurrentData.SwingRotationCurve || CurrentData.RotationFactor == FVector::ZeroVector))
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
					if (CurrentData.SwingRotationCurve)
					{
						FVector CurrentCurveValue = CurrentData.RotationFactor * CurrentData.SwingRotationCurve->GetVectorValue(CurrentCurvePosition);
						
						AnimInstanceRef->UpdateEffectorRotation(Key, OwnedMesh->GetComponentRotation().RotateVector(CurrentCurveValue).Rotation());
					}
						
					// Step 2.2.1: Apply 'Swing' translation (for effectors IK(socket)).
					if (CurrentData.SwingTranslationCurve)
					{
						FVector CurrentCurveValue = CurrentData.TranslationFactor * CurrentData.SwingTranslationCurve->GetVectorValue(CurrentCurvePosition);
						FVector Offset = CurrentData.Offset * CurrentData.TranslationFactor;
						Offset = OwnedMesh->GetComponentRotation().RotateVector(Offset);
						CurrentCurveValue = OwnedMesh->GetComponentRotation().RotateVector(CurrentCurveValue);
						AnimInstanceRef->UpdateEffectorTranslation(Key, Effector.IdealEffectorLocation + Offset + CurrentCurveValue);
					
						if (Effectors.Contains(Key))
						{
							Effectors[Key].CurrentEffectorLocation = CurrentCurveValue;
							Effectors[Key].IdealEffectorLocation = Effector.IdealEffectorLocation;
						}
						else
						{
							FGaitEffectorData NewEffector;
							Effectors.Add(Key, NewEffector);
						}
					}
				}
				// Step 2.3: If the effector is in 'Stance'.
				else
				{
					Effector.bForceSwing = false;
					Effector.BeginForceSwingInterval = CurrentAsset.GaitSwingValues[Key].BeginSwing;
					Effector.EndForceSwingInterval = CurrentAsset.GaitSwingValues[Key].EndSwing;

					// Step 2.3.1: Check if the effector need to be adjusted
					if (CurrentAsset.GaitSwingValues[Key].bAutoAdjustWithIdealEffector)
					{
						float VectorLength = (Effector.CurrentEffectorLocation - Effector.IdealEffectorLocation).Size();

						// Is the distance breaking treshold?
						if (CurrentAsset.GaitSwingValues[Key].DistanceTresholdToAdjust >= VectorLength)
						{
							// Here we compute the new interval of swing.
							float Begin = CurrentAsset.GaitSwingValues[Key].BeginSwing;
							float End = CurrentAsset.GaitSwingValues[Key].EndSwing;
							
							float Alpha = 0.f;
							if (Begin > End)
							{
								Alpha = End - Begin;
							}
							else
							{
								Alpha = (1.f - Begin) + End;
							}

							float Beta = CurrentTime + Alpha;
							// if the end swing is > 1 (absolute time) we just consider that the swing will end the next cycle.
							if (Beta > 1.f)
							{
								Beta -= 1.f;
							}

							Effector.BeginForceSwingInterval = CurrentTime;
							Effector.EndForceSwingInterval = Beta;
							Effector.bForceSwing = true;
						}
					}
				}

				// Step 3: Draw Debug.
			}
		}
	}
	else
	{
		CurrentTime = 0;
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
