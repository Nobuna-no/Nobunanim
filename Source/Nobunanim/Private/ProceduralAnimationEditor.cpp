// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ProceduralAnimationEditor.h"

//Engine
#include "LevelEditor.h"
#include "PreviewScene.h"
#include "Editor.h"
#include "Runtime/Engine/Classes/Engine/Selection.h"
#include "Engine/SkeletalMesh.h"
#include "Runtime/Core/Public/Containers/Ticker.h"
#include "Editor/UnrealEd/Public/ObjectTools.h"

//ProceduralAnimationEditor Core
#include "ProceduralAnimationEditorViewportClient.h"
#include "ProceduralAnimationEditorOptions.h"
#include "Runtime/Engine/Classes/Animation/AnimationAsset.h"
#include "ProceduralAnimationEditorCommands.h"
#include "ProceduralAnimationEditorStyle.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

//Image
#include "Runtime/Engine/Public/ImageUtils.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/Engine/Classes/EditorFramework/AssetImportData.h"
#include "Editor/UnrealEd/Classes/Factories/TextureFactory.h"
#include "Engine/Texture2D.h"
#include "Runtime/Core/Public/HAL/FileManager.h"
#include "Runtime/ImageWrapper/Public/IImageWrapper.h"
#include "Runtime/ImageWrapper/Public/IImageWrapperModule.h"
#include "Runtime/AssetRegistry/Public/AssetRegistryModule.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Core/Public/HAL/FileManagerGeneric.h"

//Slate
#include "Runtime/Engine/Public/Slate/SlateTextures.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "SSplitter.h"
#include "SProceduralAnimationEditorViewport.h"
#include "SEditorViewport.h"
#include "PropertyEditorModule.h"
#include "Editor/AdvancedPreviewScene/Public/SAdvancedPreviewDetailsTab.h"
#include "SImage.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"

static const FName ProceduralAnimationEditorTabName("ProceduralAnimationEditor");

#define LOCTEXT_NAMESPACE "FProceduralAnimationEditorModule"

void FProceduralAnimationEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FProceduralAnimationEditorStyle::Initialize();
	FProceduralAnimationEditorStyle::ReloadTextures();

	FProceduralAnimationEditorCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	// Create pressed button callback.
	PluginCommands->MapAction(
		FProceduralAnimationEditorCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FProceduralAnimationEditorModule::PluginButtonClicked),
		FCanExecuteAction());
		
	// Load level editor for viewport.
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FProceduralAnimationEditorModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
	
	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FProceduralAnimationEditorModule::AddToolbarExtension));
		
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ProceduralAnimationEditorTabName, FOnSpawnTab::CreateRaw(this, &FProceduralAnimationEditorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FProceduralAnimationEditorTabTitle", "ProceduralAnimationEditor"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);



	/** Maybe use this to update delta time of animation. */
	//Setup a timer every 0.03 seconds to crete a new image or process taken image
	//ImageTickDelegate = FTickerDelegate::CreateRaw(this, &FProceduralAnimationEditorModule::NextInQueue);
	//FTicker::GetCoreTicker().AddTicker(ImageTickDelegate, 0.03f);

	/* *OLD */

	//Get all startup images to exclude from processing
	//IFileManager& FileManager = IFileManager::Get();

	//TArray<FString> StartUpShort;
	//FileManager.FindFiles(StartUpShort, *FPaths::ScreenShotDir());
	//for (FString Short : StartUpShort)
	//{
	//	//Add path because Short is just the name, we still need to add the path to the string
	//	StartupImages.Add(FPaths::ScreenShotDir() + Short);
	//}
	/** OLD */

	//THIS IS NEEDED, if you don't do this you will crash the engine upon shutdown
	FCoreDelegates::OnPreExit.AddLambda([this]()
	{
		if (!ViewportPtr.IsValid())
			return;

		ViewportPtr->PreviewScene.Reset(); 
		ViewportPtr->TypedViewportClient.Reset(); 
		ViewportPtr.Reset();
	});

}

void FProceduralAnimationEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.


	FProceduralAnimationEditorStyle::Shutdown();

	FProceduralAnimationEditorCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProceduralAnimationEditorTabName);
}

void FProceduralAnimationEditorModule::PreUnloadCallback()
{
	
	IModuleInterface::PreUnloadCallback();
}

TSharedRef<SDockTab> FProceduralAnimationEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{

	//Create the new SProceduralAnimationEditorViewport for this tab
	ViewportPtr = SNew(SProceduralAnimationEditorViewport);

	//Setup thumbnail options property details view settings
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs Args;
	Args.bAllowSearch = false;
	Args.bCustomNameAreaLocation = true;
	Args.bShowActorLabel = false;
	Args.bHideSelectionTip = false;
	Args.bShowScrollBar = false;

	DetailsView = PropertyModule.CreateDetailView(Args);

	//Create advanced scene preview
	FAdvancedPreviewSceneModule& AdvancedPreviewSceneModule = FModuleManager::LoadModuleChecked<FAdvancedPreviewSceneModule>("AdvancedPreviewScene");
	TSharedRef<SWidget> PreviewDetails = AdvancedPreviewSceneModule.CreateAdvancedPreviewSceneSettingsWidget(ViewportPtr->PreviewScene.ToSharedRef());

	//Create thumbnmail options object
	if (!ProceduralAnimationEditorOptions)
	{
		ProceduralAnimationEditorOptions = NewObject<UProceduralAnimationEditorOptions>(GetTransientPackage(), UProceduralAnimationEditorOptions::StaticClass());
		ProceduralAnimationEditorOptions->AddToRoot();
	}

	auto var = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!

			SNew(SSplitter)
			.Orientation(Orient_Horizontal)
			+SSplitter::Slot().Value(0.25f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(5, 0, 5, 10)
				.HAlign(HAlign_Fill).VAlign(VAlign_Top).AutoHeight()
				[
					SNew(SButton)
					.OnClicked_Raw(this, &FProceduralAnimationEditorModule::GenerateFromSelection)
					[
						SNew(STextBlock).Text(FText::FromString("Generate Content Selection"))
					]
				]
				+ SVerticalBox::Slot()
				.Padding(5, 0, 5, 10)
				.HAlign(HAlign_Fill).VAlign(VAlign_Top).AutoHeight()
				[
					SNew(SButton)
					.OnClicked_Raw(this, &FProceduralAnimationEditorModule::GenerateView)
					[
						SNew(STextBlock).Text(FText::FromString("Generate Current View"))
					]
				]
				+ SVerticalBox::Slot()
				.Padding(5, 0, 5, 10)
				.HAlign(HAlign_Fill).VAlign(VAlign_Top).AutoHeight()
				[
					SNew(SButton)
					.OnClicked_Raw(this, &FProceduralAnimationEditorModule::PreviewSelected)
					[
						SNew(STextBlock).Text(FText::FromString("Preview Content Selected Mesh"))
					]
				]
				+ SVerticalBox::Slot()
				.Padding(5, 0, 5, 10)
				.HAlign(HAlign_Fill).VAlign(VAlign_Fill).AutoHeight()
				[
					DetailsView->AsShared()
				]
				+ SVerticalBox::Slot()
				.Padding(5, 0, 5, 10)
				.HAlign(HAlign_Fill).VAlign(VAlign_Fill).FillHeight(1)
				[
					SNew(SBox)
					.WidthOverride(250.f)
					[
						PreviewDetails->AsShared()
					]
				]
			]
		+ SSplitter::Slot()
			[
				SNew(SBox)
				.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillWidth(1)
			.Padding(5, 10, 5, 0)
			[
				SNew(SBox)
				[
					ViewportPtr.ToSharedRef()
				]
			]
			]
			]
		];

	//Set th objects for the details view
	DetailsView->SetObject(ProceduralAnimationEditorOptions, true);

	//Set the thumbnail options object in the viewport client
	ViewportPtr.Get()->GetViewportClient()->ProceduralAnimationEditorOptions = ProceduralAnimationEditorOptions;

	return var;
}

void FProceduralAnimationEditorModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->InvokeTab(ProceduralAnimationEditorTabName);
}

void FProceduralAnimationEditorModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FProceduralAnimationEditorCommands::Get().OpenPluginWindow);
}

FReply FProceduralAnimationEditorModule::GenerateFromSelection()
{
	//Get content selection
	TArray<FAssetData> Selection;
	GEditor->GetContentBrowserSelections(Selection);

	//Set selection as queue
	Queue = Selection;

	return FReply::Handled();
}

FReply FProceduralAnimationEditorModule::PreviewSelected()
{
	//Get content selection
	TArray<FAssetData> Selection;
	GEditor->GetContentBrowserSelections(Selection);

	//For all selected set the mesh in the viewport.. generally only the latest in selection will be viewed if you select more than one
	for (FAssetData _Data : Selection)
	{
		AssignAsset(_Data, false);
	}
	return FReply::Handled();

}

FReply FProceduralAnimationEditorModule::GenerateView()
{
	//Take a screnshot of the current view
	ViewportPtr->GetViewportClient()->TakeSingleShot();
	return FReply::Handled();
}

/*
bool FProceduralAnimationEditorModule::NextInQueue(float Delta)
{
	//If we have a queue(FAssetData)
	if (Queue.Num() > 0)
	{
		//Take the firs tin queue
		FAssetData _Data = Queue[0];
		//Delete from queue
		Queue.RemoveAt(0);
		AssignAsset(_Data, true);

	}
	//If we have created images...
	else if(CreatedImages.Num() > 0)
	{
		TArray<uint8> RawImage;

		//get first created image and remove from queu
		FString pngfile = CreatedImages[0];
		CreatedImages.RemoveAt(0);

		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		// Note: PNG format.  Other formats are supported
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

		if (FFileHelper::LoadFileToArray(RawImage, *pngfile))
		{
			if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawImage.GetData(), RawImage.Num()))
			{
				const TArray<uint8>* UncompressedBGRA = NULL;
				if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
				{
					// Setup packagename
					FString AssetName = pngfile.RightChop(pngfile.Find("/", ESearchCase::IgnoreCase, ESearchDir::FromEnd) + 1);
					FString USeAssetName = AssetName.LeftChop(AssetName.Len() - AssetName.Find( ".png", ESearchCase::IgnoreCase, ESearchDir::FromEnd));

					FString PackageName = TEXT("/Game/ProceduralAnimationEditorExports/" + USeAssetName);
					// Create new UPackage from PackageName
					UPackage* Package = CreatePackage(NULL, *PackageName);
					//Try to get the old package if this image already exists
					UPackage* OldPackage = LoadPackage(NULL, *PackageName,0);

					// Create Texture2D Factory
					auto TextureFact = NewObject<UTextureFactory>();
					TextureFact->AddToRoot();
					TextureFact->SuppressImportOverwriteDialog();

					// Get a pointer to the raw image data
					const uint8* PtrTexture = RawImage.GetData();

					// Stupidly use the damn factory
					UTexture2D* Texture = (UTexture2D*)TextureFact->FactoryCreateBinary(UTexture2D::StaticClass(), OldPackage? OldPackage : Package, *USeAssetName, RF_Standalone | RF_Public, NULL, TEXT("png"), PtrTexture, PtrTexture + RawImage.Num(), GWarn);

					if (Texture)
					{
						Texture->AssetImportData->Update(IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*USeAssetName));

						Package->SetDirtyFlag(true);
						TextureFact->RemoveFromRoot();

						//If we already have an old package we don't want to overwrite settings to defaults
						if (!OldPackage)
						{
							//Set settings to fit with UI
							Texture->Filter = TextureFilter::TF_Trilinear;
							//2D pixels for UI gives clearest results
							Texture->LODGroup = TextureGroup::TEXTUREGROUP_Pixels2D;
						}
						//Notify new asset created or store in the old package
						if (OldPackage)
						{
							OldPackage->SetDirtyFlag(true);
						}
						else
						{
							FAssetRegistryModule::AssetCreated(Texture);
						}
					}
				}
			}
		}
	}
	//If none of the above
	else
	{
		TArray<FString> AllImages;

		//Get all images in teh screenshot folter
		IFileManager& FileManager = IFileManager::Get();
		FileManager.FindFiles(AllImages, *FPaths::ScreenShotDir());
		for (FString Image : AllImages)
		{
			//Format full string
			FString Full = FPaths::ScreenShotDir() + Image;
			//If this image isn't in the startup(so wasn't known about before) we know it's new and we should process
			if (!StartupImages.Contains(Full))
			{

				//Add to startup images to prevent processing again
				StartupImages.Add(Full);
				CreatedImages.Add(Full);
			}
		}
	}

	return true;
}
*/

//remove from startup images so we can process the image again
void FProceduralAnimationEditorModule::RemoveFromPreKnown(const FString ToRemove)
{
	FString ToUseString = FPaths::ScreenShotDir()  + "Thumb_" + ToRemove + ".png";
	StartupImages.Remove(ToUseString);
}

void FProceduralAnimationEditorModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FProceduralAnimationEditorCommands::Get().OpenPluginWindow);
}


void FProceduralAnimationEditorModule::AssignAsset(FAssetData _Data, bool bTakeShot)
{
	//Cast to static mesh
	UStaticMesh* Mesh = Cast<UStaticMesh>(_Data.GetAsset());
	if (Mesh)
	{
		//If static mesh, set the static mesh and take a screenshot
		ViewportPtr->SetMesh(Mesh, bTakeShot);

		return;
	}

	//If not a mesh... try a skeletal mesh
	USkeletalMesh* SkelMesh = Cast<USkeletalMesh>(_Data.GetAsset());
	if (SkelMesh)
	{
		ViewportPtr->GetViewportClient()->SetSkelMesh(SkelMesh, nullptr, bTakeShot);

		return;
	}

	//If also not a skeletal mesh... try an animation
	UAnimationAsset* AnimationAsset = Cast<UAnimationAsset>(_Data.GetAsset());
	if (AnimationAsset)
	{
		ViewportPtr->GetViewportClient()->SetSkelMesh(AnimationAsset->GetSkeleton()->GetPreviewMesh(), AnimationAsset, bTakeShot);
		return;
	}

	//If also not a skeletal mesh... try an animation
	UMaterialInterface* MaterialAsset = Cast<UMaterialInterface>(_Data.GetAsset());
	if (MaterialAsset)
	{
		ViewportPtr->GetViewportClient()->SetMaterial(MaterialAsset, bTakeShot);
		return;
	}
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FProceduralAnimationEditorModule, ProceduralAnimationEditor)