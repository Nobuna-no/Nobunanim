// Fill out your copyright notice in the Description page of Project Settings.


#include "ProceduralGaitAnimInstance.h"

#include "Nobunanim/Private/Nobunanim.h"
#include "Nobunanim/Public/GaitDataAsset.h"
#include "Nobunanim/Public/NobunanimSettings.h"

#include <Engine/Classes/Curves/CurveVector.h>
#include <Engine/Classes/Curves/CurveLinearColor.h>
#include <Engine/Classes/Kismet/KismetSystemLibrary.h>


#define ORIENT_TO_VELOCITY(Input) LastVelocity.Rotation().RotateVector(Input)

#define SPHERECAST_IK_CORRECTION_RADIUS 30.f
#define MAX_DELTATIME_CLAMP (1.f / 30.f)



void FProceduralGaitAnimInstanceTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	if (Target && !Target->IsPendingKillOrUnreachable())
	{
		if (TickType != LEVELTICK_ViewportsOnly)// || Target->ShouldTickIfViewportsOnly())
		{
			FScopeCycleCounterUObject ActorScope(Target);
			//Target->ProceduralGaitUpdate(DeltaTime);//, TickType, *this);
		}
	}
}

FString FProceduralGaitAnimInstanceTickFunction::DiagnosticMessage()
{
	return Target->GetFullName() + TEXT("[ProceduralGaitUpdate]");
}

FName FProceduralGaitAnimInstanceTickFunction::DiagnosticContext(bool bDetailed)
{
	if (bDetailed)
	{
		// Format is "ActorNativeClass/ActorClass"
		FString ContextString = FString::Printf(TEXT("%s/%s"), *GetParentNativeClass(Target->GetClass())->GetName(), *Target->GetClass()->GetName());
		return FName(*ContextString);
	}
	else
	{
		return GetParentNativeClass(Target->GetClass())->GetFName();
	}
}


FVector GetAverage(const TArray<FVector>& Vectors)
{
	FVector Sum(0.f);
	FVector Average(0.f);

	if (Vectors.Num() > 0)
	{
		for (int32 VecIdx = 0; VecIdx < Vectors.Num(); VecIdx++)
		{
			Sum += Vectors[VecIdx];
		}

		Average = Sum / ((float)Vectors.Num());
	}

	return Average;
}



void UProceduralGaitAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	NOBUNANIM_SCOPE_COUNTER(TerrainPrediction);

	//DeltaTime = DeltaSeconds;
	
	if (!OwnedMesh)
	{
		OwnedMesh = GetOwningComponent();
	}

	//CurrentLOD = OwnedMesh->PredictedLODLevel;
	
	FRotator Rotation(0,0,0);
	
	if (CurrentLOD == 0)
	{
		Rotation = ComputeGroundReflection_LOD0();
	}
	else //if (CurrentLOD == 1)
	{
		Rotation = ComputeGroundReflection_LOD1();
	}

	GroundReflectionRotation = FMath::Lerp(GroundReflectionRotation, Rotation.GetInverse(), DeltaSeconds * GroundReflectionLerpSpeed);

	UpdateLOD();

	
	//ProceduralGaitUpdate();
}

void UProceduralGaitAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	PrimaryAnimInstanceTick.TickGroup = TG_PrePhysics;
	// Default to no tick function, but if we set 'never ticks' to false (so there is a tick function) it is enabled by default
	PrimaryAnimInstanceTick.bCanEverTick = true;
	PrimaryAnimInstanceTick.bStartWithTickEnabled = true;

	UpdateLOD(true);
	/*ACharacter* Chara = Cast<ACharacter>(GetOwningActor());
	if (Chara)
	{
		PrimaryAnimInstanceTick.AddPrerequisite(Chara, Chara->PrimaryActorTick);
		PrimaryAnimInstanceTick.SetTickFunctionEnable(Chara->PrimaryActorTick.IsTickFunctionEnabled());
		PrimaryAnimInstanceTick.Target = this;
		PrimaryAnimInstanceTick.RegisterTickFunction(Chara->GetLevel());
	}*/
}


#pragma region PROCEDURAL GAIT INTERFACE

void UProceduralGaitAnimInstance::ProceduralGaitUpdate()
{
	if (!bGaitActive)
	{
		return;
	}

	NOBUNANIM_SCOPE_COUNTER(Gait_Update);

	FVector NewCurrentLocation;
	FVector IdealEffectorLocation;
	FVector CurrentEffectorLocation;
	float Treshold = 0.f;
	bool bForceSwing = false;
	bool bAllEffectorBlendOutEnd = true;

	UWorld* World = GetWorld();
	FVector CurrentVelocity = GetOwningActor()->GetVelocity();

	/*if (!bGaitActive)
	{
		bLastFrameWasDisable = true;
	}*/


	//DeltaTime = _DeltaTime;
	DeltaTime = World->TimeSince(LastTime);// FMath::Min(World->TimeSince(LastTime), MAX_DELTATIME_CLAMP);
	LastTime = World->TimeSeconds;

	// force 60 fps refresh rate
	const FProceduralGaitLODSettings& LODSetting = UNobunanimSettings::GetLODSetting(CurrentLOD);
	if (LODSetting.bForceDeltaTimeAtTargetFPS)
	{
		DeltaTime = 1.f / LODSetting.TargetFPS;
	}

	// Update Gaits Data.
	if (GaitsData.Contains(CurrentGaitMode))
	{
		const UGaitDataAsset& CurrentAsset = *GaitsData[CurrentGaitMode];

		//if (bGaitActive)
		//{
		UpdateEffectors(CurrentAsset);
		/*if (bLastFrameWasDisable)
		{
			bLastFrameWasDisable = false;
			return;
		}*/
		//}

		bool bZeroVelocity = CurrentVelocity.SizeSquared() == 0.f;
		bool bCond = CurrentAsset.bComputeWithVelocityOnly ? !bZeroVelocity : true;

		if (bCond)
		{
			if (!bZeroVelocity)
			{
				LastVelocity = CurrentVelocity;
			}

			//Execute_SetProceduralGaitEnable(this, true);

			// Step 1: Timers.
			TimeBuffer += (DeltaTime * CurrentAsset.GetFrameRatio() * PlayRate);
			CurrentTime = FMath::Fmod(TimeBuffer, 1.f);

			TArray<FName> SwingValuesKeys;
			CurrentAsset.GaitSwingValues.GetKeys(SwingValuesKeys);

			// Step 2: Foreach swing values, we will check if we are in 'Swing' or 'Stance' according to @CurrentTime.
			for (int j = 0, m = SwingValuesKeys.Num(); j < m; ++j)
			{
				NOBUNANIM_SCOPE_COUNTER(Gait_Evaluate_PerEffector);

				FName Key = SwingValuesKeys[j];
				const FGaitSwingData& CurrentData = CurrentAsset.GaitSwingValues[Key];

				if (Effectors.Contains(Key))
				{
					FGaitEffectorData& Effector = Effectors[Key];
					
					bool bBlendIn;

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

						float BeginSwing = (UpdatedCurrentAsset.GaitSwingValues.Contains(UpdatedCurrentData.SwingTime.ParentEffector) ? (UpdatedCurrentAsset.GaitSwingValues[Key].BeginSwing) : UpdatedCurrentData.BeginSwing);
						float EndSwing = (UpdatedCurrentAsset.GaitSwingValues.Contains(UpdatedCurrentData.SwingTime.ParentEffector) ? (UpdatedCurrentAsset.GaitSwingValues[Key].EndSwing) : UpdatedCurrentData.EndSwing);

						bool bCanCompute = Effector.BlockTime == -1.f
							|| (Effector.BlockTime > BeginSwing && CurrentTime >= Effector.BlockTime)
							|| (Effector.BlockTime < BeginSwing && CurrentTime < BeginSwing && CurrentTime >= Effector.BlockTime);

						if (bCanCompute)
						{
							float MinRange, MaxRange;
							BeginSwing = Effector.bForceSwing ? Effector.BeginForceSwingInterval : BeginSwing;
							EndSwing = Effector.bForceSwing ? Effector.EndForceSwingInterval : EndSwing;

							bool InRange = IsInRange(CurrentTime, BeginSwing, EndSwing, MinRange, MaxRange);

							// Step 2.2: If the effector is in 'Swing'.
							if (InRange)
							{
								NOBUNANIM_SCOPE_COUNTER(Gait_Swing);

								Effector.bCorrectionIK = false;

								float CurrentCurvePosition = FMath::GetMappedRangeValueClamped(FVector2D(MinRange, MaxRange), FVector2D(0.f, 1.f), CurrentTime);

								// :D hue hue :D
								float lerpSpeed = UpdatedCurrentData.TranslationData.LerpSpeed <= 0 ? 0 
									: UpdatedCurrentData.TranslationData.LerpSpeed * (Effector.CurrentBlendValue == 1.f ? 1.f 
										: (bBlendIn ? (UpdatedCurrentData.BlendData.BlendInAcceleration ? UpdatedCurrentData.BlendData.BlendInAcceleration->GetFloatValue(Effector.CurrentBlendValue) 
											: 1.f) 
											: (UpdatedCurrentData.BlendData.BlendOutAcceleration ? UpdatedCurrentData.BlendData.BlendOutAcceleration->GetFloatValue(Effector.CurrentBlendValue) 
												: 1.f)));

								// Step 2.2.1: Apply 'Swing' rotation (for bones).
								if (UpdatedCurrentData.RotationData.bAffectEffector)
								{
									FVector CurrentCurveValue = UpdatedCurrentData.RotationData.RotationFactor * (UpdatedCurrentData.RotationData.SwingRotationCurve ?
										UpdatedCurrentData.RotationData.SwingRotationCurve->GetVectorValue(CurrentCurvePosition) : FVector(1, 1, 1));

									Execute_UpdateEffectorRotation(this, Key, FRotator(CurrentCurveValue.X, CurrentCurveValue.Y, CurrentCurveValue.Z)/*OwnedMesh->GetComponentRotation().RotateVector(CurrentCurveValue).Rotation()*/, lerpSpeed);
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

									
									IdealEffectorLocation = UpdatedCurrentData.TranslationData.bAdaptToGroundLevel ? Effector.GroundLocation : Effector.IdealEffectorLocation;
									//IdealEffectorLocation = UpdatedCurrentData.TranslationData.bAdaptToGroundLevel ? Effector.IdealEffectorLocation + (Effector.IdealEffectorLocation  - Effector.GroundLocation) : Effector.IdealEffectorLocation;
									NewCurrentLocation = IdealEffectorLocation + Offset + CurrentCurveValue;
									Treshold = UpdatedCurrentData.CorrectionData.DistanceTresholdToAdjust;
									bForceSwing = Effector.bForceSwing;

									/*if (UpdatedCurrentData.TranslationData.bAdaptToGroundLevel)
									{
										NewCurrentLocation = Effector.IdealEffectorLocation.Z - Effector.GroundLocation.Z;
										
									}*/
									

									Execute_UpdateEffectorTranslation(this, Key, NewCurrentLocation, lerpSpeed > 0/*!bLastFrameWasDisable*/, lerpSpeed);

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
									if (/*!bLastFrameWasDisable &&*/ World && !Effector.bCorrectionIK && UpdatedCurrentData.CorrectionData.bComputeCollision)
									{
										//const FProceduralGaitLODSettings& LODSetting = UNobunanimSettings::GetLODSetting(CurrentLOD);
										if (LODSetting.bCanComputeCollisionCorrection)
										{
											NOBUNANIM_SCOPE_COUNTER(Gait_Stance_IKCorrection);

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

											FCollisionQueryParams SweepParam(*GetOwningActor()->GetName(), false, GetOwningActor());
											TArray<FHitResult> HitResults;
											bool bFoundHit = TraceRay(World, HitResults, Origin, Dest, UpdatedCurrentData.CorrectionData.TraceChannel, SPHERECAST_IK_CORRECTION_RADIUS);


											if (bFoundHit)
											{

												FHitResult& HitResult = GetBestHitResult(HitResults, Origin);

												{
#if WITH_EDITOR
													if (HitResult.bBlockingHit && LODSetting.Debug.bShowCollisionCorrection)
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

									Execute_UpdateEffectorTranslation(this, Key, Effector.CurrentEffectorLocation, UpdatedCurrentData.TranslationData.LerpSpeed > 0/*!bLastFrameWasDisable*/, UpdatedCurrentData.TranslationData.LerpSpeed);
								}

								bForceSwing = Effector.bForceSwing = false;
								//Effector.BeginForceSwingInterval = CurrentAsset.GaitSwingValues[Key].BeginSwing;
								//Effector.EndForceSwingInterval = CurrentAsset.GaitSwingValues[Key].EndSwing;

								CurrentEffectorLocation = Effector.CurrentEffectorLocation;
								IdealEffectorLocation = Effector.IdealEffectorLocation;
								NewCurrentLocation = CurrentEffectorLocation;// Effector.IdealEffectorLocation + TranslationData.Offset + CurrentCurveValue;
								Treshold = UpdatedCurrentData.CorrectionData.DistanceTresholdToAdjust;

								bool bUseParentData = UpdatedCurrentAsset.GaitSwingValues.Contains(UpdatedCurrentData.SwingTime.ParentEffector) && Effectors.Contains(UpdatedCurrentData.SwingTime.ParentEffector);
								const FGaitCorrectionData& CurrentCorrectionData = UpdatedCurrentAsset.GaitSwingValues.Contains(UpdatedCurrentData.SwingTime.ParentEffector) ? UpdatedCurrentAsset.GaitSwingValues[Key].CorrectionData : UpdatedCurrentData.CorrectionData;
								// Step 2.3.1: Check if the effector need to be adjusted
								if (CurrentCorrectionData.bAutoAdjustWithIdealEffector)
								{
									float VectorLength = (Effector.CurrentEffectorLocation - Effector.IdealEffectorLocation).Size2D();

									// Is the distance breaking treshold?
									if ((bUseParentData && Effectors[UpdatedCurrentData.SwingTime.ParentEffector].bForceSwing) || VectorLength >= UpdatedCurrentData.CorrectionData.DistanceTresholdToAdjust)
									{
										// Here we compute the new interval of swing.
										float Begin =/* CurrentAsset.GaitSwingValues[Key].*/BeginSwing;
										float End = /*CurrentAsset.GaitSwingValues[Key].*/EndSwing;

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
									UpdatedCurrentData.CorrectionData.bAutoAdjustWithIdealEffector, bForceSwing,
									&UpdatedCurrentData.DebugData);
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
			//Execute_SetProceduralGaitEnable(this, false);
		}

	}
	else
	{
		CurrentTime = 0;
		//Execute_SetProceduralGaitEnable(this, false);
	}


#if WITH_EDITOR
	UpdateLOD();
#else
	UpdateLOD();
#endif
}

void UProceduralGaitAnimInstance::UpdateEffectorTranslation_Implementation(const FName& TargetBone, FVector Translation, bool bLerp, float LerpSpeed)
{
	const FVector* Vec = EffectorsTranslation.Find(TargetBone);
	if (!Vec)
	{
		EffectorsTranslation.Add(TargetBone, Translation);
	}
	else
	{
		EffectorsTranslation[TargetBone] = bLerp ? FMath::Lerp(*Vec, Translation, LerpSpeed * DeltaTime) : Translation;
	}
}

void UProceduralGaitAnimInstance::UpdateEffectorRotation_Implementation(const FName& TargetBone, FRotator Rotation, float LerpSpeed)
{
	const FRotator* Vec = BonesRotation.Find(TargetBone);
	if (!Vec)
	{
		BonesRotation.Add(TargetBone, Rotation);
	}
	else
	{
		BonesRotation[TargetBone] = FMath::Lerp(*Vec, Rotation, LerpSpeed * DeltaTime);
	}
}

void UProceduralGaitAnimInstance::SetProceduralGaitEnable_Implementation(bool bEnable)
{
	bGaitActive = bEnable ? 1.f : 0.f;
	
	SetProceduralGaitUpdateEnable(bGaitActive);
	//if (!bGaitActive)
	//{
	//	// WARNING, clear timer will prevent the procedural gait update!!!
	//	// GetWorld()->GetTimerManager().ClearTimer(GaitUpdateTimer);
	//	// GaitActive
	//}
}

#pragma endregion


#pragma region TERRAIN PREDICTION UTILITIES

FRotator UProceduralGaitAnimInstance::GetPlaneRotation(FVector A, FVector B, FVector C, FVector RightVector, FRotator& OutRotation, bool bComputeHalf, bool bDebug)
{
	FRotator Rotation;

	FVector AP = bComputeHalf ? (A + B) * 0.5f : A;
	FVector BP = bComputeHalf ? (B + C) * 0.5f : B;
	FVector CP = bComputeHalf ? (C + A) * 0.5f : C;

	FVector AB = BP - AP;
	FVector AC = CP - AP;
	FVector CrossABC = AB ^ AC;
	CrossABC.Normalize();

	const FVector FloorZAxis = CrossABC;
	const FVector FloorXAxis = RightVector ^ FloorZAxis;
	const FVector FloorYAxis = FloorZAxis ^ FloorXAxis;

	float OutSlopePitchDegreeAngle = 90.f - FMath::RadiansToDegrees(FMath::Acos(FloorXAxis | FVector(0, 0, 1.f)));
	float OutSlopeRollDegreeAngle = 90.f - FMath::RadiansToDegrees(FMath::Acos(FloorYAxis | FVector(0, 0, 1.f)));

	OutRotation.Pitch = 0;
	OutRotation.Roll += OutSlopeRollDegreeAngle * 0.25f;
	OutRotation.Yaw += OutSlopePitchDegreeAngle * 0.25f;

#if WITH_EDITOR
	if (bDebug)
	{
		//DEBUG_LOG_FORMAT(Log, "\t=> Rotation: %s", *OutRotation.ToString());
		DrawDebugLine(GetWorld(), AP, BP, FColor(255.f, 255.f, 255.f), false, 0.f, 0, 0.5f);
		DrawDebugLine(GetWorld(), BP, CP, FColor(255.f, 255.f, 255.f), false, 0.f, 0, 0.5f);
		DrawDebugLine(GetWorld(), CP, AP, FColor(255.f, 255.f, 255.f), false, 0.f, 0, 0.5f);
	}
#endif

	return Rotation;
}

FVector UProceduralGaitAnimInstance::TraceGroundRaycast(UWorld* World, FVector Origin, FVector Dest)
{
	const FProceduralGaitLODSettings& LODSetting = UNobunanimSettings::GetLODSetting(CurrentLOD);

	FCollisionObjectQueryParams ObjectQuery(ECollisionChannel::ECC_WorldStatic);
	FCollisionQueryParams SweepParam(*this->GetName(), LODSetting.bTraceOnComplex);
	FHitResult Hit;
	if (!World->LineTraceSingleByObjectType
	(
		Hit,
		Origin,
		Dest,
		ObjectQuery,
		SweepParam
	))
	{
		Hit.ImpactPoint = Dest;
	}

	if (GroundReflection.bShowDebugPlanes)
	{
		DrawDebugLine(World, Origin, Dest, FColor::Cyan, false, 0.f, 0, 0.5f);
		DrawDebugPoint(World, Hit.ImpactPoint, 10, FColor::Cyan, false, 0.f);
	}
	return Hit.ImpactPoint;
}

FRotator UProceduralGaitAnimInstance::ComputeGroundReflection_LOD0()
{
	// Get Socket location
	FVector F = OwnedMesh->GetSocketLocation(GroundReflection.FrontSocket);
	FVector B = OwnedMesh->GetSocketLocation(GroundReflection.BackSocket);
	FVector R = OwnedMesh->GetSocketLocation(GroundReflection.RightSocket);
	FVector L = OwnedMesh->GetSocketLocation(GroundReflection.LeftSocket);

	// Compute centroid
	TArray<FVector> Average;
	Average.Add(F);
	Average.Add(B);
	Average.Add(R);
	Average.Add(L);
	FVector C = GetAverage(Average);

	// Trace for ground
	F = TraceGroundRaycast(GetWorld(), F, F + RayVector);
	B = TraceGroundRaycast(GetWorld(), B, B + RayVector);
	R = TraceGroundRaycast(GetWorld(), R, R + RayVector);
	L = TraceGroundRaycast(GetWorld(), L, L + RayVector);
	C = TraceGroundRaycast(GetWorld(), C, C + RayVector);

	// Compute ground reflection
	FRotator Rotation(0, 0, 0);
	FVector RightVec = OwnedMesh->GetRightVector();
	GetPlaneRotation(F, R, C, RightVec, Rotation, GroundReflection.bUseHalfVector, GroundReflection.bShowDebugPlanes);
	GetPlaneRotation(C, R, B, RightVec, Rotation, GroundReflection.bUseHalfVector, GroundReflection.bShowDebugPlanes);
	GetPlaneRotation(C, B, L, RightVec, Rotation, GroundReflection.bUseHalfVector, GroundReflection.bShowDebugPlanes);
	GetPlaneRotation(F, C, L, RightVec, Rotation, GroundReflection.bUseHalfVector, GroundReflection.bShowDebugPlanes);

	return Rotation;
}

FRotator UProceduralGaitAnimInstance::ComputeGroundReflection_LOD1()
{
	FVector Front = OwnedMesh->GetSocketLocation(GroundReflection.FrontSocket);
	Front = TraceGroundRaycast(GetWorld(), Front, Front + RayVector);
	FVector Back = OwnedMesh->GetSocketLocation(GroundReflection.BackSocket);
	Back = TraceGroundRaycast(GetWorld(), Back, Back + RayVector);
	FVector Right = OwnedMesh->GetSocketLocation(GroundReflection.RightSocket);
	Right = TraceGroundRaycast(GetWorld(), Right, Right + RayVector);
	FVector Left = OwnedMesh->GetSocketLocation(GroundReflection.LeftSocket);
	Left = TraceGroundRaycast(GetWorld(), Left, Left + RayVector);

	FVector RightVec = OwnedMesh->GetRightVector();

	FRotator Rotation(0, 0, 0);
	GetPlaneRotation(Front, Right, Back, RightVec, Rotation, GroundReflection.bUseHalfVector, GroundReflection.bShowDebugPlanes);
	GetPlaneRotation(Front, Back, Left, RightVec, Rotation, GroundReflection.bUseHalfVector, GroundReflection.bShowDebugPlanes);
	GetPlaneRotation(Front, Right, Left, RightVec, Rotation, GroundReflection.bUseHalfVector, GroundReflection.bShowDebugPlanes);
	GetPlaneRotation(Right, Back, Left, RightVec, Rotation, GroundReflection.bUseHalfVector, GroundReflection.bShowDebugPlanes);

	return Rotation;
}

#pragma endregion

 
#pragma region PROCEDURAL GAIT UTILITIES
bool UProceduralGaitAnimInstance::TraceRay(UWorld* World, TArray<FHitResult>& HitResults, FVector Origin, FVector Dest, TEnumAsByte<ECollisionChannel> TraceChannel, float SphereCastRadius)
{
	const FProceduralGaitLODSettings& LODSetting = UNobunanimSettings::GetLODSetting(CurrentLOD);

	// if correction Level0 then zero computation.
	if (LODSetting.CorrectionLevel == ENobunanimIKCorrectionLevel::IKL_Level0)
	{
		return false;
	}

	FCollisionQueryParams SweepParam(*GetOwningActor()->GetName(), LODSetting.bTraceOnComplex, GetOwningActor());

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
		DrawDebugPoint(World, Dest, 10, LODSetting.Debug.LODColor, false, LODSetting.Debug.IKTraceDuration);
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


FHitResult& UProceduralGaitAnimInstance::GetBestHitResult(TArray<FHitResult>& HitResults, FVector IdealLocation)
{
	FHitResult& HitResult = HitResults[0];

	float BestDistance = 9999999.f;
	for (int i = 0, n = HitResults.Num(); i < n; ++i)
	{
		FHitResult&  CurrentHit = HitResults[i];

		float Temp = FMath::Abs(((IdealLocation) - CurrentHit.ImpactPoint).SizeSquared());
		if (Temp < BestDistance && HitResult.bBlockingHit)
		{
			BestDistance = Temp;
			HitResult = CurrentHit;
		}
	}

	return HitResult;
}


void UProceduralGaitAnimInstance::UpdateGaitMode_Implementation(const FName& NewGaitName)
{
	if (GaitsData.Contains(NewGaitName))
	{
		if (CurrentGaitMode != NewGaitName && PendingGaitMode != NewGaitName)
		{
			if (CurrentGaitMode.IsNone() || CurrentGaitMode == "None")
			{
				CurrentGaitMode = NewGaitName;

			}
			else
			{
				PendingGaitMode = NewGaitName;
			}

			Execute_SetProceduralGaitEnable(this, true);
			SetProceduralGaitUpdateEnable(true);
			//CurrentGaitMode = NewGaitName;
			//SetComponentTickEnabled(true);
		}
	}
	else
	{
		SetProceduralGaitUpdateEnable(false);
		Execute_SetProceduralGaitEnable(this, false);
		//SetComponentTickEnabled(false);
		//DEBUG_LOG_FORMAT(Warning, "Invalid NewGaitName %s. There is no gait data corresponding. Ignored.", NewGaitName);
	}
}


void UProceduralGaitAnimInstance::UpdateEffectors(const UGaitDataAsset& CurrentAsset)
{
	NOBUNANIM_SCOPE_COUNTER(Gait_UpdateEffectors);

	TArray<FName> SwingValuesKeys;
	const TMap<FName, FGaitSwingData>& SwingValues = CurrentAsset.GaitSwingValues;//GaitsData[Keys[i]]->GaitSwingValues;
	SwingValues.GetKeys(SwingValuesKeys);

	for (int j = 0, m = SwingValuesKeys.Num(); j < m; ++j)
	{
		NOBUNANIM_SCOPE_COUNTER(Gait_UpdateEffector_One);

		FName Key = SwingValuesKeys[j];
		FVector EffectorLocation = OwnedMesh->GetSocketTransform(Key, CurrentAsset.GaitSwingValues[Key].TranslationData.TransformSpace.GetValue()).GetLocation();

		if (Effectors.Contains(Key))
		{
			Effectors[Key].IdealEffectorLocation = EffectorLocation;

			if (CurrentAsset.GaitSwingValues[Key].TranslationData.bAdaptToGroundLevel)
			{
				FVector Dir = FVector::UpVector;
				FVector GroundLocation;
				TArray<FHitResult> HitResults;
				FVector GroundReferenceLocation = OwnedMesh->GetSocketLocation(CurrentAsset.GaitSwingValues[Key].TranslationData.GroundReferenceSocket);
				bool bFound = TraceRay(GetWorld(), HitResults, EffectorLocation, FVector(EffectorLocation.X, EffectorLocation.Y, GroundReferenceLocation.Z), ECollisionChannel::ECC_Visibility, SPHERECAST_IK_CORRECTION_RADIUS);
				if (!bFound)
				{
					//GroundLocation = EffectorLocation + RayVector; 
					Effectors[Key].GroundLocation = EffectorLocation;
				}
				else
				{
					FHitResult& HitResult = GetBestHitResult(HitResults, EffectorLocation);
					if (HitResult.bBlockingHit)
					{
						GroundLocation = HitResult.ImpactPoint;
						Dir = HitResult.Normal;

						if (GroundLocation.Z > GroundReferenceLocation.Z)
						{
							FVector Correction = Dir * (GroundLocation.Z - GroundReferenceLocation.Z);
							Effectors[Key].GroundLocation = EffectorLocation + Correction;
						}
						else
						{
							Effectors[Key].GroundLocation = EffectorLocation;
						}
					}
					else
					{
						Effectors[Key].GroundLocation = EffectorLocation;
						//GroundLocation = EffectorLocation + RayVector;
					}
				}

				//Effectors[Key].GroundLocation = GroundLocation;
				//Effectors[Key].GroundLocation = EffectorLocation;// +(Dir * (Effectors[Key].IdealEffectorLocation.Z - GroundLocation.Z));
				//Effectors[Key].GroundLocation.X = Effectors[Key].GroundLocation.Y = 0.f;
			}

			/*if (bLastFrameWasDisable)
			{
				Effectors[Key].CurrentEffectorLocation = EffectorLocation;
				Execute_UpdateEffectorTranslation(this, Key, EffectorLocation, false, 0);
			}*/
		}
		else
		{
			FGaitEffectorData Effector;
			Effector.IdealEffectorLocation = EffectorLocation;
			Effectors.Add(Key, Effector);
		}
	}
}


bool UProceduralGaitAnimInstance::IsInRange(float Value, float Min, float Max, float& OutRangeMin, float& OutRangeMax)
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

		return C;
	}
}

void UProceduralGaitAnimInstance::DrawGaitDebug(FVector Position, FVector EffectorLocation, FVector CurrentLocation, float Treshold, bool bAutoAdjustWithIdealEffector, bool bForceSwing, const FGaitDebugData* DebugData)
{
	NOBUNANIM_SCOPE_COUNTER(ProceduralGaitAnimInstance__DrawGaitDebug);

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
				DrawDebugCircle(World, FVector(CurrentLocation.X, CurrentLocation.Y, EffectorLocation.Z), Treshold, 20, bForceSwing ? DebugData->ForceSwingColor : DebugData->CorrectionCircleColor, false, 0.f, 0, bForceSwing ? 1.f : 0.75f, FVector(0, 1, 0), FVector(1, 0, 0), false);
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
			DrawDebugSolidBox(World, GetOwningActor()->GetActorLocation(), GetOwningActor()->GetSimpleCollisionCylinderExtent(), UNobunanimSettings::GetLODSetting(CurrentLOD).Debug.LODColor, false, 0.f);
		}
	}
#endif
}

void UProceduralGaitAnimInstance::UpdateLOD(bool bForceUpdate)
{
	if (CurrentLOD != OwnedMesh->PredictedLODLevel
		|| bForceUpdate)
	{
		CurrentLOD = OwnedMesh->PredictedLODLevel;

		// If procedural gait update is running, hard reset to adapt framerate.
		if (bUpdateGaitActive)
		{
			float Delay = 1.f / (float)UNobunanimSettings::GetLODSetting(CurrentLOD = OwnedMesh->PredictedLODLevel).TargetFPS;
			// Set timer will auto-clear if needed.
			GetWorld()->GetTimerManager().SetTimer(GaitUpdateTimer, [this]() { ProceduralGaitUpdate(); }, Delay, true, false);
		}
	}
}

void UProceduralGaitAnimInstance::SetProceduralGaitUpdateEnable(bool bEnable)
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	
	if (bEnable && !bUpdateGaitActive)
	{
		bUpdateGaitActive = true;
		float Delay = 1.f / (float)UNobunanimSettings::GetLODSetting(CurrentLOD = OwnedMesh->PredictedLODLevel).TargetFPS;
		// Set timer will auto-clear if needed.
		World->GetTimerManager().SetTimer(GaitUpdateTimer, [this]() { ProceduralGaitUpdate(); }, Delay, true, false);
	}
	else if(!bEnable && bUpdateGaitActive)
	{
		bUpdateGaitActive = false;

		World->GetTimerManager().ClearTimer(GaitUpdateTimer);
	}
}

#pragma endregion