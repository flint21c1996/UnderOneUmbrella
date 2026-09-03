// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UOUSpeechBubbleWidget.generated.h"

class UWidget;

// 액터 위에 뜨는 월드 말풍선 위젯의 C++ 부모 클래스입니다.
// BP 안에서 변수 사용을 켜지 않아도 이름으로 텍스트 위젯을 찾아 값을 넣습니다.
UCLASS(Blueprintable)
class UNDERONEUMBRELLA_API UUOUSpeechBubbleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 말풍선 문구를 바꾸고 서서히 나타나게 한 뒤, Duration 뒤에 자동으로 사라지게 합니다.
	// Duration이 0보다 작으면 자동 숨김 타이머를 걸지 않습니다.
	UFUNCTION(BlueprintCallable, Category = "Speech Bubble")
	void ShowBubble(FText BubbleText, double Duration);

	// 같은 말풍선 WBP에서 퍼즐 전/후의 색상과 애니메이션을 다르게 적용할 수 있는 표시 함수입니다.
	UFUNCTION(BlueprintCallable, Category = "Speech Bubble")
	void ShowBubbleStyled(FText BubbleText, double Duration, FName PresentationStyle);

	// 현재 말풍선을 FadeOutDuration 동안 서서히 사라지게 합니다.
	UFUNCTION(BlueprintCallable, Category = "Speech Bubble")
	void HideBubble();

	// 페이드 없이 바로 숨깁니다. BeginPlay 초기화나 강제 정리에 사용합니다.
	UFUNCTION(BlueprintCallable, Category = "Speech Bubble")
	void HideBubbleImmediately();

	// 말풍선 텍스트의 폰트 크기를 런타임에 변경합니다.
	// TXT_BubbleText가 없으면 위젯 트리에서 처음 발견한 지원 텍스트 위젯에 적용합니다.
	UFUNCTION(BlueprintCallable, Category = "Speech Bubble|Display")
	bool SetBubbleFontSize(int32 NewFontSize);

	UFUNCTION(BlueprintPure, Category = "Speech Bubble")
	FName GetCurrentPresentationStyle() const { return CurrentPresentationStyle; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Speech Bubble|Style")
	FName CurrentPresentationStyle = NAME_None;

protected:
	// BP에서 Style 이름에 맞춰 브러시, 색, 등장 애니메이션을 교체합니다.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Speech Bubble|Style")
	void BP_OnPresentationStyleChanged(FName PresentationStyle);

	// 보일 때 BubbleRoot에 적용할 Visibility입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speech Bubble|Display")
	ESlateVisibility ShownVisibility = ESlateVisibility::SelfHitTestInvisible;

	// 숨길 때 BubbleRoot에 적용할 Visibility입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speech Bubble|Display")
	ESlateVisibility HiddenVisibility = ESlateVisibility::Collapsed;

	// 알파값이 0에서 1까지 올라가는 시간입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speech Bubble|Fade", meta = (ClampMin = "0.0"))
	float FadeInDuration = 0.18f;

	// 알파값이 1에서 0까지 내려가는 시간입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speech Bubble|Fade", meta = (ClampMin = "0.0"))
	float FadeOutDuration = 0.18f;

private:
	void ApplyPresentationStyle(FName NewPresentationStyle);
	enum class EFadeState : uint8
	{
		Hidden,
		FadingIn,
		Visible,
		FadingOut
	};

	void StartFadeOut();
	void SetBubbleVisible(bool bNewVisible);
	void SetBubbleOpacity(float NewOpacity);
	bool SetBubbleText(const FText& BubbleText) const;
	UWidget* ResolveBubbleTextWidget() const;
	UWidget* ResolveBubbleRootWidget() const;
	UWorld* GetTimerWorld() const;

	FTimerHandle AutoHideTimerHandle;
	EFadeState FadeState = EFadeState::Hidden;
	float FadeElapsedTime = 0.0f;
};
