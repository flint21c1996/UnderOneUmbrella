// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UI/UOUUITypes.h"
#include "UOUDialogueSourceComponent.generated.h"

class UUOUDialogueSequenceData;
class UUOUUISubsystem;
class UDataTable;
class USceneComponent;

// Component that gives an NPC or world object dialogue content and playback rules.
// Designers can set bubble anchors, speaker name, repeat rules, and inline or DataAsset lines.
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent, DisplayName="UOU Dialogue Source"))
class UNDERONEUMBRELLA_API UUOUDialogueSourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUDialogueSourceComponent();

	// Requests this source's dialogue through the local UI subsystem.
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	bool StartDialogue(AActor* InstigatorActor);

	// Checks repeat, cooldown, and line availability before playback.
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool CanStartDialogue() const;

	// Returns the scene component the speech bubble should follow.
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	USceneComponent* ResolveBubbleAnchor() const;

	// Returns the number of playable lines from the DataAsset or inline list.
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	int32 GetLineCount() const;

	// CSV DataTable을 다시 읽어서 현재 ActorId와 DialogueState에 맞는 대사를 캐시합니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Table")
	void RefreshDialogueTable();

	// 퍼즐 진행도에 따라 사용할 대화 상태를 바꿉니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Table")
	void SetDialogueState(FName NewDialogueState);

	UFUNCTION(BlueprintPure, Category = "Dialogue|Table")
	FName GetDialogueState() const { return DialogueState; }

	UFUNCTION(BlueprintPure, Category = "Dialogue|Table")
	bool IsUsingDialogueTable() const;

	// 플레이어가 가까이 왔을 때 띄울 짧은 말풍선입니다. 비어 있으면 표시하지 않아도 됩니다.
	UFUNCTION(BlueprintPure, Category = "Dialogue|Bubble")
	FText GetProximityBubbleText() const;

	const FUOUDialogueLine* GetLine(int32 LineIndex) const;
	AActor* GetSpeakerActor() const;
	FText GetSpeakerName() const;
	void MarkDialogueStarted();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Table")
	bool bUseDialogueTable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Table", meta = (EditCondition = "bUseDialogueTable"))
	TObjectPtr<UDataTable> DialogueTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Table", meta = (EditCondition = "bUseDialogueTable"))
	FName DialogueActorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Table", meta = (EditCondition = "bUseDialogueTable"))
	FName DialogueState = TEXT("Default");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UUOUDialogueSequenceData> DialogueSequence = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FUOUDialogueLine> InlineLines;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FText DefaultSpeakerName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Bubble")
	TObjectPtr<USceneComponent> BubbleAnchor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Bubble")
	FName BubbleAnchorComponentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules")
	bool bCanReplay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules", meta = (ClampMin = "0.0"))
	float StartCooldown = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue|Runtime")
	bool bHasPlayed = false;

private:
	UUOUUISubsystem* GetUISubsystem(AActor* InstigatorActor) const;
	float GetWorldTimeSeconds() const;
	void EnsureDialogueTableCache() const;
	bool ShouldUseDialogueTable() const;
	FName GetResolvedDialogueActorId() const;

	float LastStartTime = -1000.0f;

	mutable TArray<FUOUDialogueLine> CachedTableLines;
	mutable FText CachedTableProximityBubbleText;
	mutable FText CachedTableSpeakerName;
	mutable bool bDialogueTableCacheDirty = true;
};
