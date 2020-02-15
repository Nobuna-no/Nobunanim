// Fill out your copyright notice in the Description page of Project Settings.


#include "ProceduralGaitControllerComponent.h"

#include "Nobunanim/Private/Nobunanim.h"
#include "Nobunanim/Public/GaitDataAsset.h"
#include "Nobunanim/Public/ProceduralGaitAnimInstance.h"
#include "Nobunanim/Public/NobunanimSettings.h"

#include <Engine/Classes/Curves/CurveVector.h>
#include <Engine/Classes/Curves/CurveLinearColor.h>
#include <Engine/Classes/Animation/AnimInstance.h>
#include <Engine/Classes/GameFramework/Character.h>

#include <Engine/Classes/Kismet/KismetSystemLibrary.h>
#include <Engine/Classes/Kismet/KismetMathLibrary.h>

#define ORIENT_TO_VELOCITY(Input) LastVelocity.Rotation().RotateVector(Input)

#define SPHERECAST_IK_CORRECTION_RADIUS 30.f


// Sets default values for this component's properties
UProceduralGaitControllerComponent::UProceduralGaitControllerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UProceduralGaitControllerComponent::BeginPlay()
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
						AnimInstance = OwnedMesh->GetPostProcessInstance();
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
							DEBUG_LOG_FORMAT(Warning, "No valid anim instance own by actor %s.", *Owner->GetName());
						}
					}
				}
				else
				{
					SetComponentTickEnabled(false);
				}
			}
		}
	}

	{
		SetComponentTickEnabled(AnimInstanceRef);
	}
}



bool UProceduralGaitControllerComponent::TraceRay(UWorld* World, TArray<FHitResult>& HitResults, FVector Origin, FVector Dest, TEnumAsByte<ECollisionChannel> TraceChannel, float SphereCastRadius)
{
	const FProceduralGaitLODSettings& LODSetting = UNobunanimSettings::GetLODSetting(CurrentLOD);
	
	// if correction Level0 then zero computation.
	if (LODSetting.CorrectionLevel == ENobunanimIKCorrectionLevel::IKL_Level0)
	{
		return false;
	}

	FCollisionQueryParams SweepParam(*GetOwner()->GetName(), LODSetting.bTraceOnComplex, GetOwner());

	bool bFoundHit = World->LineTraceMultiByChannel
	(
		HitResults,
		Origin,
		Dest,
		TraceChannel,
		SweepParam,
		FCollisionResponseParams::DefaultResponseParam
	);

#if WITH_EDITOR
	if (LODSetting.Debug.bShowCollisionCorrection)
	{
		DrawDebugDirectionalArrow(World, Origin, Dest, 5, LODSetting.Debug.IKTraceColor, false, LODSetting.Debug.IKTraceDuration, 0, 0.5f);
	}
#endif

	if (LODSetting.CorrectionLevel == ENobunanimIKCorrectionLevel::IKL_Level1)
	{
		return bFoundHit;
	}

	if (!bFoundHit)
	{
		bFoundHit = World->SweepMultiByChannel
		(
			HitResults,
			Origin,
			Dest,
			FQuat::Identity,
			TraceChannel,
			FCollisionShape::MakeSphere(SphereCastRadius),
			SweepParam,
			FCollisionResponseParams::DefaultResponseParam
		);

#if WITH_EDITOR
		if (LODSetting.Debug.bShowCollisionCorrection)
		{
			DrawDebugCapsule(World, (Dest + Origin) * 0.5f, ((Dest - Origin)).Size()* 0.5f, SphereCastRadius, FQuat((Origin - Dest).GetUnsafeNormal().Rotation()), LODSetting.Debug.LODColor, false, LODSetting.Debug.IKTraceDuration, 0, .5f);
		}
#endif
	}

	return bFoundHit;
}

FHitResult& UProceduralGaitControllerComponent::GetBestHitResult(TArray<FHitResult>& HitResults, FVector IdealLocation)
{
	FHitResult& HitResult = HitResults[0];

	float BestDistance = 9999999.f;
	for (int i = 0, n = HitResults.Num(); i < n; ++i)
	{
		FHitResult&  CurrentHit = HitResults[i];

		float Temp = FMath::Abs(((IdealLocation)-CurrentHit.ImpactPoint).SizeSquared());
		if (Temp < BestDistance)
		{
			BestDistance = Temp;
			HitResult = CurrentHit;
		}
	}

	return HitResult;
}

// Called every frame
void UProceduralGaitControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	NOBUNANIM_SCOPE_COUNTER(ProceduralGait_Tick);

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	

	FVector NewCurrentLocation;
	FVector IdealEffectorLocation;
	FVector CurrentEffectorLocation;
	float Treshold = 0.f;
	bool bForceSwing = false;
	bool bAllEffectorBlendOutEnd = true;
	
	UWorld* World = GetWorld();
	FVector CurrentVelocity = GetOwner()->GetVelocity();

	if (!bGaitActive)
	{
		bLastFrameWasDisable = true;
	}


	// force 60 fps refresh rate
	const FProceduralGaitLODSettings& LODSetting = UNobunanimSettings::GetLODSetting(CurrentLOD);
	if (LODSetting.bForceDeltaTimeAtTargetFPS)
	{
		DeltaTime = 1.f / LODSetting.TargetFPS;
	}

	// Update Gaits Data.
	if (bGaitActive && GaitsData.Contains(CurrentGaitMode))
	{
		const UGaitDataAsset& CurrentAsset = *GaitsData[CurrentGaitMode];

		//if (bGaitActive)
		//{
		UpdateEffectors(CurrentAsset);
		if (bLastFrameWasDisable)
		{
			bLastFrameWasDisable = false;
			return;
		}
		//}

		bool bZeroVelocity = CurrentVelocity.SizeSquared() == 0.f;
		bool bCond = CurrentAsset.bComputeWithVelocityOnly ? !bZeroVelocity : true;
		
		if (bCond)
		{
			if (!bZeroVelocity)
			{
				LastVelocity = CurrentVelocity;
			}

			AnimInstanceRef->Execute_SetProceduralGaitEnable(AnimInstanceRef, true);
			
			// Step 1: Timers.
			TimeBuffer += (DeltaTime * CurrentAsset.GetFrameRatio() * PlayRate);
			CurrentTime = FMath::Fmod(TimeBuffer, 1.f);

			TArray<FName> SwingValuesKeys;
			CurrentAsset.GaitSwingValues.GetKeys(SwingValuesKeys);

			// Step 2: Foreach swing values, we will check if we are in 'Swing' or 'Stance' according to @CurrentTime.
			for (int j = 0, m = SwingValuesKeys.Num(); j < m; ++j)
			{
				FName Key = SwingValuesKeys[j];
				const FGaitSwingData& CurrentData = CurrentAsset.GaitSwingValues[Key];

				if (Effectors.Contains(Key))
				{
					FGaitEffectorData& Effector = Effectors[Key];

					// Update blend value
					{
						// If there isn't any pending gait, so it's a blend in
						if (PendingGaitMode.IsNone() || Effector.CurrentGait == PendingGaitMode)
						{
							if (CurrentData.BlendData.BlendInTime == 0)
							{
								Effector.CurrentBlendValue = 1.f;
							}
							else
							{
								bBlendIn = true;
								if (Effector.CurrentBlendValue < 1.f)
								{
									Effector.CurrentBlendValue = FMath::Clamp(Effector.CurrentBlendValue + (DeltaTime / CurrentData.BlendData.BlendInTime), 0.f, 1.f);
								}
							}

							if (PendingGaitMode.IsNone())
							{
								bAllEffectorBlendOutEnd = false;
							}
						}
						else
						{
							if (CurrentData.BlendData.BlendOutTime == 0)
							{
								Effector.CurrentBlendValue = 0.f;
								Effector.CurrentGait = PendingGaitMode;
							}
							else
							{
								bBlendIn = false;
								if (Effector.CurrentBlendValue > 0.f)
								{
									Effector.CurrentBlendValue = FMath::Clamp(Effector.CurrentBlendValue - (DeltaTime / CurrentData.BlendData.BlendOutTime), 0.f, 1.f);
								}

								// if end blend out, swap gait.
								if (Effector.CurrentBlendValue == 0.f)
								{
									Effector.CurrentGait = PendingGaitMode;
								}
							}
							bAllEffectorBlendOutEnd = false;
						}
					}

					// no need to check, check is made when @UpdateGaitMode.
					const UGaitDataAsset& UpdatedCurrentAsset = *GaitsData[Effector.CurrentGait.IsNone() ? CurrentGaitMode : Effector.CurrentGait];
					if (UpdatedCurrentAsset.GaitSwingValues.Contains(Key))
					{
						const FGaitSwingData& UpdatedCurrentData = UpdatedCurrentAsset.GaitSwingValues[Key];

						// Step 2.1: Check if valid swing data.
						/*if (Effector.bForceSwing)
						{
							if (!CurrentData.CorrectionData.CorrectionSwingTranslationCurve)
							{
								DEBUG_LOG_FORMAT(Warning, "Correction is query but there is no valid CorrectionSwingTranslationCurve. From actor %s.", *GetOwner()->GetName());
								continue;
							}
						}*/
						/*else if ((!CurrentData.TranslationData.SwingTranslationCurve || CurrentData.TranslationData.TranslationFactor == FVector::ZeroVector)
							&& (!CurrentData.RotationData.SwingRotationCurve || CurrentData.RotationData.RotationFactor == FVector::ZeroVector))
						{
							continue;
						}*/

						bool bCanCompute = Effector.BlockTime == -1.f
							|| (Effector.BlockTime > UpdatedCurrentData.BeginSwing && CurrentTime >= Effector.BlockTime)
							|| (Effector.BlockTime < UpdatedCurrentData.BeginSwing && CurrentTime < UpdatedCurrentData.BeginSwing && CurrentTime >= Effector.BlockTime);

						if (bCanCompute)
						{
							float MinRange, MaxRange;
							bool InRange = IsInRange(CurrentTime,
								Effector.bForceSwing ? Effector.BeginForceSwingInterval : UpdatedCurrentData.BeginSwing,
								Effector.bForceSwing ? Effector.EndForceSwingInterval : UpdatedCurrentData.EndSwing,
								MinRange, MaxRange);

							// Step 2.2: If the effector is in 'Swing'.
							if (InRange)
							{
								Effector.bCorrectionIK = false;

								float CurrentCurvePosition = FMath::GetMappedRangeValueClamped(FVector2D(MinRange, MaxRange), FVector2D(0.f, 1.f), CurrentTime);
								float lerpSpeed = UpdatedCurrentData.TranslationData.LerpSpeed * (Effector.CurrentBlendValue == 1.f ? 1.f : (bBlendIn ? (UpdatedCurrentData.BlendData.BlendInAcceleration ? UpdatedCurrentData.BlendData.BlendInAcceleration->GetFloatValue(Effector.CurrentBlendValue) : 1.f) : (UpdatedCurrentData.BlendData.BlendOutAcceleration ? UpdatedCurrentData.BlendData.BlendOutAcceleration->GetFloatValue(Effector.CurrentBlendValue) : 1.f)));

								// Step 2.2.1: Apply 'Swing' rotation (for bones).
								if (UpdatedCurrentData.RotationData.bAffectEffector)
								{
									FVector CurrentCurveValue = UpdatedCurrentData.RotationData.RotationFactor * (UpdatedCurrentData.RotationData.SwingRotationCurve ? 
										UpdatedCurrentData.RotationData.SwingRotationCurve->GetVectorValue(CurrentCurvePosition) : FVector(1, 1, 1));
								
									AnimInstanceRef->Execute_UpdateEffectorRotation(AnimInstanceRef, Key, FRotator(CurrentCurveValue.X, CurrentCurveValue.Y, CurrentCurveValue.Z)/*OwnedMesh->GetComponentRotation().RotateVector(CurrentCurveValue).Rotation()*/, lerpSpeed);
								}

								// Step 2.2.1: Apply 'Swing' translation (for effectors IK(socket)).
								//if (UpdatedCurrentData.TranslationData.SwingTranslationCurve || (Effector.bForceSwing && UpdatedCurrentData.CorrectionData.CorrectionSwingTranslationCurve))
								if (UpdatedCurrentData.TranslationData.bAffectEffector)
								{
									FVector CurrentCurveValue = UpdatedCurrentData.TranslationData.TranslationFactor * UpdatedCurrentData.TranslationData.TranslationSwingScale
										* (Effector.bForceSwing ? (UpdatedCurrentData.CorrectionData.CorrectionSwingTranslationCurve ? UpdatedCurrentData.CorrectionData.CorrectionSwingTranslationCurve->GetVectorValue(CurrentCurvePosition) : FVector(1, 1, 1))
											: (UpdatedCurrentData.TranslationData.SwingTranslationCurve ? UpdatedCurrentData.TranslationData.SwingTranslationCurve->GetVectorValue(CurrentCurvePosition) : FVector(1, 1, 1)));

									CurrentCurveValue = UpdatedCurrentData.TranslationData.bOrientToVelocity ?
										ORIENT_TO_VELOCITY(CurrentCurveValue) : OwnedMesh->GetComponentRotation().RotateVector(CurrentCurveValue);

									FVector Offset = UpdatedCurrentData.TranslationData.Offset * UpdatedCurrentData.TranslationData.TranslationFactor;
									Offset = UpdatedCurrentData.TranslationData.bOrientToVelocity ?
										ORIENT_TO_VELOCITY(Offset) : OwnedMesh->GetComponentRotation().RotateVector(Offset);

									CurrentEffectorLocation = Effector.CurrentEffectorLocation;
									IdealEffectorLocation = Effector.IdealEffectorLocation;
									NewCurrentLocation = Effector.IdealEffectorLocation + Offset + CurrentCurveValue;
									Treshold = UpdatedCurrentData.CorrectionData.DistanceTresholdToAdjust;
									bForceSwing = Effector.bForceSwing;

									if (UpdatedCurrentData.TranslationData.bAdaptToGroundLevel)
									{
										NewCurrentLocation.Z += Effector.IdealEffectorLocation.Z - Effector.GroundLocation.Z;
									}

									AnimInstanceRef->Execute_UpdateEffectorTranslation(AnimInstanceRef, Key, NewCurrentLocation, !bLastFrameWasDisable, lerpSpeed);

									Effector.CurrentEffectorLocation = NewCurrentLocation;
								}
							}
							// Step 2.3: If the effector is in 'Stance'.
							else
							{
								if (Effector.bForceSwing)
								{
									Effector.BlockTime = UpdatedCurrentData.EndSwing >= 0.99f ? 0.f : UpdatedCurrentData.EndSwing;
								}
								else
								{
									Effector.BlockTime = -1.f;

									// check for foot ik.
									if (!bLastFrameWasDisable && World && !Effector.bCorrectionIK && UpdatedCurrentData.CorrectionData.bComputeCollision)
									{
										//const FProceduralGaitLODSettings& LODSetting = UNobunanimSettings::GetLODSetting(CurrentLOD);
										if (LODSetting.bCanComputeCollisionCorrection)
										{
											NOBUNANIM_SCOPE_COUNTER(ProceduralGait_StanceIKCorrection);

											// Get origin for IK
											FVector Origin;
											if (UpdatedCurrentData.CorrectionData.bUseCurrentEffector)
											{
												Origin = Effector.CurrentEffectorLocation;
											}
											else
											{
												Origin = UpdatedCurrentData.CorrectionData.OriginCollisionSocketName.IsNone() ?
													OwnedMesh->GetSocketLocation(Key) :
													OwnedMesh->GetSocketLocation(UpdatedCurrentData.CorrectionData.OriginCollisionSocketName);
											}
											FVector Dir = UpdatedCurrentData.CorrectionData.bOrientToVelocity ? ORIENT_TO_VELOCITY(UpdatedCurrentData.CorrectionData.AbsoluteDirection) : UpdatedCurrentData.CorrectionData.AbsoluteDirection;

											// Get Dest
											FVector Dest = Origin + Dir;

											// Add inverse absolute direction
											Origin -= Dir;

											FCollisionQueryParams SweepParam(*GetOwner()->GetName(), false, GetOwner());
											TArray<FHitResult> HitResults;
											bool bFoundHit = TraceRay(World, HitResults, Origin, Dest, UpdatedCurrentData.CorrectionData.TraceChannel, SPHERECAST_IK_CORRECTION_RADIUS);


											if (bFoundHit)
											{

												FHitResult& HitResult = GetBestHitResult(HitResults, Origin);

												{
#if WITH_EDITOR
													if (LODSetting.Debug.bShowCollisionCorrection)
													{
														if (HitResult.bBlockingHit)
														{
															DrawDebugPoint(World, HitResult.ImpactPoint + ORIENT_TO_VELOCITY(UpdatedCurrentData.CorrectionData.CollisionSnapOffset), 10.f, FColor::Red, false, 3.f);
														}
													}
#endif
												}

												// if hit ground
												if (HitResult.bBlockingHit)
												{
													Effector.CurrentEffectorLocation = HitResult.ImpactPoint + ORIENT_TO_VELOCITY(UpdatedCurrentData.CorrectionData.CollisionSnapOffset);
													Effector.bCorrectionIK = true;
													if (UpdatedCurrentData.EventData.bRaiseOnCollisionEvent)
													{
														OnCollisionEvent.Broadcast(Key, Effector.CurrentEffectorLocation);
													}
												}
											}
										}
									}

									AnimInstanceRef->Execute_UpdateEffectorTranslation(AnimInstanceRef, Key, Effector.CurrentEffectorLocation, !bLastFrameWasDisable, UpdatedCurrentData.TranslationData.LerpSpeed);
								}

								bForceSwing = Effector.bForceSwing = false;
								//Effector.BeginForceSwingInterval = CurrentAsset.GaitSwingValues[Key].BeginSwing;
								//Effector.EndForceSwingInterval = CurrentAsset.GaitSwingValues[Key].EndSwing;

								CurrentEffectorLocation = Effector.CurrentEffectorLocation;
								IdealEffectorLocation = Effector.IdealEffectorLocation;
								NewCurrentLocation = CurrentEffectorLocation;// Effector.IdealEffectorLocation + TranslationData.Offset + CurrentCurveValue;
								Treshold = UpdatedCurrentData.CorrectionData.DistanceTresholdToAdjust;

								// Step 2.3.1: Check if the effector need to be adjusted
								if (!bLastFrameWasDisable && CurrentAsset.GaitSwingValues[Key].CorrectionData.bAutoAdjustWithIdealEffector)
								{
									float VectorLength = (Effector.CurrentEffectorLocation - Effector.IdealEffectorLocation).Size2D();

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
										if (Beta >= 1.f)
										{
											Beta -= 1.f;
										}

										Effector.BeginForceSwingInterval = CurrentTime;
										Effector.EndForceSwingInterval = Beta;
										bForceSwing = Effector.bForceSwing = true;
										Effector.BlockTime = -1.f;
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
			}

			// Step 3: if all effector end blend out swtich gait
			if (bAllEffectorBlendOutEnd == true)
			{
				CurrentGaitMode = PendingGaitMode;
				PendingGaitMode = "";
			}
		}
		else
		{
			CurrentTime = 0;
			AnimInstanceRef->Execute_SetProceduralGaitEnable(AnimInstanceRef, false);
		}

	}
	else
	{
		CurrentTime = 0;
		if (AnimInstanceRef)
		{
			AnimInstanceRef->Execute_SetProceduralGaitEnable(AnimInstanceRef, false);
		}
	}

#if WITH_EDITOR
	UpdateLOD(true);
#else
	UpdateLOD();
#endif
	
}

void UProceduralGaitControllerComponent::ComputeCollisionCorrection(const FGaitCorrectionData* CorrectionData, FGaitEffectorData& Effector)
{

}

void UProceduralGaitControllerComponent::UpdateGaitMode_Implementation(const FName& NewGaitName)
{
	if (GaitsData.Contains(NewGaitName))
	{
		if (CurrentGaitMode != NewGaitName)
		{
			if (CurrentGaitMode.IsNone() || CurrentGaitMode == "None")
			{
				CurrentGaitMode = NewGaitName;
			}
			else
			{
				PendingGaitMode = NewGaitName;
			}
			//CurrentGaitMode = NewGaitName;
			SetComponentTickEnabled(true);
		}
	}
	else
	{
		SetComponentTickEnabled(false);
		//DEBUG_LOG_FORMAT(Warning, "Invalid NewGaitName %s. There is no gait data corresponding. Ignored.", NewGaitName);
	}
}

void UProceduralGaitControllerComponent::UpdateEffectors(const UGaitDataAsset& CurrentAsset)
{
	NOBUNANIM_SCOPE_COUNTER(ProceduralGait_UpdateEffectors);

	TArray<FName> Keys;
	GaitsData.GetKeys(Keys);

	for (int i = 0, n = Keys.Num(); i < n; ++i)
	{
		TArray<FName> SwingValuesKeys;
		TMap<FName, FGaitSwingData>& SwingValues = GaitsData[Keys[i]]->GaitSwingValues;
		SwingValues.GetKeys(SwingValuesKeys);

		for (int j = 0, m = SwingValuesKeys.Num(); j < m; ++j)
		{
			FName Key = SwingValuesKeys[j];
			FVector EffectorLocation = OwnedMesh->GetSocketLocation(Key);
			if (Effectors.Contains(Key))
			{
				Effectors[Key].IdealEffectorLocation = EffectorLocation;

				if (CurrentAsset.GaitSwingValues.Contains(Key) && CurrentAsset.GaitSwingValues[Key].TranslationData.bAdaptToGroundLevel)
				{
					FVector GroundLocation;
					TArray<FHitResult> HitResults;
					bool bFound = TraceRay(GetWorld(), HitResults, EffectorLocation, EffectorLocation + FVector(0, 0, -100.f), ECollisionChannel::ECC_WorldStatic, SPHERECAST_IK_CORRECTION_RADIUS);
					if (!bFound)
					{
						GroundLocation = EffectorLocation + FVector(0, 0, -100.f);
					}
					else
					{
						FHitResult& HitResult = GetBestHitResult(HitResults, EffectorLocation);
						GroundLocation = HitResult.ImpactPoint;
					}

					Effectors[Key].GroundLocation = GroundLocation;
				}

				if (bLastFrameWasDisable)
				{
					Effectors[Key].CurrentEffectorLocation = EffectorLocation;
					AnimInstanceRef->Execute_UpdateEffectorTranslation(AnimInstanceRef, Key, EffectorLocation, false, 0);
				}
			}
			else
			{
				FGaitEffectorData Effector;
				Effector.IdealEffectorLocation = EffectorLocation;
				Effectors.Add(Key, Effector);
			}
		}
	}
}


bool UProceduralGaitControllerComponent::IsInRange(float Value, float Min, float Max, float& OutRangeMin, float& OutRangeMax)
{
	bool A = Value >= Min && Value <= Max;
	bool APrime = Value >= Max;

	// Gave me headache ._.
	OutRangeMin = A ? Min : (APrime ? Min : Min - 1.f);
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

void UProceduralGaitControllerComponent::DrawGaitDebug(FVector Position, FVector EffectorLocation, FVector CurrentLocation, float Treshold, bool bAutoAdjustWithIdealEffector, bool bForceSwing, const FGaitDebugData* DebugData)
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

			FRotator Rot = LastVelocity.Rotation();
			DrawDebugDirectionalArrow(World, EffectorLocation, EffectorLocation + (Rot.RotateVector(FVector::ForwardVector) * 10.f), 1, DebugData->VelocityColor, false, 0, 0, 1.f);
			
			if (bAutoAdjustWithIdealEffector)
			{
				DrawDebugCircle(World, FVector(CurrentLocation.X, CurrentLocation.Y, EffectorLocation.Z), Treshold, 20, bForceSwing ? DebugData->ForceSwingColor : DebugData->CorrectionCircleColor, false, 0.f, 0, bForceSwing ? 1.f : 0.75f, FVector(0,1,0), FVector(1,0,0), false);
			}
		}
	}

#if WITH_EDITOR
	if (UNobunanimSettings::GetLODSetting(CurrentLOD).Debug.bShowLOD 
		|| bShowLOD
		)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			DrawDebugSolidBox(World, GetOwner()->GetActorLocation(), GetOwner()->GetSimpleCollisionCylinderExtent(), UNobunanimSettings::GetLODSetting(CurrentLOD).Debug.LODColor, false, 0.f);
		}
	}
#endif
}


#if WITH_EDITOR
void UProceduralGaitControllerComponent::ShowGaitDebug()
{
	bShowDebug = !bShowDebug;
}

void UProceduralGaitControllerComponent::ShowGaitLOD()
{
	bShowLOD = !bShowLOD;
}
#endif

void UProceduralGaitControllerComponent::UpdateLOD(bool bForceUpdate)
{
	if (CurrentLOD != OwnedMesh->PredictedLODLevel
		|| bForceUpdate)
	{
		SetComponentTickInterval(1.f / (float)UNobunanimSettings::GetLODSetting(CurrentLOD = OwnedMesh->PredictedLODLevel).TargetFPS);// LODTargetFPS[CurrentLOD = OwnedMesh->PredictedLODLevel]);
	}
}

