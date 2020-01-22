// Copyright 2017 Google Inc.

#include "NobunanimSettings.h"

/** Static accessor of FramePerSecond. */
int32 UNobunanimSettings::GetFramePerSecond()
{
	return GetDefault<UNobunanimSettings>()->FramePerSecond;
}

/** Gets the specified LOD setting. Return default one if invalid settings or @Lod. */
FProceduralGaitLODSettings UNobunanimSettings::GetLODSetting(int32 Lod)
{
	const UNobunanimSettings* Default = GetDefault<UNobunanimSettings>();

	return Default->ProceduralGaitLODSettings.Contains(Lod) ? Default->ProceduralGaitLODSettings[Lod] : FProceduralGaitLODSettings();
}