// Copyright (c) Panda Studios Comm. V.  - All Rights Reserves. Under no circumstance should this could be distributed, used, copied or be published without written approved of Panda Studios Comm. V. 

#include "SProceduralAnimationEditorViewport.h"
#include "SViewport.h"
#include "Editor/LevelEditor/Private/SLevelViewportToolBar.h"
#include "AssetEditorModeManager.h"
#include "Editor/AdvancedPreviewScene/Public/AdvancedPreviewScene.h"
#include "Editor/AdvancedPreviewScene/Public/AdvancedPreviewSceneModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "ProceduralAnimationEditorViewportClient.h"


void SProceduralAnimationEditorViewport::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(MeshComp);
	Collector.AddReferencedObject(MaterialComp);
	Collector.AddReferencedObject(SkelMeshComp);
	Collector.AddReferencedObject(PostComp);
}

TSharedRef<class SEditorViewport> SProceduralAnimationEditorViewport::GetViewportWidget()
{
	return SharedThis(this);
}

TSharedPtr<FExtender> SProceduralAnimationEditorViewport::GetExtenders() const
{
	TSharedPtr<FExtender> Result(MakeShareable(new FExtender));
	return Result;
}

void SProceduralAnimationEditorViewport::OnFloatingButtonClicked()
{
	// Nothing
}

//Just create the advnaced preview scene and initiate components
SProceduralAnimationEditorViewport::SProceduralAnimationEditorViewport() : PreviewScene(MakeShareable(new FAdvancedPreviewScene(FPreviewScene::ConstructionValues())))
{

	MeshComp = NewObject<UStaticMeshComponent>();

	MaterialComp = NewObject<UStaticMeshComponent>();

	SkelMeshComp = NewObject<USkeletalMeshComponent>();

	PostComp = NewObject <UPostProcessComponent>();
}

SProceduralAnimationEditorViewport::~SProceduralAnimationEditorViewport() 
{
	if (TypedViewportClient.IsValid())
	{
		TypedViewportClient->Viewport = NULL;
	}
}

void SProceduralAnimationEditorViewport::Construct(const FArguments& InArgs)
{
	SEditorViewport::Construct(SEditorViewport::FArguments());
}


TSharedRef<FEditorViewportClient> SProceduralAnimationEditorViewport::MakeEditorViewportClient()
{
	TypedViewportClient = MakeShareable(new FProceduralAnimationEditorViewportClient(SharedThis(this), PreviewScene.ToSharedRef()));
	return TypedViewportClient.ToSharedRef(); 
}

void SProceduralAnimationEditorViewport::BindCommands()
{
	SEditorViewport::BindCommands();
}

EVisibility SProceduralAnimationEditorViewport::GetTransformToolbarVisibility() const
{
	return EVisibility::Visible;
}

void SProceduralAnimationEditorViewport::OnFocusViewportToSelection()
{

}


void SProceduralAnimationEditorViewport::SetMesh(class UStaticMesh* inMesh, bool bTakeShot)
{
	//Set the mesh inside the client's meshcomp
	TypedViewportClient->SetMesh(inMesh, bTakeShot);
}

FText SProceduralAnimationEditorViewport::GetTitleText() const
{
	//return the title text of the viewport
	return FText::FromString("ProceduralAnimationEditor Generator");
}
