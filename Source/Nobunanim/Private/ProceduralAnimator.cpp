// Fill out your copyright notice in the Description page of Project Settings.


#include "ProceduralAnimator.h"

#include <Engine.h>
#include <Math/NumericLimits.h>
#include <Components/SkeletalMeshComponent.h>

#include <Nobunanim/Public/NobunanimSettings.h>

//#include <Nobunanim/Public/LocomotionComponent.h>

#define FRAME_PER_SECOND 60.f

#define CHECK_INIT() if (!bIsInitialized) { DEBUG_LOG(Warning, "Trying to call a method from a UProceduralAnimator despite it isn't initialized"); return; }
#define ORIENT_TO_VEL(Input) adjData.VelocityFactor * (adjData.bUseSimulatedVelocity ? adjData.SimulatedVelocity : lastVelocity).Rotation().RotateVector(Input)

void UProceduralAnimator::Initialize(USkeletalMeshComponent* _Mesh/*, ULocomotionComponent* _Locomotion*/)
{
	if (_Mesh)
	{
		world = _Mesh->GetWorld();
		mesh = _Mesh;

		bIsInitialized = world->IsGameWorld();

		//locomotion = _Locomotion;
	}
}

void UProceduralAnimator::Add(UProceduralAnimAsset* _Anim, int _FrameDuration, bool bCached)
{
	CHECK_INIT()
	
	if (_Anim == nullptr)
	{
		DEBUG_LOG(Warning, "Trying to add invalid procedural anim asset.");
		return;
	}

	FName animName = _Anim->GetFName();

	if (!currentAnimSet.Contains(animName))
	{
		currentAnimSet.Add(animName, _Anim);
		timeBuffers.Add(animName, 0.f);
		refreshRateBuffers.Add(animName, 0.f);

		// Extract each effectors from the procedural anim.
		for (TMap<FName, FProceduralAnimData>::TConstIterator it{ _Anim->Effectors.CreateConstIterator() }; it; ++it)
		{
			if (!effectors.Contains(it->Key))
			{
				effectors.Add(it->Key);
			}
		}

		UpdateGlobalWeight();
	}
	if (_FrameDuration > 0)
	{
		FTimerDelegate timerDelegate = FTimerDelegate::CreateUObject(this, &UProceduralAnimator::RemoveFromName, animName);
		float rate { _FrameDuration / FRAME_PER_SECOND };

		if (!timerHandles.Contains(animName))
		{
			timerHandles.Add(animName);
		}

		world->GetTimerManager().SetTimer(timerHandles[animName], timerDelegate, rate, false, rate);
	}
	else
	{
		// if infinite duration, try to remove existing timer.
		if (timerHandles.Contains(animName))
		{
			world->GetTimerManager().ClearTimer(timerHandles[animName]);
		}
	}

}

void UProceduralAnimator::Remove(UProceduralAnimAsset* _Anim)
{
	RemoveFromName(_Anim->GetFName());
}

void UProceduralAnimator::RemoveFromName(FName _Key)
{
	CHECK_INIT()

	if (!currentAnimSet.Contains(_Key))
	{
		return;
	}

	if (timerHandles.Contains(_Key))
	{
		world->GetTimerManager().ClearTimer(timerHandles[_Key]);
		timerHandles.Remove(_Key);
	}

	currentAnimSet.Remove(_Key);
	timeBuffers.Remove(_Key);
	refreshRateBuffers.Remove(_Key);
	UpdateGlobalWeight();
}

void UProceduralAnimator::SetActive(bool _bEnable)
{
	CHECK_INIT()

	if (!world || !world->IsGameWorld())
	{
		return;
	}

	if (_bEnable /*&& bEvaluationActive*/)
	{
		bEvaluationActive = true;
		float delay = 1.f / (float)UNobunanimSettings::GetLODSetting(currentLOD = mesh->PredictedLODLevel).TargetFPS;

		// Set timer will auto-clear if needed.
		world->GetTimerManager().SetTimer(updateTimerHandle, this, &UProceduralAnimator::Evaluate_Internal, delay, true, false);
		
		Execute_SetProceduralGaitEnable(this, true);
	}
	else if (!_bEnable /*&& bEvaluationActive*/)
	{
		bEvaluationActive = false;
		world->GetTimerManager().ClearTimer(updateTimerHandle);

		Execute_SetProceduralGaitEnable(this, false);
	}
}


void UProceduralAnimator::Evaluate_Internal()
{
	CHECK_INIT()
		
	if (!world || !world->IsGameWorld())
	{
		return;
	}

	// Update effectors.
	UpdateEffectors();

	// Update deltatime.
	{
		deltaTime = world->TimeSince(lastTime);
		lastTime = world->TimeSeconds;
	}
	
	// Update last velocity.
	{
		FVector currentVelocity = mesh->GetOwner()->GetVelocity();
		if (bConserveVelocityIfZero && currentVelocity.SizeSquared() != 0.f)
		{
			lastVelocity = currentVelocity;
			lastVelocity.Normalize();
		}

		//if (locomotion != nullptr)
		//{
		//	direction = locomotion->GetDirection() / 180.f * PI;
		//}
	}

	// Evaluate anim set.
	EvaluateAnimSet();

	updateCount++;
}

void UProceduralAnimator::EvaluateAnimSet()
{
	CHECK_INIT()

	for (TMap<FName, UProceduralAnimAsset*>::TConstIterator it{ currentAnimSet.CreateConstIterator() }; it; ++it)
	{
		float weight { it->Value->Weight * globalWeight };

		// Ignore animation with 0 alpha.
		if (weight == 0 && currentAnimSet.Num() > 1)
		{
			continue;
		}

		float buffToAdd = deltaTime * it->Value->GetFrameRatio() *  it->Value->PlayRate;
		// Update time buffer.
		timeBuffers[it->Key] += buffToAdd;

		// Modulo to 1.
		float currentTime = FMath::Fmod(timeBuffers[it->Key], 1.f);
		

		for (TMap<FName, FProceduralAnimData>::TConstIterator ite{ it->Value->Effectors.CreateConstIterator() }; ite; ++ite)
		{
			if (effectors.Contains(ite->Key))
			{
				EvaluateEffector(it->Value, ite->Value, ite->Key, currentTime);
			}
		}

		for (TMap<FName, float>::TConstIterator ite{ it->Value->EventsToTrigger.CreateConstIterator() }; ite; ++ite)
		{
			if (!PAAEventRaised.Contains(it->Key))
			{
				PAAEventRaised.Add(it->Key, TArray<FName>());
				PAAEventRaised[it->Key].Add(ite->Key);
				OnProceduralAnimAssetEvent.Broadcast(ite->Key);
				continue;
			}
			else if( (currentTime >= ite->Value || (ite->Value == 1.f && currentTime + buffToAdd >= 1.f))
				&& !PAAEventRaised[it->Key].Contains(ite->Key))
			{
				PAAEventRaised[it->Key].Add(ite->Key);
				OnProceduralAnimAssetEvent.Broadcast(ite->Key);
			}
		}

		// @WIP: If current PAA end
		if (PAAEventRaised.Contains(it->Key) && currentTime + buffToAdd >= 1.f)
		{
			PAAEventRaised[it->Key].Empty();
			PAAEventRaised.Remove(it->Key);
		}
	}
}


void UProceduralAnimator::UpdateEffectors()
{
	for (TMap<FName, FEffectorAnimData>::TIterator it { effectors.CreateIterator() }; it; ++it)
	{
		// Get world space ideal effector location.
		FVector EffectorLocation = mesh->GetSocketLocation(it->Key);
		it->Value.IdLocation = EffectorLocation;
	}
}

void UProceduralAnimator::EvaluateEffector(const UProceduralAnimAsset* _AnimAsset, const FProceduralAnimData& _AnimData, const FName& _EffectorName, float _CurrentTime)
{
	CHECK_INIT()

	if (!_AnimData.bEvaluate)
	{
		return;
	}

	// No need to check.
	FEffectorAnimData& effector = effectors[_EffectorName];

	//float beginSwing, endSwing;
	FName parentName = _AnimData.TimeData.ParentEffector;
	if (!parentName.IsNone() && effectors.Contains(parentName))
	{
		effector.currentBeginSwing = _AnimData.TimeData.bBindToParentBeginSwing ? effectors[parentName].currentBeginSwing : _AnimData.TimeData.BeginSwing;
		if (_AnimData.TimeData.bBindToParentEndSwing)
		{
			effector.currentEndSwing = effectors[parentName].currentEndSwing;
		}
		else
		{
			effector.currentEndSwing = effector.currentBeginSwing + _AnimData.TimeData.EndSwing;
			effector.currentEndSwing = effector.currentEndSwing > 1.f ? effector.currentEndSwing - 1.f : effector.currentEndSwing;
		}
	}
	else if (!effector.bForceSwing)
	{
		effector.currentBeginSwing = _AnimData.TimeData.BeginSwing;
		effector.currentEndSwing = _AnimData.TimeData.EndSwing;
	}
	
	float minRange, maxRange;
	
	bool bInRange = IsInSwingRange(_CurrentTime, effector.currentBeginSwing, effector.currentEndSwing, minRange, maxRange);
	if (bInRange)
	{
		if (!effector.bStartSwing)
		{
			effector.CurrentDestination = effector.CurrentLocation;
			effector.bStartSwing = true;
		}
		else if (effector.bForceSwing)
		{
			effector.CurrentDestination = effector.CurrentLocation;
		}

		float currentCurvePosition = FMath::GetMappedRangeValueClamped(FVector2D(minRange, maxRange), FVector2D(0.f, 1.f), _CurrentTime);
		
		// Per axis acceleration.
		const FProceduralAnimData_SwingAcceleration& accelData = _AnimData.MovementData.AccelerationData;
		effector.CurrentLocation.X = FMath::Lerp(effector.CurrentDestination.X, effector.IdLocation.X,
			accelData.AccelerationCurveX.GetRichCurveConst() ? 
			accelData.AccelerationCurveX.GetRichCurveConst()->Eval(currentCurvePosition) : currentCurvePosition);
		effector.CurrentLocation.Y = FMath::Lerp(effector.CurrentDestination.Y, effector.IdLocation.Y,
			accelData.AccelerationCurveY.GetRichCurveConst() ?
			accelData.AccelerationCurveY.GetRichCurveConst()->Eval(currentCurvePosition) : currentCurvePosition);
		effector.CurrentLocation.Z = FMath::Lerp(effector.CurrentDestination.Z, effector.IdLocation.Z,
			accelData.AccelerationCurveZ.GetRichCurveConst() ?
			accelData.AccelerationCurveZ.GetRichCurveConst()->Eval(currentCurvePosition) : currentCurvePosition);
		
		// TO DO: Store the velocity one time per swing!
		// Store the current time per effector and each time 

		// Additive rotation data.
		if (_AnimData.MovementData.AdditiveRotationData.bEvaluate)
		{
			const FProceduralAnimData_AdditiveRotation& rotationData = _AnimData.MovementData.AdditiveRotationData;
			FVector currentCurveValue = rotationData.Scale * rotationData.Factor * (rotationData.AdditiveCurve ?
				rotationData.AdditiveCurve->GetVectorValue(currentCurvePosition) : FVector::ZeroVector);
			FRotator additive = _AnimData.MovementData.AdditiveRotationData.AdditiveOffset;

			FRotator curveVal = FRotator(currentCurveValue.Y, currentCurveValue.Z, currentCurveValue.X);

			if (rotationData.bOrientToDirection)
			{
				FVector currentVel = (_AnimData.AdjustmentData.bUseSimulatedVelocity ? _AnimData.AdjustmentData.SimulatedVelocity : lastVelocity) * _AnimData.AdjustmentData.VelocityFactor;
				FVector currentOrientation = FVector(FMath::Cos(direction), FMath::Sin(direction), 0) * currentVel.Size2D();
				
				if (curveVal != FRotator::ZeroRotator)
				{
					curveVal.Roll *= currentOrientation.X;
					curveVal.Pitch *= currentOrientation.Y;
				}
				if (additive != FRotator::ZeroRotator)
				{
					additive.Roll *= currentOrientation.X;
					additive.Pitch *= currentOrientation.Y;
				}
			}
			/*else if (rotationData.bScaleByVelocity)
			{
				FVector currentVel = (_AnimData.AdjustmentData.bUseSimulatedVelocity ? _AnimData.AdjustmentData.SimulatedVelocity : lastVelocity) * _AnimData.AdjustmentData.VelocityFactor;
				
				curveVal.Roll *= currentVel.X;
				curveVal.Pitch *= currentVel.Y;
				curveVal.Yaw *= currentVel.Z;

				additive.Roll *= currentVel.X;
				additive.Pitch *= currentVel.Y;
				additive.Yaw *= currentVel.Z;
			}*/

			FRotator finalRot = curveVal + additive;
			Execute_UpdateEffectorRotation(this, _EffectorName, finalRot, rotationData.LerpSpeed/* * (1 / _AnimAsset->GetFrameRatio())*/);
		}

		// Additive translation data.z
		if (_AnimData.MovementData.AdditiveTranslationData.bEvaluate)
		{
			const FProceduralAnimData_AdditiveTranslation& transData = _AnimData.MovementData.AdditiveTranslationData;

			FVector currentCurveValue = transData.Scale * transData.Factor * (transData.AdditiveCurve ?
				transData.AdditiveCurve->GetVectorValue(currentCurvePosition) : FVector(1, 1, 1));
			
			FVector offset = transData.AdditiveOffset * transData.Factor;

			if (transData.bOrientToVelocity)
			{
				const FProceduralAnimData_Adjustment adjData = _AnimData.AdjustmentData;
				currentCurveValue = ORIENT_TO_VEL(currentCurveValue);
				offset = ORIENT_TO_VEL(offset);
			}
			else
			{
				currentCurveValue = mesh->GetComponentRotation().RotateVector(currentCurveValue);
				offset = mesh->GetComponentRotation().RotateVector(offset);
			}

			FVector finalTranslation = offset + currentCurveValue + (transData.TransformSpace == ERelativeTransformSpace::RTS_World ? 
				effector.CurrentLocation
				: mesh->GetSocketTransform(_EffectorName, transData.TransformSpace.GetValue()).GetLocation());

			Execute_UpdateEffectorTranslation(this, _EffectorName, finalTranslation, transData.LerpSpeed > 0, transData.LerpSpeed/** (1 / _AnimAsset->GetFrameRatio())*/);
		}
		else
		{
			Execute_UpdateEffectorTranslation(this, _EffectorName, effector.CurrentLocation, _AnimData.MovementData.LerpSpeed > 0, _AnimData.MovementData.LerpSpeed/** (1 / _AnimAsset->GetFrameRatio())*/);
		}
	}
	else
	{
		effector.bStartSwing = false;
		effector.bForceSwing = false;

		if (_AnimData.AdjustmentData.bForceSwingIfTooFar && effectorsLocation.Contains(_EffectorName))
		{
			const FProceduralAnimData_Adjustment& adjustData = _AnimData.AdjustmentData;
			// Compute lenght.
			float distance = (adjustData.FactorPerAxis * (effectorsLocation[_EffectorName] - effector.IdLocation)).SizeSquared();

			if (distance >= FMath::Square(adjustData.DistanceTreshold))
			{
				// Test combine swing.
				effector.currentBeginSwing = _CurrentTime;
				effector.currentEndSwing = _AnimData.TimeData.EndSwing;

				// edge case: When just after the swing the effectors need to be adjusted.
				if (FMath::IsNearlyEqual(effector.currentBeginSwing, effector.currentEndSwing, swingTimeTreshold))
				{
					effector.currentEndSwing = _AnimData.TimeData.BeginSwing;
				}

				effector.bForceSwing = true;
			}
		}

	}

}

void UProceduralAnimator::UpdateGlobalWeight()
{
	globalWeight = 0.0f;
	for (TMap<FName, UProceduralAnimAsset*>::TConstIterator it{ currentAnimSet.CreateConstIterator() }; it; ++it)
	{
		globalWeight += it->Value->Weight;
	}

	globalWeight = globalWeight < TNumericLimits<float>().Min() ? 1.0f : 1.0f / globalWeight;
}

bool UProceduralAnimator::IsInSwingRange(float Value, float Min, float Max, float& OutRangeMin, float& OutRangeMax)
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


void UProceduralAnimator::UpdateEffectorTranslation_Implementation(const FName& _Socket, FVector _Translation, bool _bLerp, float _LerpSpeed)
{
	const FVector* vec = effectorsLocation.Find(_Socket);
	if (!vec)
	{
		effectorsLocation.Add(_Socket, _Translation);
	}
	else
	{
		effectorsLocation[_Socket] = _bLerp ? FMath::VInterpTo(*vec, _Translation, FMath::Clamp(deltaTime, 0.f, 1.f), _LerpSpeed) : _Translation;
	}
}

void UProceduralAnimator::UpdateEffectorRotation_Implementation(const FName& _Socket, FRotator _Rotation, float _LerpSpeed)
{
	const FRotator* rot = effectorsRotation.Find(_Socket);
	if (!rot)
	{
		effectorsRotation.Add(_Socket, _Rotation);
	}
	else
	{
		if (_LerpSpeed <= 0)
		{
			effectorsRotation[_Socket] = _Rotation;
		}

		effectorsRotation[_Socket] = FMath::RInterpTo(*rot, _Rotation, FMath::Clamp(deltaTime, 0.f, 1.f), _LerpSpeed);
	}
}

void UProceduralAnimator::SetProceduralGaitEnable_Implementation(bool _bEnable)
{
}
