// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Timer/UOUTimerConditionComponent.h"

#include "Engine/World.h"
#include "TimerManager.h"

UUOUTimerConditionComponent::UUOUTimerConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUTimerConditionComponent::BeginPlay()
{
	Super::BeginPlay();

	TimerDuration = FMath::Max(0.0f, TimerDuration);
	SetSatisfiedState(bInitialSatisfied, false);

	if (bAutoStartOnBeginPlay)
	{
		StartTimer();
	}
}

void UUOUTimerConditionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearTimerHandle();
	bTimerRunning = false;

	Super::EndPlay(EndPlayReason);
}

FText UUOUTimerConditionComponent::GetDebugSummaryText_Implementation() const
{
	UWorld* World = GetWorld();
	const bool bPaused = World != nullptr && World->GetTimerManager().IsTimerPaused(TimerHandle);
	const TCHAR* TimerState = bPaused
		? TEXT("Paused")
		: (bTimerRunning ? TEXT("Running") : TEXT("Stopped"));

	const TArray<FString> DebugLines = {
		FString::Printf(TEXT("Timer Condition: %s"), IsSatisfied() ? TEXT("Satisfied") : TEXT("Unsatisfied")),
		FString::Printf(TEXT("Timer: %s"), TimerState),
		FString::Printf(TEXT("Duration: %.2f"), TimerDuration),
		FString::Printf(TEXT("Remaining: %.2f"), GetRemainingTime())
	};

	return FText::FromString(FString::Join(DebugLines, LINE_TERMINATOR));
}

void UUOUTimerConditionComponent::StartTimer()
{
	ClearTimerHandle();

	TimerDuration = FMath::Max(0.0f, TimerDuration);
	bTimerRunning = true;

	if (bResetSatisfiedOnStart)
	{
		SetSatisfiedState(false, true);
	}

	OnTimerStarted.Broadcast();

	if (TimerDuration <= KINDA_SMALL_NUMBER)
	{
		HandleTimerFinished();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TimerHandle,
			this,
			&UUOUTimerConditionComponent::HandleTimerFinished,
			TimerDuration,
			false);
	}
}

void UUOUTimerConditionComponent::RestartTimer()
{
	StartTimer();
}

void UUOUTimerConditionComponent::StopTimer()
{
	ClearTimerHandle();
	bTimerRunning = false;

	if (bSetUnsatisfiedOnStop)
	{
		SetSatisfiedState(false, true);
	}

	OnTimerStopped.Broadcast();
}

void UUOUTimerConditionComponent::ResetTimerCondition()
{
	ClearTimerHandle();
	bTimerRunning = false;
	SetSatisfiedState(bInitialSatisfied, true);
	OnTimerStopped.Broadcast();
}

void UUOUTimerConditionComponent::PauseTimer()
{
	if (UWorld* World = GetWorld())
	{
		if (World->GetTimerManager().TimerExists(TimerHandle))
		{
			World->GetTimerManager().PauseTimer(TimerHandle);
		}
	}
}

void UUOUTimerConditionComponent::ResumeTimer()
{
	if (UWorld* World = GetWorld())
	{
		if (World->GetTimerManager().IsTimerPaused(TimerHandle))
		{
			World->GetTimerManager().UnPauseTimer(TimerHandle);
		}
	}
}

void UUOUTimerConditionComponent::FinishTimerImmediately()
{
	HandleTimerFinished();
}

bool UUOUTimerConditionComponent::IsTimerActive() const
{
	UWorld* World = GetWorld();
	return bTimerRunning && World != nullptr && World->GetTimerManager().TimerExists(TimerHandle);
}

float UUOUTimerConditionComponent::GetRemainingTime() const
{
	UWorld* World = GetWorld();
	if (World == nullptr || !World->GetTimerManager().TimerExists(TimerHandle))
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, World->GetTimerManager().GetTimerRemaining(TimerHandle));
}

void UUOUTimerConditionComponent::HandleTimerFinished()
{
	ClearTimerHandle();
	bTimerRunning = false;
	SetSatisfiedState(true, true);
	OnTimerFinished.Broadcast();
}

void UUOUTimerConditionComponent::ClearTimerHandle()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle);
	}
}
