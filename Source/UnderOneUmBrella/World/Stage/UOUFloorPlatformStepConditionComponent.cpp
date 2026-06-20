// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Stage/UOUFloorPlatformStepConditionComponent.h"

#include "GameFramework/Actor.h"
#include "World/Stage/UOUFloorPlatformActor.h"

UUOUFloorPlatformStepConditionComponent::UUOUFloorPlatformStepConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUFloorPlatformStepConditionComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveTargetPlatform();
	SubscribeTargetPlatform();
	RefreshConditionState();
}

void UUOUFloorPlatformStepConditionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnsubscribeTargetPlatform();

	Super::EndPlay(EndPlayReason);
}

TArray<FString> UUOUFloorPlatformStepConditionComponent::GetPuzzleDebugInfo_Implementation() const
{
	const int32 LastArrivedIndex = TargetPlatform != nullptr
		? TargetPlatform->GetLastArrivedMoveStepIndex()
		: INDEX_NONE;

	return {
		FString::Printf(
			TEXT("Floor Step Condition: %s"),
			IsSatisfied() ? TEXT("Satisfied") : TEXT("Unsatisfied")),
		FString::Printf(TEXT("Target Platform: %s"), *GetNameSafe(TargetPlatform.Get())),
		FString::Printf(TEXT("Required Step: %d"), RequiredStepIndex),
		FString::Printf(TEXT("Last Arrived Step: %d"), LastArrivedIndex),
		FString::Printf(
			TEXT("Moving: %s / Require Stop: %s"),
			TargetPlatform != nullptr && TargetPlatform->IsMoving() ? TEXT("Y") : TEXT("N"),
			bRequireNotMoving ? TEXT("Y") : TEXT("N"))
	};
}

void UUOUFloorPlatformStepConditionComponent::GetPuzzleDebugInputActors_Implementation(
	TArray<AActor*>& OutInputActors) const
{
	if (TargetPlatform != nullptr)
	{
		OutInputActors.AddUnique(TargetPlatform.Get());
	}
}

void UUOUFloorPlatformStepConditionComponent::RefreshConditionState()
{
	ResolveTargetPlatform();

	const bool bNextSatisfied = TargetPlatform != nullptr
		&& TargetPlatform->IsAtMoveStepIndex(RequiredStepIndex, bRequireNotMoving);

	SetSatisfiedState(bNextSatisfied, true);
}

void UUOUFloorPlatformStepConditionComponent::HandleTargetPlatformMoveFinished(AUOUFloorPlatformActor* Platform)
{
	if (Platform != TargetPlatform)
	{
		return;
	}

	RefreshConditionState();
}

void UUOUFloorPlatformStepConditionComponent::HandleTargetPlatformCompletionStateChanged(
	EOUUPuzzleResultAction Action,
	bool bIsCompleted)
{
	if (Action != EOUUPuzzleResultAction::Activate)
	{
		return;
	}

	RefreshConditionState();
}

void UUOUFloorPlatformStepConditionComponent::ResolveTargetPlatform()
{
	if (TargetPlatform != nullptr || !bAutoResolveOwnerPlatform)
	{
		return;
	}

	TargetPlatform = Cast<AUOUFloorPlatformActor>(GetOwner());
}

void UUOUFloorPlatformStepConditionComponent::SubscribeTargetPlatform()
{
	UnsubscribeTargetPlatform();

	if (TargetPlatform == nullptr)
	{
		return;
	}

	SubscribedTargetPlatform = TargetPlatform;
	SubscribedTargetPlatform->OnMoveFinished.RemoveDynamic(
		this,
		&UUOUFloorPlatformStepConditionComponent::HandleTargetPlatformMoveFinished);
	SubscribedTargetPlatform->OnMoveFinished.AddDynamic(
		this,
		&UUOUFloorPlatformStepConditionComponent::HandleTargetPlatformMoveFinished);

	if (FOnUOUPuzzleResultCompletionStateChangedNativeSignature* CompletionStateChangedEvent =
		SubscribedTargetPlatform->GetPuzzleResultCompletionStateChangedEvent())
	{
		CompletionStateChangedHandle = CompletionStateChangedEvent->AddUObject(
			this,
			&UUOUFloorPlatformStepConditionComponent::HandleTargetPlatformCompletionStateChanged);
	}
}

void UUOUFloorPlatformStepConditionComponent::UnsubscribeTargetPlatform()
{
	if (SubscribedTargetPlatform == nullptr)
	{
		CompletionStateChangedHandle.Reset();
		return;
	}

	SubscribedTargetPlatform->OnMoveFinished.RemoveDynamic(
		this,
		&UUOUFloorPlatformStepConditionComponent::HandleTargetPlatformMoveFinished);

	if (CompletionStateChangedHandle.IsValid())
	{
		if (FOnUOUPuzzleResultCompletionStateChangedNativeSignature* CompletionStateChangedEvent =
			SubscribedTargetPlatform->GetPuzzleResultCompletionStateChangedEvent())
		{
			CompletionStateChangedEvent->Remove(CompletionStateChangedHandle);
		}
	}

	CompletionStateChangedHandle.Reset();
	SubscribedTargetPlatform = nullptr;
}
