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

	// 현재 말풍선을 FadeOutDuration 동안 서서히 사라지게 합니다.
	UFUNCTION(BlueprintCallable, Category = "Speech Bubble")
	void HideBubble();

	// 페이드 없이 바로 숨깁니다. BeginPlay 초기화나 강제 정리에 사용합니다.
	UFUNCTION(BlueprintCallable, Category = "Speech Bubble")
	void HideBubbleImmediately();

protected:
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
	UWidget* ResolveBubbleRootWidget() const;
	UWorld* GetTimerWorld() const;

	FTimerHandle AutoHideTimerHandle;
	EFadeState FadeState = EFadeState::Hidden;
	float FadeElapsedTime = 0.0f;
};
