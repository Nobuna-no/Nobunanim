// Fill out your copyright notice in the Description page of Project Settings.


#include "GaitDataAsset.h"

#include "Nobunanim/Public/NobunanimSettings.h"


/** Return the ratio according to the animation framecount. (1 sec = 60frames). */
float UGaitDataAsset::GetFrameRatio() const
{
	return (float)UNobunanimSettings::GetFramePerSecond() / (float)AnimationFrameCount;// /*/ (float)TargetFramePerSecond*/;
}