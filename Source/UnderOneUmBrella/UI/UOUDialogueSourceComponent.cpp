// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUDialogueSourceComponent.h"

#include "Engine/DataTable.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UI/UOUDialogueSequenceData.h"
#include "UI/UOUUISubsystem.h"

namespace
{
	FText NormalizeDialogueDisplayText(const FText& SourceText)
	{
		if (SourceText.IsEmpty())
		{
			return SourceText;
		}

		// 데이터 테이블에서는 실제 줄바꿈 대신 \n처럼 적는 편이 안전해서,
		// 화면에 넘기기 전에 사람이 적은 이스케이프 문자를 실제 표시 문자로 바꿉니다.
		FString DisplayString = SourceText.ToString();
		DisplayString.ReplaceInline(TEXT("\\r\\n"), TEXT("\n"), ESearchCase::CaseSensitive);
		DisplayString.ReplaceInline(TEXT("\\n"), TEXT("\n"), ESearchCase::CaseSensitive);
		DisplayString.ReplaceInline(TEXT("\\t"), TEXT("\t"), ESearchCase::CaseSensitive);

		return FText::FromString(DisplayString);
	}
}

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
	if (!bDialogueAvailable)
	{
		return false;
	}

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

void UUOUDialogueSourceComponent::SetDialogueAvailable(bool bNewAvailable)
{
	bDialogueAvailable = bNewAvailable;
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
	bProximityBubbleTableCacheDirty = true;
	EnsureDialogueTableCache();
}

void UUOUDialogueSourceComponent::RefreshProximityBubbleTable()
{
	bProximityBubbleTableCacheDirty = true;
	EnsureProximityBubbleTableCache();
}

void UUOUDialogueSourceComponent::SetDialogueState(FName NewDialogueState)
{
	if (DialogueState == NewDialogueState)
	{
		return;
	}

	DialogueState = NewDialogueState;
	bDialogueTableCacheDirty = true;
	bProximityBubbleTableCacheDirty = true;
}

FText UUOUDialogueSourceComponent::GetProximityBubbleText() const
{
	if (!bEnableProximityBubble)
	{
		return FText::GetEmpty();
	}

	if (ShouldUseProximityBubbleTable())
	{
		EnsureProximityBubbleTableCache();
		return CachedProximityBubbleText;
	}

	if (ShouldUseDialogueTable())
	{
		EnsureDialogueTableCache();
		return CachedTableProximityBubbleText;
	}

	return FText::GetEmpty();
}

float UUOUDialogueSourceComponent::GetProximityBubbleDuration() const
{
	if (!bEnableProximityBubble)
	{
		return 0.0f;
	}

	if (ShouldUseProximityBubbleTable())
	{
		EnsureProximityBubbleTableCache();
		return CachedProximityBubbleDuration;
	}

	return 3.0f;
}

FName UUOUDialogueSourceComponent::GetProximityBubbleStyle() const
{
	if (ShouldUseProximityBubbleTable())
	{
		EnsureProximityBubbleTableCache();
		return !CachedProximityBubbleStyle.IsNone()
			? CachedProximityBubbleStyle
			: GetResolvedProximityBubbleState();
	}

	if (ShouldUseDialogueTable())
	{
		EnsureDialogueTableCache();
		return !CachedTableProximityBubbleStyle.IsNone()
			? CachedTableProximityBubbleStyle
			: DialogueState;
	}

	return DialogueState;
}

void UUOUDialogueSourceComponent::SetProximityBubbleEnabled(bool bNewEnabled)
{
	bEnableProximityBubble = bNewEnabled;
}

void UUOUDialogueSourceComponent::SetDialogueBubbleEnabled(bool bNewEnabled)
{
	bEnableDialogueBubble = bNewEnabled;
}

void UUOUDialogueSourceComponent::SetSpeechBubbleEnabled(bool bNewEnabled)
{
	SetProximityBubbleEnabled(bNewEnabled);
	SetDialogueBubbleEnabled(bNewEnabled);
}

void UUOUDialogueSourceComponent::ResetDialoguePlayback()
{
	bHasPlayed = false;
	LastStartTime = -1000.0f;
}

void UUOUDialogueSourceComponent::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
	case EOUUPuzzleResultAction::Resume:
		SetSpeechBubbleEnabled(false);
		break;
	case EOUUPuzzleResultAction::Deactivate:
	case EOUUPuzzleResultAction::Pause:
		SetSpeechBubbleEnabled(true);
		break;
	case EOUUPuzzleResultAction::Toggle:
		SetSpeechBubbleEnabled(!bEnableProximityBubble || !bEnableDialogueBubble);
		break;
	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
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
	CachedTableProximityBubbleStyle = NAME_None;
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
			CachedTableProximityBubbleText = NormalizeDialogueDisplayText(Row.ProximityBubbleText);
			CachedTableProximityBubbleStyle = !Row.PresentationStyle.IsNone()
				? Row.PresentationStyle
				: DialogueState;
		}

		if (CachedTableSpeakerName.IsEmpty() && !Row.SpeakerName.IsEmpty())
		{
			CachedTableSpeakerName = Row.SpeakerName;
		}

		FUOUDialogueLine Line;
		Line.LineId = Row.LineId;
		Line.SpeakerName = Row.SpeakerName;
		Line.BubbleText = NormalizeDialogueDisplayText(Row.BubbleText);
		Line.DialogueText = NormalizeDialogueDisplayText(Row.DialogueText);
		Line.Emotion = Row.Emotion;
		Line.PresentationStyle = !Row.PresentationStyle.IsNone()
			? Row.PresentationStyle
			: DialogueState;
		Line.BubbleDuration = Row.BubbleDuration;
		Line.DialogueDuration = Row.DialogueDuration;
		Line.bWaitForInput = Row.bWaitForInput;
		Line.bShowBubbleFirst = Row.bShowBubbleFirst;
		CachedTableLines.Add(Line);
	}
}

void UUOUDialogueSourceComponent::EnsureProximityBubbleTableCache() const
{
	if (!bProximityBubbleTableCacheDirty)
	{
		return;
	}

	CachedProximityBubbleText = FText::GetEmpty();
	CachedProximityBubbleDuration = 3.0f;
	CachedProximityBubbleStyle = NAME_None;
	bProximityBubbleTableCacheDirty = false;

	if (!ShouldUseProximityBubbleTable())
	{
		return;
	}

	TArray<FUOUProximityBubbleTableRow*> Rows;
	ProximityBubbleTable->GetAllRows<FUOUProximityBubbleTableRow>(TEXT("UOUDialogueSourceComponent"), Rows);

	const FName ResolvedActorId = GetResolvedProximityBubbleActorId();
	const FName ResolvedState = GetResolvedProximityBubbleState();

	TArray<FUOUProximityBubbleTableRow> MatchingRows;
	for (const FUOUProximityBubbleTableRow* Row : Rows)
	{
		if (Row == nullptr)
		{
			continue;
		}

		if (Row->ActorId != ResolvedActorId || Row->DialogueState != ResolvedState)
		{
			continue;
		}

		MatchingRows.Add(*Row);
	}

	MatchingRows.Sort([](const FUOUProximityBubbleTableRow& Left, const FUOUProximityBubbleTableRow& Right)
	{
		return Left.LineOrder < Right.LineOrder;
	});

	for (const FUOUProximityBubbleTableRow& Row : MatchingRows)
	{
		if (!Row.BubbleText.IsEmpty())
		{
			CachedProximityBubbleText = NormalizeDialogueDisplayText(Row.BubbleText);
			CachedProximityBubbleDuration = Row.BubbleDuration;
			CachedProximityBubbleStyle = !Row.PresentationStyle.IsNone()
				? Row.PresentationStyle
				: ResolvedState;
			return;
		}
	}
}

bool UUOUDialogueSourceComponent::ShouldUseDialogueTable() const
{
	return bUseDialogueTable && DialogueTable != nullptr && !GetResolvedDialogueActorId().IsNone();
}

bool UUOUDialogueSourceComponent::ShouldUseProximityBubbleTable() const
{
	return bUseProximityBubbleTable
		&& ProximityBubbleTable != nullptr
		&& !GetResolvedProximityBubbleActorId().IsNone()
		&& !GetResolvedProximityBubbleState().IsNone();
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

FName UUOUDialogueSourceComponent::GetResolvedProximityBubbleActorId() const
{
	if (!ProximityBubbleActorId.IsNone())
	{
		return ProximityBubbleActorId;
	}

	return GetResolvedDialogueActorId();
}

FName UUOUDialogueSourceComponent::GetResolvedProximityBubbleState() const
{
	if (!ProximityBubbleState.IsNone())
	{
		return ProximityBubbleState;
	}

	return DialogueState;
}
