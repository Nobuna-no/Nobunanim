// Copyright (c) Panda Studios Comm. V.  - All Rights Reserves. Under no circumstance should this could be distributed, used, copied or be published without written approved of Panda Studios Comm. V. 

#pragma once

#include "CoreMinimal.h"
#include "Components/Viewport.h"
#include "SViewport.h"
#include "Editor/AdvancedPreviewScene/Public/AdvancedPreviewScene.h"
#include "Editor/AdvancedPreviewScene/Public/AdvancedPreviewSceneModule.h"
#include "SLevelViewport.h"
#include "SEditorViewport.h"
#include "Editor/UnrealEd/Public/SCommonEditorViewportToolbarBase.h"
#include "SlateFwd.h"
#include "UObject/GCObject.h"


class SProceduralAnimationEditorViewport : public SEditorViewport, public FGCObject, public ICommonEditorViewportToolbarInfoProvider
{
public:

	//Constructor and destructor
	//SProceduralAnimationEditorViewport();

	SLATE_BEGIN_ARGS(SProceduralAnimationEditorViewport) {}
	SLATE_END_ARGS()

	/** The scene for this viewport. */
	TSharedPtr<FAdvancedPreviewScene> PreviewScene;


	//FGCObject 
	void AddReferencedObjects(FReferenceCollector& Collector) override;
	//Toolbar interface
	virtual TSharedRef<class SEditorViewport> GetViewportWidget() override;
	virtual TSharedPtr<FExtender> GetExtenders() const override;
	virtual void OnFloatingButtonClicked() override;

	//All components to use in the client
	UStaticMeshComponent* MeshComp;
	UStaticMeshComponent* MaterialComp;
	USkeletalMeshComponent* SkelMeshComp;
	UPostProcessComponent* PostComp;

	SProceduralAnimationEditorViewport();
	~SProceduralAnimationEditorViewport();
	/*
	* Construct this viewport widget
	*
	* @param	inArgs		Arguments
	*/
	void Construct(const FArguments& InArgs);

	/*
	* Construct this viewport widget
	*
	* @return	the created EditorViewportClient
	*/
 	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

	void BindCommands() override;
	EVisibility GetTransformToolbarVisibility() const;
	void OnFocusViewportToSelection() override;
	/*
	*	Get the viewport client
	*	
	*	@return the viewport client
	*/
	TSharedPtr<class FProceduralAnimationEditorViewportClient> GetViewportClient() { return TypedViewportClient;  };

	
	/*
	*	Set mesh of the mesh component
	*
	*	@param	inMesh		Mesh to change it with
	*	@param	bTakeShot	Take a shot in this change?
	*/
	void SetMesh(class UStaticMesh* inMesh, bool bTakeShot = false);

	//Shared ptr to the client
	TSharedPtr<class FProceduralAnimationEditorViewportClient> TypedViewportClient;

protected:
	FText GetTitleText() const;

private:
	
	
};
