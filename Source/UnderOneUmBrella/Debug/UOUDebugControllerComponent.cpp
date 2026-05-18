// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/UOUDebugControllerComponent.h"

UUOUDebugControllerComponentBase::UUOUDebugControllerComponentBase()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUDebugControllerComponentBase::SetDebugEnabled(bool bNewEnabled)
{
	bEnabled = bNewEnabled;
}

bool UUOUDebugControllerComponentBase::IsDebugEnabled() const
{
	return bEnabled;
}

FName UUOUDebugControllerComponentBase::GetDebugCategoryName() const
{
	switch (DebugCategory)
	{
	case EUOUDebugCategory::Player:
		return TEXT("Player");
	case EUOUDebugCategory::NPC:
		return TEXT("NPC");
	case EUOUDebugCategory::Puzzle:
		return TEXT("Puzzle");
	case EUOUDebugCategory::Interaction:
		return TEXT("Interaction");
	case EUOUDebugCategory::VFX:
		return TEXT("VFX");
	case EUOUDebugCategory::Performance:
		return TEXT("Performance");
	case EUOUDebugCategory::System:
		return TEXT("System");
	default:
		return NAME_None;
	}
}

UUOUPlayerDebugControllerComponent::UUOUPlayerDebugControllerComponent()
{
	DebugCategory = EUOUDebugCategory::Player;
	DebugColor = FColor::Green;
	Priority = 100;
}

UUOUNPCDebugControllerComponent::UUOUNPCDebugControllerComponent()
{
	DebugCategory = EUOUDebugCategory::NPC;
	DebugColor = FColor::Orange;
	Priority = 70;
}

UUOUPuzzleDebugControllerComponent::UUOUPuzzleDebugControllerComponent()
{
	DebugCategory = EUOUDebugCategory::Puzzle;
	DebugColor = FColor::Cyan;
	Priority = 80;
}

UUOUInteractionDebugControllerComponent::UUOUInteractionDebugControllerComponent()
{
	DebugCategory = EUOUDebugCategory::Interaction;
	DebugColor = FColor::Yellow;
	Priority = 60;
}

UUOUVFXDebugControllerComponent::UUOUVFXDebugControllerComponent()
{
	DebugCategory = EUOUDebugCategory::VFX;
	DebugColor = FColor::Purple;
	Priority = 40;
}

UUOUPerformanceDebugControllerComponent::UUOUPerformanceDebugControllerComponent()
{
	DebugCategory = EUOUDebugCategory::Performance;
	DebugColor = FColor::Silver;
	Priority = 90;
}
