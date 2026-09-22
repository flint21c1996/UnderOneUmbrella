#include "World/Stage/UOUIllusionTraversalProbe.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Player/UOUCameraControllerComponent.h"
#include "Player/UOUCharacter.h"

void AUOUIllusionTraversalProbe::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestoreVirtualWalking(EndPlayReason == EEndPlayReason::Destroyed);
	Super::EndPlay(EndPlayReason);
}

void AUOUIllusionTraversalProbe::StopVirtualWalking()
{
	bEnableTextureAutoTraversal = false;
	RestoreVirtualWalking(true);
	DescentSource.Reset();
	DescentTarget.Reset();
}

void AUOUIllusionTraversalProbe::RestoreVirtualWalking(bool bReturnToEntry)
{
	ResetVirtualVisual();
	if (ACharacter* Character = VirtualCharacter.Get())
	{
		auto* Movement = Character->GetCharacterMovement();
		// 충돌을 다시 켜기 전에 복구 위치로 이동한다. 공중 정지 상태에서 해제해도 고립되지 않는다.
		if (bReturnToEntry)
		{
			Character->SetActorLocation(EntryLocation, false, nullptr, ETeleportType::TeleportPhysics);
			// 취소 직전의 입력과 속도로 출발 위치에서 다시 밀려나지 않게 한다.
			Character->ConsumeMovementInputVector();
			Movement->StopMovementImmediately();
		}
		Character->GetCapsuleComponent()->SetCollisionEnabled(SavedCollision);
		if (Movement->MovementMode == MOVE_Custom && Movement->CustomMovementMode == 201)
		{
			Movement->SetMovementMode(static_cast<EMovementMode>(SavedMovementMode), SavedCustomMode);
			Movement->bForceNextFloorCheck = true;
		}
		Movement->SetComponentTickEnabled(bSavedMovementTick);
	}
	bVirtualWalking = false;
	bHasTraversalBoundary = false;
	VirtualCharacter.Reset();
	VirtualSupport.Reset();
	EntryPlatform.Reset();
	bHasVirtualPrediction = false;
	VirtualTrail.Reset();
}

bool AUOUIllusionTraversalProbe::TryFinishVirtualWalking()
{
	ACharacter* Character = VirtualCharacter.Get();
	if (!bVirtualWalking || !Character || !VirtualSupport.IsValid()) return false;
	// 초록 검사점이 아니라 실제 발 바로 아래의 도착 발판을 확인한다. 먼 아래층은 착지로 보지 않는다.
	auto* Capsule = Character->GetCapsuleComponent();
	auto* Movement = Character->GetCharacterMovement();
	const FVector Feet = Capsule->GetComponentLocation() - FVector::UpVector * Capsule->GetScaledCapsuleHalfHeight();
	FHitResult Ground;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(IllusionVirtualArrival), false, Character);
	Query.AddIgnoredActor(this);
	TArray<AActor*> Attached;
	Character->GetAttachedActors(Attached, true, true);
	Query.AddIgnoredActors(Attached);
	if (!GetWorld()->LineTraceSingleByChannel(Ground, Feet + FVector::UpVector * 2,
		Feet - FVector::UpVector * 3, TraceChannel, Query)
		|| !Movement->IsWalkable(Ground) || !AllowedPlatforms.Contains(Ground.GetActor())) return false;
	// 출발면으로 돌아오거나 옆의 등록 발판에 닿아도 실제 바닥이 있으면 가상 보행을 끝낸다.
	// 검사점의 목적면과 실제 발밑면이 같아야 한다는 제한 때문에 충돌이 꺼진 채 남지 않게 한다.
	// 이미 일반 보행 여유 안에 있으면 위치를 다시 올리지 않는다.
	// 여유가 부족한 이전 상태도 시선 방향으로만 보정하고, 보정한 발밑을 다시 확인한다.
	const float Gap = Feet.Z - Ground.ImpactPoint.Z;
	if (Gap < UCharacterMovementComponent::MIN_FLOOR_DIST || Gap > UCharacterMovementComponent::MAX_FLOOR_DIST)
	{
		const auto* PC = Cast<APlayerController>(Character->GetController());
		const FVector ViewDirection = PC && PC->PlayerCameraManager
			? PC->PlayerCameraManager->GetCameraRotation().Vector() : EntryViewRotation.Vector();
		FVector SafeFeet;
		const float Clearance = (UCharacterMovementComponent::MIN_FLOOR_DIST + UCharacterMovementComponent::MAX_FLOOR_DIST) * 0.5f;
		if (!CalculateScreenPreservingFeet(Feet, Ground.ImpactPoint, Ground.ImpactNormal, ViewDirection, Clearance, SafeFeet)) return false;
		FHitResult SafeGround;
		if (!GetWorld()->LineTraceSingleByChannel(SafeGround, SafeFeet + FVector::UpVector * 2,
			SafeFeet - FVector::UpVector * 3, TraceChannel, Query)
			|| SafeGround.GetComponent() != Ground.GetComponent() || !Movement->IsWalkable(SafeGround)) return false;
		Character->SetActorLocation(SafeFeet + FVector::UpVector * Capsule->GetScaledCapsuleHalfHeight(),
			false, nullptr, ETeleportType::TeleportPhysics);
	}
	RestoreVirtualWalking(false);
	TextureTraversalReason = TEXT("도착 발판에 실제 발 접촉: 이동 확정 / 회전해도 위치 유지");
	return true;
}

void AUOUIllusionTraversalProbe::CancelVirtualWalkingForRotation()
{
	if (!bVirtualWalking) return;
	// 자동 이동 설정은 유지한다. 회전이 안정되면 현재 충돌 표면으로 다음 연결을 다시 검사한다.
	RestoreVirtualWalking(true);
	CameraStableTime = 0;
	TextureTraversalReason = TEXT("도착 전 화면 회전: 출발 위치 복귀 / 일반 보행");
}

void AUOUIllusionTraversalProbe::TickVirtualWalking(float DeltaSeconds)
{
#if UOU_WITH_DEVELOPMENT_TOOLS
	bHasVirtualPrediction = false;
	ACharacter* Character = IsValid(TargetCharacter) ? TargetCharacter.Get() : UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!bVirtualWalking) bHasTraversalBoundary = false;
	if (bVirtualWalking && (!bProbeEnabled || !bEnableTextureAutoTraversal || VirtualCharacter.Get() != Character))
	{
		StopVirtualWalking();
		return;
	}
	if (!bProbeEnabled || !Character) return;
	auto* PC = Cast<APlayerController>(Character->GetController());
	if (!PC || !PC->IsLocalController() || !PC->PlayerCameraManager)
	{
		if (bVirtualWalking) StopVirtualWalking();
		return;
	}
	auto* Movement = Character->GetCharacterMovement();
	auto* Capsule = Character->GetCapsuleComponent();
	const auto& View = PC->PlayerCameraManager->GetCameraCacheView();
	// 특수 이동이 제어권을 가져가면 실험 이동을 중단하고 해당 이동 모드는 보존한다.
	if (bVirtualWalking && (Movement->MovementMode != MOVE_Custom || Movement->CustomMovementMode != 201))
	{
		RestoreVirtualWalking(false);
		return;
	}
	if (bVirtualWalking && !Character->GetActorLocation().Equals(LastVirtualLocation, 2.0f))
	{
		// 외부 순간이동이나 다른 이동 시스템의 위치 변경을 가상 보행이 되돌리지 않게 한다.
		RestoreVirtualWalking(false);
		return;
	}
	// 이동 중 착지는 이번 프레임의 이동 뒤에 확정한다. 틱이 꺼진 채 한 프레임을 건너뛰지 않는다.
	const auto* Camera = Character->FindComponentByClass<UUOUCameraControllerComponent>();
	if (bVirtualWalking && (!View.Rotation.Equals(EntryViewRotation, 0.05f)
		|| (Camera && Camera->IsSnapCameraRotationInProgress())))
	{
		// 회전 시에는 기존과 동일하게 실제 착지 확정을 출발 복귀보다 우선한다.
		if (!TryFinishVirtualWalking()) CancelVirtualWalkingForRotation();
		return;
	}
	// 일반 이동과 가상 이동이 동일한 입력 의도를 사용한다. 소비된 입력 큐를 정지로 오인하지 않는다.
	const auto* Player = Cast<AUOUCharacter>(Character);
	const FVector Input = Player ? Player->GetAcceptedMovementInput()
		: (bVirtualWalking ? Character->GetPendingMovementInputVector() : Character->GetLastMovementInputVector());
	if (bVirtualWalking) Character->ConsumeMovementInputVector();
	const bool bHasInput = !Input.GetSafeNormal2D().IsNearlyZero();
	if (bHasInput) LastProbeDirection = Input.GetSafeNormal2D();
	if (LastProbeDirection.IsNearlyZero()) LastProbeDirection = Character->GetActorForwardVector().GetSafeNormal2D();
	const FVector TravelVelocity = bVirtualWalking ? CalculateWalkingVelocity(Movement, Input, DeltaSeconds)
		: FVector(Movement->Velocity.X, Movement->Velocity.Y, 0);
	const float Speed = TravelVelocity.Size2D();
	const bool bHasMotion = bVirtualWalking ? Speed > UE_KINDA_SMALL_NUMBER : bHasInput;
	const FVector Direction = bVirtualWalking && bHasMotion ? TravelVelocity.GetSafeNormal2D() : LastProbeDirection;
	if (bVirtualWalking && !bHasMotion)
	{
		Movement->Velocity = FVector::ZeroVector;
		if (TryFinishVirtualWalking()) return;
	}
	if (bVirtualWalking && bHasMotion && RetreatVirtualWalking(Direction, Speed * FMath::Max(DeltaSeconds, 0.0f), Speed)) return;
	if (!bVirtualWalking && TryPendingDescent(Character, PC, bHasInput ? Direction : FVector::ZeroVector)) return;
	const auto Wait = [this](const TCHAR* Message) { TextureTraversalReason = Message; };
	// 입력이 없어도 마지막 방향으로 표시를 갱신한다. 제동 중에는 남은 속도만큼 이동한다.
	if (View.ProjectionMode != ECameraProjectionMode::Orthographic || CameraStableTime < 0.15f)
	{
		if (bVirtualWalking)
		{
			ReleaseVirtualWalking(Direction, Speed, TEXT("카메라 조건 변경: 일반 이동 복구"));
			return;
		}
		Wait(TEXT("카메라 안정 대기: 일반 보행 유지 / 새 착시 진입 보류")); return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	UPrimitiveComponent* Source = bVirtualWalking ? VirtualSupport.Get() : Movement->CurrentFloor.HitResult.GetComponent();
	if (!Source || !AllowedPlatforms.Contains(Source->GetOwner()))
	{
		ReleaseVirtualWalking(Direction, Speed, TEXT("지지면 해제: 일반 이동 복구")); return;
	}
	if (!bVirtualWalking && (!Movement->IsMovingOnGround() || !Movement->CurrentFloor.IsWalkableFloor())) return;
	const FVector ReferenceNormal = bVirtualWalking ? VirtualNormal : Movement->CurrentFloor.HitResult.ImpactNormal.GetSafeNormal();
	const FVector Feet = Capsule->GetComponentLocation() - FVector::UpVector * Capsule->GetScaledCapsuleHalfHeight();
	if (!bVirtualWalking)
	{
		FHitResult Visible;
		if (!TraceScreenPoint(PC, Character, Feet, Visible) || Visible.GetComponent() != Source
			|| FVector::Dist(Visible.ImpactPoint, Feet) > 15)
		{
			Wait(TEXT("이미 가려진 발: 기존 뒤쪽 보행 유지")); return;
		}
	}
	// 입력 방향은 기존 캐릭터 계산을 재사용하고 현재 프레임의 충돌 표면으로 검사한다.
	// 프레임 시간을 잘라내면 낮은 FPS에서 착시 보행만 느려진다. 경과 시간을 그대로 사용한다.
	const FVector Step = bVirtualWalking && bHasMotion ? TravelVelocity * FMath::Max(DeltaSeconds, 0.0f) : FVector::ZeroVector;
	float SampleDistance = bVirtualWalking ? ActiveProbeDistance : FMath::Max(1.0f, ProbeDistance);
	FVector Expected = Feet + Step + Direction * SampleDistance;
	// 진입 이후에는 출발 경계를 유지하여 실제 발과 경계의 관계를 계속 볼 수 있게 한다.
	if (!bVirtualWalking) UpdateTraversalBoundary(PC, Character, Source, Feet, Expected);
	FHitResult Hit;
	// 캐릭터와 우산 등 부착 액터를 제외한 현재 충돌 표면을 직접 사용한다.
	bool bHitSurface = false;
	if (!bVirtualWalking)
	{
		// 가장 먼저 만난 연결 경계만 후보로 삼는다. 끝점 뒤의 발판으로 건너뛰지 않는다.
		bHitSurface = bHasTraversalBoundary;
		Hit = BoundaryTargetHit;
		SampleDistance = BoundaryTravelDistance;
		Expected = Feet + Direction * SampleDistance;
	}
	else
	{
		bHitSurface = TraceScreenPoint(PC, Character, Expected, Hit);
		if (!bHitSurface || Hit.GetComponent() != VirtualSupport.Get() || !Movement->IsWalkable(Hit))
		{
			// 먼 전방점이 발판 밖이어도 다음 실제 발 위치가 지지되면 현재 표면에서 계속 걷는다.
			Expected = Feet + Step;
			SampleDistance = 0;
			bHitSurface = TraceScreenPoint(PC, Character, Expected, Hit);
		}
	}
	if (!bHitSurface || !Hit.GetComponent()
		|| !AllowedPlatforms.Contains(Hit.GetActor()) || !Movement->IsWalkable(Hit))
	{
		if (bVirtualWalking && TryFinishVirtualWalking()) return;
		if (bHasMotion) ReleaseVirtualWalking(Direction, Speed, TEXT("실제 발 지지면 없음: 입력을 유지하고 일반 이동으로 전환"));
		else Wait(TEXT("정지 위치 유지: 전방 표면 없음"));
		return;
	}
	// 진행 중 다른 면을 새 목적지로 삼아 깊이를 다시 바꾸지 않는다. 완전히 정지했을 때만 표시를 갱신한다.
	if (bHasMotion && !ValidateVirtualSupport(Hit.GetComponent(), Direction, Speed)) return;
	// 발밑과 전방의 충돌 노멀을 비교한다. 재질 노멀이나 과거 프레임에 의존하지 않는다.
	if (FVector::DotProduct(ReferenceNormal, Hit.ImpactNormal.GetSafeNormal()) < NormalAgreement)
	{
		if (bHasMotion) ReleaseVirtualWalking(Direction, Speed, TEXT("다른 경사: 착시 연결 대신 일반 이동"));
		else Wait(TEXT("정지 위치 유지: 전방 경사 불일치"));
		return;
	}
	const FVector Ray = View.Rotation.Vector();
	const FVector ExactSurface = Hit.ImpactPoint;
	CandidateLocation = ExactSurface;
	CandidateNormal = Hit.ImpactNormal.GetSafeNormal();
	CandidatePlatform = Hit.GetActor();
	const float Clearance = (UCharacterMovementComponent::MIN_FLOOR_DIST + UCharacterMovementComponent::MAX_FLOOR_DIST) * 0.5f;
	const FVector RawFeet = SampleDistance > 0 ? CalculateVirtualFeet(ExactSurface, Direction, SampleDistance) : ExactSurface;
	if (!CalculateScreenPreservingFeet(Feet + Step, RawFeet, Hit.ImpactNormal, Ray, Clearance, PredictedFeetLocation))
	{
		if (bHasMotion) ReleaseVirtualWalking(Direction, Speed, TEXT("시선과 표면이 평행: 일반 이동 유지"));
		return;
	}
	bHasVirtualPrediction = true;
	if (!bHasMotion) { Wait(TEXT("정지 중 검사점 표시: 실제 이동 없음")); return; }
	if (!bVirtualWalking && (Hit.GetComponent() == Source
		|| FMath::Abs(CalculateDepthGap(Expected, ExactSurface, Ray)) < MinimumDepthGap))
	{
		Wait(TEXT("같은 지지면 또는 작은 깊이 차이: 기존 보행 유지")); return;
	}
	if (!bEnableTextureAutoTraversal) { Wait(TEXT("초록 지지점·보라 예상 발 확인 / 자동 이동 꺼짐")); return; }
	if (!bVirtualWalking && ExactSurface.Z < Feet.Z - 5.0f)
	{
		// 내려갈 곳은 예약만 한다. 앞쪽 초록 점이 아니라 실제 빨간 발이 출발면을 벗어날 때 옮긴다.
		DescentSource = Source;
		DescentTarget = Hit.GetComponent();
		DescentEntry = Character->GetActorLocation();
		DescentDirection = Direction;
		DescentView = View.Rotation;
		DescentUntil = Now + 0.25;
		Wait(TEXT("내려가기 예약: 실제 발이 출발 발판을 벗어날 때 전환"));
		return;
	}
	if (!bVirtualWalking)
	{
		// 다음 프레임에 걸을 구간까지 포함하여 경계에 닿기 직전에만 깊이를 전환한다.
		// 전방 검사 거리를 늘려도 진입 시점은 앞당겨지지 않는다.
		const float NextTravel = CalculateWalkingVelocity(Movement, Input, DeltaSeconds).Size2D() * FMath::Max(DeltaSeconds, 0.0f);
		if (!IsEntryBoundaryReached(NextTravel))
		{
			Wait(TEXT("연결 후보 확보: 발이 진입 경계에 접근할 때까지 일반 보행"));
			return;
		}
		// 일반 이동 설정은 진입할 때만 저장한다. 이후에는 중력과 충돌 보정이 가상 발 위치를 덮어쓰지 않는다.
		VirtualCharacter = Character;
		EntryLocation = Character->GetActorLocation();
		EntryTravelDirection = Direction;
		EntryPlatform = Source->GetOwner();
		EntryViewRotation = View.Rotation;
		SavedMovementMode = Movement->MovementMode;
		SavedCustomMode = Movement->CustomMovementMode;
		bSavedMovementTick = Movement->IsComponentTickEnabled();
		SavedCollision = Capsule->GetCollisionEnabled();
		Movement->SetMovementMode(MOVE_Custom, 201);
		Movement->SetComponentTickEnabled(false);
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		bVirtualWalking = true;
		ActiveProbeDistance = SampleDistance;
		VirtualTrail.Reset();
		VirtualTrail.Add(EntryLocation);
	}
	// 한 번의 연결은 처음 선택한 목적면만 사용한다.
	if (!VirtualSupport.IsValid()) VirtualSupport = Hit.GetComponent();
	VirtualNormal = Hit.ImpactNormal.GetSafeNormal();
	// 순간이동용 카메라 오프셋은 호출하지 않는다. 깊이 이동을 속도에 섞지 않아 걷기 애니메이션을 보존한다.
	const FVector Center = PredictedFeetLocation + FVector::UpVector * Capsule->GetScaledCapsuleHalfHeight();
	Character->SetActorLocation(Center, false, nullptr, ETeleportType::TeleportPhysics);
	LastVirtualLocation = Center;
	if (VirtualTrail.Num() == 1 || FVector::Dist(VirtualTrail.Last(), Center) >= 1.0f) VirtualTrail.Add(Center);
	Movement->Velocity = Direction * Speed;
	if (Movement->bOrientRotationToMovement)
		Character->SetActorRotation(FMath::RInterpTo(Character->GetActorRotation(), Direction.Rotation(), DeltaSeconds, 10));
	Wait(TEXT("가상 발 보행: 등록된 현재 충돌 표면 사용"));
	if (VirtualTrail.Num() > 8192) { ReleaseVirtualWalking(Direction, Speed, TEXT("경로 기록 한도: 일반 이동 복귀")); return; }
	TryFinishVirtualWalking();
#endif
}
