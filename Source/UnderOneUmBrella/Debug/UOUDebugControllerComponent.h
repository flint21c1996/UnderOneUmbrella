// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Debug/UOUDebugTypes.h"
#include "UOUDebugControllerComponent.generated.h"

// AUOUDebugController가 제어하는 디버그 카테고리별 기본 컴포넌트입니다.
UCLASS(Abstract, Blueprintable, ClassGroup=(Debug), meta=(BlueprintSpawnableComponent))
class UNDERONEUMBRELLA_API UUOUDebugControllerComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUDebugControllerComponentBase();

	UPROPERTY(BlueprintReadWrite, Transient, Category = "Debug|Common", meta = (ToolTip = "런타임에서 이 디버그 카테고리를 켜거나 끕니다."))
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Debug|Common", meta = (ToolTip = "이 컴포넌트가 담당하는 디버그 카테고리입니다."))
	EUOUDebugCategory DebugCategory = EUOUDebugCategory::System;

	UPROPERTY(BlueprintReadWrite, Category = "Debug|Common", meta = (ToolTip = "디버그 라인, 라벨, 보조 표시에 사용할 기본 색상입니다."))
	FColor DebugColor = FColor::White;

	UPROPERTY(BlueprintReadWrite, Category = "Debug|Common", meta = (ToolTip = "표시 우선순위입니다. 값이 높을수록 예산 제한이 생겼을 때 먼저 표시됩니다."))
	int32 Priority = 0;

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void SetDebugEnabled(bool bNewEnabled);

	UFUNCTION(BlueprintPure, Category = "Debug")
	bool IsDebugEnabled() const;

	UFUNCTION(BlueprintPure, Category = "Debug")
	FName GetDebugCategoryName() const;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Player Debug Controller"))
class UNDERONEUMBRELLA_API UUOUPlayerDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUPlayerDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Player", meta = (ToolTip = "메인 뷰포트에 플레이어 디버그 정보를 표시합니다."))
	bool bShowViewportHUD = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Player", meta = (ToolTip = "현재 플레이어 상호작용 대상을 포함합니다."))
	bool bShowInteractionTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Player", meta = (ToolTip = "플레이어 이동, 입력, 상태 값을 포함합니다."))
	bool bShowMovementState = true;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU NPC Debug Controller"))
class UNDERONEUMBRELLA_API UUOUNPCDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUNPCDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|NPC", meta = (ToolTip = "NPC 액터 근처에 월드 라벨을 표시합니다."))
	bool bShowWorldLabels = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|NPC", meta = (ToolTip = "NPC 이동 목표를 표시합니다."))
	bool bShowMoveTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|NPC", meta = (ToolTip = "NPC 경로와 방향 보조선을 표시합니다."))
	bool bShowPath = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|NPC", meta = (ToolTip = "현재 NPC AI 또는 액션 상태를 표시합니다."))
	bool bShowState = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|NPC", meta = (ToolTip = "현재 요청된 NPC 애니메이션과 활성 애니메이션 정보를 표시합니다."))
	bool bShowAnimation = true;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Puzzle Debug Controller"))
class UNDERONEUMBRELLA_API UUOUPuzzleDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUPuzzleDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "퍼즐 액터 위에 요약 라벨을 표시합니다."))
	bool bShowWorldLabels = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "퍼즐 입력, 조건, 결과 액터 사이의 연결선을 표시합니다."))
	bool bShowConnections = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "짧은 퍼즐 요약 텍스트를 표시합니다."))
	bool bShowSummaryText = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "퍼즐 노드의 현재 활성 또는 비활성 상태를 표시합니다."))
	bool bShowNodeState = true;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Interaction Debug Controller"))
class UNDERONEUMBRELLA_API UUOUInteractionDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUInteractionDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Interaction", meta = (ToolTip = "상호작용 트레이스 범위와 방향을 표시합니다."))
	bool bShowTrace = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Interaction", meta = (ToolTip = "현재 상호작용 후보를 표시합니다."))
	bool bShowCandidate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Interaction", meta = (ToolTip = "상호작용이 막힌 이유를 표시합니다."))
	bool bShowFailReason = false;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU VFX Debug Controller"))
class UNDERONEUMBRELLA_API UUOUVFXDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUVFXDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|VFX", meta = (ToolTip = "활성 VFX 또는 파티클 수를 표시합니다."))
	bool bShowParticleCount = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|VFX", meta = (ToolTip = "Niagara 컴포넌트 소유자 위치를 표시합니다."))
	bool bShowNiagaraOwners = false;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Performance Debug Controller"))
class UNDERONEUMBRELLA_API UUOUPerformanceDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUPerformanceDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Performance", meta = (ToolTip = "메인 뷰포트에 성능 정보를 표시합니다."))
	bool bShowViewportStats = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Performance", meta = (ToolTip = "현재 FPS 값을 표시합니다."))
	bool bShowFPS = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Performance", meta = (ToolTip = "프레임 시간을 ms 단위로 표시합니다."))
	bool bShowFrameTime = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Performance", meta = (ToolTip = "액터, 컴포넌트, 월드 개수 요약을 표시합니다."))
	bool bShowWorldCounts = false;
};
