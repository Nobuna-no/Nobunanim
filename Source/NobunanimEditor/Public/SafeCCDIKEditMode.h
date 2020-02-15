// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NobunanimBaseEditMode.h"

class FSafeCCDIKEditMode : public FNobunanimBaseEditMode
{
public:
	/** IAnimNodeEditMode interface */
	virtual void EnterMode(class UAnimGraphNode_Base* InEditorNode, struct FAnimNode_Base* InRuntimeNode) override;
	virtual void ExitMode() override;
	virtual FVector GetWidgetLocation() const override;
	virtual FWidget::EWidgetMode GetWidgetMode() const override;
	virtual void DoTranslation(FVector& InTranslation) override;
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
private:
	struct FAnimNode_SafeCCDIK* RuntimeNode;
	class UAnimGraphNode_SafeCCDIK* GraphNode;
};