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
						DEBUG_LOG_FORMAT(Warning, "Invalid anim instance own by actor %s.", *Owner->GetName());
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
	FCollisionQueryParams SweepParam(*GetOwner()->GetName(), false, GetOwner());

#if WITH_EDITOR
	const FProceduralGaitLODSettings& LODSetting = UNobunanimSettings::GetLODSetting(CurrentLOD);
#endif

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
		DrawDebugDirectionalArrow(World, Origin, Dest, 5, LODSetting.Debug.IKTraceColor, false, .5f, 0, 0.5f);
	}
#endif

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
			DrawDebugCapsule(World, (Dest + Origin) * 0.5f, ((Dest - Origin)).Size()* 0.5f, SphereCastRadius, FQuat((Dest - Origin).GetUnsafeNormal().Rotation().GetInverse()), LODSetting.Debug.LODColor, false, 0.5f, 0, .5f);
		}
#endif
	}

	return bFoundHit;
}

FHitResult& UProceduralGaitControllerComponent::GetBestHitResult(TArray<FHitResult>& HitResults, FVector IdealLocation)
{
//#if WITH_EDITOR
//	const FProceduralGaitLODSettings& LODSetting = UNobunanimSettings::GetLODSetting(CurrentLOD);
//#endif

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

//#if WITH_EDITOR
//		if (LODSetting.Debug.bShowCollisionCorrection && CurrentHit.bBlockingHit)
//		{
//			if (CurrentHit.bBlockingHit)
//			{
//				DrawDebugPoint(World, CurrentHit.ImpactPoint, 5.f, FColor::Yellow, false, 3.f);
//			}
//		}
//#endif
	}

	return HitResult;
}

// Called every frame
void UProceduralGaitControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	UpdateEffectors();
	
	FVector NewCurrentLocation;
	FVector IdealEffectorLocation;
	FVector CurrentEffectorLocation;
	float Treshold;
	bool bForceSwing = false;
	
	FVector CurrentVelocity = GetOwner()->GetVelocity();
	
	UWorld* World = GetWorld();

	// Update Gaits Data.
	if (GaitsData.Contains(CurrentGaitMode))
	{
		UGaitDataAsset& CurrentAsset = *GaitsData[CurrentGaitMode];
		
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
				FGaitSwingData& CurrentData = CurrentAsset.GaitSwingValues[Key];

				if (Effectors.Contains(Key))
				{
					FGaitEffectorData& Effector = Effectors[Key];

					// Step 2.1: Check if valid swing data.
					if (Effector.bForceSwing)
					{
						if (!CurrentData.CorrectionData.CorrectionSwingTranslationCurve)
						{
							DEBUG_LOG_FORMAT(Warning, "Correction is query but there is no valid CorrectionSwingTranslationCurve. From actor %s.", *GetOwner()->GetName());
							continue;
						}
					}
					/*else if ((!CurrentData.TranslationData.SwingTranslationCurve || CurrentData.TranslationData.TranslationFactor == FVector::ZeroVector)
						&& (!CurrentData.RotationData.SwingRotationCurve || CurrentData.RotationData.RotationFactor == FVector::ZeroVector))
					{
						continue;
					}*/

					bool bCanCompute = Effector.BlockTime == -1.f
						|| (Effector.BlockTime > CurrentData.BeginSwing && CurrentTime >= Effector.BlockTime)
						|| (Effector.BlockTime < CurrentData.BeginSwing && CurrentTime < CurrentData.BeginSwing && CurrentTime >= Effector.BlockTime);

					if (bCanCompute)
					{
						float MinRange, MaxRange;
						bool InRange = IsInRange(CurrentTime,
							Effector.bForceSwing ? Effector.BeginForceSwingInterval : CurrentData.BeginSwing,
							Effector.bForceSwing ? Effector.EndForceSwingInterval : CurrentData.EndSwing,
							MinRange, MaxRange);

						// Step 2.2: If the effector is in 'Swing'.
						if (InRange)
						{
							Effector.bCorrectionIK = false;

							float CurrentCurvePosition = FMath::GetMappedRangeValueClamped(FVector2D(MinRange, MaxRange), FVector2D(0.f, 1.f), CurrentTime);

							// Step 2.2.1: Apply 'Swing' rotation (for bones).
							if (CurrentData.RotationData.SwingRotationCurve)
							{
								FVector CurrentCurveValue = CurrentData.RotationData.RotationFactor * CurrentData.RotationData.SwingRotationCurve->GetVectorValue(CurrentCurvePosition);

								AnimInstanceRef->Execute_UpdateEffectorRotation(AnimInstanceRef, Key, OwnedMesh->GetComponentRotation().RotateVector(CurrentCurveValue).Rotation(), CurrentData.TranslationData.LerpSpeed);
							}

							// Step 2.2.1: Apply 'Swing' translation (for effectors IK(socket)).
							if (CurrentData.TranslationData.SwingTranslationCurve || (Effector.bForceSwing && CurrentData.CorrectionData.CorrectionSwingTranslationCurve))
							{
								FVector CurrentCurveValue = CurrentData.TranslationData.TranslationFactor * CurrentData.TranslationData.TranslationSwingScale 
									* (Effector.bForceSwing ? CurrentData.CorrectionData.CorrectionSwingTranslationCurve->GetVectorValue(CurrentCurvePosition) 
										: CurrentData.TranslationData.SwingTranslationCurve->GetVectorValue(CurrentCurvePosition));
								
								CurrentCurveValue = CurrentData.TranslationData.bOrientToVelocity ?
									ORIENT_TO_VELOCITY(CurrentCurveValue) : OwnedMesh->GetComponentRotation().RotateVector(CurrentCurveValue);
								FVector Offset = CurrentData.TranslationData.Offset * CurrentData.TranslationData.TranslationFactor;
								Offset = CurrentData.TranslationData.bOrientToVelocity ?
									ORIENT_TO_VELOCITY(Offset) : OwnedMesh->GetComponentRotation().RotateVector(Offset);

								CurrentEffectorLocation = Effector.CurrentEffectorLocation;
								IdealEffectorLocation = Effector.IdealEffectorLocation;
								NewCurrentLocation = Effector.IdealEffectorLocation + Offset + CurrentCurveValue;
								Treshold = CurrentData.CorrectionData.DistanceTresholdToAdjust;
								bForceSwing = Effector.bForceSwing;

								if (CurrentData.TranslationData.bAdaptToGroundLevel)
								{
									NewCurrentLocation.Z += Effector.IdealEffectorLocation.Z - Effector.GroundLocation.Z;
								}

								AnimInstanceRef->Execute_UpdateEffectorTranslation(AnimInstanceRef, Key, NewCurrentLocation, CurrentData.TranslationData.LerpSpeed);

								Effector.CurrentEffectorLocation = NewCurrentLocation;
							}
						}
						// Step 2.3: If the effector is in 'Stance'.
						else
						{
							if (Effector.bForceSwing)
							{
								Effector.BlockTime = CurrentData.EndSwing >= 0.99f ? 0.f : CurrentData.EndSwing;
							}
							else
							{
								Effector.BlockTime = -1.f;

								// check for foot ik.
								if (World && !Effector.bCorrectionIK && CurrentData.CorrectionData.bComputeCollision)
								{
									const FProceduralGaitLODSettings& LODSetting = UNobunanimSettings::GetLODSetting(CurrentLOD);
									if (LODSetting.bCanComputeCollisionCorrection)
									{
										// Get origin for IK
										FVector Origin;
										if (CurrentData.CorrectionData.bUseCurrentEffector)
										{
											Origin = Effector.CurrentEffectorLocation;
										}
										else
										{
											Origin = CurrentData.CorrectionData.OriginCollisionSocketName.IsNone() ?
												OwnedMesh->GetSocketLocation(Key) :
												OwnedMesh->GetSocketLocation(CurrentData.CorrectionData.OriginCollisionSocketName);
										}
										FVector Dir = CurrentData.CorrectionData.bOrientToVelocity ? ORIENT_TO_VELOCITY(CurrentData.CorrectionData.AbsoluteDirection) : CurrentData.CorrectionData.AbsoluteDirection;

										// Get Dest
										FVector Dest = Origin + Dir;

										// Add inverse absolute direction
										Origin -= Dir;

										FCollisionQueryParams SweepParam(*GetOwner()->GetName(), false, GetOwner());
										TArray<FHitResult> HitResults;
										bool bFoundHit = TraceRay(World, HitResults, Origin, Dest, CurrentData.CorrectionData.TraceChannel, CurrentData.CorrectionData.CollisionRadius);

										/*bool bFoundHit = World->LineTraceMultiByChannel
										(
											HitResults,
											Origin,
											Dest,
											CurrentData.CorrectionData.TraceChannel,
											SweepParam,
											FCollisionResponseParams::DefaultResponseParam
										);

									#if WITH_EDITOR
										if (LODSetting.Debug.bShowCollisionCorrection)
										{
											DrawDebugDirectionalArrow(World, Origin, Dest, 5, LODSetting.Debug.IKTraceColor, false, .5f, 0, 0.5f);
										}
									#endif

										if (!bFoundHit)
										{
											bFoundHit = World->SweepMultiByChannel
											(
												HitResults,
												Origin,
												Dest,
												FQuat::Identity,
												CurrentData.CorrectionData.TraceChannel,
												FCollisionShape::MakeSphere(CurrentData.CorrectionData.CollisionRadius),
												SweepParam,
												FCollisionResponseParams::DefaultResponseParam
											);

										#if WITH_EDITOR
											if (LODSetting.Debug.bShowCollisionCorrection)
											{
												DrawDebugCapsule(World, (Dest + Origin) * 0.5f, ((Dest - Origin)).Size()* 0.5f, CurrentData.CorrectionData.CollisionRadius, FQuat((Dest - Origin).GetUnsafeNormal().Rotation().GetInverse()), LODSetting.Debug.LODColor, false, 0.5f, 0, .5f);
											}
										#endif
										}
									*/

										if (bFoundHit)
										{
											
											FHitResult& HitResult = GetBestHitResult(HitResults, Origin);

											/*float BestDistance = 9999999.f;
											for (int i = 0, n = HitResults.Num(); i < n; ++i)
											{
												FHitResult& CurrentHit = HitResults[i];

												float Temp = FMath::Abs(((Origin) - CurrentHit.ImpactPoint).SizeSquared());
												if (Temp < BestDistance)
												{
													BestDistance = Temp;
													HitResult = CurrentHit;
												}

											#if WITH_EDITOR
												if (LODSetting.Debug.bShowCollisionCorrection && CurrentHit.bBlockingHit)
												{
													if (CurrentHit.bBlockingHit)
													{
														DrawDebugPoint(World, CurrentHit.ImpactPoint, 5.f, FColor::Yellow, false, 3.f);
													}
												}
											#endif
											}*/

											{
												//DrawDebugSphere(World, Origin, CurrentData.CorrectionData.CollisionRadius, 12, LODSetting.Debug.LODColor, false, 0.f);
												//DrawDebugSphereTraceSingle(,);
												if (HitResult.bBlockingHit)
												{
													DrawDebugPoint(World, HitResult.ImpactPoint + ORIENT_TO_VELOCITY(CurrentData.CorrectionData.CollisionSnapOffset), 10.f, FColor::Red, false, 3.f);
													//DrawDebugPoint(World, HitResult.ImpactPoint + CurrentData.CorrectionData.CollisionSnapOffset, 2.5f, FColor::Orange, false, 5);
													//DrawDebugLine(World, Origin, Origin + CurrentData.CorrectionData.AbsoluteDirection, FColor::Green, false, 5, 0, .5f);
												}
											}

											// if hit ground
											if (HitResult.bBlockingHit)
											{
												Effector.CurrentEffectorLocation = HitResult.ImpactPoint + ORIENT_TO_VELOCITY(CurrentData.CorrectionData.CollisionSnapOffset);
												AnimInstanceRef->Execute_UpdateEffectorTranslation(AnimInstanceRef, Key, Effector.CurrentEffectorLocation, CurrentData.TranslationData.LerpSpeed);
												Effector.bCorrectionIK = true;
												//Effector.BlockTime = CurrentData.EndSwing;
												//Effector.bForceSwing = true;
											}
										}
									}
								}
							
								AnimInstanceRef->Execute_UpdateEffectorTranslation(AnimInstanceRef, Key, Effector.CurrentEffectorLocation, CurrentData.TranslationData.LerpSpeed);
							}

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

			SwingValuesKeys.Reset(0);
			CurrentAsset.GroundReflectionPerEffector.GetKeys(SwingValuesKeys);
			// Step 3: Ground reflection
			for (int j = 0, m = SwingValuesKeys.Num(); j < m; ++j)
			{
				FName Key = SwingValuesKeys[j];
				FGaitGroundReflectionData& CurrentData = CurrentAsset.GroundReflectionPerEffector[Key];

				FRotator FinalRot;
				for (int k = 0, n = CurrentData.Planes.Num(); k < n; ++k)
				{
					if (Effectors.Contains(CurrentData.Planes[k].EffectorName1) 
						&& Effectors.Contains(CurrentData.Planes[k].EffectorName2)
						&& Effectors.Contains(CurrentData.Planes[k].EffectorName3))
					{
						FVector A = Effectors.Find(CurrentData.Planes[k].EffectorName1)->GroundLocation;
						FVector B = Effectors.Find(CurrentData.Planes[k].EffectorName2)->GroundLocation;
						FVector C = Effectors.Find(CurrentData.Planes[k].EffectorName3)->GroundLocation;
						//FPlane GroundReflection(A, B, C);

						FVector AB = B - A;
						FVector AC = C - A;
						FVector CrossABC = AB ^ AC;
						CrossABC.Normalize();

						/*float OutPitch = 0, OutRoll = 0;
						UKismetMathLibrary::GetSlopeDegreeAngles(
							OwnedMesh->GetRightVector(),
							CrossABC,
							FVector(0,0, 1.f),
							OutPitch,
							OutRoll
						);*/

						const FVector FloorZAxis = CrossABC;
						const FVector FloorXAxis = OwnedMesh->GetRightVector() ^ FloorZAxis;
						const FVector FloorYAxis = FloorZAxis ^ FloorXAxis;
						
						//DEBUG_LOG_FORMAT(Log, "\t=>FloorZAxis: %s\n\t=>FloorXAxis: %s\n\t=>FloorYAxis: %s\n", *FloorZAxis.ToString(), *FloorXAxis.ToString(), *FloorYAxis.ToString());
						
						float OutSlopePitchDegreeAngle = 90.f - FMath::RadiansToDegrees(FMath::Acos(FloorXAxis | FVector(0, 0, 1.f)));
						float OutSlopeRollDegreeAngle = 90.f - FMath::RadiansToDegrees(FMath::Acos(FloorYAxis | FVector(0, 0, 1.f)));

						FinalRot.Roll += OutSlopeRollDegreeAngle;
						FinalRot.Pitch = 0;
						FinalRot.Yaw += OutSlopePitchDegreeAngle;

						DrawDebugLine(World, A, B, FColor(255.f, 255.f, 255.f), false, 0.f, 0, 0.5f);
						DrawDebugLine(World, B, C, FColor(255.f, 255.f, 255.f), false, 0.f, 0, 0.5f);
						DrawDebugLine(World, C, A, FColor(255.f, 255.f, 255.f), false, 0.f, 0, 0.5f);
					}
				}
							
				FinalRot.Roll /= (m + 1);
				FinalRot.Yaw /= (m + 1);
				FinalRot = FinalRot.GetInverse();
				AnimInstanceRef->Execute_UpdateEffectorRotation(AnimInstanceRef, Key, FinalRot, 10.f);
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
		AnimInstanceRef->Execute_SetProceduralGaitEnable(AnimInstanceRef, false);
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
		CurrentGaitMode = NewGaitName;
		SetComponentTickEnabled(true);
	}
	else
	{
		SetComponentTickEnabled(false);
		//DEBUG_LOG_FORMAT(Warning, "Invalid NewGaitName %s. There is no gait data corresponding. Ignored.", NewGaitName);
	}
}

void UProceduralGaitControllerComponent::UpdateEffectors()
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

				FVector GroundLocation;
				TArray<FHitResult> HitResults;
				bool bFound = TraceRay(GetWorld(), HitResults, EffectorLocation, EffectorLocation + FVector(0, 0, -100.f), ECollisionChannel::ECC_WorldStatic, 30.f);
				if (!bFound)
				{
					GroundLocation = EffectorLocation + FVector(0, 0, -100.f);
				}
				else
				{
					FHitResult& HitResult = GetBestHitResult(HitResults, EffectorLocation);
					GroundLocation = HitResult.ImpactPoint;
				}

				Effectors[SwingValuesKeys[j]].GroundLocation = GroundLocation;
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

	if (UNobunanimSettings::GetLODSetting(CurrentLOD).Debug.bShowLOD 
#if WITH_EDITOR
		|| bShowLOD
#endif
		)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			DrawDebugSolidBox(World, GetOwner()->GetActorLocation(), GetOwner()->GetSimpleCollisionCylinderExtent(), UNobunanimSettings::GetLODSetting(CurrentLOD).Debug.LODColor, false, 0.f);
		}
	}
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

