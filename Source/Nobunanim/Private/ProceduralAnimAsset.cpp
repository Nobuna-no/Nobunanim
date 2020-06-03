// Fill out your copyright notice in the Description page of Project Settings.


#include "ProceduralAnimAsset.h"

#include <Engine.h>
#include <Math/NumericLimits.h>
#include <Components/SkeletalMeshComponent.h>


float UProceduralAnimAsset::GetFrameRatio() const
{
	return (float)AnimationFrameRate / (float)AnimationFrameCount;
}