// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/UOUUITypes.h"
#include "UOUDialogueBoxWidget.generated.h"

class UButton;
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
