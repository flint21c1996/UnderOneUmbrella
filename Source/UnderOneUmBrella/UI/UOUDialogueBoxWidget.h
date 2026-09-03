// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "UI/UOUUITypes.h"
#include "UOUDialogueBoxWidget.generated.h"

class UBorder;
class UButton;
class USizeBox;
class UTextBlock;
class UWidget;
class UUOUUISubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUOUDialogueBoxAdvanceRequestedSignature);

// 화면 아래에 표시되는 대화 박스 위젯의 C++ 기반 클래스입니다.
// 실제 배경, 테두리, 애니메이션은 이 클래스를 부모로 둔 WBP에서 꾸미고, 이 클래스는 대사 데이터 연결과 진행 요청만 담당합니다.
UCLASS(Blueprintable)
class UNDERONEUMBRELLA_API UUOUDialogueBoxWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 대화 한 줄을 화면에 표시합니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue Box")
	void ShowDialogueLine(AActor* SpeakerActor, const FUOUDialogueLine& Line);

	// 대화 박스를 숨기고 현재 표시 중인 내용을 비웁니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue Box")
	void HideDialogueBox();

	// 버튼이나 위젯 클릭으로 다음 대사를 요청합니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue Box")
	void RequestAdvanceDialogue();

	// 현재 대화 박스가 보이는 상태인지 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Dialogue Box")
	bool IsDialogueBoxVisible() const { return bDialogueBoxVisible; }

	// 디자이너에서 연결한 9-Slice 배경과 콘텐츠 여백을 다시 적용합니다.
	// DialogueSizeBox의 크기 제한은 디자이너에서 설정한 값을 그대로 사용합니다.
	// 런타임에 스타일 값을 바꾼 뒤 호출할 수도 있습니다.
	UFUNCTION(BlueprintCallable, Category = "Dialogue Box|Adaptive Layout")
	void RefreshAdaptiveLayout();

	UPROPERTY(BlueprintAssignable, Category = "Dialogue Box")
	FUOUDialogueBoxAdvanceRequestedSignature OnAdvanceRequested;

	// WBP에서 대화 박스 전체 루트로 연결할 위젯입니다. 비어 있으면 이 위젯 전체의 Visibility를 제어합니다.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Dialogue Box|Bind")
	TObjectPtr<UWidget> DialogueRoot = nullptr;

	// WBP에서 화자 이름 TextBlock으로 연결할 위젯입니다.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Dialogue Box|Bind")
	TObjectPtr<UTextBlock> SpeakerNameText = nullptr;

	// WBP에서 대사 본문 TextBlock으로 연결할 위젯입니다.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Dialogue Box|Bind")
	TObjectPtr<UTextBlock> DialogueText = nullptr;

	// WBP에서 9-Slice 배경으로 사용할 Border입니다. 위젯 이름을 DialoguePanel로 맞추면 자동 연결됩니다.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Dialogue Box|Bind")
	TObjectPtr<UBorder> DialoguePanel = nullptr;

	// 대화창의 크기를 디자이너에서 설정할 SizeBox입니다. 위젯 이름을 DialogueSizeBox로 맞추면 자동 연결됩니다.
	// C++에서는 이 SizeBox의 너비/높이 오버라이드를 변경하지 않습니다.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Dialogue Box|Bind")
	TObjectPtr<USizeBox> DialogueSizeBox = nullptr;

	// WBP에서 다음 대사 버튼으로 연결할 위젯입니다.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Dialogue Box|Bind")
	TObjectPtr<UButton> AdvanceButton = nullptr;

	// 대화 박스가 표시될 때 사용할 Visibility입니다. UI 클릭을 막고 싶으면 Not Hit-Testable 계열로 BP에서 조정하면 됩니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue Box|Display")
	ESlateVisibility ShownVisibility = ESlateVisibility::SelfHitTestInvisible;

	// 대화 박스를 숨길 때 사용할 Visibility입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue Box|Display")
	ESlateVisibility HiddenVisibility = ESlateVisibility::Collapsed;

	// 버튼 클릭 시 UISubsystem에 다음 대사를 바로 요청할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue Box|Input")
	bool bAdvanceDialogueOnButtonClick = true;

	// 켜면 NineSliceBrush를 DialoguePanel에 적용합니다. 끄면 디자이너에서 Border에 지정한 Brush를 그대로 사용합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Box|Adaptive Layout")
	bool bApplyNineSliceBrush = false;

	// Draw As는 실행 시 Box로 강제되어 모서리는 유지하고 중앙과 변만 늘어납니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Box|Adaptive Layout", meta = (EditCondition = "bApplyNineSliceBrush"))
	FSlateBrush NineSliceBrush;

	// 테두리와 실제 대사 내용 사이의 여백입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Box|Adaptive Layout")
	FMargin ContentPadding = FMargin(32.0f, 24.0f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue Box|Runtime")
	TObjectPtr<AActor> CurrentSpeakerActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue Box|Runtime")
	FUOUDialogueLine CurrentLine;

protected:
	// BP에서 등장 애니메이션, 이름판 갱신, 효과음 등을 추가하기 위한 훅입니다.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Dialogue Box")
	void BP_OnDialogueLineShown(AActor* SpeakerActor, const FUOUDialogueLine& Line);

	// BP에서 퇴장 애니메이션, 잔상 정리 등을 추가하기 위한 훅입니다.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Dialogue Box")
	void BP_OnDialogueBoxHidden();

private:
	UFUNCTION()
	void HandleAdvanceButtonClicked();

	void SetDialogueBoxVisible(bool bNewVisible);
	UUOUUISubsystem* GetUISubsystem() const;

	bool bDialogueBoxVisible = false;
};
