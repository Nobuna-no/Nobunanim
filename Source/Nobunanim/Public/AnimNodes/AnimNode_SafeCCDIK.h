// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_SafeCCDIK.generated.h"

/**
*	Controller which implements the CCDIK IK approximation algorithm
*/

/** Transient structure for CCDIK node evaluation */
struct SafeCCDIKChainLink
{
public:
	/** Transform of bone in component space. */
	FTransform Transform;

	/** Transform of bone in local space. This is mutable as their component space changes or parents*/
	FTransform LocalTransform;

	/** Bone Index in SkeletalMesh */
	FCompactPoseBoneIndex BoneIndex;

	/** Transform Index that this control will output */
	int32 TransformIndex;

	/** Child bones which are overlapping this bone.
	 * They have a zero length distance, so they will inherit this bone's transformation. */
	TArray<int32> ChildZeroLengthTransformIndices;

	float CurrentAngleDelta;

	SafeCCDIKChainLink()
		: BoneIndex(INDEX_NONE)
		, TransformIndex(INDEX_NONE)
		, CurrentAngleDelta(0.f)
	{
	}

	SafeCCDIKChainLink(const FTransform& InTransform, const FTransform& InLocalTransform, const FCompactPoseBoneIndex& InBoneIndex, const int32& InTransformIndex)
		: Transform(InTransform)
		, LocalTransform(InLocalTransform)
		, BoneIndex(InBoneIndex)
		, TransformIndex(InTransformIndex)
		, CurrentAngleDelta(0.f)
	{
	}
};

USTRUCT(BlueprintType)
struct NOBUNANIM_API FAnimNode_SafeCCDIK : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

	/** Coordinates for target location of tip bone - if EffectorLocationSpace is bone, this is the offset from Target Bone to use as target location*/
	UPROPERTY(EditAnywhere, Category = Effector, meta = (PinShownByDefault))
	FVector EffectorLocation;

	/** Reference frame of Effector Transform. */
	UPROPERTY(EditAnywhere, Category = Effector)
	TEnumAsByte<enum EBoneControlSpace> EffectorLocationSpace;

	/** If EffectorTransformSpace is a bone, this is the bone to use. **/
	UPROPERTY(EditAnywhere, Category = Effector)
	FBoneSocketTarget EffectorTarget;

	/** Name of tip bone */
	UPROPERTY(EditAnywhere, Category = Solver)
	FBoneReference TipBone;

	/** Name of the root bone*/
	UPROPERTY(EditAnywhere, Category = Solver)
	FBoneReference RootBone;

	/** Tolerance for final tip location delta from EffectorLocation*/
	UPROPERTY(EditAnywhere, Category = Solver)
	float Precision;

	/** Maximum number of iterations allowed, to control performance. */
	UPROPERTY(EditAnywhere, Category = Solver)
	int32 MaxIterations;

	/** Toggle drawing of axes to debug joint rotation*/
	UPROPERTY(EditAnywhere, Category = Solver)
	bool bStartFromTail;

	/** Tolerance for final tip location delta from EffectorLocation*/
	UPROPERTY(EditAnywhere, Category = Solver)
	bool bEnableRotationLimit;

private:
	/** symmetry rotation limit per joint. Index 0 matches with root bone and last index matches with tip bone. */
	UPROPERTY(EditAnywhere, EditFixedSize, Category = Solver)
	TArray<float> RotationLimitPerJoints;

public:
	FAnimNode_SafeCCDIK();

	// FAnimNode_Base interface
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

	// Convenience function to get current (pre-translation iteration) component space location of bone by bone index
	FVector GetCurrentLocation(FCSPose<FCompactPose>& MeshBases, const FCompactPoseBoneIndex& BoneIndex);

	static FTransform GetTargetTransform(const FTransform& InComponentTransform, FCSPose<FCompactPose>& MeshBases, FBoneSocketTarget& InTarget, EBoneControlSpace Space, const FVector& InOffset);

	// return true if updated
	bool UpdateChainLink(TArray<SafeCCDIKChainLink>& Chain, int32 LinkIndex, const FVector& TargetPos) const;
public:
#if WITH_EDITOR
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	TArray<FVector> DebugLines;
#endif // #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// resize rotation limit array based on set up
	void ResizeRotationLimitPerJoints(int32 NewSize);
#endif
};