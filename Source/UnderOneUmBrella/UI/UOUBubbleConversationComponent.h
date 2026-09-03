// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Puzzle/Core/UOUPuzzleResultCompletionState.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UI/UOUBubbleConversationData.h"
#include "UOUBubbleConversationComponent.generated.h"

class UUOUUISubsystem;
class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUOUBubbleConversationStartedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FUOUBubbleConversationLineStartedSignature,
	int32,
	LineIndex,
	FName,
	SpeakerId,
	AActor*,
	SpeakerActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FUOUBubbleConversationFinishedSignature,
	bool,
	bCompletedNaturally);

// ConditionGroup 결과를 받아 여러 NPC의 SpeechBubbleWidget을 한 줄씩 순서대로 재생합니다.
// 짧은 대화는 InlineLines에 직접 작성하고, 재사용할 대화는 ConversationData로 분리할 수 있습니다.
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent, DisplayName="UOU Bubble Conversation"))
class UNDERONEUMBRELLA_API UUOUBubbleConversationComponent
	: public UActorComponent
	, public IUOUPuzzleResultReceiver
	, public IUOUPuzzleResultCompletionState
{
	GENERATED_BODY()

public:
	UUOUBubbleConversationComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 대사에서 사용하는 SpeakerId와 실제 월드 NPC를 연결합니다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Bubble Conversation|Participants", meta = (
		DisplayName = "대화 참가자",
		ToolTip = "대화에 참여할 NPC들을 등록합니다. 참가자 ID와 실제 월드 NPC를 연결한 뒤, Inline Lines의 화자 ID에 같은 값을 사용하세요."))
	TArray<FUOUBubbleConversationParticipant> Participants;

	// 지정되어 있고 유효한 줄이 있으면 InlineLines보다 우선 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bubble Conversation|Content", meta = (
		DisplayName = "대화 데이터 에셋 (선택)",
		ToolTip = "재사용할 UOU Bubble Conversation Data를 지정합니다. 유효한 줄이 들어 있는 에셋을 지정하면 아래 인라인 대사보다 우선합니다. 짧은 맵 전용 대화라면 비워두세요."))
	TObjectPtr<UUOUBubbleConversationData> ConversationData = nullptr;

	// 특정 레벨에서만 사용하는 짧은 대사는 여기에 바로 작성할 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bubble Conversation|Content", meta = (
		DisplayName = "인라인 대사",
		ToolTip = "이 컴포넌트에서 직접 작성하는 대사 목록입니다. 배열 순서대로 재생됩니다. 대화 데이터 에셋이 지정되어 있으면 에셋의 대사가 우선합니다."))
	TArray<FUOUBubbleConversationLine> InlineLines;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bubble Conversation|Playback", meta = (
		DisplayName = "게임 시작 시 자동 재생",
		ToolTip = "체크하면 BeginPlay에서 자동으로 대화를 시작합니다. ConditionGroup 결과로 실행할 때는 보통 체크하지 않습니다."))
	bool bStartOnBeginPlay = false;

	// 재생 중 Activate가 다시 들어왔을 때 처음부터 다시 시작할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bubble Conversation|Playback", meta = (
		DisplayName = "재생 중 재활성화 시 처음부터",
		ToolTip = "대화가 진행 중일 때 Activate가 다시 들어오면 현재 대화를 취소하고 첫 줄부터 다시 시작합니다. 체크하지 않으면 중복 Activate를 무시합니다."))
	bool bRestartWhenActivated = false;

	// 다음 화자로 넘어갈 때 이전 화자의 말풍선을 즉시 정리합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bubble Conversation|Playback", meta = (
		DisplayName = "다음 대사 전 이전 말풍선 숨김",
		ToolTip = "다음 줄로 넘어갈 때 이전 화자의 말풍선을 즉시 정리합니다. 두 NPC의 말풍선이 겹치지 않게 하려면 체크하세요."))
	bool bHidePreviousBubbleWhenAdvancing = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bubble Conversation|Runtime", meta = (
		DisplayName = "재생 중",
		ToolTip = "현재 대화가 시작되어 아직 마지막 줄까지 끝나지 않았는지 보여주는 런타임 값입니다."))
	bool bIsPlaying = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bubble Conversation|Runtime", meta = (
		DisplayName = "일시정지 중",
		ToolTip = "Pause 액션으로 대화가 일시정지된 상태인지 보여주는 런타임 값입니다."))
	bool bIsPaused = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bubble Conversation|Runtime", meta = (
		DisplayName = "현재 대사 인덱스",
		ToolTip = "현재 재생 중이거나 대기 중인 대사 번호입니다. 재생 중이 아니면 -1입니다."))
	int32 CurrentLineIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bubble Conversation|Runtime", meta = (
		DisplayName = "대화 완료됨",
		ToolTip = "마지막 대사까지 정상 재생되어 Activate 결과가 완료됐는지 나타냅니다. 다른 ConditionGroup에서 완료 조건으로 사용할 수 있습니다."))
	bool bHasCompletedResultSinceLastActivation = false;

	UPROPERTY(BlueprintAssignable, Category = "Bubble Conversation|Events")
	FUOUBubbleConversationStartedSignature OnConversationStarted;

	UPROPERTY(BlueprintAssignable, Category = "Bubble Conversation|Events")
	FUOUBubbleConversationLineStartedSignature OnConversationLineStarted;

	UPROPERTY(BlueprintAssignable, Category = "Bubble Conversation|Events")
	FUOUBubbleConversationFinishedSignature OnConversationFinished;

	UFUNCTION(BlueprintCallable, Category = "Bubble Conversation")
	bool StartConversation();

	UFUNCTION(BlueprintCallable, Category = "Bubble Conversation")
	void StopConversation();

	UFUNCTION(BlueprintCallable, Category = "Bubble Conversation")
	void PauseConversation();

	UFUNCTION(BlueprintCallable, Category = "Bubble Conversation")
	void ResumeConversation();

	UFUNCTION(BlueprintPure, Category = "Bubble Conversation")
	int32 GetConversationLineCount() const;

	UFUNCTION(BlueprintPure, Category = "Bubble Conversation")
	AActor* ResolveSpeakerActor(FName SpeakerId) const;

	UFUNCTION(BlueprintPure, Category = "Bubble Conversation")
	UWidgetComponent* ResolveSpeakerBubbleWidgetComponent(FName SpeakerId) const;

	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;
	virtual bool IsPuzzleResultCompleted_Implementation(EOUUPuzzleResultAction Action) const override;
	virtual FOnUOUPuzzleResultCompletionStateChangedNativeSignature* GetPuzzleResultCompletionStateChangedEvent() override;

private:
	enum class EPlaybackPhase : uint8
	{
		None,
		WaitingBeforeLine,
		ShowingLine,
		WaitingAfterLine
	};

	const TArray<FUOUBubbleConversationLine>& GetResolvedLines() const;
	int32 FindNextPlayableLineIndex(int32 StartIndex) const;
	bool IsLinePlayable(const FUOUBubbleConversationLine& Line) const;
	UUOUUISubsystem* GetUISubsystem() const;
	void AdvanceToNextLine();
	void ShowCurrentLine();
	void BeginWaitingAfterLine();
	void ScheduleShowCurrentLine(float DelaySeconds);
	void ScheduleBeginWaitingAfterLine(float DelaySeconds);
	void ScheduleAdvance(float DelaySeconds);
	void FinishConversation(bool bCompletedNaturally, bool bBroadcastFinished = true);
	void HideCurrentSpeakerBubble(bool bImmediately) const;
	void SetActivationResultCompleted(bool bNewCompleted);
	void SetDeactivationResultCompleted(bool bNewCompleted);

	FTimerHandle PlaybackTimerHandle;
	TWeakObjectPtr<AActor> CurrentSpeakerActor;
	TWeakObjectPtr<UWidgetComponent> CurrentSpeechBubbleWidgetComponent;
	EPlaybackPhase PlaybackPhase = EPlaybackPhase::None;
	float PausedPhaseRemainingSeconds = 0.0f;
	bool bActivated = false;
	bool bHasCompletedResultSinceLastDeactivation = false;

	FOnUOUPuzzleResultCompletionStateChangedNativeSignature OnPuzzleResultCompletionStateChanged;
};
