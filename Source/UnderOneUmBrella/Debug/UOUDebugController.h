// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Debug/UOUDebugTypes.h"
#include "UOUDebugController.generated.h"

class USceneComponent;
class UUOUDebugControllerComponentBase;
class UUOUInteractionDebugControllerComponent;
class UUOUNPCDebugControllerComponent;
class UUOUPlayerDebugControllerComponent;
class UUOUPerformanceDebugControllerComponent;
class UUOUPuzzleDebugControllerComponent;
class UUOUVFXDebugControllerComponent;

// 레벨에 배치해서 전체 디버그 표시 옵션을 관리하는 컨트롤러입니다.
UCLASS(Blueprintable, meta = (DisplayName = "UOU Debug Controller", ToolTip = "레벨 전체의 UOU 디버그 표시 옵션을 제어합니다."))
class UNDERONEUMBRELLA_API AUOUDebugController : public AActor
{
	GENERATED_BODY()

public:
	AUOUDebugController();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
	TObjectPtr<USceneComponent> RootSceneComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Controller", meta = (ToolTip = "통합 디버그 시스템 전체를 켜거나 끕니다."))
	bool bEnableDebugTools = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Controller", meta = (ToolTip = "뷰포트에 간단한 디버그 컨트롤러 상태를 표시합니다."))
	bool bShowControllerStatus = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Categories", meta = (ToolTip = "플레이어 관련 디버그 표시를 켭니다."))
	bool bEnablePlayerDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Categories", meta = (ToolTip = "NPC 관련 디버그 표시를 켭니다."))
	bool bEnableNPCDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Categories", meta = (ToolTip = "퍼즐 관련 디버그 표시를 켭니다."))
	bool bEnablePuzzleDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Categories", meta = (ToolTip = "상호작용 관련 디버그 표시를 켭니다."))
	bool bEnableInteractionDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Categories", meta = (ToolTip = "VFX 관련 디버그 표시를 켭니다."))
	bool bEnableVFXDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Categories", meta = (ToolTip = "성능 관련 디버그 표시를 켭니다."))
	bool bEnablePerformanceDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Display", meta = (ClampMin = "0.0", ToolTip = "월드 디버그 UI가 표시되는 기본 거리입니다."))
	float WorldDebugVisibleDistance = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Display", meta = (ClampMin = "1", ToolTip = "한 번에 표시할 월드 디버그 UI의 최대 개수입니다."))
	int32 MaxVisibleWorldDebugItems = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Display", meta = (ToolTip = "가장 가까운 액터나 포커스된 액터만 상세 표시하기 위한 예약 옵션입니다."))
	bool bOnlyShowFocusedActor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Display", meta = (ToolTip = "월드 디버그 텍스트에 DrawDebugString 기본 그림자를 사용합니다."))
	bool bUseWorldTextShadow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors", meta = (ToolTip = "플레이어 디버그 텍스트와 보조 표시의 기본 색상입니다."))
	FColor PlayerDebugColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors", meta = (ToolTip = "NPC 디버그 텍스트의 기본 색상입니다."))
	FColor NPCDebugColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors", meta = (ToolTip = "퍼즐 디버그 라벨의 기본 색상입니다."))
	FColor PuzzleDebugColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors", meta = (ToolTip = "상호작용 디버그 텍스트와 보조 표시의 기본 색상입니다."))
	FColor InteractionDebugColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors", meta = (ToolTip = "VFX 디버그 텍스트와 보조 표시의 기본 색상입니다."))
	FColor VFXDebugColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors", meta = (ToolTip = "성능 디버그 텍스트의 기본 색상입니다."))
	FColor PerformanceDebugColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors", meta = (ToolTip = "시스템 디버그 텍스트의 기본 색상입니다."))
	FColor SystemDebugColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors|NPC", meta = (ToolTip = "NPC 이동 목표 화살표와 허용 반경 색상입니다."))
	FColor NPCMoveTargetColor = FColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors|NPC", meta = (ToolTip = "NPC 경로 지점과 경로 선 색상입니다."))
	FColor NPCPathColor = FColor::Cyan;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors|Puzzle", meta = (ToolTip = "켜면 퍼즐 연결선 색상이 provider 색상 대신 컨트롤러 색상을 사용합니다."))
	bool bOverrideProviderPuzzleConnectionColors = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors|Puzzle", meta = (ToolTip = "입력 액터에서 조건 액터로 이어지는 퍼즐 연결선 색상입니다."))
	FColor PuzzleInputConnectionColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors|Puzzle", meta = (ToolTip = "조건 액터에서 컨디션 그룹 노드로 이어지는 퍼즐 연결선 색상입니다."))
	FColor PuzzleConditionConnectionColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors|Puzzle", meta = (ToolTip = "컨디션 그룹 노드에서 결과 액터로 이어지는 퍼즐 연결선 색상입니다."))
	FColor PuzzleResultConnectionColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Colors|Puzzle", meta = (ToolTip = "보이지 않는 컨디션 그룹 노드 지점의 색상입니다."))
	FColor PuzzleConditionGroupNodeColor = FColor::White;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug|Controllers")
	TObjectPtr<UUOUPlayerDebugControllerComponent> PlayerDebugController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug|Controllers")
	TObjectPtr<UUOUNPCDebugControllerComponent> NPCDebugController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug|Controllers")
	TObjectPtr<UUOUPuzzleDebugControllerComponent> PuzzleDebugController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug|Controllers")
	TObjectPtr<UUOUInteractionDebugControllerComponent> InteractionDebugController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug|Controllers")
	TObjectPtr<UUOUVFXDebugControllerComponent> VFXDebugController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug|Controllers")
	TObjectPtr<UUOUPerformanceDebugControllerComponent> PerformanceDebugController;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Debug|Controllers", meta = (ToolTip = "이 컨트롤러에 붙은 디버그 컨트롤러 컴포넌트 런타임 목록입니다."))
	TArray<TObjectPtr<UUOUDebugControllerComponentBase>> DebugControllerComponents;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Debug")
	void RefreshDebugControllerComponents();

	UFUNCTION(BlueprintPure, Category = "Debug")
	bool IsCategoryEnabled(EUOUDebugCategory Category) const;

	UFUNCTION(BlueprintPure, Category = "Debug|Colors")
	FColor GetDebugCategoryColor(EUOUDebugCategory Category) const;

	UFUNCTION(BlueprintPure, Category = "Debug|Colors")
	FColor GetPuzzleConnectionColor(EUOUDebugConnectionType ConnectionType) const;

	UFUNCTION(BlueprintPure, Category = "Debug|Colors")
	FColor GetDebugConnectionColor(const FUOUDebugConnection& Connection) const;

	UFUNCTION(BlueprintPure, Category = "Debug")
	UUOUDebugControllerComponentBase* FindDebugControllerComponent(EUOUDebugCategory Category) const;

	const TArray<TObjectPtr<UUOUDebugControllerComponentBase>>& GetDebugControllerComponents() const;
};
