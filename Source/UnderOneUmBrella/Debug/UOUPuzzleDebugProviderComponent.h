// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Debug/UOUDebugProviderComponent.h"
#include "UOUPuzzleDebugProviderComponent.generated.h"

class AUOUPuzzleConditionGroupActor;
class UUOUPuzzleConditionSourceComponent;

// 입력, 조건, 결과 액터 관계를 시각화하는 퍼즐 디버그 provider입니다.
UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Puzzle Debug Provider"))
class UNDERONEUMBRELLA_API UUOUPuzzleDebugProviderComponent : public UUOUDebugProviderComponent
{
	GENERATED_BODY()

public:
	UUOUPuzzleDebugProviderComponent();

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "버튼, 광원, 레버처럼 퍼즐 조건에 직접 영향을 주는 액터 목록입니다."))
	TArray<TObjectPtr<AActor>> InputActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "켜면 조건 소스 컴포넌트에서 런타임 input 액터를 자동으로 수집합니다."))
	bool bAutoCollectInputActorsFromConditionSources = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "입력 액터에서 조건 액터로 이어지는 연결선을 표시합니다."))
	bool bShowInputConnections = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "조건 액터에서 컨디션 그룹 노드로 이어지는 연결선을 표시합니다."))
	bool bShowConditionConnections = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "컨디션 그룹 노드에서 결과 액터로 이어지는 연결선을 표시합니다."))
	bool bShowResultConnections = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "보이지 않는 컨디션 그룹 노드 위치에 작은 점을 표시합니다."))
	bool bShowConditionGroupNode = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ClampMin = "0.0"))
	float ConditionGroupNodeSize = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle")
	FVector ConditionGroupNodeOffset = FVector(0.0f, 0.0f, 80.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle")
	FColor InputConnectionColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle")
	FColor ConditionConnectionColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle")
	FColor ResultConnectionColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ClampMin = "0.0"))
	float ConnectionThickness = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "퍼즐 연결선 위에 텍스트 라벨을 표시합니다. 화면을 덜 어지럽히기 위해 기본값은 꺼져 있습니다."))
	bool bShowConnectionLabels = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "라벨 보드에 간단한 런타임 조건 값을 표시합니다."))
	bool bShowConditionDetails = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (EditCondition = "bShowConditionDetails", ClampMin = "0"))
	int32 MaxConditionDetailLines = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "연결된 액터와 컴포넌트의 간단한 디버그 값을 컨디션 그룹 라벨에 모아 표시합니다."))
	bool bShowConnectedActorDebugInfo = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (EditCondition = "bShowConnectedActorDebugInfo", ClampMin = "0"))
	int32 MaxConnectedActorDebugInfoLines = 12;

	virtual FText GetDebugDisplayName_Implementation() const override;
	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual void GetDebugConnections_Implementation(TArray<FUOUDebugConnection>& OutConnections) const override;

	FVector GetConditionGroupNodeWorldLocation() const;

private:
	const AUOUPuzzleConditionGroupActor* GetConditionGroupActor() const;
	void CollectResolvedInputActors(TArray<AActor*>& OutInputActors) const;
	void CollectInputActorsFromConditionSource(const UUOUPuzzleConditionSourceComponent* ConditionSource, TArray<AActor*>& OutInputActors) const;
};
