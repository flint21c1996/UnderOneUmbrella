// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "UOUBubbleConversationData.generated.h"

// 월드에 배치된 화자를 대화 데이터의 SpeakerId와 연결합니다.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUBubbleConversationParticipant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bubble Conversation", meta = (
		DisplayName = "참가자 ID",
		ToolTip = "이 NPC를 대사에서 구분할 이름입니다. 예: NPC_A, TicketSeller. Conversation Lines의 화자 ID와 철자까지 같아야 합니다."))
	FName ParticipantId = NAME_None;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Bubble Conversation", meta = (
		DisplayName = "화자 액터",
		ToolTip = "이 참가자 ID에 연결할 월드의 실제 NPC입니다. 아래 말풍선 컴포넌트를 직접 지정하면 그 컴포넌트의 소유 액터를 화자로 사용하므로 비워도 됩니다."))
	TObjectPtr<AActor> SpeakerActor = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Bubble Conversation", meta = (
		DisplayName = "말풍선 위젯 컴포넌트",
		UseComponentPicker,
		AllowAnyActor,
		AllowedClasses = "/Script/UMG.WidgetComponent",
		ToolTip = "월드에서 화자의 말풍선 Widget Component를 직접 선택합니다. 이 참조 안에 대상 액터와 컴포넌트가 함께 저장됩니다. 비워 두면 위 화자 액터에서 SpeechBubbleWidget 또는 이름에 Bubble이 포함된 컴포넌트를 자동으로 찾습니다."))
	FComponentReference SpeechBubbleWidgetReference;
};

// 두 명 이상의 월드 액터가 주고받는 말풍선 대화의 한 줄입니다.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUBubbleConversationLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bubble Conversation", meta = (
		DisplayName = "대사 ID (선택)",
		ToolTip = "대사를 사람이 구분하기 위한 선택 항목입니다. 비워도 재생되며, 예: NoticeNoise, AskQuestion처럼 적을 수 있습니다."))
	FName LineId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bubble Conversation", meta = (
		DisplayName = "화자 ID",
		ToolTip = "이 줄을 말할 NPC의 참가자 ID입니다. 위 Participants에 등록한 참가자 ID와 철자까지 동일하게 입력하세요."))
	FName SpeakerId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bubble Conversation", meta = (
		DisplayName = "말풍선 대사",
		MultiLine = true,
		ToolTip = "NPC 머리 위 말풍선에 표시할 문장입니다. 비어 있으면 이 줄은 건너뜁니다. 줄바꿈은 \\n으로 입력할 수 있습니다."))
	FText BubbleText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bubble Conversation", meta = (
		ClampMin = "0",
		DisplayName = "폰트 크기 (선택)",
		ToolTip = "이 대사에 적용할 글자 크기입니다. 1 이상이면 말풍선 위젯의 폰트 크기를 덮어쓰고, 0이면 위젯에 설정된 현재 크기를 그대로 사용합니다."))
	int32 FontSizeOverride = 0;

	// 이 줄을 표시하기 전에 기다릴 시간입니다. NPC 사이의 호흡을 조절할 때 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bubble Conversation", meta = (
		ClampMin = "0.0",
		DisplayName = "대사 전 대기 시간",
		ForceUnits = "s",
		ToolTip = "바로 앞 대사가 끝난 뒤, 이 말풍선을 띄우기 전까지 기다릴 시간(초)입니다. NPC 사이의 말 간격을 조절합니다."))
	float DelayBefore = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bubble Conversation", meta = (
		ClampMin = "0.01",
		DisplayName = "말풍선 표시 시간",
		ForceUnits = "s",
		ToolTip = "이 말풍선이 화면에 유지되는 시간(초)입니다. 시간이 끝나면 다음 대사로 넘어갑니다."))
	float BubbleDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bubble Conversation", meta = (
		ClampMin = "0.0",
		DisplayName = "대사 후 대기 시간",
		ForceUnits = "s",
		ToolTip = "말풍선 표시 시간이 끝난 뒤 말풍선의 페이드 아웃을 시작하고, 다음 대사로 넘어가기 전에 기다릴 시간(초)입니다. 페이드 아웃이 잘리지 않게 하려면 말풍선 위젯의 Fade Out Duration 이상으로 지정하세요."))
	float DelayAfter = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bubble Conversation", meta = (
		DisplayName = "표현 스타일 (선택)",
		ToolTip = "말풍선의 색상이나 애니메이션을 구분하는 선택 키입니다. WBP_NPCSpeechBubble에서 같은 이름의 스타일을 처리할 때만 사용하며, 기본 모양이면 None으로 둡니다."))
	FName PresentationStyle = NAME_None;
};

// 여러 레벨에서 재사용할 수 있는 말풍선 대화 순서입니다.
// 실제 월드 NPC는 이 에셋에 저장하지 않고 Conversation Component의 Participants에서 연결합니다.
UCLASS(BlueprintType)
class UNDERONEUMBRELLA_API UUOUBubbleConversationData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bubble Conversation", meta = (
		DisplayName = "대화 줄 목록",
		ToolTip = "여러 NPC가 말할 순서대로 줄을 추가합니다. 각 줄의 화자 ID는 컴포넌트의 Participants에 등록한 ID와 일치해야 합니다."))
	TArray<FUOUBubbleConversationLine> Lines;

	UFUNCTION(BlueprintPure, Category = "Bubble Conversation")
	bool HasValidLines() const;
};
