// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUDialogueSourceComponent.h"

#include "Engine/DataTable.h"
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
	if (ShouldUseDialogueTable())
	{
		EnsureDialogueTableCache();
		return CachedTableLines.Num();
	}

	if (DialogueSequence != nullptr && DialogueSequence->Lines.Num() > 0)
	{
		return DialogueSequence->Lines.Num();
	}

	return InlineLines.Num();
}

void UUOUDialogueSourceComponent::RefreshDialogueTable()
{
	bDialogueTableCacheDirty = true;
	EnsureDialogueTableCache();
}

void UUOUDialogueSourceComponent::SetDialogueState(FName NewDialogueState)
{
	if (DialogueState == NewDialogueState)
	{
		return;
	}

	DialogueState = NewDialogueState;
	bDialogueTableCacheDirty = true;
}

FText UUOUDialogueSourceComponent::GetProximityBubbleText() const
{
	if (ShouldUseDialogueTable())
	{
		EnsureDialogueTableCache();
		return CachedTableProximityBubbleText;
	}

	return FText::GetEmpty();
}

bool UUOUDialogueSourceComponent::IsUsingDialogueTable() const
{
	return ShouldUseDialogueTable();
}

const FUOUDialogueLine* UUOUDialogueSourceComponent::GetLine(int32 LineIndex) const
{
	if (LineIndex < 0)
	{
		return nullptr;
	}

	if (ShouldUseDialogueTable())
	{
		EnsureDialogueTableCache();
		return CachedTableLines.IsValidIndex(LineIndex) ? &CachedTableLines[LineIndex] : nullptr;
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
	if (ShouldUseDialogueTable())
	{
		EnsureDialogueTableCache();
		if (!CachedTableSpeakerName.IsEmpty())
		{
			return CachedTableSpeakerName;
		}
	}

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

void UUOUDialogueSourceComponent::EnsureDialogueTableCache() const
{
	if (!bDialogueTableCacheDirty)
	{
		return;
	}

	CachedTableLines.Reset();
	CachedTableProximityBubbleText = FText::GetEmpty();
	CachedTableSpeakerName = FText::GetEmpty();
	bDialogueTableCacheDirty = false;

	if (!ShouldUseDialogueTable())
	{
		return;
	}

	TArray<FUOUDialogueTableRow*> Rows;
	DialogueTable->GetAllRows<FUOUDialogueTableRow>(TEXT("UOUDialogueSourceComponent"), Rows);

	const FName ResolvedActorId = GetResolvedDialogueActorId();
	TArray<FUOUDialogueTableRow> MatchingRows;
	for (const FUOUDialogueTableRow* Row : Rows)
	{
		if (Row == nullptr)
		{
			continue;
		}

		if (Row->ActorId != ResolvedActorId || Row->DialogueState != DialogueState)
		{
			continue;
		}

		MatchingRows.Add(*Row);
	}

	MatchingRows.Sort([](const FUOUDialogueTableRow& Left, const FUOUDialogueTableRow& Right)
	{
		return Left.LineOrder < Right.LineOrder;
	});

	for (const FUOUDialogueTableRow& Row : MatchingRows)
	{
		if (CachedTableProximityBubbleText.IsEmpty() && !Row.ProximityBubbleText.IsEmpty())
		{
			CachedTableProximityBubbleText = Row.ProximityBubbleText;
		}

		if (CachedTableSpeakerName.IsEmpty() && !Row.SpeakerName.IsEmpty())
		{
			CachedTableSpeakerName = Row.SpeakerName;
		}

		FUOUDialogueLine Line;
		Line.LineId = Row.LineId;
		Line.SpeakerName = Row.SpeakerName;
		Line.BubbleText = Row.BubbleText;
		Line.DialogueText = Row.DialogueText;
		Line.Emotion = Row.Emotion;
		Line.BubbleDuration = Row.BubbleDuration;
		Line.DialogueDuration = Row.DialogueDuration;
		Line.bWaitForInput = Row.bWaitForInput;
		Line.bShowBubbleFirst = Row.bShowBubbleFirst;
		CachedTableLines.Add(Line);
	}
}

bool UUOUDialogueSourceComponent::ShouldUseDialogueTable() const
{
	return bUseDialogueTable && DialogueTable != nullptr && !GetResolvedDialogueActorId().IsNone();
}

FName UUOUDialogueSourceComponent::GetResolvedDialogueActorId() const
{
	if (!DialogueActorId.IsNone())
	{
		return DialogueActorId;
	}

	const AActor* OwnerActor = GetOwner();
	return OwnerActor != nullptr ? OwnerActor->GetFName() : NAME_None;
}
