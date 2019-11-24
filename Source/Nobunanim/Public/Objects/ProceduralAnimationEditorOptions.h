// Copyright (c) Panda Studios Comm. V.  - All Rights Reserves. Under no circumstance should this could be distributed, used, copied or be published without written approved of Panda Studios Comm. V. 

#pragma once

#include <Engine/DataAsset.h>
#include "UObject/NoExportTypes.h"
#include <Map.h>
#include "ProceduralAnimationEditorOptions.generated.h"



UCLASS(BlueprintType, Blueprintable)
class PROCEDURALANIMATIONEDITOR_API UProceduralAnimationEditorOptions : public UDataAsset
{
	GENERATED_BODY()

	public:
		//Screenshot X Size
		UPROPERTY(EditAnywhere, Category = "Animation")
		int32 ScreenshotXSize = 512;
		//Screenshot Y Size
		UPROPERTY(EditAnywhere, Category = "Animation")
		int32 ScreenshotYSize = 512;

		UPROPERTY(EditAnywhere, Category = "Animation")
		UProceduralAnimationEditorOptions* Yolo;
};
