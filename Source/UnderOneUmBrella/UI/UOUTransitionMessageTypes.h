// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UOUTransitionMessageTypes.generated.h"

USTRUCT(BlueprintType)
struct FUOUTransitionMessageSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition Message", meta = (DisplayName = "전환 문구", MultiLine = "true", ToolTip = "화면 전환 중 중앙에 표시할 문구입니다. 비워두면 표시하지 않습니다."))
	FText MessageText = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition Message", meta = (ClampMin = "1", DisplayName = "글자 크기", ToolTip = "전환 문구의 글자 크기입니다."))
	int32 FontSize = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition Message", meta = (DisplayName = "글자 색상", ToolTip = "전환 문구의 글자 색상입니다."))
	FLinearColor TextColor = FLinearColor(0.92f, 0.92f, 0.86f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition Message", meta = (ClampMin = "100.0", DisplayName = "줄바꿈 너비", ToolTip = "문구가 이 너비를 넘으면 자동으로 줄바꿈합니다."))
	float WrapTextAt = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition Message", meta = (ClampMin = "0", DisplayName = "표시 우선순위", ToolTip = "Viewport에 추가할 때 사용할 ZOrder입니다. 기존 HUD보다 위에 보이도록 충분히 큰 값을 사용합니다."))
	int32 ViewportZOrder = 10000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition Message|Timing", meta = (ClampMin = "0.0", DisplayName = "문구 페이드 인 시간", ToolTip = "검은 화면 위에서 문구가 서서히 나타나는 시간입니다."))
	float MessageFadeInDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition Message|Timing", meta = (ClampMin = "0.0", DisplayName = "문구 유지 시간", ToolTip = "문구가 완전히 보이는 상태로 유지되는 시간입니다."))
	float MessageHoldDuration = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition Message|Timing", meta = (ClampMin = "0.0", DisplayName = "문구 페이드 아웃 시간", ToolTip = "문구가 사라지는 시간입니다."))
	float MessageFadeOutDuration = 0.25f;

	bool ShouldDisplay() const
	{
		return !MessageText.IsEmpty();
	}

	float GetTotalDisplayDuration() const
	{
		return ShouldDisplay()
			? FMath::Max(0.0f, MessageFadeInDuration)
				+ FMath::Max(0.0f, MessageHoldDuration)
				+ FMath::Max(0.0f, MessageFadeOutDuration)
			: 0.0f;
	}
};
