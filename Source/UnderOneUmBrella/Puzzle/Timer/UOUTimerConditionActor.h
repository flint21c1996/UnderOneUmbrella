// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Debug/UOUDebugProvider.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUTimerConditionActor.generated.h"

class USceneComponent;
class UUOUTimerConditionComponent;

// 지정한 시간이 지난 뒤 조건을 만족시키는 배치용 액터입니다.
UCLASS(meta=(DisplayName="UOU Timer Condition Actor"))
class UNDERONEUMBRELLA_API AUOUTimerConditionActor : public AActor, public IUOUPuzzleResultReceiver, public IUOUDebugProvider
{
	GENERATED_BODY()

public:
	AUOUTimerConditionActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;
	virtual EUOUDebugCategory GetDebugCategory_Implementation() const override;
	virtual bool IsDebugProviderEnabled_Implementation() const override;
	virtual FText GetDebugDisplayName_Implementation() const override;
	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual FVector GetDebugWorldLocation_Implementation() const override;
	virtual void GetDebugConnections_Implementation(TArray<FUOUDebugConnection>& OutConnections) const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UUOUTimerConditionComponent> TimerConditionComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "켜져 있으면 Puzzle 디버그 월드 라벨에 타이머 상태와 남은 시간을 표시합니다."))
	bool bRegisterDebugProvider = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "타이머 디버그 라벨을 액터 위치에서 얼마나 띄울지 정합니다."))
	FVector DebugWorldLocationOffset = FVector(0.0f, 0.0f, 140.0f);
};
