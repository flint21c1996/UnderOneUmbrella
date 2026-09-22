#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Debug/UOUDebugProvider.h"
#include "UOUIllusionTraversalProbe.generated.h"

class ACharacter;
class APlayerController;
class UPrimitiveComponent;
class USkeletalMeshComponent;
class UCharacterMovementComponent;

// 현재 프레임의 충돌 표면으로 계산한 착시 이동 진단 결과다.
UENUM(BlueprintType)
enum class EUOUIllusionProbeStatus : uint8
{
	Disabled, NoPlayer, NotOrthographic, CameraMoving, NotGrounded, NoInput,
	UnregisteredFloor, FeetOccluded, NoSurface, SameSurface, UnregisteredTarget,
	NotWalkable, NoDepthGap, NarrowSurface, BlockedLanding, Candidate
};

// 등록된 발판의 충돌 위치·노멀만 검사하며, 명시적으로 켠 경우에만 자동 이동하는 실험 액터다.
UCLASS(meta = (DisplayName = "UOU Illusion Traversal Probe"))
class UNDERONEUMBRELLA_API AUOUIllusionTraversalProbe : public AActor, public IUOUDebugProvider
{
	GENERATED_BODY()
public:
	AUOUIllusionTraversalProbe();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 실험을 중단하면 진입 전 위치와 이동 설정을 복구하여 공중에 고립되지 않게 한다.
	UFUNCTION(BlueprintCallable, Category = "착시 이동")
	void StopVirtualWalking();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "착시 이동")
	bool bVirtualWalking = false;

	UPROPERTY(EditAnywhere, Category = "착시 이동", meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "노멀 일치 기준"))
	float NormalAgreement = 0.98f;

	UPROPERTY(EditAnywhere, Category = "착시 이동|표시", meta = (DisplayName = "다리 가림 보정"))
	bool bCorrectVirtualOcclusion = true;

	UPROPERTY(EditAnywhere, Category = "착시 이동|표시", meta = (ClampMin = "0.0", Units = "cm", DisplayName = "표시 깊이 보정 최대 거리"))
	float MaximumVisualDepthOffset = 2000.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "착시 이동")
	FVector PredictedFeetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "착시 진단", meta = (DisplayName = "진단 사용"))
	bool bProbeEnabled = true;

	// 기존 BP·맵의 자동 이동 설정을 보존하려고 저장용 변수명만 유지한다. 텍스처는 사용하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "착시 이동", meta = (DisplayName = "충돌 기반 자동 이동", ToolTip = "등록 발판의 현재 충돌 위치와 노멀로 연결합니다. 캐릭터와 부착 액터는 검사에서 제외합니다. 내려가기는 실제 발의 출발면 이탈 후 전환하며, 후퇴·회전 규칙은 기존과 같습니다."))
	bool bEnableTextureAutoTraversal = false;

	// 기존 BP 조회 노드의 연결도 유지하되 표시와 내용은 충돌 판정 상태로 통일한다.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "착시 이동", meta = (DisplayName = "충돌 이동 판정"))
	FString TextureTraversalReason = TEXT("충돌 검사 대기");

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "착시 이동", meta = (DisplayName = "마지막 하강 카메라 진단"))
	FString DescentCameraDiagnostics;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "착시 진단", meta = (DisplayName = "검사 캐릭터", ToolTip = "비워두면 첫 로컬 플레이어 캐릭터를 검사합니다."))
	TObjectPtr<ACharacter> TargetCharacter;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "착시 진단", meta = (DisplayName = "착시 이동 허용 발판", ToolTip = "출발·도착 발판을 모두 등록합니다. Movable도 허용하며 검사 시점의 충돌 표면을 사용합니다. 보이는 표면과 충돌 모양이 일치해야 합니다."))
	TArray<TObjectPtr<AActor>> AllowedPlatforms;

	UPROPERTY(EditAnywhere, Category = "착시 진단", meta = (DisplayName = "전방 검사 거리", ClampMin = "1.0", Units = "cm"))
	float ProbeDistance = 100.0f;

	UPROPERTY(EditAnywhere, Category = "착시 이동", meta = (DisplayName = "진입 경계 여유", ClampMin = "0.0", ClampMax = "10.0", Units = "cm"))
	float EntryBoundaryTolerance = 3.0f;

	UPROPERTY(EditAnywhere, Category = "착시 진단", meta = (DisplayName = "시선 검사 길이", ClampMin = "100.0", Units = "cm"))
	float TraceDistance = 100000.0f;

	UPROPERTY(EditAnywhere, Category = "착시 진단", meta = (DisplayName = "최소 깊이 차이", ClampMin = "1.0", Units = "cm"))
	float MinimumDepthGap = 30.0f;

	UPROPERTY(EditAnywhere, Category = "착시 진단", meta = (DisplayName = "표면 검사 채널"))
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "착시 진단|결과")
	EUOUIllusionProbeStatus Status = EUOUIllusionProbeStatus::Disabled;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "착시 진단|결과")
	FString Reason;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "착시 진단|결과")
	TObjectPtr<AActor> CurrentPlatform;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "착시 진단|결과")
	TObjectPtr<AActor> CandidatePlatform;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "착시 진단|결과")
	FVector FeetLocation = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "착시 진단|결과")
	FVector CandidateLocation = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "착시 진단|결과")
	FVector CandidateNormal = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "착시 진단|결과")
	float CandidateDepthGap = 0.0f;

	virtual EUOUDebugCategory GetDebugCategory_Implementation() const override { return EUOUDebugCategory::Puzzle; }
	virtual bool IsDebugProviderEnabled_Implementation() const override { return bProbeEnabled; }
	virtual FText GetDebugDisplayName_Implementation() const override;
	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual FVector GetDebugWorldLocation_Implementation() const override;
#if UOU_WITH_DEVELOPMENT_TOOLS
	virtual void GatherDevelopmentDebugDraw(IUOUDevelopmentDebugDrawContext& Context) const override;
#endif

	// 두 월드 좌표의 시선 방향 거리 차이다. 깊이 텍스처나 GPU 결과를 사용하지 않는다.
	static float CalculateDepthGap(const FVector& ExpectedLocation, const FVector& SurfaceLocation, const FVector& ViewDirection);
	// 초록 지지점에서 진행 방향 검사 간격을 빼서 빨간 발 위치를 구한다.
	static FVector CalculateVirtualFeet(const FVector& Surface, const FVector& Direction, float Distance)
	{
		return Surface - Direction.GetSafeNormal2D() * FMath::Max(1.0f, Distance);
	}
	// 바닥 여유를 월드 위로 더하지 않고 시선 방향으로 확보하여 직교 화면 좌표를 보존한다.
	static bool CalculateScreenPreservingFeet(const FVector& ExpectedFeet, const FVector& PlanePoint,
		const FVector& Normal, const FVector& ViewDirection, float FloorClearance, FVector& OutFeet);
private:
	friend class FUOUIllusionBoundaryFlowTest;
	friend class FUOUIllusionWalkingVelocityTest;
#if WITH_DEV_AUTOMATION_TESTS
	// 테스트에서는 화면 역투영만 대체하고 실제 월드 충돌과 이동 판정은 그대로 실행한다.
	TFunction<bool(const FVector&, FHitResult&)> TestScreenTrace;
#endif
	static FVector CalculateWalkingVelocity(UCharacterMovementComponent* Movement, const FVector& Input, float DeltaSeconds);
	bool IsEntryBoundaryReached(float TravelDistance) const;
	friend class FUOUIllusionCollisionOnlyTest;
	// 화면 좌표 역투영과 실제 충돌 질의를 분리하여 렌더링 없이도 동일한 질의를 검증한다.
	bool TraceViewRay(ACharacter* Character, const FVector& Origin, const FVector& Direction, FHitResult& Hit) const;
	float ActiveProbeDistance = 100.0f;
	friend class FUOUIllusionGroundReturnTest;
	bool ValidateVirtualSupport(UPrimitiveComponent* Surface, const FVector& Direction, float Speed);
	// 화면에서 출발 표면이 끝나는 점과 그 직후 도착 표면을 따로 보관한다.
	void UpdateTraversalBoundary(APlayerController* Controller, ACharacter* Character,
		UPrimitiveComponent* Source, const FVector& Feet, const FVector& Expected);
	bool bHasTraversalBoundary = false;
	FVector BoundarySourcePoint = FVector::ZeroVector;
	FVector BoundaryTargetPoint = FVector::ZeroVector;
	FHitResult BoundaryTargetHit;
	float BoundaryTravelDistance = 0.0f;
	bool RetreatVirtualWalking(const FVector& Direction, float Distance, float Speed);
	void ReleaseVirtualWalking(const FVector& Direction, float Speed, const TCHAR* ReasonText);
	bool TryPendingDescent(ACharacter* Character, APlayerController* Controller, const FVector& Direction);
	TArray<FVector> VirtualTrail;
	FVector EntryTravelDirection = FVector::ZeroVector;
	TWeakObjectPtr<UPrimitiveComponent> DescentSource;
	TWeakObjectPtr<UPrimitiveComponent> DescentTarget;
	FVector DescentEntry = FVector::ZeroVector;
	FVector DescentDirection = FVector::ZeroVector;
	FRotator DescentView = FRotator::ZeroRotator;
	double DescentUntil = -1;
	void UpdateVirtualVisual();
	void ResetVirtualVisual();
	TWeakObjectPtr<USkeletalMeshComponent> VisualMesh;
	FVector AppliedVisualOffset = FVector::ZeroVector;
	float VisualDepthOffset = 0.0f;
	FVector LastProbeDirection = FVector::ZeroVector;
	friend class FUOUIllusionVirtualReturnTest;
	void TickVirtualWalking(float DeltaSeconds);
	void RestoreVirtualWalking(bool bReturnToEntry);
	bool TryFinishVirtualWalking();
	void CancelVirtualWalkingForRotation();
	TWeakObjectPtr<ACharacter> VirtualCharacter;
	TWeakObjectPtr<UPrimitiveComponent> VirtualSupport;
	TWeakObjectPtr<AActor> EntryPlatform;
	FVector VirtualNormal = FVector::UpVector;
	FVector EntryLocation = FVector::ZeroVector;
	FVector LastVirtualLocation = FVector::ZeroVector;
	FRotator EntryViewRotation = FRotator::ZeroRotator;
	uint8 SavedMovementMode = 0;
	uint8 SavedCustomMode = 0;
	bool bSavedMovementTick = true;
	ECollisionEnabled::Type SavedCollision = ECollisionEnabled::QueryAndPhysics;
	bool bHasVirtualPrediction = false;
	void UpdateProbe();
	void SetResult(EUOUIllusionProbeStatus NewStatus, const TCHAR* Message);
	bool TraceScreenPoint(APlayerController* Controller, ACharacter* Character, const FVector& WorldPoint, FHitResult& Hit) const;
	bool bHasCameraHistory = false;
	bool bHasFeet = false;
	bool bHasCandidate = false;
	FRotator PreviousCameraRotation = FRotator::ZeroRotator;
	float CameraStableTime = 0.0f;
	float LastDeltaSeconds = 0.0f;
	TWeakObjectPtr<ACharacter> PreviousCharacter;
};
