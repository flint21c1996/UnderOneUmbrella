// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "World/Light/UOULightColorReceiverComponent.h"
#include "UOULightPaintColorConditionComponent.generated.h"

// 지속형 PaintTint가 지정한 RGB 상태에 도달하면 만족되는 퍼즐 조건 소스입니다.
// 색이 변하는 액터에 Light Color Receiver와 함께 붙이고 ConditionGroup의 ConditionActor로 연결합니다.
UCLASS(
	ClassGroup = (Puzzle),
	meta = (
		BlueprintSpawnableComponent,
		DisplayName = "UOU Light Paint Color Condition",
		ToolTip = "오브젝트에 남은 PaintTint가 지정한 RGB 색에 도달했는지 퍼즐 조건으로 변환합니다."))
class UNDERONEUMBRELLA_API UUOULightPaintColorConditionComponent
	: public UUOUPuzzleConditionSourceComponent
{
	GENERATED_BODY()

public:
	UUOULightPaintColorConditionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual void GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light|Paint", meta = (ToolTip = "명시적인 참조가 없으면 소유 액터의 UOU Light Color Receiver를 자동으로 찾습니다."))
	bool bAutoFindColorReceiver = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light|Paint", meta = (UseComponentPicker, AllowedClasses = "/Script/UnderOneUmBrella.UOULightColorReceiverComponent", DisplayName = "Color Receiver", ToolTip = "조건을 판단할 UOU Light Color Receiver입니다. 실제로 보이는 Current Paint Tint를 읽습니다."))
	FComponentReference ColorReceiverReference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light|Paint", meta = (InvalidEnumValues = "None", ToolTip = "조건을 만족시킬 RGB 물감 상태입니다. RGB는 흰색 PaintTint를 뜻합니다."))
	EUOULightColorState RequiredColorState = EUOULightColorState::Red;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light|Paint", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "불만족 상태에서 목표색에 이 값 이내로 가까워지면 만족합니다."))
	float MatchTolerance = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light|Paint", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "만족 상태에서 목표색과 이 값보다 멀어지면 해제합니다. Match Tolerance보다 작으면 자동으로 같은 값까지 올립니다."))
	float ReleaseTolerance = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Light|Paint", meta = (ToolTip = "켜면 목표색에 한 번 도달한 뒤 다른 색으로 변해도 조건을 만족 상태로 유지합니다."))
	bool bLatchOnceSatisfied = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime", meta = (ToolTip = "런타임에 실제로 연결된 색상 수신 컴포넌트입니다."))
	TObjectPtr<UUOULightColorReceiverComponent> ColorReceiver = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime", meta = (ToolTip = "마지막 조건 판정에 사용한 실제 PaintTint입니다."))
	FLinearColor ObservedPaintTint = FLinearColor::White;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime", meta = (ToolTip = "Required Color State와 수신체 설정으로 계산한 목표 PaintTint입니다."))
	FLinearColor RequiredPaintTint = FLinearColor::White;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Light|Paint", meta = (ToolTip = "참조를 다시 찾고 현재 PaintTint로 조건을 즉시 재평가합니다."))
	void RefreshNow();

protected:
	UFUNCTION()
	void HandlePaintTintChanged(FLinearColor NewPaintTint);

	void ResolveColorReceiver();
	void SubscribeColorReceiver();
	void UnsubscribeColorReceiver();
	void RefreshSatisfiedState();
	bool HasColorReceiverReference() const;
	bool IsWithinTolerance(const FLinearColor& Current, const FLinearColor& Required, float Tolerance) const;
};
