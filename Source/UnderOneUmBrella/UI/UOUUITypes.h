// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Player/UOUUmbrellaComponent.h"
#include "UOUUITypes.generated.h"

// Snapshot data used by the HUD to draw umbrella ownership, state, and water level at once.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUUmbrellaHUDState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "UI|Umbrella")
	bool bHasUmbrella = false;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Umbrella")
	EUOUUmbrellaState UmbrellaState = EUOUUmbrellaState::Closed;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Umbrella")
	float StoredWater = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Umbrella")
	float MaxStoredWater = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Umbrella")
	float StoredWaterRatio = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Umbrella")
	float PlayerRainAmount = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Umbrella")
	bool bBlockingRain = false;
};

// A single dialogue beat that can feed both the NPC bubble and the bottom dialogue box.
// TODO: NPC의 짧은 반복 대사, 느낌표와 물음표 반응, 플레이어 행동에 따른 아이콘 전환을 별도 반응 데이터로 확장합니다.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUDialogueLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	FName LineId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	FText SpeakerName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	FText BubbleText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	FText DialogueText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	FName Emotion = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue", meta = (ClampMin = "0.0"))
	float BubbleDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue", meta = (ClampMin = "0.0"))
	float DialogueDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	bool bWaitForInput = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	bool bShowBubbleFirst = true;
};

// CSV DataTable에서 읽어오는 대화 한 줄입니다.
// 같은 ActorId와 DialogueState를 가진 행들을 LineOrder 순서로 모아 하나의 대화 리스트로 사용합니다.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUDialogueTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	FName ActorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	FName DialogueState = TEXT("Default");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	int32 LineOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	FName LineId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	FText SpeakerName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	FText ProximityBubbleText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	FText BubbleText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	FText DialogueText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	FName Emotion = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue", meta = (ClampMin = "0.0"))
	float BubbleDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue", meta = (ClampMin = "0.0"))
	float DialogueDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	bool bWaitForInput = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Dialogue")
	bool bShowBubbleFirst = true;
};

// Short title-card data for chapter names, place names, or stage introductions.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUTitleDisplayData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Title")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Title")
	FText Subtitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Title", meta = (ClampMin = "0.0"))
	float DisplayDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Title")
	bool bCanReplay = false;
};
