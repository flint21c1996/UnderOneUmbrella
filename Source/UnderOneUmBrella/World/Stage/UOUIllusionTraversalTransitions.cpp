#include "World/Stage/UOUIllusionTraversalProbe.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/UOUCameraControllerComponent.h"
#include "World/Stage/UOUFadeTeleportTriggerActor.h"
#include "TimerManager.h"

namespace
{
	// 실제 릭과 캐시를 함께 기록해 카메라 갱신 지연과 지속 오프셋을 구분한다.
	FString CaptureDescentCamera(ACharacter* Character)
	{
		if (!IsValid(Character)) return TEXT("캐릭터 없음");
		const auto* Camera = Character->FindComponentByClass<UCameraComponent>();
		const auto* Boom = Character->FindComponentByClass<USpringArmComponent>();
		const auto* Rig = Character->FindComponentByClass<UUOUCameraControllerComponent>();
		const auto* PC = Cast<APlayerController>(Character->GetController());
		const FVector Cached = PC && PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
		return FString::Printf(TEXT("캐릭터 %s / 카메라 %s / 캐시 %s / 추적 %s / 누적 %s / 폭 %.1f"),
			*Character->GetActorLocation().ToCompactString(),
			*(Camera ? Camera->GetComponentLocation() : FVector::ZeroVector).ToCompactString(), *Cached.ToCompactString(),
			*(Boom ? Boom->TargetOffset : FVector::ZeroVector).ToCompactString(),
			*(Rig ? Rig->TeleportFollowOffset : FVector::ZeroVector).ToCompactString(), Camera ? Camera->OrthoWidth : 0.0f);
	}
}

bool AUOUIllusionTraversalProbe::ValidateVirtualSupport(UPrimitiveComponent* Surface, const FVector& Direction, float Speed)
{
	if (!bVirtualWalking || Surface == VirtualSupport.Get()) return true;
	// 전방점이 다른 면을 찍어도 실제 발이 바닥에 닿았다면 낙하보다 착지를 먼저 확정한다.
	if (TryFinishVirtualWalking()) return false;
	// 옆으로 벗어나 바닥이 보이더라도 그 깊이로 순간이동하지 않는다.
	// 실제 발 접촉은 앞서 검사했으므로 여기서는 현재 위치와 입력 속도를 보존하고 일반 이동에 넘긴다.
	ReleaseVirtualWalking(Direction, Speed, TEXT("목적면 이탈: 깊이 재이동 없이 일반 이동 복구"));
	return false;
}

void AUOUIllusionTraversalProbe::ReleaseVirtualWalking(const FVector& Direction, float Speed, const TCHAR* ReasonText)
{
	if (ACharacter* Character = VirtualCharacter.Get())
	{
		// 예약 하강이 아니라 연결 실패 낙하였는지도 출력 로그에서 구분한다.
		UE_LOG(LogTemp, Log, TEXT("[IllusionFallback] %s / %s"), ReasonText, *CaptureDescentCamera(Character));
		RestoreVirtualWalking(false);
		auto* Movement = Character->GetCharacterMovement();
		// 판정 불가를 공중 고정으로 처리하지 않는다. 정상 충돌·중력으로 제어권을 돌려준다.
		Movement->SetMovementMode(MOVE_Falling);
		Movement->Velocity = Direction * Speed;
	}
	TextureTraversalReason = ReasonText;
}

bool AUOUIllusionTraversalProbe::RetreatVirtualWalking(const FVector& Direction, float Distance, float Speed)
{
	ACharacter* Character = VirtualCharacter.Get();
	if (!Character || VirtualTrail.Num() < 2) return false;
	const FVector ViewDirection = EntryViewRotation.Vector();
	const FVector Current = Character->GetActorLocation();
	// 깊이 이동 자체는 후퇴 방향 판정에 넣지 않는다. 화면에서 실제로 걸어온 선분만 사용한다.
	FVector BackDirection = FVector::ZeroVector;
	for (int32 Index = VirtualTrail.Num() - 1; Index > 0; --Index)
	{
		BackDirection = FVector::VectorPlaneProject(VirtualTrail[Index - 1] - VirtualTrail[Index], ViewDirection).GetSafeNormal();
		if (!BackDirection.IsNearlyZero()) break;
	}
	if (BackDirection.IsNearlyZero()) BackDirection = FVector::VectorPlaneProject(EntryLocation - Current, ViewDirection).GetSafeNormal();
	if (BackDirection.IsNearlyZero()) BackDirection = -FVector::VectorPlaneProject(EntryTravelDirection, ViewDirection).GetSafeNormal();
	const FVector ScreenDirection = FVector::VectorPlaneProject(Direction, ViewDirection).GetSafeNormal();
	// 대각선·옆걸음 입력을 과거 경로로 강제하지 않는다. 거의 정확한 역방향만 되짚는다.
	if (FVector::DotProduct(ScreenDirection, BackDirection) < 0.985) return false;
	float Remaining = Distance * FVector::VectorPlaneProject(Direction, ViewDirection).Size();
	FVector Next = Current;
	while (VirtualTrail.Num() > 1)
	{
		const FVector Target = VirtualTrail[VirtualTrail.Num() - 2];
		const float Segment = FVector::VectorPlaneProject(Target - Next, ViewDirection).Size();
		if (Segment > Remaining && Segment > 0.01f)
		{
			Next = FMath::Lerp(Next, Target, Remaining / Segment);
			VirtualTrail.Last() = Next;
			break;
		}
		Next = Target;
		Remaining = FMath::Max(0.0f, Remaining - Segment);
		VirtualTrail.Pop();
	}
	Character->SetActorLocation(Next, false, nullptr, ETeleportType::TeleportPhysics);
	LastVirtualLocation = Next;
	Character->GetCharacterMovement()->Velocity = Direction * Speed;
	if (VirtualTrail.Num() == 1)
	{
		RestoreVirtualWalking(false);
		TextureTraversalReason = TEXT("검증된 경로로 후퇴 완료: 출발 발판 보행 복구");
	}
	else TextureTraversalReason = TEXT("이미 지나온 경로로 후퇴 중");
	return true;
}

bool AUOUIllusionTraversalProbe::TryPendingDescent(ACharacter* Character, APlayerController* Controller, const FVector& Direction)
{
	if (!DescentSource.IsValid() || !DescentTarget.IsValid()) return false;
	const auto ClearPending = [this]() { DescentSource.Reset(); DescentTarget.Reset(); };
	const auto& View = Controller->PlayerCameraManager->GetCameraCacheView();
	if (!bEnableTextureAutoTraversal || GetWorld()->GetTimeSeconds() > DescentUntil
		|| !View.Rotation.Equals(DescentView, 0.05f) || Direction.IsNearlyZero()
		|| FVector::DotProduct(Direction, DescentDirection) < 0.95
		|| !AllowedPlatforms.Contains(DescentSource->GetOwner()) || !AllowedPlatforms.Contains(DescentTarget->GetOwner()))
	{
		ClearPending(); return false;
	}
	auto* Movement = Character->GetCharacterMovement();
	if (!Movement->IsMovingOnGround() && !Movement->IsFalling()) { ClearPending(); return false; }
	const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Feet = Character->GetActorLocation() - FVector::UpVector * HalfHeight;
	FVector SourceFeet = Feet;
	// 가장자리에서 일반 이동이 이미 낙하를 시작했어도 출발면 높이에서 발의 이탈을 판정한다.
	SourceFeet.Z = DescentEntry.Z - HalfHeight;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(IllusionDescentEdge), false, Character);
	TArray<AActor*> Attached;
	Character->GetAttachedActors(Attached, true, true);
	Query.AddIgnoredActors(Attached);
	FHitResult SourceHit;
	if (GetWorld()->LineTraceSingleByChannel(SourceHit, SourceFeet + FVector::UpVector * 5,
		SourceFeet - FVector::UpVector * 5, TraceChannel, Query) && SourceHit.GetComponent() == DescentSource.Get()) return false;
	FHitResult Landing;
	// 목적지는 예약 당시 좌표를 맹신하지 않고 빨간 발의 현재 화면 위치에서 다시 확인한다.
	if (!TraceScreenPoint(Controller, Character, Feet, Landing) || Landing.GetComponent() != DescentTarget.Get()
		|| !Movement->IsWalkable(Landing) || Landing.ImpactPoint.Z >= Feet.Z - 5) return false;
	const FVector Velocity = Movement->Velocity;
	// 상승과 같은 시선 방향 보정으로 바닥 여유를 확보한다. 월드 위로 올려 화면이 들썩이지 않게 한다.
	const float Clearance = (UCharacterMovementComponent::MIN_FLOOR_DIST + UCharacterMovementComponent::MAX_FLOOR_DIST) * 0.5f;
	FVector LandingFeet;
	if (!CalculateScreenPreservingFeet(Feet, Landing.ImpactPoint, Landing.ImpactNormal,
		View.Rotation.Vector(), Clearance, LandingFeet)) return false;
	FHitResult SafeGround;
	if (!GetWorld()->LineTraceSingleByChannel(SafeGround, LandingFeet + FVector::UpVector * 2,
		LandingFeet - FVector::UpVector * 3, TraceChannel, Query)
		|| SafeGround.GetComponent() != Landing.GetComponent() || !Movement->IsWalkable(SafeGround)) return false;
	const FVector Destination = LandingFeet + FVector::UpVector * HalfHeight;
	DescentCameraDiagnostics = TEXT("하강 전: ") + CaptureDescentCamera(Character);
	// 수동 순간이동 트리거와 중첩되어 카메라 오프셋이 누적되는 것을 방지한다.
	// 착지 시선은 출발 높이로 덮어쓴 점이 아니라 현재 실제 발을 사용해 화면 위치를 보존한다.
	if (!AUOUFadeTeleportTriggerActor::TeleportForIllusion(Character, Destination, false))
	{
		ClearPending();
		TextureTraversalReason = TEXT("하강 위치 변경 실패: 일반 이동 유지");
		return false;
	}
	Movement->SetMovementMode(MOVE_Walking);
	Movement->Velocity = FVector(Velocity.X, Velocity.Y, 0);
	Movement->bForceNextFloorCheck = true;
	DescentCameraDiagnostics += TEXT("\n하강 직후: ") + CaptureDescentCamera(Character);
	UE_LOG(LogTemp, Log, TEXT("[IllusionDescent] %s"), *DescentCameraDiagnostics);
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,
		[this, WeakCharacter = TWeakObjectPtr<ACharacter>(Character)]()
		{
			DescentCameraDiagnostics += TEXT("\n다음 틱: ") + CaptureDescentCamera(WeakCharacter.Get());
			UE_LOG(LogTemp, Log, TEXT("[IllusionDescent] %s"), *DescentCameraDiagnostics);
		}));
	ClearPending();
	TextureTraversalReason = TEXT("출발면 이탈 후 낮은 발판 도착 확정");
	return true;
}
