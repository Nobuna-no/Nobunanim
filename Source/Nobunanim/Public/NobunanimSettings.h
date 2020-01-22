// Copyright 2017 Google Inc.

#pragma once

#include <Engine/DeveloperSettings.h>
#include "NobunanimSettings.generated.h"

USTRUCT(BlueprintType)
struct NOBUNANIM_API FProceduralGaitLODSettingsDebugData
{
	GENERATED_BODY()

	/** Should debug LOD be displayed. */
	UPROPERTY(Category = "[NOBUNANIM]|Settings|Procedural Gait|LOD|Debug", EditAnywhere, Config)
	bool bShowLOD = false;

	/** Color of LOD. Also use for sphere trace IK. */
	UPROPERTY(Category = "[NOBUNANIM]|Settings|Procedural Gait|LOD|Debug", EditAnywhere, Config)
	FColor LODColor = FColor::White;

	/** Should debug IKs (traces) be displayed. */
	UPROPERTY(Category = "[NOBUNANIM]|Settings|Procedural Gait|LOD|Debug", EditAnywhere, Config)
	bool bShowCollisionCorrection = true;

	/** IK trace color. */
	UPROPERTY(Category = "[NOBUNANIM]|Settings|Procedural Gait|LOD|Debug", EditAnywhere, Config)
	FColor IKTraceColor = FColor::Red;
};

USTRUCT(BlueprintType)
struct NOBUNANIM_API FProceduralGaitLODSettings
{
	GENERATED_BODY()

	/** Target frame per second (refresh rate) of the animation for this LOD. */
	UPROPERTY(Category = "[NOBUNANIM]|Settings|Procedural Gait|LOD", EditAnywhere, Config, meta = (ClampMn = "0"))
	int32 TargetFPS = 60;

	/** If required, may this LOD be allow to compute collision correction? 
	* Can be used to optimized high LOD (far away) by disabling the collision computation. */
	UPROPERTY(Category = "[NOBUNANIM]|Settings|Procedural Gait|LOD", EditAnywhere, Config)
	bool bCanComputeCollisionCorrection = true;

	/** Debug Data. */
	UPROPERTY(Category = "[NOBUNANIM]|Settings|Procedural Gait|LOD", EditAnywhere, Config)
	FProceduralGaitLODSettingsDebugData Debug;
};

UCLASS(Category = "[NOBUNANIM]|Settings", Config = Game, defaultConfig)
class NOBUNANIM_API UNobunanimSettings : public UDeveloperSettings
{
	GENERATED_BODY()

	protected:
		/**
		*	Number of frame for 1 second.
		*	Ratio to convert seconds in frames.
		*	All the procedural gait system depends on this ratio (refresh rate, animation speed...).
		*/
		UPROPERTY(Category = "[NOBUNANIM]|Settings|Procedural Gait", EditAnywhere, Config, meta = (ClampMn = "0"))
		int32 FramePerSecond = 60;

		/** Map of procedural gait settings. */
		UPROPERTY(Category = "[NOBUNANIM]|Settings|Procedural Gait|LOD", EditAnywhere, Config)
		TMap<int32, FProceduralGaitLODSettings> ProceduralGaitLODSettings;

	public:
		/** Static accessor of FramePerSecond. */
		UFUNCTION(Category = "[NOBUNANIM]|Settings|Procedural Gait|LOD", BlueprintPure)
		static int32 GetFramePerSecond();

		/** Gets the specified LOD setting. Return default one if invalid settings or @Lod. */
		UFUNCTION(Category = "STARK|Settings|Matter", BlueprintPure)
		static FProceduralGaitLODSettings GetLODSetting(int32 Lod);
};