// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Debug/UOUPuzzleDebugInfoProvider.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUFlyingSwarmEffectActor.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class UInstancedStaticMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

class AUOUFlyingSwarmEffectActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUFlyingSwarmEffectActorEvent, AUOUFlyingSwarmEffectActor*, EffectActor);

UENUM(BlueprintType)
enum class EUOUPaperPlaneSwarmFlightPattern : uint8
{
	WideGlide UMETA(DisplayName = "Wide Glide"),
	DiveAndRise UMETA(DisplayName = "Dive And Rise"),
	SCurve UMETA(DisplayName = "S Curve"),
	OverpassTurnback UMETA(DisplayName = "Overpass Turnback")
};

UENUM(BlueprintType)
enum class EUOUPaperPlaneSwarmRenderMode : uint8
{
	CodeDrivenMesh UMETA(DisplayName = "Code Driven Mesh"),
	Niagara UMETA(DisplayName = "Niagara")
};

USTRUCT(BlueprintType)
struct FUOUPaperPlaneSwarmParticleRandom
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Particle")
	float RandomPhase = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Particle")
	float RandomDelay = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Particle")
	float RandomSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Particle")
	float OrbitSpeedRandom = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Particle")
	float RandomRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Particle")
	float RandomHeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Particle")
	float SideOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Particle")
	float HeightOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Particle")
	float BankAmount = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Particle")
	float ScaleRandom = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Particle")
	int32 PatternIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Particle")
	float PatternPhase = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Particle")
	float SwoopAmount = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Particle")
	float SwoopHeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Particle")
	float SwoopSideAmount = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Particle")
	float SwoopSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Particle")
	FVector FarPoint = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FUOUPaperPlaneSwarmRandomRanges
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random", meta = (ClampMin = "1"))
	int32 FlightPatternCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float RandomPhaseMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float RandomPhaseMax = 6.283185f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float RandomDelayMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float RandomDelayMax = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float RandomSpeedMin = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float RandomSpeedMax = 1.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float OrbitSpeedRandomMin = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float OrbitSpeedRandomMax = 1.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float RandomRadiusMin = -80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float RandomRadiusMax = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float RandomHeightMin = -120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float RandomHeightMax = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float SideOffsetMin = -500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float SideOffsetMax = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float HeightOffsetMin = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float HeightOffsetMax = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float BankAmountMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float BankAmountMax = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float ScaleRandomMin = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float ScaleRandomMax = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float PatternPhaseMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float PatternPhaseMax = 6.283185f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float SwoopAmountMin = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float SwoopAmountMax = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float SwoopHeightMin = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float SwoopHeightMax = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float SwoopSideAmountMin = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float SwoopSideAmountMax = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float SwoopSpeedMin = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	float SwoopSpeedMax = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	FVector FarPointMin = FVector(-2300.0f, -2350.0f, 1500.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Random")
	FVector FarPointMax = FVector(2300.0f, -1500.0f, 3000.0f);
};

USTRUCT(BlueprintType)
struct FUOUPaperPlaneSwarmMotionInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	FVector StartPosition = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	FVector TargetPosition = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	float FlightAlpha = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	float WrapAlpha = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	float Time = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	float WrapRadius = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	float WrapHeight = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	float MinOrbitHeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	float OrbitSpeed = 3.14159f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	float WobbleRightAmount = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	float WobbleUpAmount = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	int32 FlightPatternCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	float GlideSwoopAmount = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	float GlideSwoopHeight = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	float GlideSideAmount = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	float GlideOvershootAmount = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion", meta = (ClampMin = "0.05", ClampMax = "0.95"))
	float FarReachAlpha = 0.58f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	FVector TargetForward = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	FVector TargetRight = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	FVector TargetUp = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion")
	FVector PreviousPosition = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paper Plane Swarm|Motion", meta = (ClampMin = "0.0001"))
	float DeltaTime = 0.01666667f;
};

USTRUCT(BlueprintType)
struct FUOUPaperPlaneSwarmMotionResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	float FlightT = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	float WrapT = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	float Radius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	int32 PatternIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	FVector PreWrapPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	FVector FarPoint = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	FVector ControlPoint = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	FVector ControlPointA = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	FVector ControlPointB = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	FVector OvershootPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	FVector BezierPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	FVector PathPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	FVector WrapPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	FVector Position = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	FVector ForwardDirection = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	FVector UpDirection = FVector::UpVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	float BankRadians = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Motion")
	float Scale = 1.0f;
};

// 종이비행기 Mesh Particle 군집이 출발 지점에서 목표 주변 궤도로 진입하도록 Niagara를 제어하는 액터입니다.
// 실제 게임플레이 액터를 여러 개 만들지 않고 퍼즐 Result와 시각 연출을 연결할 때 사용합니다.
UCLASS(Blueprintable, meta=(DisplayName="UOU Paper Plane Swarm Effect Actor"))
class UNDERONEUMBRELLA_API AUOUFlyingSwarmEffectActor
	: public AActor
	, public IUOUPuzzleResultReceiver
	, public IUOUPuzzleDebugInfoProvider
{
	GENERATED_BODY()

public:
	AUOUFlyingSwarmEffectActor();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Effect")
	TObjectPtr<UNiagaraComponent> SwarmEffect = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Effect")
	TObjectPtr<UInstancedStaticMeshComponent> PaperPlaneInstances = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Effect", meta = (ToolTip = "Code Driven Mesh는 C++ Tick에서 종이비행기 위치와 회전을 직접 계산합니다. Niagara는 기존 시스템 호환용입니다."))
	EUOUPaperPlaneSwarmRenderMode RenderMode = EUOUPaperPlaneSwarmRenderMode::CodeDrivenMesh;

	// 기존 액터 호환용입니다. 실제 메시는 PaperPlaneInstances 컴포넌트의 Static Mesh가 우선입니다.
	UPROPERTY()
	bool bUseDefaultConeMesh = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Effect", meta = (EditCondition = "RenderMode == EUOUPaperPlaneSwarmRenderMode::CodeDrivenMesh", ToolTip = "기존 액터 호환용 fallback Static Mesh입니다. CodeDrivenMesh에서는 PaperPlaneInstances 컴포넌트의 Static Mesh가 우선이고, 컴포넌트 mesh가 비어 있을 때만 이 값을 사용합니다."))
	TObjectPtr<UStaticMesh> PaperPlaneMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Effect", meta = (EditCondition = "RenderMode == EUOUPaperPlaneSwarmRenderMode::CodeDrivenMesh", ToolTip = "Mesh의 로컬 진행 방향을 C++ 계산 방향에 맞추는 추가 회전 보정값입니다. 최종 회전은 이 값과 PaperPlaneInstances 컴포넌트 Rotation을 함께 곱해 계산합니다. 실제 종이비행기 mesh처럼 기수가 로컬 +X를 향하면 0,0,0을 사용합니다."))
	FRotator PaperPlaneMeshRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Effect", meta = (EditCondition = "RenderMode == EUOUPaperPlaneSwarmRenderMode::CodeDrivenMesh", ClampMin = "0.001", ToolTip = "C++ 렌더링 모드에서 종이비행기 인스턴스에 곱할 추가 스케일입니다. 최종 스케일은 이 값과 PaperPlaneInstances 컴포넌트 Scale, 개별 랜덤 스케일을 함께 곱해 계산합니다."))
	FVector PaperPlaneMeshScale = FVector(1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Scale", meta = (ClampMin = "0.001", ToolTip = "각 종이비행기에 적용할 무작위 크기 배율의 최소값입니다."))
	float PlaneScaleRandomMin = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Scale", meta = (ClampMin = "0.001", ToolTip = "각 종이비행기에 적용할 무작위 크기 배율의 최대값입니다."))
	float PlaneScaleRandomMax = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Effect", meta = (DisplayName = "Swarm System", ToolTip = "종이비행기 군집 연출에 사용할 Niagara System입니다. 비워두면 SwarmEffect 컴포넌트에 직접 지정된 System을 사용합니다."))
	TObjectPtr<UNiagaraSystem> SwarmEffectSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Effect", meta = (ToolTip = "게임 시작과 동시에 연출을 시작할지 정합니다. 퍼즐 Result로만 켤 때는 꺼둡니다."))
	bool bActivateOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Effect", meta = (ToolTip = "Activate를 다시 받을 때 Niagara를 처음부터 다시 재생할지 정합니다."))
	bool bResetSystemOnActivate = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Effect", meta = (ToolTip = "Deactivate 시 컴포넌트를 즉시 숨길지 정합니다. Niagara 안에서 페이드아웃을 처리하려면 꺼둡니다."))
	bool bHideEffectWhenInactive = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Paper Plane Swarm|Source", meta = (ToolTip = "종이비행기가 출발할 액터입니다. 비워두면 이 액터의 위치를 사용합니다. 우체통 액터를 넣는 용도입니다."))
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Source", meta = (ToolTip = "SourceActor 기준 로컬 출발 오프셋입니다. 우체통 입구 위치를 맞출 때 사용합니다."))
	FVector SourceLocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Source", meta = (ToolTip = "SourceActor 루트 컴포넌트에 있는 소켓 이름입니다. 지정하면 해당 소켓 기준으로 SourceLocalOffset을 더합니다."))
	FName SourceSocketName = NAME_None;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Paper Plane Swarm|Target", meta = (ToolTip = "종이비행기가 감쌀 목표 액터입니다. 비워두고 Use Player Pawn When Target Missing을 켜면 첫 플레이어 Pawn을 사용합니다."))
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Target", meta = (ToolTip = "TargetActor 기준 로컬 목표 오프셋입니다. 캐릭터 허리나 머리 높이를 맞출 때 사용합니다."))
	FVector TargetLocalOffset = FVector(0.0f, 0.0f, 120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Target", meta = (ToolTip = "TargetActor 루트 컴포넌트에 있는 소켓 이름입니다. 지정하면 해당 소켓 기준으로 TargetLocalOffset을 더합니다."))
	FName TargetSocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Target", meta = (ToolTip = "TargetActor가 비어 있을 때 첫 플레이어 Pawn을 목표로 사용할지 정합니다."))
	bool bUsePlayerPawnWhenTargetMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Target", meta = (ToolTip = "활성화 중 TargetActor 위치를 계속 따라갈지 정합니다. 끄면 Activate 시점의 목표 위치를 유지합니다."))
	bool bFollowTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Target", meta = (ToolTip = "켜면 TargetActor의 회전을 궤도 기준축으로 사용합니다. 끄면 목표 위치만 따라가고 궤도는 월드 축 기준으로 유지합니다."))
	bool bUseTargetRotationForOrbit = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Target", meta = (ToolTip = "켜면 TargetLocalOffset 계산에 TargetActor의 스케일을 반영합니다. 끄면 대상 스케일이 바뀌어도 오프셋 거리는 유지됩니다."))
	bool bUseTargetScaleForOffset = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Shape", meta = (ClampMin = "0", ToolTip = "Niagara가 생성할 종이비행기 수입니다. Niagara의 Spawn Burst Count에 이 값을 연결합니다."))
	int32 PlaneCount = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Shape", meta = (ClampMin = "0.0", ToolTip = "목표 주변을 감싸는 기본 궤도 반경입니다."))
	float WrapRadius = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Shape", meta = (ClampMin = "0.0", ToolTip = "목표 주변 궤도의 세로 흔들림 높이입니다."))
	float WrapHeight = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Shape", meta = (ClampMin = "0.0", ToolTip = "목표 위치 기준으로 궤도 비행이 내려갈 수 있는 최소 높이입니다. 0이면 목표 위치보다 아래로 내려가지 않습니다."))
	float MinOrbitHeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Shape", meta = (ClampMin = "0.0", ToolTip = "목표 주변 궤도 회전 속도입니다."))
	float OrbitSpeed = 3.14159f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Shape", meta = (ClampMin = "0.001", ToolTip = "각 종이비행기의 공전 속도에 곱할 무작위 배율의 최소값입니다."))
	float OrbitSpeedRandomMin = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Shape", meta = (ClampMin = "0.001", ToolTip = "각 종이비행기의 공전 속도에 곱할 무작위 배율의 최대값입니다."))
	float OrbitSpeedRandomMax = 1.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Shape", meta = (ClampMin = "0.0", ToolTip = "비행 경로 좌우 흔들림 강도입니다."))
	float WobbleRightAmount = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Shape", meta = (ClampMin = "0.0", ToolTip = "비행 경로 상하 흔들림 강도입니다."))
	float WobbleUpAmount = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Glide Patterns", meta = (ClampMin = "1", ClampMax = "4", ToolTip = "활공 단계에서 사용할 패턴 수입니다. 현재 1~4번 패턴을 지원합니다."))
	int32 FlightPatternCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Glide Patterns", meta = (ClampMin = "0.0", ToolTip = "Orbit 진입 전에 한 번 크게 치고 나가는 활공 거리 강도입니다."))
	float GlideSwoopAmount = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Glide Patterns", meta = (ClampMin = "0.0", ToolTip = "활공 중 위아래로 치고 올라가거나 내려가는 높이 강도입니다."))
	float GlideSwoopHeight = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Glide Patterns", meta = (ClampMin = "0.0", ToolTip = "활공 중 좌우로 크게 빠지는 폭입니다."))
	float GlideSideAmount = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Glide Patterns", meta = (ClampMin = "0.0", ToolTip = "목표를 살짝 지나쳤다가 돌아오는 패턴의 오버슈트 거리입니다."))
	float GlideOvershootAmount = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Glide Patterns", meta = (ClampMin = "0.05", ClampMax = "0.95", ToolTip = "전체 활공 시간 중 Far Point에 도달하는 비율입니다. 낮을수록 빨리 멀리 나가고, 높을수록 멀리 나가는 구간이 길어집니다."))
	float FarReachAlpha = 0.58f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Glide Patterns", meta = (ToolTip = "종이비행기가 먼저 날아갈 월드 Far Point 랜덤 범위의 최소값입니다."))
	FVector FarPointMin = FVector(-2300.0f, -2350.0f, 1500.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Glide Patterns", meta = (ToolTip = "종이비행기가 먼저 날아갈 월드 Far Point 랜덤 범위의 최대값입니다."))
	FVector FarPointMax = FVector(2300.0f, -1500.0f, 3000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Timing", meta = (ClampMin = "0.01", ToolTip = "출발 지점에서 목표 주변 진입점까지 곡선 비행하는 시간입니다. FlightAlpha 계산에 사용합니다."))
	float FlightDuration = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Timing", meta = (ClampMin = "0.0", ToolTip = "목표 주변 Orbit으로 흡수되기 시작하는 시간입니다. FlightDuration보다 조금 빠르게 두면 자연스럽습니다."))
	float WrapStartTime = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Timing", meta = (ClampMin = "0.01", ToolTip = "곡선 비행 위치에서 목표 주변 Orbit 위치로 전환되는 시간입니다. WrapAlpha 계산에 사용합니다."))
	float WrapDuration = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Random", meta = (ToolTip = "켜면 매번 같은 Seed를 Niagara에 전달합니다."))
	bool bUseFixedRandomSeed = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Random", meta = (EditCondition = "bUseFixedRandomSeed"))
	int32 RandomSeed = 2026;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Random", meta = (ToolTip = "고정 Seed를 쓰지 않을 때 Activate마다 Seed를 증가시켜 매번 다른 군집 패턴을 만듭니다."))
	bool bAdvanceRandomSeedOnActivate = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Debug", meta = (ToolTip = "게임 중 출발점, 목표점, 궤도 반경을 디버그로 표시합니다."))
	bool bDrawRuntimeDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Debug", meta = (ClampMin = "0.0", ToolTip = "디버그 선 두께입니다."))
	float RuntimeDebugThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Debug", meta = (ClampMin = "0", ClampMax = "24", ToolTip = "solver 기준 경로를 표시할 샘플 종이비행기 수입니다."))
	int32 RuntimeDebugSampleCount = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Debug", meta = (ClampMin = "1", ClampMax = "32", ToolTip = "샘플 Bezier 경로를 몇 개 선분으로 나눠 표시할지 정합니다."))
	int32 RuntimeDebugPathSegments = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName StartPositionParameterName = TEXT("StartPosition");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName TargetPositionParameterName = TEXT("TargetPosition");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName FlightAlphaParameterName = TEXT("FlightAlpha");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName WrapAlphaParameterName = TEXT("WrapAlpha");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName TimeParameterName = TEXT("Time");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName WrapRadiusParameterName = TEXT("WrapRadius");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName WrapHeightParameterName = TEXT("WrapHeight");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName MinOrbitHeightParameterName = TEXT("MinOrbitHeight");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName OrbitSpeedParameterName = TEXT("OrbitSpeed");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName OrbitSpeedRandomMinParameterName = TEXT("OrbitSpeedRandomMin");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName OrbitSpeedRandomMaxParameterName = TEXT("OrbitSpeedRandomMax");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName WobbleRightAmountParameterName = TEXT("WobbleRightAmount");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName WobbleUpAmountParameterName = TEXT("WobbleUpAmount");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName PlaneScaleRandomMinParameterName = TEXT("PlaneScaleRandomMin");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName PlaneScaleRandomMaxParameterName = TEXT("PlaneScaleRandomMax");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName FlightPatternCountParameterName = TEXT("FlightPatternCount");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName GlideSwoopAmountParameterName = TEXT("GlideSwoopAmount");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName GlideSwoopHeightParameterName = TEXT("GlideSwoopHeight");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName GlideSideAmountParameterName = TEXT("GlideSideAmount");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName GlideOvershootAmountParameterName = TEXT("GlideOvershootAmount");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName PlaneCountParameterName = TEXT("PlaneCount");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName TargetForwardParameterName = TEXT("TargetForward");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName TargetRightParameterName = TEXT("TargetRight");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName TargetUpParameterName = TEXT("TargetUp");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters")
	FName RandomSeedParameterName = TEXT("RandomSeed");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters|Legacy", meta = (AdvancedDisplay, ToolTip = "기존 NS_PaperPlaneSwarm이 SourcePosition을 받을 때만 사용하는 호환 파라미터입니다."))
	FName LegacySourcePositionParameterName = TEXT("SourcePosition");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters|Legacy", meta = (AdvancedDisplay, ToolTip = "기존 NS_PaperPlaneSwarm이 SourceForward를 받을 때만 사용하는 호환 파라미터입니다."))
	FName LegacySourceForwardParameterName = TEXT("SourceForward");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters|Legacy", meta = (AdvancedDisplay, ToolTip = "기존 NS_PaperPlaneSwarm이 SourceRight를 받을 때만 사용하는 호환 파라미터입니다."))
	FName LegacySourceRightParameterName = TEXT("SourceRight");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters|Legacy", meta = (AdvancedDisplay, ToolTip = "기존 NS_PaperPlaneSwarm이 OrbitRadius를 받을 때만 사용하는 호환 파라미터입니다."))
	FName LegacyOrbitRadiusParameterName = TEXT("OrbitRadius");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters|Legacy", meta = (AdvancedDisplay, ToolTip = "기존 NS_PaperPlaneSwarm이 OrbitHeight를 받을 때만 사용하는 호환 파라미터입니다."))
	FName LegacyOrbitHeightParameterName = TEXT("OrbitHeight");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters|Legacy", meta = (AdvancedDisplay, ToolTip = "기존 NS_PaperPlaneSwarm이 EffectAge를 받을 때만 사용하는 호환 파라미터입니다."))
	FName LegacyEffectAgeParameterName = TEXT("EffectAge");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters|Legacy", meta = (AdvancedDisplay, ToolTip = "기존 NS_PaperPlaneSwarm이 TravelDuration을 받을 때만 사용하는 호환 파라미터입니다."))
	FName LegacyTravelDurationParameterName = TEXT("TravelDuration");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters|Legacy", meta = (AdvancedDisplay, ToolTip = "기존 NS_PaperPlaneSwarm이 HoldDuration을 받을 때만 사용하는 호환 파라미터입니다."))
	FName LegacyHoldDurationParameterName = TEXT("HoldDuration");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters|Legacy", meta = (AdvancedDisplay, ToolTip = "기존 NS_PaperPlaneSwarm이 FadeOutDuration을 받을 때만 사용하는 호환 파라미터입니다."))
	FName LegacyFadeOutDurationParameterName = TEXT("FadeOutDuration");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Paper Plane Swarm|Niagara Parameters|Legacy", meta = (AdvancedDisplay, ToolTip = "기존 NS_PaperPlaneSwarm이 EffectActive를 받을 때만 사용하는 호환 파라미터입니다."))
	FName LegacyEffectActiveParameterName = TEXT("EffectActive");

	UPROPERTY(BlueprintAssignable, Category = "Paper Plane Swarm|Events")
	FUOUFlyingSwarmEffectActorEvent OnEffectStarted;

	UPROPERTY(BlueprintAssignable, Category = "Paper Plane Swarm|Events")
	FUOUFlyingSwarmEffectActorEvent OnEffectStopped;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Paper Plane Swarm|Runtime")
	bool bIsEffectActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Paper Plane Swarm|Runtime")
	float EffectElapsedTime = 0.0f;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Paper Plane Swarm|Actions")
	void ActivateEffect();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Paper Plane Swarm|Actions")
	void DeactivateEffect();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Paper Plane Swarm|Actions")
	void RestartEffect();

	UFUNCTION(BlueprintCallable, Category = "Paper Plane Swarm|Actions")
	void SetEffectActive(bool bNewActive);

	UFUNCTION(BlueprintPure, Category = "Paper Plane Swarm|Runtime")
	bool IsEffectActive() const;

	UFUNCTION(BlueprintPure, Category = "Paper Plane Swarm|Runtime")
	float GetFlightAlpha() const;

	UFUNCTION(BlueprintPure, Category = "Paper Plane Swarm|Runtime")
	float GetWrapAlpha() const;

	UFUNCTION(BlueprintPure, Category = "Paper Plane Swarm|Timing", meta = (DisplayName = "Calculate Flight Alpha"))
	static float CalculateFlightAlpha(float InElapsedTime, float InFlightDuration);

	UFUNCTION(BlueprintPure, Category = "Paper Plane Swarm|Timing", meta = (DisplayName = "Calculate Wrap Alpha"))
	static float CalculateWrapAlpha(float InElapsedTime, float InWrapStartTime, float InWrapDuration);

	UFUNCTION(BlueprintPure, Category = "Paper Plane Swarm|Motion", meta = (DisplayName = "Solve Paper Plane Swarm Motion"))
	static FUOUPaperPlaneSwarmMotionResult SolvePaperPlaneSwarmMotion(const FUOUPaperPlaneSwarmMotionInput& MotionInput, const FUOUPaperPlaneSwarmParticleRandom& ParticleRandom);

	UFUNCTION(BlueprintPure, Category = "Paper Plane Swarm|Random", meta = (DisplayName = "Make Paper Plane Swarm Particle Random"))
	static FUOUPaperPlaneSwarmParticleRandom MakePaperPlaneSwarmParticleRandom(int32 ParticleIndex, int32 InRandomSeed, const FUOUPaperPlaneSwarmRandomRanges& RandomRanges);

	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;
	virtual TArray<FString> GetPuzzleDebugInfo_Implementation() const override;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif

private:
	int32 RuntimeRandomSeed = 2026;
	FTransform ActiveStartTransform = FTransform::Identity;
	FTransform ActiveTargetTransform = FTransform::Identity;
	TArray<FUOUPaperPlaneSwarmParticleRandom> RuntimeParticles;
	TArray<FVector> RuntimePreviousPositions;
	TArray<TObjectPtr<UStaticMeshComponent>> RuntimePlaneMeshComponents;

	void ApplyEffectSystem();
	void ApplyNiagaraParameters();
	void ApplyRenderMode();
	UStaticMesh* GetResolvedPaperPlaneMesh() const;
	bool EnsurePaperPlaneMesh();
	FQuat GetResolvedPaperPlaneMeshRotationOffset() const;
	FVector GetResolvedPaperPlaneBaseScale() const;
	void ClearCodeDrivenPlaneComponents();
	void RebuildCodeDrivenPlaneInstances();
	void UpdateCodeDrivenPlaneInstances(float DeltaSeconds);
	FUOUPaperPlaneSwarmMotionInput BuildMotionInput(float DeltaSeconds) const;
	FUOUPaperPlaneSwarmRandomRanges BuildRandomRanges() const;
	void DrawRuntimeDebug() const;
	FTransform GetCurrentStartTransform() const;
	FTransform GetCurrentTargetTransform() const;

#if WITH_EDITOR
	bool ShouldRebuildCodeDrivenPlaneInstancesAfterEditorChange(FName PropertyName) const;
#endif

	FTransform ResolveSourceTransform() const;
	FTransform ResolveTargetTransform() const;
	FTransform ResolveReferenceTransform(const AActor* ReferenceActor, FName SocketName, const FVector& LocalOffset, bool bUseReferenceScaleForOffset) const;
	AActor* ResolveTargetActor() const;
	void ResolveTargetOrbitAxes(const FTransform& TargetTransform, FVector& OutForward, FVector& OutRight, FVector& OutUp) const;

	static FVector GetSafeTransformAxis(const FTransform& Transform, EAxis::Type Axis, const FVector& Fallback);
};
