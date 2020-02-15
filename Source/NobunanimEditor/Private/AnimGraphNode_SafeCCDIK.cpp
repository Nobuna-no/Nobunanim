// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphNode_SafeCCDIK.h"
#include "Animation/AnimInstance.h"
#include "AnimNodeEditModes.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_SafeCCDIK 

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_SafeCCDIK::UAnimGraphNode_SafeCCDIK(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UAnimGraphNode_SafeCCDIK::GetControllerDescription() const
{
	return LOCTEXT("SafeCCDIK", "SafeCCDIK");
}

FText UAnimGraphNode_SafeCCDIK::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if ((TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle))
	{
		return GetControllerDescription();
	}
	// @TODO: the bone can be altered in the property editor, so we have to 
	//        choose to mark this dirty when that happens for this to properly work
	else //if (!CachedNodeTitles.IsTitleCached(TitleType, this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
		Args.Add(TEXT("RootBoneName"), FText::FromName(Node.RootBone.BoneName));
		Args.Add(TEXT("TipBoneName"), FText::FromName(Node.TipBone.BoneName));

		// FText::Format() is slow, so we cache this to save on performance
		if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_SafeCCDIKBone_ListTitle", "{ControllerDescription} - Root: {RootBoneName}, Tip: {TipBoneName} "), Args), this);
		}
		else
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_SafeCCDIKBone_Title", "{ControllerDescription}\nRoot: {RootBoneName} - Tip: {TipBoneName} "), Args), this);
		}
	}
	return CachedNodeTitles[TitleType];
}

void UAnimGraphNode_SafeCCDIK::CopyNodeDataToPreviewNode(FAnimNode_Base* InPreviewNode)
{
	FAnimNode_SafeCCDIK* CCDIK = static_cast<FAnimNode_SafeCCDIK*>(InPreviewNode);

	// copies Pin values from the internal node to get data which are not compiled yet
	CCDIK->EffectorLocation = Node.EffectorLocation;
}

FEditorModeID UAnimGraphNode_SafeCCDIK::GetEditorMode() const
{
	return "AnimGraph.SkeletalControl.SafeCCDIK";
}

void UAnimGraphNode_SafeCCDIK::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FBoneReference, BoneName))
	{
		USkeleton* Skeleton = GetAnimBlueprint()->TargetSkeleton;
		if (Node.TipBone.BoneName != NAME_None && Node.RootBone.BoneName != NAME_None)
		{
			const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
			const int32 TipBoneIndex = RefSkeleton.FindBoneIndex(Node.TipBone.BoneName);
			const int32 RootBoneIndex = RefSkeleton.FindBoneIndex(Node.RootBone.BoneName);

			if (TipBoneIndex != INDEX_NONE && RootBoneIndex != INDEX_NONE)
			{
				const int32 Depth = RefSkeleton.GetDepthBetweenBones(TipBoneIndex, RootBoneIndex);
				if (Depth >= 0)
				{
					Node.ResizeRotationLimitPerJoints(Depth + 1);
				}
				else
				{
					Node.ResizeRotationLimitPerJoints(0);
				}
			}
			else
			{
				Node.ResizeRotationLimitPerJoints(0);
			}
		}
		else
		{
			Node.ResizeRotationLimitPerJoints(0);
		}
	}
}

#undef LOCTEXT_NAMESPACE