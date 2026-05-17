// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Debug/UOUDebugTypes.h"
#include "UOUDebugControllerComponent.generated.h"

// AUOUDebugController 아래에서 도메인별 디버그 옵션을 묶는 기본 컴포넌트입니다.
UCLASS(Abstract, Blueprintable, ClassGroup=(Debug), meta=(BlueprintSpawnableComponent))
class UNDERONEUMBRELLA_API UUOUDebugControllerComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUDebugControllerComponentBase();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|공통", meta = (ToolTip = "이 도메인의 디버그 표시를 활성화합니다."))
	bool bEnabled = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "디버그|공통", meta = (ToolTip = "이 컴포넌트가 담당하는 디버그 카테고리입니다."))
	EUOUDebugCategory DebugCategory = EUOUDebugCategory::System;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|공통", meta = (ToolTip = "월드 디버그 라인, 라벨 등에 사용할 기본 색상입니다."))
	FColor DebugColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|공통", meta = (ToolTip = "동시에 표시할 때 우선순위를 정합니다. 값이 높을수록 먼저 표시합니다."))
	int32 Priority = 0;

	UFUNCTION(BlueprintCallable, Category = "디버그")
	void SetDebugEnabled(bool bNewEnabled);

	UFUNCTION(BlueprintPure, Category = "디버그")
	bool IsDebugEnabled() const;

	UFUNCTION(BlueprintPure, Category = "디버그")
	FName GetDebugCategoryName() const;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Character Debug Controller"))
class UNDERONEUMBRELLA_API UUOUCharacterDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUCharacterDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|캐릭터", meta = (ToolTip = "캐릭터 핵심 상태를 메인 뷰포트에 표시합니다."))
	bool bShowViewportHUD = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|캐릭터", meta = (ToolTip = "현재 상호작용 대상 정보를 캐릭터 디버그에 포함합니다."))
	bool bShowInteractionTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|캐릭터", meta = (ToolTip = "캐릭터 이동, 입력, 상태 값을 디버그에 포함합니다."))
	bool bShowMovementState = true;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU NPC Debug Controller"))
class UNDERONEUMBRELLA_API UUOUNPCDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUNPCDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|NPC", meta = (ToolTip = "NPC 근처에 월드 디버그 라벨을 표시합니다."))
	bool bShowWorldLabels = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|NPC", meta = (ToolTip = "NPC가 이동하려는 목표 위치를 표시합니다."))
	bool bShowMoveTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|NPC", meta = (ToolTip = "NPC의 이동 경로나 방향 보조선을 표시합니다."))
	bool bShowPath = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|NPC", meta = (ToolTip = "NPC의 현재 AI/액션 상태를 표시합니다."))
	bool bShowState = true;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Puzzle Debug Controller"))
class UNDERONEUMBRELLA_API UUOUPuzzleDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUPuzzleDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|퍼즐", meta = (ToolTip = "퍼즐 액터 위에 요약 라벨을 표시합니다."))
	bool bShowWorldLabels = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|퍼즐", meta = (ToolTip = "퍼즐 액터끼리의 연결 관계를 라인으로 표시합니다."))
	bool bShowConnections = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|퍼즐", meta = (ToolTip = "퍼즐 구조를 한 줄 설명으로 표시합니다."))
	bool bShowSummaryText = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|퍼즐", meta = (ToolTip = "퍼즐 노드의 현재 활성/비활성 상태를 표시합니다."))
	bool bShowNodeState = true;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Interaction Debug Controller"))
class UNDERONEUMBRELLA_API UUOUInteractionDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUInteractionDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|상호작용", meta = (ToolTip = "상호작용 탐색 Trace와 범위를 표시합니다."))
	bool bShowTrace = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|상호작용", meta = (ToolTip = "현재 선택된 상호작용 후보를 표시합니다."))
	bool bShowCandidate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|상호작용", meta = (ToolTip = "상호작용 불가 사유를 표시합니다."))
	bool bShowFailReason = false;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU VFX Debug Controller"))
class UNDERONEUMBRELLA_API UUOUVFXDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUVFXDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|VFX", meta = (ToolTip = "활성 VFX나 파티클 수를 표시합니다."))
	bool bShowParticleCount = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|VFX", meta = (ToolTip = "Niagara 컴포넌트의 소유자와 위치를 표시합니다."))
	bool bShowNiagaraOwners = false;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Performance Debug Controller"))
class UNDERONEUMBRELLA_API UUOUPerformanceDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUPerformanceDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|성능", meta = (ToolTip = "성능 정보를 메인 뷰포트에 표시합니다."))
	bool bShowViewportStats = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|성능", meta = (ToolTip = "FPS 값을 표시합니다."))
	bool bShowFPS = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|성능", meta = (ToolTip = "프레임 시간을 표시합니다."))
	bool bShowFrameTime = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|성능", meta = (ToolTip = "액터, 컴포넌트 등 월드 단위 개수를 표시합니다."))
	bool bShowWorldCounts = false;
};

