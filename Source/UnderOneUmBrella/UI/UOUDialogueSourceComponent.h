// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UI/UOUUITypes.h"
#include "UOUDialogueSourceComponent.generated.h"

class UUOUDialogueSequenceData;
class UUOUUISubsystem;
class UDataTable;
class USceneComponent;

// Component that gives an NPC or world object dialogue content and playback rules.
// Designers can set bubble anchors, speaker name, repeat rules, and inline or DataAsset lines.
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent, DisplayName="UOU Dialogue Source"))
class UNDERONEUMBRELLA_API UUOUDialogueSourceComponent : public UActorComponent, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	UUOUDialogueSourceComponent();

	// Requests this source's dialogue through the local UI subsystem.
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	bool StartDialogue(AActor* InstigatorActor);

	// 현재 DialogueState의 BubbleText만 순서대로 재생합니다.
	// 일반 대화 UI와 카메라 연출, 입력 잠금, 대화 완료 이벤트는 시작하지 않습니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Bubble")
	bool StartBubbleOnlyDialogue();

	// Checks repeat, cooldown, and line availability before playback.
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool CanStartDialogue() const;

	// Speech Bubble 표시와 독립적으로 실제 대화 시작 가능 여부를 변경합니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Rules")
	void SetDialogueAvailable(bool bNewAvailable);

	UFUNCTION(BlueprintPure, Category = "Dialogue|Rules")
	bool IsDialogueAvailable() const { return bDialogueAvailable; }

	// Returns the scene component the speech bubble should follow.
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	USceneComponent* ResolveBubbleAnchor() const;

	// Returns the number of playable lines from the DataAsset or inline list.
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	int32 GetLineCount() const;

	// CSV DataTable을 다시 읽어서 현재 ActorId와 DialogueState에 맞는 대사를 캐시합니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Table")
	void RefreshDialogueTable();

	// 접근 말풍선 전용 CSV DataTable을 다시 읽습니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Proximity Bubble")
	void RefreshProximityBubbleTable();

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

	UFUNCTION(BlueprintPure, Category = "Dialogue|Bubble")
	float GetProximityBubbleDuration() const;

	// 접근 말풍선의 UMG 스타일 키입니다. 데이터에 값이 없으면 현재 DialogueState를 사용합니다.
	UFUNCTION(BlueprintPure, Category = "Dialogue|Bubble")
	FName GetProximityBubbleStyle() const;

	// 근처 접근 말풍선을 표시할 수 있는지 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Dialogue|Bubble")
	bool IsProximityBubbleEnabled() const { return bEnableProximityBubble; }

	// 대화 진행 중 말풍선을 표시할 수 있는지 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Dialogue|Bubble")
	bool IsDialogueBubbleEnabled() const { return bEnableDialogueBubble; }

	// 근처 접근 말풍선 표시 여부를 바꿉니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Bubble")
	void SetProximityBubbleEnabled(bool bNewEnabled);

	// 대화 진행 중 말풍선 표시 여부를 바꿉니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Bubble")
	void SetDialogueBubbleEnabled(bool bNewEnabled);

	// 근처 말풍선과 대화 중 말풍선을 한 번에 켜거나 끕니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Bubble")
	void SetSpeechBubbleEnabled(bool bNewEnabled);

	// 상태가 바뀐 뒤 bCanReplay=false 때문에 새 대화가 막히지 않도록 재생 기록을 초기화합니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Rules")
	void ResetDialoguePlayback();

	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Proximity Bubble", meta = (AdvancedDisplay))
	bool bUseProximityBubbleTable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Proximity Bubble", meta = (AdvancedDisplay, EditCondition = "bUseProximityBubbleTable"))
	TObjectPtr<UDataTable> ProximityBubbleTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Proximity Bubble", meta = (AdvancedDisplay, EditCondition = "bUseProximityBubbleTable"))
	FName ProximityBubbleActorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Proximity Bubble", meta = (AdvancedDisplay, EditCondition = "bUseProximityBubbleTable"))
	FName ProximityBubbleState = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue", meta = (AdvancedDisplay))
	TObjectPtr<UUOUDialogueSequenceData> DialogueSequence = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue", meta = (AdvancedDisplay))
	TArray<FUOUDialogueLine> InlineLines;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FText DefaultSpeakerName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Bubble")
	TObjectPtr<USceneComponent> BubbleAnchor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Bubble")
	FName BubbleAnchorComponentName = NAME_None;

	// 플레이어가 가까이 왔을 때 뜨는 물음표, 느낌표 같은 접근 말풍선을 사용할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Bubble")
	bool bEnableProximityBubble = true;

	// 대화가 진행되는 동안 각 대사에 설정된 말풍선을 사용할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Bubble")
	bool bEnableDialogueBubble = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules")
	bool bCanReplay = true;

	// false면 Speech Bubble은 유지할 수 있지만 카메라 포커스와 대사 UI를 시작할 수 없습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules")
	bool bDialogueAvailable = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules", meta = (ClampMin = "0.0"))
	float StartCooldown = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue|Runtime")
	bool bHasPlayed = false;

private:
	UUOUUISubsystem* GetUISubsystem(AActor* InstigatorActor) const;
	float GetWorldTimeSeconds() const;
	void EnsureDialogueTableCache() const;
	void EnsureProximityBubbleTableCache() const;
	bool ShouldUseDialogueTable() const;
	bool ShouldUseProximityBubbleTable() const;
	FName GetResolvedDialogueActorId() const;
	FName GetResolvedProximityBubbleActorId() const;
	FName GetResolvedProximityBubbleState() const;

	float LastStartTime = -1000.0f;

	mutable TArray<FUOUDialogueLine> CachedTableLines;
	mutable FText CachedTableProximityBubbleText;
	mutable FName CachedTableProximityBubbleStyle = NAME_None;
	mutable FText CachedTableSpeakerName;
	mutable bool bDialogueTableCacheDirty = true;

	mutable FText CachedProximityBubbleText;
	mutable float CachedProximityBubbleDuration = 3.0f;
	mutable FName CachedProximityBubbleStyle = NAME_None;
	mutable bool bProximityBubbleTableCacheDirty = true;
};
