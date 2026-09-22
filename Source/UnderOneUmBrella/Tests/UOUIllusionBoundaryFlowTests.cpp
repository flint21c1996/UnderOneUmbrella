#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "World/Stage/UOUIllusionTraversalProbe.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUOUIllusionWalkingVelocityTest,
	"UnderOneUmbrella.IllusionProbe.WalkingAccelerationAndBraking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUIllusionWalkingVelocityTest::RunTest(const FString& Parameters)
{
	const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("IllusionVelocityWorld"), nullptr, false, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("속도 테스트 월드"), World)) return false;
	auto* Character = World->SpawnActor<ACharacter>();
	auto* Move = Character->GetCharacterMovement();
	Move->GroundFriction = 2;
	Move->BrakingDecelerationWalking = 100;
	Move->MaxWalkSpeed = 300;
	Move->MaxAcceleration = 600;
	Move->Velocity = FVector(200, 0, 0);
	const FVector Before = Move->Velocity;
	const FVector Released = AUOUIllusionTraversalProbe::CalculateWalkingVelocity(Move, FVector::ZeroVector, 0.016f);
	TestTrue(TEXT("키 해제 한 프레임에 강제 정지하지 않음"), Released.X > 0 && Released.X < Before.X);
	TestTrue(TEXT("속도 계산만으로 이동 상태를 덮어쓰지 않음"), Move->Velocity.Equals(Before));
	Move->SetMovementMode(MOVE_Walking);
	Move->CalcVelocity(0.016f, Move->GroundFriction, false, Move->BrakingDecelerationWalking);
	TestTrue(TEXT("일반 보행의 엔진 제동 결과와 일치"), Move->Velocity.Equals(Released, 0.001));
	const FVector Resumed = AUOUIllusionTraversalProbe::CalculateWalkingVelocity(Move, FVector::ForwardVector, 0.016f);
	TestTrue(TEXT("짧게 떼었다 눌러도 기존 속도부터 가속"), Resumed.X > Released.X);
	Move->Velocity = FVector(300, 0, 0);
	const FVector Diagonal = AUOUIllusionTraversalProbe::CalculateWalkingVelocity(Move, FVector(1, 1, 0).GetSafeNormal(), 0.016f);
	TestTrue(TEXT("대각선 입력의 측면 성분 보존"), Diagonal.Y > 0);
	TestTrue(TEXT("대각선 최고 속도 제한"), Diagonal.Size() <= 300.001);
	for (int32 Step = 0; Step < 120; ++Step)
		Move->Velocity = AUOUIllusionTraversalProbe::CalculateWalkingVelocity(Move, FVector::ZeroVector, 0.016f);
	TestTrue(TEXT("무입력 제동은 결국 완전히 정지"), Move->Velocity.IsNearlyZero());
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUOUIllusionBoundaryFlowTest,
	"UnderOneUmbrella.IllusionProbe.BoundaryEntrySupportAndDescent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUIllusionBoundaryFlowTest::RunTest(const FString& Parameters)
{
	const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("IllusionBoundaryWorld"), nullptr, false, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("경계 통합 테스트 월드"), World)) return false;
	const auto MakeFloor = [World](const FVector& Location, const FVector& Extent)
	{
		auto* Actor = World->SpawnActor<AActor>();
		auto* Box = NewObject<UBoxComponent>(Actor);
		Actor->SetRootComponent(Box);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Box->RegisterComponent();
		Actor->SetActorLocation(Location);
		return Box;
	};
	// 화면에서 Y=100을 경계로 이어지지만 실제 높이는 500cm 다른 두 발판이다.
	auto* Low = MakeFloor(FVector(0, -950, -10), FVector(2000, 1050, 10));
	auto* High = MakeFloor(FVector(-1000, 600, 490), FVector(800, 500, 10));
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Character = World->SpawnActor<ACharacter>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	auto* PC = World->SpawnActor<APlayerController>();
	PC->SetAsLocalPlayerController();
	auto* Probe = World->SpawnActor<AUOUIllusionTraversalProbe>();
	PC->Possess(Character);
	PC->PlayerCameraManager = World->SpawnActor<APlayerCameraManager>();
	FMinimalViewInfo View;
	View.ProjectionMode = ECameraProjectionMode::Orthographic;
	View.Rotation = FRotator(-30, 0, 0);
	PC->PlayerCameraManager->SetCameraCachePOV(View);
	auto* Move = Character->GetCharacterMovement();
	const float Half = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float Clearance = (UCharacterMovementComponent::MIN_FLOOR_DIST + UCharacterMovementComponent::MAX_FLOOR_DIST) * 0.5f;
	Probe->TargetCharacter = Character;
	Probe->AllowedPlatforms = {Low->GetOwner(), High->GetOwner()};
	Probe->bEnableTextureAutoTraversal = true;
	Probe->ProbeDistance = 200;
	Probe->CameraStableTime = 1;
	bool bGap = false;
	Probe->TestScreenTrace = [Probe, Character, View, &bGap](const FVector& Point, FHitResult& Hit)
	{
		// 화면 역투영 대신 동일한 직교 평행선을 만들며, 교차 계산은 실제 월드 충돌에 맡긴다.
		if (bGap && Point.Y >= 30 && Point.Y <= 50) return false;
		return Probe->TraceViewRay(Character, Point - View.Rotation.Vector() * 5000, View.Rotation.Vector(), Hit);
	};
	const auto SetLowFeet = [&](float Y)
	{
		Character->SetActorLocation(FVector(0, Y, Half + Clearance));
		Move->SetMovementMode(MOVE_Walking);
		FHitResult Ground;
		Probe->TraceViewRay(Character, FVector(0, Y, 50), -FVector::UpVector, Ground);
		Move->CurrentFloor.HitResult = Ground;
		Move->CurrentFloor.bBlockingHit = true;
		Move->CurrentFloor.bWalkableFloor = true;
		Move->Velocity = FVector(0, 200, 0);
		Character->AddMovementInput(FVector::RightVector, 1, true);
		Character->ConsumeMovementInputVector();
	};
	SetLowFeet(0);
	AddInfo(FString::Printf(TEXT("초기 조건: 로컬 %d / 컨트롤러 %s / 지상 %d / 바닥 %s / 입력 %s / 직교 %d"),
		PC->IsLocalController(), *GetNameSafe(Character->GetController()), Move->IsMovingOnGround(),
		*GetNameSafe(Move->CurrentFloor.HitResult.GetComponent()), *Character->GetLastMovementInputVector().ToString(),
		PC->PlayerCameraManager->GetCameraCacheView().ProjectionMode == ECameraProjectionMode::Orthographic));
	Probe->TickVirtualWalking(0.016f);
	AddInfo(TEXT("첫 진입 판정: ") + Probe->TextureTraversalReason);
	TestTrue(TEXT("멀리서도 연결 경계는 발견"), Probe->bHasTraversalBoundary);
	TestFalse(TEXT("후보 발견만으로 조기 상승하지 않음"), Probe->bVirtualWalking);
	TestTrue(TEXT("경계 전에는 원래 높이 유지"), FMath::IsNearlyEqual(Character->GetActorLocation().Z, Half + Clearance, 0.001f));
	bGap = true;
	Probe->TickVirtualWalking(0.016f);
	TestFalse(TEXT("중간 빈 공간을 넘어 연결하지 않음"), Probe->bHasTraversalBoundary || Probe->bVirtualWalking);
	bGap = false;
	SetLowFeet(95);
	const FVector BeforeEntry = Character->GetActorLocation();
	Probe->TickVirtualWalking(0.016f);
	TestTrue(TEXT("경계에 접근하면 지속 입력으로 상승 진입"), Probe->bVirtualWalking);
	AddInfo(TEXT("근접 진입 판정: ") + Probe->TextureTraversalReason);
	TestTrue(TEXT("상승 진입 순간 화면 좌표 보존"), FVector::VectorPlaneProject(
		Character->GetActorLocation() - BeforeEntry, View.Rotation.Vector()).IsNearlyZero(0.001));
	for (int32 Step = 0; Step < 5 && Probe->bVirtualWalking; ++Step)
	{
		Character->AddMovementInput(FVector::RightVector, 1, true);
		Probe->TickVirtualWalking(0.016f);
	}
	TestFalse(TEXT("실제 발이 높은 발판에 닿으면 가상 보행 종료"), Probe->bVirtualWalking);
	TestTrue(TEXT("상승 완료 후 일반 걷기 복구"), Move->IsMovingOnGround());
	TestTrue(TEXT("상승 완료 때 속도를 0으로 만들지 않음"), Move->Velocity.Y > 0);

	// 먼 전방점은 밖이지만 발의 화면 위치에는 목적면이 남아 있는 상황이다.
	Probe->VirtualCharacter = Character;
	Probe->VirtualSupport = High;
	Probe->bVirtualWalking = true;
	Probe->EntryViewRotation = View.Rotation;
	Probe->VirtualNormal = FVector::UpVector;
	Probe->ActiveProbeDistance = 1500;
	Probe->VirtualTrail.Reset();
	Character->SetActorLocation(FVector(-2400, 500, Half + 1200));
	Probe->LastVirtualLocation = Character->GetActorLocation();
	Probe->VirtualTrail.Add(Probe->LastVirtualLocation);
	Move->SetMovementMode(MOVE_Custom, 201);
	Move->SetComponentTickEnabled(false);
	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Move->Velocity = FVector(0, 200, 0);
	Character->AddMovementInput(FVector::RightVector, 1, true);
	Probe->TickVirtualWalking(0.016f);
	TestFalse(TEXT("전방점 이탈만으로 낙하하지 않음"), Move->IsFalling());
	TestTrue(TEXT("전방점 이탈 프레임에도 전진 유지"), Character->GetActorLocation().Y > 500);
	Probe->RestoreVirtualWalking(false);

	// 내려가기는 실제 발이 높은 출발면 밖으로 나간 뒤 낮은 면으로 확정한다.
	Character->SetActorLocation(FVector(-866, 99, Half + 500 + Clearance));
	Move->SetMovementMode(MOVE_Walking);
	Move->Velocity = FVector(0, -200, 0);
	Probe->DescentSource = High;
	Probe->DescentTarget = Low;
	Probe->DescentEntry = FVector(-866, 101, Half + 500 + Clearance);
	Probe->DescentDirection = -FVector::RightVector;
	Probe->DescentView = View.Rotation;
	Probe->DescentUntil = World->GetTimeSeconds() + 1;
	const FVector BeforeDescent = Character->GetActorLocation();
	FHitResult DebugLanding;
	const bool bDebugLanding = Probe->TraceScreenPoint(PC, Character, BeforeDescent - FVector::UpVector * Half, DebugLanding);
	AddInfo(FString::Printf(TEXT("하강 시선: 충돌 %d / 표면 %s / 위치 %s / 모드 %d"), bDebugLanding,
		*GetNameSafe(DebugLanding.GetComponent()), *DebugLanding.ImpactPoint.ToString(), static_cast<int32>(Move->MovementMode)));
	TestTrue(TEXT("실제 경계 이탈 후 하강 확정"), Probe->TryPendingDescent(Character, PC, -FVector::RightVector));
	TestTrue(TEXT("하강 보정도 화면상 들썩임 없음"), FVector::VectorPlaneProject(
		Character->GetActorLocation() - BeforeDescent, View.Rotation.Vector()).IsNearlyZero(0.001));
	TestTrue(TEXT("하강 후 속도 유지"), Move->Velocity.Equals(FVector(0, -200, 0)));

	// 카메라 좌우뿐 아니라 시선 반대 방향(뒤→앞)도 검증한다. 두 윗면이 맞닿는 뒤쪽 경계를 사용한다.
	Low->SetBoxExtent(FVector(1050, 2000, 10));
	Low->GetOwner()->SetActorLocation(FVector(2050, 0, -10));
	High->SetBoxExtent(FVector(500, 800, 10));
	High->GetOwner()->SetActorLocation(FVector(-266, 0, 490));
	Character->SetActorLocation(FVector(1102, 0, Half + Clearance));
	Move->SetMovementMode(MOVE_Walking);
	FHitResult ForwardGround;
	Probe->TraceViewRay(Character, FVector(1102, 0, 50), -FVector::UpVector, ForwardGround);
	Move->CurrentFloor.HitResult = ForwardGround;
	Move->CurrentFloor.bBlockingHit = true;
	Move->CurrentFloor.bWalkableFloor = true;
	Move->Velocity = FVector(-200, 0, 0);
	Character->AddMovementInput(-FVector::ForwardVector, 1, true);
	Character->ConsumeMovementInputVector();
	const FVector BeforeForwardEntry = Character->GetActorLocation();
	Probe->TickVirtualWalking(0.016f);
	TestTrue(TEXT("뒤에서 앞으로 지속 입력 상승 진입"), Probe->bVirtualWalking);
	TestTrue(TEXT("뒤에서 앞으로 진입할 때도 화면 위치 보존"), FVector::VectorPlaneProject(
		Character->GetActorLocation() - BeforeForwardEntry, View.Rotation.Vector()).IsNearlyZero(0.001));
	for (int32 Step = 0; Step < 8 && Probe->bVirtualWalking; ++Step)
	{
		Character->AddMovementInput(-FVector::ForwardVector, 1, true);
		Probe->TickVirtualWalking(0.016f);
	}
	TestTrue(TEXT("뒤에서 앞으로 올라온 뒤 실제 착지 확정"), !Probe->bVirtualWalking && Move->IsMovingOnGround());
	TestTrue(TEXT("뒤에서 앞으로 이동해도 속도 유지"), Move->Velocity.X < 0);
	Probe->TestScreenTrace = nullptr;
	World->DestroyWorld(false);
	return true;
}
#endif
