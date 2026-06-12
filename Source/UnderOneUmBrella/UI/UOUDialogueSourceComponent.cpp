// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUDialogueSourceComponent.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UI/UOUDialogueSequenceData.h"
#include "UI/UOUUISubsystem.h"

UUOUDialogueSourceComponent::UUOUDialogueSourceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UUOUDialogueSourceComponent::StartDialogue(AActor* InstigatorActor)
{
	if (!CanStartDialogue())
	{
		return false;
	}

	UUOUUISubsystem* UISubsystem = GetUISubsystem(InstigatorActor);
	if (UISubsystem == nullptr)
	{
		return false;
	}

	MarkDialogueStarted();
	UISubsystem->StartDialogue(this, InstigatorActor);
	return true;
}

bool UUOUDialogueSourceComponent::CanStartDialogue() const
{
	if (GetLineCount() <= 0)
	{
		return false;
	}

	if (!bCanReplay && bHasPlayed)
	{
		return false;
	}

	return GetWorldTimeSeconds() - LastStartTime >= StartCooldown;
}

USceneComponent* UUOUDialogueSourceComponent::ResolveBubbleAnchor() const
{
	if (BubbleAnchor != nullptr)
	{
		return BubbleAnchor;
	}

	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return nullptr;
	}

	if (!BubbleAnchorComponentName.IsNone())
	{
		TArray<USceneComponent*> SceneComponents;
		OwnerActor->GetComponents(SceneComponents);
		for (USceneComponent* SceneComponent : SceneComponents)
		{
			if (SceneComponent != nullptr && SceneComponent->GetFName() == BubbleAnchorComponentName)
			{
				return SceneComponent;
			}
		}
	}

	return OwnerActor->GetRootComponent();
}

int32 UUOUDialogueSourceComponent::GetLineCount() const
{
	if (DialogueSequence != nullptr && DialogueSequence->Lines.Num() > 0)
	{
		return DialogueSequence->Lines.Num();
	}

	return InlineLines.Num();
}

const FUOUDialogueLine* UUOUDialogueSourceComponent::GetLine(int32 LineIndex) const
{
	if (LineIndex < 0)
	{
		return nullptr;
	}

	if (DialogueSequence != nullptr && DialogueSequence->Lines.IsValidIndex(LineIndex))
	{
		return &DialogueSequence->Lines[LineIndex];
	}

	return InlineLines.IsValidIndex(LineIndex) ? &InlineLines[LineIndex] : nullptr;
}

AActor* UUOUDialogueSourceComponent::GetSpeakerActor() const
{
	return GetOwner();
}

FText UUOUDialogueSourceComponent::GetSpeakerName() const
{
	return DefaultSpeakerName;
}

void UUOUDialogueSourceComponent::MarkDialogueStarted()
{
	bHasPlayed = true;
	LastStartTime = GetWorldTimeSeconds();
}

UUOUUISubsystem* UUOUDialogueSourceComponent::GetUISubsystem(AActor* InstigatorActor) const
{
	APlayerController* PlayerController = Cast<APlayerController>(InstigatorActor);
	if (PlayerController == nullptr)
	{
		const APawn* InstigatorPawn = Cast<APawn>(InstigatorActor);
		PlayerController = InstigatorPawn != nullptr ? Cast<APlayerController>(InstigatorPawn->GetController()) : nullptr;
	}

	if (PlayerController == nullptr && GetWorld() != nullptr)
	{
		PlayerController = GetWorld()->GetFirstPlayerController();
	}

	ULocalPlayer* LocalPlayer = PlayerController != nullptr ? PlayerController->GetLocalPlayer() : nullptr;
	return LocalPlayer != nullptr ? LocalPlayer->GetSubsystem<UUOUUISubsystem>() : nullptr;
}

float UUOUDialogueSourceComponent::GetWorldTimeSeconds() const
{
	const UWorld* World = GetWorld();
	return World != nullptr ? World->GetTimeSeconds() : 0.0f;
}