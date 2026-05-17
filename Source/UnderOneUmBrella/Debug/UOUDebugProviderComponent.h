// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Debug/UOUDebugProvider.h"
#include "UOUDebugProviderComponent.generated.h"

// 단순한 액터 디버그 정보는 이 컴포넌트를 붙여 통합 디버그 시스템에 등록할 수 있습니다.
UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Debug Provider"))
class UNDERONEUMBRELLA_API UUOUDebugProviderComponent : public UActorComponent, public IUOUDebugProvider
{
	GENERATED_BODY()

public:
	UUOUDebugProviderComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그", meta = (ToolTip = "이 Provider가 디버그 정보를 제공할지 결정합니다."))
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그", meta = (ToolTip = "이 Provider가 속한 디버그 카테고리입니다."))
	EUOUDebugCategory DebugCategory = EUOUDebugCategory::System;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그", meta = (ToolTip = "월드 라벨에 표시할 이름입니다. 비워두면 Owner 이름을 사용합니다."))
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그", meta = (MultiLine = "true", ToolTip = "월드 라벨이나 상세 UI에 표시할 짧은 요약입니다."))
	FText SummaryText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그", meta = (ToolTip = "Owner 위치 기준 디버그 라벨이 표시될 오프셋입니다."))
	FVector WorldLocationOffset = FVector(0.0f, 0.0f, 120.0f);

	virtual EUOUDebugCategory GetDebugCategory_Implementation() const override;
	virtual bool IsDebugProviderEnabled_Implementation() const override;
	virtual FText GetDebugDisplayName_Implementation() const override;
	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual FVector GetDebugWorldLocation_Implementation() const override;
};

