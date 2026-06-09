// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "UOUInteractionHintComponent.generated.h"

class UUserWidget;

// 플레이어가 가까이 왔을 때 액터 위의 힌트 위젯을 켜고 끄는 감지 컴포넌트입니다.
// 말풍선, 물음표, 상호작용 키 안내처럼 월드 액터 위에 잠깐 떠야 하는 UI에 사용합니다.
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent, DisplayName="UOU Interaction Hint"))
class UNDERONEUMBRELLA_API UUOUInteractionHintComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UUOUInteractionHintComponent();

	virtual void BeginPlay() override;

	// 현재 연결된 힌트 위젯을 표시합니다.
	UFUNCTION(BlueprintCallable, Category = "Interaction Hint")
	void ShowHint();

	// 현재 연결된 힌트 위젯을 숨깁니다.
	UFUNCTION(BlueprintCallable, Category = "Interaction Hint")
	void HideHint();

	// 소유 액터에서 힌트 위젯 컴포넌트를 다시 찾아 캐시합니다.
	UFUNCTION(BlueprintCallable, Category = "Interaction Hint")
	UWidgetComponent* ResolveHintWidgetComponent();

	// 직접 지정할 힌트 위젯 컴포넌트입니다. 비어 있으면 이름 또는 첫 번째 WidgetComponent로 자동 탐색합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction Hint|Widget")
	TObjectPtr<UWidgetComponent> HintWidgetComponent = nullptr;

	// 직접 지정하지 않았을 때 소유 액터 안에서 찾을 WidgetComponent 이름입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction Hint|Widget")
	FName HintWidgetComponentName = TEXT("SpeechBubbleWidget");

	// 이름으로 찾지 못했을 때 첫 번째 WidgetComponent를 자동으로 사용할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction Hint|Widget")
	bool bAutoFindFirstWidgetComponent = true;

	// 힌트 위젯에 전달할 텍스트입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction Hint|Display")
	FText HintText;

	// ShowBubble 함수에 넘길 표시 시간입니다. 0보다 작으면 매우 긴 시간으로 처리합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction Hint|Display", meta = (ClampMin = "-1.0"))
	float HintDuration = -1.0f;

	// 위젯 블루프린트에서 힌트를 표시하는 함수 이름입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction Hint|Display")
	FName ShowFunctionName = TEXT("ShowBubble");

	// 위젯 블루프린트에서 힌트를 숨기는 함수 이름입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction Hint|Display")
	FName HideFunctionName = TEXT("HideBubble");

	// true면 플레이어 폰만 힌트를 켤 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction Hint|Rules")
	bool bOnlyPlayerPawn = true;

	// true면 Pawn 계열 액터만 힌트를 켤 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction Hint|Rules")
	bool bOnlyPawn = true;

	// BeginPlay 때 힌트 위젯 컴포넌트를 숨겨서 처음에는 보이지 않게 합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction Hint|Rules")
	bool bHideHintOnBeginPlay = true;

	// true면 WidgetComponent 자체의 표시 상태도 같이 제어합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction Hint|Rules")
	bool bControlWidgetComponentVisibility = true;

	// 현재 힌트가 표시 중인지 확인하기 위한 런타임 상태입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction Hint|Runtime")
	bool bHintVisible = false;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	bool PassesOverlapRules(AActor* OtherActor) const;
	void SetWidgetComponentVisible(bool bNewVisible) const;
	void CallWidgetShowFunction(UUserWidget* UserWidget) const;
	void CallWidgetHideFunction(UUserWidget* UserWidget) const;
	UUserWidget* GetHintUserWidget();
};
