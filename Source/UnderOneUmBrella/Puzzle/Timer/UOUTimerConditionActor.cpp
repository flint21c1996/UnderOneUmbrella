// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Timer/UOUTimerConditionActor.h"

#include "Components/SceneComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "Debug/UOUPuzzleDebugInfoProvider.h"
#include "Puzzle/Timer/UOUTimerConditionComponent.h"

AUOUTimerConditionActor::AUOUTimerConditionActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	TimerConditionComponent = CreateDefaultSubobject<UUOUTimerConditionComponent>(TEXT("TimerConditionComponent"));
}

void AUOUTimerConditionActor::BeginPlay()
{
	Super::BeginPlay();

	if (bRegisterDebugProvider)
	{
		if (UWorld* World = GetWorld())
		{
			if (UUOUDebugSubsystem* DebugSubsystem = World->GetSubsystem<UUOUDebugSubsystem>())
			{
				DebugSubsystem->RegisterDebugProvider(this);
			}
		}
	}
}

void AUOUTimerConditionActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UUOUDebugSubsystem* DebugSubsystem = World->GetSubsystem<UUOUDebugSubsystem>())
		{
			DebugSubsystem->UnregisterDebugProvider(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AUOUTimerConditionActor::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	if (TimerConditionComponent == nullptr)
	{
		return;
	}

	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		TimerConditionComponent->StartTimer();
		break;

	case EOUUPuzzleResultAction::Deactivate:
		TimerConditionComponent->ResetTimerCondition();
		break;

	case EOUUPuzzleResultAction::Pause:
		TimerConditionComponent->PauseTimer();
		break;

	case EOUUPuzzleResultAction::Resume:
		TimerConditionComponent->ResumeTimer();
		break;

	case EOUUPuzzleResultAction::Toggle:
		if (TimerConditionComponent->bTimerRunning)
		{
			TimerConditionComponent->StopTimer();
			return;
		}
		TimerConditionComponent->StartTimer();
		break;

	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
}

EUOUDebugCategory AUOUTimerConditionActor::GetDebugCategory_Implementation() const
{
	return EUOUDebugCategory::Puzzle;
}

bool AUOUTimerConditionActor::IsDebugProviderEnabled_Implementation() const
{
	return bRegisterDebugProvider;
}

FText AUOUTimerConditionActor::GetDebugDisplayName_Implementation() const
{
#if WITH_EDITOR
	return FText::FromString(GetActorLabel());
#else
	return FText::FromString(GetName());
#endif
}

FText AUOUTimerConditionActor::GetDebugSummaryText_Implementation() const
{
	if (TimerConditionComponent == nullptr)
	{
		return FText::FromString(TEXT("Timer Condition: Missing Component"));
	}

	const TArray<FString> DebugLines = IUOUPuzzleDebugInfoProvider::Execute_GetPuzzleDebugInfo(TimerConditionComponent);
	return FText::FromString(FString::Join(DebugLines, LINE_TERMINATOR));
}

FVector AUOUTimerConditionActor::GetDebugWorldLocation_Implementation() const
{
	return GetActorLocation() + DebugWorldLocationOffset;
}

void AUOUTimerConditionActor::GetDebugConnections_Implementation(TArray<FUOUDebugConnection>& OutConnections) const
{
	OutConnections.Reset();
}
