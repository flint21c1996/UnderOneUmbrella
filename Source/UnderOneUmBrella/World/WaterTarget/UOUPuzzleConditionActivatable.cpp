// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUPuzzleConditionActivatable.h"

bool IUOUPuzzleConditionActivatable::CanActivateByPuzzleCondition_Implementation(const FUOUPuzzleConditionContext& Context) const
{
	return true;
}

void IUOUPuzzleConditionActivatable::ActivateByPuzzleCondition_Implementation(const FUOUPuzzleConditionContext& Context)
{
}

void IUOUPuzzleConditionActivatable::DeactivateByPuzzleCondition_Implementation(const FUOUPuzzleConditionContext& Context)
{
}

void IUOUPuzzleConditionActivatable::OnPuzzleConditionStateChanged_Implementation(const FUOUPuzzleConditionContext& Context)
{
}
