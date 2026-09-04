// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/UOUStageSelectTypes.h"
#include "Styling/SlateBrush.h"
#include "UOUStageSelectPopupWidget.generated.h"

class UImage;
class UTextBlock;

/**
 * 스테이지 선택 팝업에 표시할 데이터를 전달하는 C++ 경계입니다.
 * 레이아웃과 연출은 Blueprint가 담당하고, 이 클래스는 적용된 스테이지 데이터를 보관합니다.
 */
UCLASS(Blueprintable)
class UNDERONEUMBRELLA_API UUOUStageSelectPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 선택된 스테이지 데이터를 저장하고 C++/Blueprint 표시를 갱신합니다. */
	UFUNCTION(BlueprintCallable, Category = "Stage Select")
	void SetStageData(const FUOUStageDefinition& InStageData);

	UFUNCTION(BlueprintPure, Category = "Stage Select")
	const FUOUStageDefinition& GetStageData() const { return StageData; }

	UFUNCTION(BlueprintPure, Category = "Stage Select")
	bool HasStageData() const { return bHasStageData; }

protected:
	virtual void NativeConstruct() override;

	/** 가장 최근에 적용된 스테이지 데이터입니다. 레이아웃과 애니메이션은 WBP가 담당합니다. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Stage Select")
	FUOUStageDefinition StageData;

	/** WBP에 같은 이름의 TextBlock이 있으면 별 획득/전체/부족 수량을 표시합니다. */
	UPROPERTY(Transient, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RewardProgressText;

	/** WBP에 같은 이름의 Image가 있으면 Reward 수집 완료 여부에 맞는 Brush를 표시합니다. */
	UPROPERTY(Transient, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> RewardStatusImage;

	/** 아직 수집하지 못한 Reward가 하나라도 있을 때 표시할 Brush입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Select|Rewards")
	FSlateBrush EmptyRewardBrush;

	/** 모든 Reward를 수집하여 MissingRewardCount가 0일 때 표시할 Brush입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Select|Rewards")
	FSlateBrush CompletedRewardBrush;

	/** WBP에서 구현하여 텍스트, 이미지, 애니메이션 등 Blueprint 표현을 갱신합니다. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Stage Select", meta = (DisplayName = "On Stage Data Changed"))
	void BP_OnStageDataChanged(const FUOUStageDefinition& NewStageData);

private:
	/** 현재 StageData의 보상 수량을 RewardProgressText에 반영합니다. */
	void RefreshRewardProgressText();

	/** MissingRewardCount에 따라 RewardStatusImage의 Brush를 갱신합니다. */
	void RefreshRewardStatusImage();

	UPROPERTY(Transient)
	bool bHasStageData = false;
};
