// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Debug/UOUDebugTypes.h"
#include "UOUDebugControllerComponent.generated.h"

// AUOUDebugController가 제어하는 디버그 카테고리별 기본 컴포넌트입니다.
UCLASS(Abstract, Blueprintable, ClassGroup=(Debug), HideCategories=("Debug|Common", Common), meta=(BlueprintSpawnableComponent))
class UNDERONEUMBRELLA_API UUOUDebugControllerComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUDebugControllerComponentBase();

	UPROPERTY(BlueprintReadWrite, Transient, AdvancedDisplay, Category = "Debug|Common", meta = (ToolTip = "런타임에서 이 디버그 카테고리를 켜거나 끕니다."))
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, AdvancedDisplay, Category = "Debug|Common", meta = (ToolTip = "이 컴포넌트가 담당하는 디버그 카테고리입니다."))
	EUOUDebugCategory DebugCategory = EUOUDebugCategory::System;

	UPROPERTY(BlueprintReadWrite, AdvancedDisplay, Category = "Debug|Common", meta = (ToolTip = "디버그 라인, 라벨, 보조 표시에 사용할 기본 색상입니다."))
	FColor DebugColor = FColor::White;

	UPROPERTY(BlueprintReadWrite, AdvancedDisplay, Category = "Debug|Common", meta = (ToolTip = "표시 우선순위입니다. 값이 높을수록 예산 제한이 생겼을 때 먼저 표시됩니다."))
	int32 Priority = 0;

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void SetDebugEnabled(bool bNewEnabled);

	UFUNCTION(BlueprintPure, Category = "Debug")
	bool IsDebugEnabled() const;

	UFUNCTION(BlueprintPure, Category = "Debug")
	FName GetDebugCategoryName() const;
};

UCLASS(ClassGroup=(Debug), HideCategories=("Debug|Common", Common), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Player Debug Controller"))
class UNDERONEUMBRELLA_API UUOUPlayerDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUPlayerDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Player", meta = (ToolTip = "메인 뷰포트에 플레이어 디버그 정보를 표시합니다."))
	bool bShowViewportHUD = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Player", meta = (ToolTip = "플레이어 주변 월드 디버그 도형과 보조선을 표시합니다."))
	bool bShowWorldDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Player", meta = (ToolTip = "플레이어 보유 우산 상태와 물, 비 노출 정보를 포함합니다."))
	bool bShowUmbrellaState = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Player", meta = (ToolTip = "마지막 물 붓기 라인트레이스와 전달 결과를 포함합니다."))
	bool bShowPourState = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Player", meta = (ToolTip = "현재 플레이어 상호작용 대상을 포함합니다."))
	bool bShowInteractionTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Player", meta = (ToolTip = "밀고 당기기 후보, 잡기 상태, 실패 이유를 포함합니다."))
	bool bShowPushPullState = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Player", meta = (ToolTip = "플레이어 이동, 입력, 상태 값을 포함합니다."))
	bool bShowMovementState = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Player", meta = (ToolTip = "플레이어 입력 분기와 입력 이벤트 카운트를 포함합니다."))
	bool bShowInputState = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Player", meta = (ToolTip = "카메라 yaw, 거리, 가림 처리 개수를 포함합니다."))
	bool bShowCameraState = false;
};

UCLASS(ClassGroup=(Debug), HideCategories=("Debug|Common", Common), meta=(BlueprintSpawnableComponent, DisplayName = "UOU NPC Debug Controller"))
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

UCLASS(ClassGroup=(Debug), HideCategories=("Debug|Common", Common), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Puzzle Debug Controller"))
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "Heat Wire 스플라인 포인트와 현재 열 진행 위치를 월드 디버그 구체로 표시합니다."))
	bool bShowHeatWirePathDebug = true;
};

UCLASS(ClassGroup=(Debug), HideCategories=("Debug|Common", Common), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Interaction Debug Controller"))
class UNDERONEUMBRELLA_API UUOUInteractionDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUInteractionDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Interaction", meta = (ToolTip = "메인 뷰포트에 상호작용 디버그 정보를 표시합니다."))
	bool bShowScreenDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Interaction", meta = (ToolTip = "상호작용 트레이스 범위와 방향을 표시합니다."))
	bool bShowTrace = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Interaction", meta = (ToolTip = "현재 상호작용 후보를 표시합니다."))
	bool bShowCandidate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Interaction", meta = (ToolTip = "상호작용이 막힌 이유를 표시합니다."))
	bool bShowFailReason = false;
};

UCLASS(ClassGroup=(Debug), HideCategories=("Debug|Common", Common), meta=(BlueprintSpawnableComponent, DisplayName = "UOU VFX Debug Controller"))
class UNDERONEUMBRELLA_API UUOUVFXDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUVFXDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|VFX", meta = (ToolTip = "VFX 관련 월드 디버그 도형과 보조선을 표시합니다."))
	bool bShowWorldDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|VFX", meta = (ToolTip = "활성 VFX 또는 파티클 수를 표시합니다."))
	bool bShowParticleCount = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|VFX", meta = (ToolTip = "Niagara 컴포넌트 소유자 위치를 표시합니다."))
	bool bShowNiagaraOwners = false;
};

UCLASS(ClassGroup=(Debug), HideCategories=("Debug|Common", Common), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Performance Debug Controller"))
class UNDERONEUMBRELLA_API UUOUPerformanceDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUPerformanceDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Performance", meta = (ToolTip = "메인 뷰포트에 성능 정보를 표시합니다."))
	bool bShowViewportStats = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Performance", meta = (ClampMin = "0.05", UIMin = "0.05", ToolTip = "성능 표시 값을 다시 계산하는 주기입니다. 값이 낮을수록 숫자가 빠르게 흔들립니다."))
	float ViewportStatsUpdateInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Performance", meta = (ToolTip = "현재 FPS 값을 표시합니다."))
	bool bShowFPS = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Performance", meta = (ToolTip = "Frame, Game, Draw, RHI, GPU, Input 시간을 ms 단위로 표시합니다."))
	bool bShowFrameTime = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Performance", meta = (ToolTip = "현재 프로세스 메모리 사용량을 표시합니다."))
	bool bShowMemory = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Performance", meta = (ToolTip = "렌더 해상도, 드로우콜, 프리미티브 수를 표시합니다."))
	bool bShowRenderStats = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Performance", meta = (ToolTip = "액터, 컴포넌트, 월드 개수 요약을 표시합니다."))
	bool bShowWorldCounts = false;
};
