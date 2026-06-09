// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUFloorPlatformActor.generated.h"

class UCurveFloat;
class UBoxComponent;
class USceneComponent;
class USplineComponent;
class UStaticMeshComponent;
class UUOUFloorPlatformCarryComponent;
class UUOUFloorPlatformStepComponent;
class AUOUFloorPlatformActor;
class AUOUFloorPlatformTargetActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUFloorPlatformMoveFinishedSignature, AUOUFloorPlatformActor*, Platform);

// 플랫폼 이동 시간을 어떤 감속과 가속 곡선으로 보정할지 정합니다.
UENUM(BlueprintType)
enum class EUOUFloorPlatformEasingMode : uint8
{
	Linear,
	EaseIn,
	EaseOut,
	EaseInOut
};

// 플랫폼이 시작점과 목표점 사이의 위치를 어떤 경로로 지나갈지 정합니다.
UENUM(BlueprintType)
enum class EUOUFloorPlatformPathMode : uint8
{
	Linear,
	CubicBezier
};

// 플랫폼 이동 중 회전을 어떤 기준으로 처리할지 정합니다.
// TransformLerp는 기존처럼 목표 마커의 위치와 회전을 직접 보간하고, Hinge는 한 변을 고정한 접힘 회전을 사용합니다.
UENUM(BlueprintType)
enum class EUOUFloorPlatformRotationMode : uint8
{
	TransformLerp,
	Hinge
};

// 힌지 회전에서 고정할 플랫폼의 변을 정합니다.
// 방향은 PlatformMesh의 로컬 바운드를 기준으로 계산합니다.
UENUM(BlueprintType)
enum class EUOUFloorPlatformHingeEdge : uint8
{
	PositiveX,
	NegativeX,
	PositiveY,
	NegativeY,
	Custom
};

// 층 기반 스테이지에서 한 층의 플랫폼 이동을 담당하는 액터입니다.
// 퍼즐 결과를 받으면 시작 위치와 목표 위치 사이를 이동하고 완료 상태를 외부에 알립니다.
UCLASS(meta=(DisplayName="UOU Floor Platform Actor"))
class AUOUFloorPlatformActor : public AActor, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	AUOUFloorPlatformActor();

	// 이동 중일 때 플랫폼 위치를 목표 위치까지 보간합니다.
	virtual void Tick(float DeltaSeconds) override;

	// 플랫폼 전체의 기준 루트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	// 블루프린트에서 메쉬를 바로 지정해 테스트할 수 있는 기본 시각 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform")
	TObjectPtr<UStaticMeshComponent> PlatformMesh = nullptr;

	// 플랫폼 이동 시 같이 데려갈 액터를 찾는 감지 박스입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform|Carry")
	TObjectPtr<UBoxComponent> CarryDetectionBox = nullptr;

	// 플랫폼 위 액터 운반과 물리 상태 복구를 담당하는 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform|Carry")
	TObjectPtr<UUOUFloorPlatformCarryComponent> CarryComponent = nullptr;

	// 순차 목표 마커 선택과 반복 이동 인덱스를 담당하는 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform|Move Steps")
	TObjectPtr<UUOUFloorPlatformStepComponent> StepComponent = nullptr;

	// 플랫폼 시작점과 목표 지점을 실제 선으로 이어서 보여주는 에디터 확인용 경로입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform|Preview")
	TObjectPtr<USplineComponent> MovePreviewPath = nullptr;

	// 이 플랫폼이 어떤 층에 속하는지 구분하기 위한 값입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform")
	int32 FloorIndex = 4;

	// 지정하면 이 플랫폼의 이동 결과를 기준 플랫폼의 현재 Transform에 상대적으로 합성합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Movement Base", meta = (DisplayName = "Movement Base Platform"))
	TObjectPtr<AUOUFloorPlatformActor> MovementBasePlatform = nullptr;

	// 플랫폼이 처음 출발해야 하는 위치입니다.
	// 에디터에서 이동 테스트를 하더라도 이 값이 있으면 시작점이 덮어써지지 않습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Start", meta = (DisplayName = "Saved Start Transform"))
	FTransform SavedStartTransform = FTransform::Identity;

	// 저장된 시작 위치를 사용할지 정합니다.
	// 꺼져 있으면 처음 배치된 현재 위치를 시작점으로 한 번 저장합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Start", meta = (DisplayName = "Use Saved Start Transform"))
	bool bUseSavedStartTransform = false;

	// 에디터에서 다시 열었을 때 액터를 저장된 시작 위치로 되돌릴지 정합니다.
	// 이동 테스트 후 저장해도 다음 작업을 시작 위치에서 이어가게 하기 위한 옵션입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Start", meta = (DisplayName = "Keep Actor At Saved Start In Editor"))
	bool bKeepActorAtSavedStartInEditor = true;

	// 시작 위치에서 목표 위치까지 이동하는 데 걸리는 시간입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Movement", meta = (ClampMin = "0.0"))
	float MoveDuration = 2.0f;

	// 이동 알파 값을 보정할 때 사용하는 선택 곡선입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Movement")
	TObjectPtr<UCurveFloat> MoveCurve = nullptr;

	// MoveCurve가 없을 때 사용할 기본 가속과 감속 방식입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Movement")
	EUOUFloorPlatformEasingMode EasingMode = EUOUFloorPlatformEasingMode::EaseInOut;

	// EaseIn, EaseOut, EaseInOut 계산의 강도를 조절합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Movement", meta = (ClampMin = "0.1"))
	float EaseExponent = 2.0f;

	// 목표 위치까지 직선으로 갈지, 베지어 곡선을 따라 갈지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Movement")
	EUOUFloorPlatformPathMode PathMode = EUOUFloorPlatformPathMode::Linear;

	// 플랫폼 회전을 일반 보간으로 할지, 선택한 변을 중심으로 접히게 할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Movement")
	EUOUFloorPlatformRotationMode RotationMode = EUOUFloorPlatformRotationMode::TransformLerp;

	// Hinge 모드에서 움직이지 않는 고정 변입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Movement", meta = (EditCondition = "RotationMode == EUOUFloorPlatformRotationMode::Hinge", EditConditionHides, DisplayAfter = "RotationMode"))
	EUOUFloorPlatformHingeEdge HingeEdge = EUOUFloorPlatformHingeEdge::NegativeY;

	// HingeEdge가 Custom일 때 사용하는 액터 로컬 기준 힌지 위치입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Movement", meta = (EditCondition = "RotationMode == EUOUFloorPlatformRotationMode::Hinge && HingeEdge == EUOUFloorPlatformHingeEdge::Custom", EditConditionHides, DisplayAfter = "HingeEdge"))
	FVector CustomHingeLocalOffset = FVector::ZeroVector;

	// CubicBezier 경로에서 시작점 쪽 조절점을 시작점 기준 로컬 오프셋으로 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Movement")
	FVector BezierStartControlOffset = FVector(400.0f, 0.0f, 0.0f);

	// CubicBezier 경로에서 목표점 쪽 조절점을 목표점 기준 로컬 오프셋으로 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Movement")
	FVector BezierEndControlOffset = FVector(-400.0f, 0.0f, 0.0f);

	// 여러 목표 마커를 순서대로 사용해서 Activate마다 다음 위치로 이동할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Move Steps", meta = (DisplayName = "Use Move Steps"))
	bool bUseSequentialTargetMarkers = false;

	// 플랫폼이 차례대로 이동할 목표 마커 목록입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Move Steps", meta = (DisplayName = "Move Step Markers"))
	TArray<TObjectPtr<AUOUFloorPlatformTargetActor>> SequentialTargetMarkers;

	// 다음 Activate가 사용할 순차 목표 인덱스입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Move Steps", meta = (ClampMin = "0", DisplayName = "Initial Step Index"))
	int32 InitialSequentialTargetIndex = 0;

	// 마지막 목표까지 간 뒤 다시 첫 목표로 돌아갈지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Move Steps", meta = (DisplayName = "Loop Move Steps"))
	bool bLoopSequentialTargetMarkers = false;

	// 반복 이동에서 마지막 단계가 끝난 뒤 시작 위치를 한 번 거쳐 첫 단계로 돌아갈지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Move Steps", meta = (DisplayName = "Loop Through Start"))
	bool bLoopMoveStepsThroughStart = true;

	// Activate가 한 번 들어왔을 때 연속으로 소비할 이동 스텝 수입니다.
	// 예를 들어 2로 두면 퍼즐 버튼 한 번으로 현재 위치에서 다음 마커, 그 다음 마커까지 이어서 이동합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Move Steps", meta = (ClampMin = "1", UIMin = "1", DisplayName = "Move Step Count Per Activate"))
	int32 MoveStepCountPerActivate = 1;

	// 에디터에서 이동과 회전 결과를 미리 볼지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Preview", meta = (DisplayName = "Show Step Platform Preview"))
	bool bShowTransformPreview = true;

	// 에디터에서 플랫폼 시작점과 목표 지점을 잇는 이동 경로 선을 보여줄지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Preview", meta = (DisplayName = "Show Move Path Preview"))
	bool bShowMovePreviewPath = true;

	// 런타임에서 현재 플랫폼이 어떤 마커를 향해 이동 중인지 월드 디버그로 표시합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Debug", meta = (DisplayName = "Draw Runtime Move Debug"))
	bool bDrawRuntimeMoveDebug = false;

	// 0이면 매 프레임 갱신용으로 한 프레임만 표시하고, 값이 있으면 해당 시간만큼 디버그 선을 유지합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Debug", meta = (ClampMin = "0.0", DisplayName = "Runtime Debug Draw Duration"))
	float RuntimeDebugDrawDuration = 0.0f;

	// 디버그 텍스트와 마커가 플랫폼과 겹치지 않도록 위로 올리는 거리입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Debug", meta = (DisplayName = "Runtime Debug Draw Offset"))
	FVector RuntimeDebugDrawOffset = FVector(0.0f, 0.0f, 160.0f);

	// 목표 위치에 도착한 뒤 플레이어와 다른 오브젝트가 이 플랫폼과 충돌하지 않게 할지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Rules")
	bool bDisableCollisionAtTarget = false;

	// 게임 시작 시 이미 목표 위치에 있어야 하는 플랫폼인지 정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor Platform|Rules", meta = (EditCondition = "!bUseSequentialTargetMarkers", EditConditionHides, DisplayName = "Start At Target Single Target Only"))
	bool bStartAtTarget = false;

	// 에디터에서 이동 기준점을 현재 액터 위치로 다시 잡을 때 사용합니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Floor Platform|Editor")
	void CaptureCurrentAsStart();

	// 플랫폼을 목표 위치로 이동시키기 시작합니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Floor Platform|Actions")
	void MoveToTarget();

	// 순차 목표 마커를 사용할 때 현재 인덱스의 목표로 이동합니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Floor Platform|Actions", meta = (DisplayName = "Move To Next Step"))
	void MoveToNextSequentialTarget();

	// 플랫폼을 시작 위치로 되돌립니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Floor Platform|Actions")
	void ResetPlatform();

	// 플랫폼을 이동 없이 바로 목표 위치로 보냅니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Floor Platform|Actions")
	void SnapToTarget();

	// 목표 위치를 시작 위치와 같게 맞춰서 다시 배치하기 쉬운 상태로 되돌립니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Floor Platform|Move Steps", meta = (DisplayName = "Set Current Step To Start"))
	void SetTargetToStart();

	// 현재 선택된 이동 마커를 힌지 회전 결과 위치로 맞춥니다.
	// 마커를 원하는 회전값으로 돌린 뒤 누르면 한 변이 붙은 최종 위치로 보정됩니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Floor Platform|Movement", meta = (DisplayName = "Snap Current Step To Hinge Result"))
	void SnapCurrentStepToHingeResult();

	// 현재 목표 위치를 기준으로 새 순차 목표 마커를 만들고 배열 끝에 추가합니다.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Floor Platform|Move Steps", meta = (DisplayName = "Add Move Step"))
	void AddSequentialTargetMarker();

	// 현재 플랫폼이 이동 중인지 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Floor Platform|Runtime")
	bool IsMoving() const;

	// 현재 플랫폼이 목표 위치에 도착한 상태인지 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Floor Platform|Runtime")
	bool IsAtTarget() const;

	// 플랫폼 목표 이동이 끝났을 때 호출되는 이벤트입니다.
	UPROPERTY(BlueprintAssignable, Category = "Floor Platform|Events")
	FUOUFloorPlatformMoveFinishedSignature OnMoveFinished;

	// 퍼즐 조건 그룹에서 받은 결과 액션을 플랫폼 이동 명령으로 변환합니다.
	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

protected:
	virtual void PostLoad() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	// CallInEditor로 이동을 시작했을 때 에디터 뷰포트에서도 이동이 갱신되게 합니다.
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif

	// 시작 위치와 목표 위치를 현재 설정값 기준으로 계산합니다.
	void RefreshTargetTransforms();

	// 저장된 시작 위치가 있으면 사용하고, 없으면 현재 위치를 시작 위치로 저장합니다.
	void EnsureStartTransform();

	// 실행 중 이동 단계 인덱스를 에디터에 저장되는 초기 단계로 되돌립니다.
	void ResetRuntimeStepIndex();

	// 이동 완료 상태를 확정하고 이벤트를 전달합니다.
	void FinishMoveToTarget();

	// 목표 도착 상태에 따라 액터 전체 충돌을 켜거나 끕니다.
	void ApplyTargetCollisionState();

	// 순차 목표 마커를 실제 이동 목표로 사용할 수 있는지 확인합니다.
	bool ShouldUseSequentialTargetMarkers() const;

	// 순차 목표 배열에서 사용할 수 있는 목표 마커를 찾아 반환합니다.
	AUOUFloorPlatformTargetActor* GetSequentialTargetMarkerAt(int32 TargetIndex) const;

	// 현재 순차 목표 인덱스에서 사용할 수 있는 목표 마커를 반환합니다.
	AUOUFloorPlatformTargetActor* GetCurrentSequentialTargetMarker() const;

	// 이동과 스냅이 같은 기준으로 다음 순차 목표를 고를 수 있게 목표 트랜스폼과 인덱스를 계산합니다.
	bool ResolveNextSequentialTargetTransform(FTransform& OutTargetTransform, int32& OutTargetIndex) const;

	// 다음 Activate가 사용할 순차 목표 인덱스를 이동 완료 후 갱신합니다.
	void AdvanceSequentialTargetIndex();

	// 현재 위치에서 지정한 목표 트랜스폼까지 이동을 시작합니다.
	bool BeginMoveToTransform(const FTransform& InTargetTransform, AUOUFloorPlatformTargetActor* TargetMarker);

	// 요청된 스텝 수만큼 이동을 예약하고, 멈춰 있다면 즉시 첫 이동을 시작합니다.
	void RequestSequentialMoveSteps(int32 StepCount);

	// 예약된 이동 스텝이 남아 있으면 다음 마커 이동을 하나 시작합니다.
	bool TryStartQueuedSequentialMove();

	// 현재 이동 구간의 시작과 목표를 기준으로 실제 플랫폼 트랜스폼을 계산합니다.
	FTransform BuildActivePlatformTransformAtAlpha(float Alpha) const;

	// 유효한 기준 플랫폼을 반환합니다. 자기 자신이나 사라진 Actor는 기준으로 사용하지 않습니다.
	AUOUFloorPlatformActor* GetMovementBasePlatformActor() const;

	// 기준 플랫폼의 참조 Transform과 현재 상대 Transform을 갱신합니다.
	void RefreshMovementBaseReferenceTransform();

	// 현재 월드 Transform을 기준 플랫폼 현재 Transform 기준 상대값으로 다시 저장합니다.
	void RefreshCurrentMovementBaseRelativeTransform();

	// 기준 플랫폼이 움직였을 때 저장된 상대 Transform으로 현재 플랫폼 월드 Transform을 다시 합성합니다.
	void ApplyMovementBaseIdleTransform();

	// 월드 Transform을 기준 플랫폼 상대 Transform으로 변환합니다.
	FTransform ConvertWorldToMovementBaseRelativeTransform(const FTransform& WorldTransform, bool bUseReferenceTransform) const;

	// 기준 플랫폼의 현재 Transform과 상대 Transform을 합성해 월드 Transform을 만듭니다.
	FTransform ConvertMovementBaseRelativeToWorldTransform(const FTransform& RelativeTransform) const;

	// 배치된 마커/시작 Transform을 현재 기준 플랫폼 위치에 맞춘 월드 Transform으로 해석합니다.
	FTransform ResolveAuthoredWorldTransformForMovementBase(const FTransform& AuthoredWorldTransform) const;

	// 실제 이동 목표를 기준 플랫폼 상대 Transform으로 해석합니다.
	FTransform ResolveMoveTargetRelativeTransform(const FTransform& TargetWorldTransform, const AUOUFloorPlatformTargetActor* TargetMarker) const;

	// 실제 이동 목표를 현재 기준 플랫폼 위치에 맞춘 월드 Transform으로 해석합니다.
	FTransform ResolveMoveTargetWorldTransform(const FTransform& TargetWorldTransform, const AUOUFloorPlatformTargetActor* TargetMarker) const;

	// 두 트랜스폼 사이를 선택된 경로 모드와 회전 보간으로 계산합니다.
	FTransform BuildTransformBetween(const FTransform& FromTransform, const FTransform& ToTransform, float Alpha) const;

	// 지정한 이동 설정으로 두 트랜스폼 사이의 실제 플랫폼 트랜스폼을 계산합니다.
	FTransform BuildTransformBetween(
		const FTransform& FromTransform,
		const FTransform& ToTransform,
		float Alpha,
		EUOUFloorPlatformRotationMode InRotationMode,
		EUOUFloorPlatformHingeEdge InHingeEdge,
		const FVector& InCustomHingeLocalOffset) const;

	// 선택된 힌지 변을 월드에 고정한 채 목표 회전값까지 접히는 변환을 계산합니다.
	FTransform BuildHingeTransformBetween(
		const FTransform& FromTransform,
		const FTransform& ToTransform,
		float Alpha,
		EUOUFloorPlatformHingeEdge InHingeEdge,
		const FVector& InCustomHingeLocalOffset) const;

	// PlatformMesh의 바운드와 커스텀 값을 이용해 액터 로컬 기준 힌지 위치를 계산합니다.
	bool ResolveHingeLocalOffset(
		FVector& OutHingeLocalOffset,
		EUOUFloorPlatformHingeEdge InHingeEdge,
		const FVector& InCustomHingeLocalOffset) const;

	// 목표 마커가 가진 구간별 설정과 플랫폼 기본 설정을 합쳐 이번 이동에 사용할 값을 결정합니다.
	void ResolveMoveSettingsFromTarget(
		const AUOUFloorPlatformTargetActor* TargetMarker,
		EUOUFloorPlatformRotationMode& OutRotationMode,
		EUOUFloorPlatformHingeEdge& OutHingeEdge,
		FVector& OutCustomHingeLocalOffset) const;

	// 실제 이동 중 Tick에서 같은 설정을 계속 쓰도록 현재 구간 설정을 보관합니다.
	void CacheActiveMoveSettings(const AUOUFloorPlatformTargetActor* TargetMarker);

	// 선택된 경로 모드에 따라 시작점과 목표점 사이의 위치를 계산합니다.
	FVector EvaluatePathLocation(const FTransform& FromTransform, const FTransform& ToTransform, float Alpha) const;

	// 순차 목표 마커들의 미리보기 메쉬 표시 상태를 현재 프리뷰 옵션에 맞춰 갱신합니다.
	void SyncSequentialTargetMarkerPreviews();

	// 에디터에서 이동 경로와 목표 위치 미리보기를 갱신합니다.
	void UpdateEditorPreviewVisuals();

	// 런타임에서 현재 이동 목표, 다음 목표, 전체 step 마커를 디버그 드로우로 표시합니다.
	void DrawRuntimeMoveDebug() const;

	// 이동 곡선이 있으면 곡선 값을 사용하고 없으면 기본 알파를 그대로 사용합니다.
	float ResolveMoveAlpha(float RawAlpha) const;

private:
	// 이동을 시작한 순간부터 누적된 시간입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform|Runtime", meta = (AllowPrivateAccess = "true"))
	float MoveElapsedTime = 0.0f;

	// 현재 플랫폼이 이동 중인지 저장합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform|Runtime", meta = (AllowPrivateAccess = "true"))
	bool bIsMoving = false;

	// 현재 플랫폼이 목표 위치에 도착한 상태인지 저장합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Platform|Runtime", meta = (AllowPrivateAccess = "true"))
	bool bIsAtTarget = false;

	// 플랫폼이 이동을 시작하는 기준 트랜스폼입니다.
	UPROPERTY(Transient)
	FTransform StartTransform = FTransform::Identity;

	// 플랫폼이 목표 상태에서 도착해야 하는 트랜스폼입니다.
	UPROPERTY(Transient)
	FTransform TargetTransform = FTransform::Identity;

	// 현재 진행 중인 이동 구간의 실제 시작 트랜스폼입니다.
	UPROPERTY(Transient)
	FTransform MoveStartTransform = FTransform::Identity;

	// 현재 진행 중인 이동 구간의 실제 목표 트랜스폼입니다.
	UPROPERTY(Transient)
	FTransform MoveTargetTransform = FTransform::Identity;

	// 기준 플랫폼이 있을 때 현재 월드 위치를 기준 플랫폼 현재 Transform 기준 상대값으로 저장합니다.
	UPROPERTY(Transient)
	FTransform CurrentMovementBaseRelativeTransform = FTransform::Identity;

	// 기준 플랫폼이 있을 때 현재 이동 구간의 시작 상대 Transform입니다.
	UPROPERTY(Transient)
	FTransform MoveStartMovementBaseRelativeTransform = FTransform::Identity;

	// 기준 플랫폼이 있을 때 현재 이동 구간의 목표 상대 Transform입니다.
	UPROPERTY(Transient)
	FTransform MoveTargetMovementBaseRelativeTransform = FTransform::Identity;

	// 마커와 시작 위치를 해석할 때 사용하는 기준 플랫폼의 참조 Transform입니다.
	UPROPERTY(Transient)
	FTransform MovementBaseReferenceTransform = FTransform::Identity;

	// 기준 플랫폼 참조 Transform을 한 번이라도 잡았는지 기록합니다.
	UPROPERTY(Transient)
	bool bHasMovementBaseReferenceTransform = false;

	// 현재 이동 구간에서 사용할 회전 방식입니다.
	UPROPERTY(Transient)
	EUOUFloorPlatformRotationMode ActiveMoveRotationMode = EUOUFloorPlatformRotationMode::TransformLerp;

	// 현재 이동 구간에서 사용할 힌지 고정 변입니다.
	UPROPERTY(Transient)
	EUOUFloorPlatformHingeEdge ActiveMoveHingeEdge = EUOUFloorPlatformHingeEdge::NegativeY;

	// 현재 이동 구간에서 사용할 커스텀 힌지 로컬 위치입니다.
	UPROPERTY(Transient)
	FVector ActiveMoveCustomHingeLocalOffset = FVector::ZeroVector;

	// 현재 이동이 향하고 있는 마커입니다.
	// 도착 후 자동으로 다음 스텝을 이어갈지 같은 마커별 규칙을 확인하는 데 사용합니다.
	UPROPERTY(Transient)
	TObjectPtr<AUOUFloorPlatformTargetActor> ActiveMoveTargetMarker = nullptr;

	// 시작 위치가 한 번이라도 기록되었는지 확인하는 값입니다.
	UPROPERTY(Transient)
	bool bHasCapturedStartTransform = false;

	// 현재 실행 세션에서 다음으로 사용할 이동 단계입니다.
	// 에디터 이동 테스트나 런타임 이동으로 바뀌지만 맵에는 저장되지 않습니다.
	UPROPERTY(Transient)
	int32 RuntimeSequentialTargetIndex = 0;

	// 현재 이동이 끝난 뒤 이어서 실행해야 하는 남은 스텝 수입니다.
	UPROPERTY(Transient)
	int32 PendingSequentialMoveCount = 0;

	// 에디터 버튼으로 위치를 바꾸는 중인지 기록합니다.
	// 자동 시작 위치 복귀가 스냅이나 이동 완료를 즉시 되돌리지 않게 막습니다.
	UPROPERTY(Transient)
	bool bIsApplyingEditorTransform = false;

};
