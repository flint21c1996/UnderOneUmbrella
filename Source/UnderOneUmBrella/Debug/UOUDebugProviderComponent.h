// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Debug/UOUDebugProvider.h"
#include "UOUDebugProviderComponent.generated.h"

// UOU 디버그 시스템에 참여할 액터에 붙이는 컴포넌트 기반 provider입니다.
UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Debug Provider"))
class UNDERONEUMBRELLA_API UUOUDebugProviderComponent : public UActorComponent, public IUOUDebugProvider
{
	GENERATED_BODY()

public:
	UUOUDebugProviderComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (ToolTip = "이 provider가 디버그 정보를 제공할지 결정합니다."))
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (ToolTip = "레벨 디버그 컨트롤러가 제어하는 디버그 카테고리입니다."))
	EUOUDebugCategory DebugCategory = EUOUDebugCategory::System;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (ToolTip = "디버그 라벨에 표시할 선택 이름입니다. 비어 있으면 소유자 이름을 사용합니다."))
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (MultiLine = "true", ToolTip = "디버그 라벨에 표시할 선택 요약 텍스트입니다."))
	FText SummaryText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (ToolTip = "이 provider의 디버그 라벨에 사용할 월드 위치 오프셋입니다."))
	FVector WorldLocationOffset = FVector(0.0f, 0.0f, 120.0f);

	virtual EUOUDebugCategory GetDebugCategory_Implementation() const override;
	virtual bool IsDebugProviderEnabled_Implementation() const override;
	virtual FText GetDebugDisplayName_Implementation() const override;
	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual FVector GetDebugWorldLocation_Implementation() const override;
	virtual void GetDebugConnections_Implementation(TArray<FUOUDebugConnection>& OutConnections) const override;
};
