#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "World/Stage/UOUIllusionTraversalProbe.h"
#include "Player/UOUCharacter.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "Components/BoxComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUOUIllusionExistingBlueprintTest,
	"UnderOneUmbrella.IllusionProbe.ExistingBlueprintLoadsWithoutBuffers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUIllusionExistingBlueprintTest::RunTest(const FString& Parameters)
{
	// 기존 BP를 수정·저장하지 않고 로드하여 부모 속성 정리 후에도 사용 가능한지 검증한다.
	UClass* ProbeClass = LoadClass<AUOUIllusionTraversalProbe>(nullptr,
		TEXT("/Game/UOU/BluePrint/World/Stage/BP_UOU_IllusionTraversalProbe.BP_UOU_IllusionTraversalProbe_C"));
	if (!TestNotNull(TEXT("기존 착시 검사 BP 로드"), ProbeClass)) return false;
	TestNotNull(TEXT("기존 자동 이동 저장 속성 유지"), ProbeClass->FindPropertyByName(TEXT("bEnableTextureAutoTraversal")));
	TestNotNull(TEXT("기존 허용 발판 목록 속성 유지"), ProbeClass->FindPropertyByName(TEXT("AllowedPlatforms")));
	TestNull(TEXT("텍스처 미리보기 옵션 제거"), ProbeClass->FindPropertyByName(TEXT("bShowBufferPreview")));
	TestNull(TEXT("과거 텍스처 대기 옵션 제거"), ProbeClass->FindPropertyByName(TEXT("MaximumTextureAge")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUOUIllusionCollisionOnlyTest,
	"UnderOneUmbrella.IllusionProbe.CollisionOnlyCurrentSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUIllusionCollisionOnlyTest::RunTest(const FString& Parameters)
{
	// 카메라 뷰포트·텍스처·렌더러 없이 실제 충돌 질의에서 위치와 노멀을 얻는다.
	const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("IllusionCollisionOnlyWorld"),
		nullptr, false, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("충돌 전용 테스트 월드"), World)) return false;
	const auto MakeBox = [World](const FVector& Position, const FVector& Extent)
	{
		auto* Actor = World->SpawnActor<AActor>();
		auto* Box = NewObject<UBoxComponent>(Actor);
		Actor->SetRootComponent(Box);
		Box->SetMobility(EComponentMobility::Movable);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Box->RegisterComponent();
		Actor->SetActorLocation(Position);
		return Actor;
	};
	auto* Floor = MakeBox(FVector(0, 0, -10), FVector(100, 100, 10));
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Character = World->SpawnActor<ACharacter>(FVector(0, 0, 300), FRotator::ZeroRotator, Spawn);
	auto* Probe = World->SpawnActor<AUOUIllusionTraversalProbe>();
	if (!Character || !Probe) { World->DestroyWorld(false); return false; }
	Probe->AllowedPlatforms.Add(Floor);
	// 시선 위에 캐릭터와 부착물을 두어 자기 가림이 발판 판정을 덮지 않는지 확인한다.
	auto* Attached = MakeBox(FVector(0, 0, 200), FVector(30, 30, 10));
	Attached->AttachToActor(Character, FAttachmentTransformRules::KeepWorldTransform);
	FHitResult Hit;
	const FVector Origin(0, 0, 1000), Direction(0, 0, -1);
	TestTrue(TEXT("렌더링 없이 표면 검사 성공"), Probe->TraceViewRay(Character, Origin, Direction, Hit));
	TestTrue(TEXT("캐릭터와 부착물 대신 바닥 검출"), Hit.GetActor() == Floor);
	TestTrue(TEXT("현재 충돌 월드 좌표 사용"), Hit.ImpactPoint.Equals(FVector::ZeroVector, 0.001));
	TestTrue(TEXT("텍스처 없이 충돌 노멀 사용"), Hit.ImpactNormal.Equals(FVector::UpVector, 0.001));
	TestTrue(TEXT("충돌 노멀로 보행 가능한 윗면 판정"), Character->GetCharacterMovement()->IsWalkable(Hit));
	TestEqual(TEXT("충돌 위치에서 시선 깊이 계산"),
		AUOUIllusionTraversalProbe::CalculateDepthGap(Origin, Hit.ImpactPoint, Direction), 1000.0f);
	// 같은 게임 프레임에 발판을 옮겨도 과거 텍스처 좌표가 남지 않아야 한다.
	Floor->SetActorLocation(FVector(0, 0, 90));
	TestTrue(TEXT("움직인 발판 즉시 재검사"), Probe->TraceViewRay(Character, Origin, Direction, Hit));
	TestTrue(TEXT("이동 직후의 표면 좌표 반영"), Hit.GetActor() == Floor && Hit.ImpactPoint.Equals(FVector(0, 0, 100), 0.001));
	// 미등록 장애물을 관통해서 뒤의 등록 발판을 선택하지 않는다.
	auto* Obstacle = MakeBox(FVector(0, 0, 600), FVector(50, 50, 10));
	TestTrue(TEXT("앞쪽 장애물 검출"), Probe->TraceViewRay(Character, Origin, Direction, Hit));
	TestTrue(TEXT("첫 충돌을 그대로 반환"), Hit.GetActor() == Obstacle);
	TestFalse(TEXT("미등록 장애물은 허용 발판이 아님"), Probe->AllowedPlatforms.Contains(Hit.GetActor()));
	TestFalse(TEXT("표면 없는 시선은 실패"), Probe->TraceViewRay(Character, FVector(500, 0, 1000), Direction, Hit));
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUOUIllusionGroundReturnTest,
	"UnderOneUmbrella.IllusionProbe.PhysicalGroundEndsVirtualWalking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUIllusionGroundReturnTest::RunTest(const FString& Parameters)
{
	const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("IllusionGroundReturnWorld"),
		nullptr, false, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("실제 충돌 바닥 테스트 월드"), World)) return false;
	auto* Floor = World->SpawnActor<AActor>();
	auto* Box = NewObject<UBoxComponent>(Floor);
	Floor->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(1000, 1000, 10));
	Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Box->SetCollisionResponseToAllChannels(ECR_Block);
	Box->RegisterComponent();
	Floor->SetActorLocation(FVector(0, 0, -10));
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Character = World->SpawnActor<ACharacter>(FVector(0, 0, 300), FRotator::ZeroRotator, Spawn);
	auto* Probe = World->SpawnActor<AUOUIllusionTraversalProbe>();
	Probe->AllowedPlatforms.Add(Floor);
	Probe->VirtualCharacter = Character;
	Probe->VirtualSupport = Box;
	Probe->EntryPlatform = Floor;
	Probe->bVirtualWalking = true;
	Probe->SavedMovementMode = MOVE_Walking;
	Probe->bSavedMovementTick = true;
	Probe->SavedCollision = ECollisionEnabled::QueryAndPhysics;
	auto* Movement = Character->GetCharacterMovement();
	Movement->SetMovementMode(MOVE_Custom, 201);
	Movement->SetComponentTickEnabled(false);
	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	const double HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Character->SetActorLocation(FVector(0, 0, HalfHeight + 50));
	TestFalse(TEXT("먼 아래층은 실제 발 접촉으로 보지 않음"), Probe->TryFinishVirtualWalking());
	// 출발면으로 옆걸음 복귀한 상황을 실제 충돌 바닥으로 재현한다.
	Character->SetActorLocation(FVector(0, 0, HalfHeight + 2));
	Movement->Velocity = FVector(120, 60, 0);
	TestTrue(TEXT("출발 발판에 실제 발이 닿으면 일반 보행 복구"), Probe->TryFinishVirtualWalking());
	TestFalse(TEXT("출발 바닥에서 가상 보행 고착 방지"), Probe->bVirtualWalking);
	TestTrue(TEXT("일반 이동 틱 복구"), Movement->IsComponentTickEnabled());
	TestTrue(TEXT("걷기 모드 복구"), Movement->IsMovingOnGround());
	TestTrue(TEXT("입력 속도 보존"), Movement->Velocity.Equals(FVector(120, 60, 0)));
	TestTrue(TEXT("바닥에서 위치가 뛰지 않음"), Character->GetActorLocation().Equals(FVector(0, 0, HalfHeight + 2)));
	// 기존 구현은 표면에 딱 붙인 발을 착지 시 월드 위로 2cm 올려 화면에서도 들썩였다.
	Probe->VirtualCharacter = Character;
	Probe->VirtualSupport = Box;
	Probe->bVirtualWalking = true;
	Probe->EntryViewRotation = FRotator(-30, 0, 0);
	Movement->SetMovementMode(MOVE_Custom, 201);
	Movement->SetComponentTickEnabled(false);
	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	const FVector BeforeLanding(0, 0, HalfHeight);
	Character->SetActorLocation(BeforeLanding);
	Movement->Velocity = FVector(120, 60, 0);
	TestTrue(TEXT("여유 없는 가상 발도 착지 가능"), Probe->TryFinishVirtualWalking());
	const FVector LandingDelta = Character->GetActorLocation() - BeforeLanding;
	TestTrue(TEXT("착지 보정은 화면 수평·수직 위치를 보존"),
		FVector::VectorPlaneProject(LandingDelta, Probe->EntryViewRotation.Vector()).IsNearlyZero(0.001));
	TestTrue(TEXT("착지 후에도 수평 속도 보존"), Movement->Velocity.Equals(FVector(120, 60, 0)));
	// 공중에서 다른 등록 표면을 찍어도 그 표면 깊이로 재배치하지 않는다.
	auto* Other = NewObject<UBoxComponent>(Floor);
	Probe->VirtualCharacter = Character;
	Probe->VirtualSupport = Box;
	Probe->bVirtualWalking = true;
	Probe->bHasTraversalBoundary = true;
	Movement->SetMovementMode(MOVE_Custom, 201);
	Movement->SetComponentTickEnabled(false);
	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	const FVector BeforeSideExit(300, 0, HalfHeight + 200);
	Character->SetActorLocation(BeforeSideExit);
	TestTrue(TEXT("동일 목적면은 가상 보행 유지"), Probe->ValidateVirtualSupport(Box, FVector::RightVector, 140));
	TestFalse(TEXT("새 목적면으로 재투영 금지"), Probe->ValidateVirtualSupport(Other, FVector::RightVector, 140));
	TestTrue(TEXT("목적면 이탈 시 현재 위치 보존"), Character->GetActorLocation().Equals(BeforeSideExit));
	TestFalse(TEXT("목적면 이탈 시 공중 고정 해제"), Probe->bVirtualWalking);
	TestTrue(TEXT("목적면 이탈 시 일반 이동과 중력 복구"), Movement->IsComponentTickEnabled() && Movement->IsFalling());
	TestTrue(TEXT("목적면 이탈 시 이동 속도 보존"), Movement->Velocity.Equals(FVector(0, 140, 0)));
	TestFalse(TEXT("이전 연결 경계 표시 제거"), Probe->bHasTraversalBoundary);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUOUIllusionInputTest,
	"UnderOneUmbrella.IllusionProbe.InputSurvivesMovementConsumption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUIllusionInputTest::RunTest(const FString& Parameters)
{
	const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("IllusionInputWorld"),
		nullptr, false, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("입력 테스트 월드"), World)) return false;
	auto* Character = World->SpawnActor<AUOUCharacter>();
	auto* Controller = World->SpawnActor<APlayerController>();
	if (!Character || !Controller) { AddError(TEXT("입력 테스트 액터 생성 실패")); World->DestroyWorld(false); return false; }
	Controller->Possess(Character);
	// 실제 이동 입력 처리 함수를 거친 뒤 엔진 입력 큐를 먼저 소비하는 순서를 재현한다.
	Character->Move(FInputActionValue(FVector2D(0, -1)));
	const FVector Held = Character->GetAcceptedMovementInput();
	TestFalse(TEXT("S 입력 허용"), Held.IsNearlyZero());
	Character->ConsumeMovementInputVector();
	TestTrue(TEXT("입력 소비 이후에도 같은 이동 의도"), Character->GetAcceptedMovementInput().Equals(Held));
	Character->GetCharacterMovement()->SetMovementMode(MOVE_Custom, 201);
	Character->GetCharacterMovement()->SetComponentTickEnabled(false);
	for (int32 Index = 0; Index < 120; ++Index)
	{
		Character->Move(FInputActionValue(FVector2D(0, -1)));
		Character->ConsumeMovementInputVector();
		TestTrue(TEXT("가상 보행 중 지속 입력"), Character->GetAcceptedMovementInput().Equals(Held));
	}
	Character->Move(FInputActionValue(FVector2D::ZeroVector));
	TestTrue(TEXT("키 해제 즉시 정지"), Character->GetAcceptedMovementInput().IsNearlyZero());
	Character->Move(FInputActionValue(FVector2D(0, -1)));
	Character->AcceptedMovementInputFrame = GFrameCounter - 1;
	TestTrue(TEXT("이전 프레임 입력 재사용 방지"), Character->GetAcceptedMovementInput().IsNearlyZero());
	Character->Move(FInputActionValue(FVector2D(0, -1)));
	Controller->SetIgnoreMoveInput(true);
	TestTrue(TEXT("입력 차단 존중"), Character->GetAcceptedMovementInput().IsNearlyZero());
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUOUIllusionProbeDepthTest,
	"UnderOneUmbrella.IllusionProbe.DepthConvention",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUIllusionProbeDepthTest::RunTest(const FString& Parameters)
{
	// 화면 평면 안의 이동과 카메라 시선 방향의 깊이 이동을 구분한다.
	const FVector Origin(10, 20, 30);
	TestEqual(TEXT("시선 앞으로 이동한 깊이"), AUOUIllusionTraversalProbe::CalculateDepthGap(Origin, Origin + FVector(100, 0, 0), FVector(2, 0, 0)), 100.0f);
	TestEqual(TEXT("시선 뒤로 이동한 깊이"), AUOUIllusionTraversalProbe::CalculateDepthGap(Origin, Origin - FVector(100, 0, 0), FVector::ForwardVector), -100.0f);
	TestEqual(TEXT("화면 평면 이동은 깊이 변화 없음"), AUOUIllusionTraversalProbe::CalculateDepthGap(Origin, Origin + FVector(0, 100, 50), FVector::ForwardVector), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUOUIllusionProbeSafetyTest,
	"UnderOneUmbrella.IllusionProbe.DiagnosticDoesNotMovePlayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUIllusionProbeSafetyTest::RunTest(const FString& Parameters)
{
	const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("IllusionProbeTestWorld"),
		nullptr, false, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("진단 테스트 월드"), World)) return false;
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Character = World->SpawnActor<ACharacter>(FVector(100, 200, 1000), FRotator::ZeroRotator, Spawn);
	auto* Probe = World->SpawnActor<AUOUIllusionTraversalProbe>();
	if (!Character || !Probe)
	{
		AddError(TEXT("테스트 액터 생성 실패"));
		World->DestroyWorld(false);
		return false;
	}
	Probe->TargetCharacter = Character;
	TestFalse(TEXT("자동 이동은 명시적으로 켜야 함"), Probe->bEnableTextureAutoTraversal);
	const FVector Position = Character->GetActorLocation();
	const FVector Velocity(120, 30, 0);
	Character->GetCharacterMovement()->Velocity = Velocity;
	Probe->Tick(0.016f);
	TestTrue(TEXT("카메라 없는 진단은 보류"), Probe->Status == EUOUIllusionProbeStatus::NoPlayer);
	TestTrue(TEXT("진단은 위치를 바꾸지 않음"), Character->GetActorLocation().Equals(Position));
	TestTrue(TEXT("진단은 속도를 바꾸지 않음"), Character->GetCharacterMovement()->Velocity.Equals(Velocity));
	Probe->bProbeEnabled = false;
	Probe->Tick(0.016f);
	TestTrue(TEXT("진단 비활성 상태"), Probe->Status == EUOUIllusionProbeStatus::Disabled);
	TestNull(TEXT("이전 후보 참조 제거"), Probe->CandidatePlatform.Get());
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUOUIllusionVirtualFeetTest,
	"UnderOneUmbrella.IllusionProbe.VirtualFeetOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUIllusionVirtualFeetTest::RunTest(const FString& Parameters)
{
	// 캐릭터 위치는 카메라 뒤로 밀지 않고 진행 방향의 검사 간격만큼 되돌린다.
	const FVector Surface(100, 200, 300);
	TestTrue(TEXT("진행 방향 뒤의 발 위치"), AUOUIllusionTraversalProbe::CalculateVirtualFeet(Surface, FVector(2, 0, 0), 50).Equals(FVector(50, 200, 300)));
	TestTrue(TEXT("입력 크기에 따라 검사 간격이 변하지 않음"), AUOUIllusionTraversalProbe::CalculateVirtualFeet(Surface, FVector(0.2, 0, 0), 50).Equals(FVector(50, 200, 300)));
	TestTrue(TEXT("반대 입력의 발 위치"), AUOUIllusionTraversalProbe::CalculateVirtualFeet(Surface, FVector(-1, 0, 0), 50).Equals(FVector(150, 200, 300)));
	TestTrue(TEXT("정지 방향은 지지점 유지"), AUOUIllusionTraversalProbe::CalculateVirtualFeet(Surface, FVector::ZeroVector, 50).Equals(Surface));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUOUIllusionAscentScreenTest,
	"UnderOneUmbrella.IllusionProbe.AscentScreenPositionAndSpeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUIllusionAscentScreenTest::RunTest(const FString& Parameters)
{
	const float Clearance = (UCharacterMovementComponent::MIN_FLOOR_DIST + UCharacterMovementComponent::MAX_FLOOR_DIST) * 0.5f;
	const FVector View = FRotator(-30, 0, 0).Vector();
	const FVector Start(0, 0, Clearance);
	const FVector Plane(0, 0, 500);
	FVector Feet;
	TestTrue(TEXT("상승 깊이 보정 계산"), AUOUIllusionTraversalProbe::CalculateScreenPreservingFeet(
		Start, Plane, FVector::UpVector, View, Clearance, Feet));
	TestTrue(TEXT("상승 순간 화면 위로 밀리지 않음"), FVector::VectorPlaneProject(Feet - Start, View).IsNearlyZero(0.001));
	TestTrue(TEXT("착지에 필요한 바닥 여유를 처음부터 확보"), FMath::IsNearlyEqual(Feet.Z, 500.0 + Clearance, 0.001));
	// 좌→우와 뒤→앞을 각각 1초간 진행시켜 프레임 수와 무관한 화면 이동량을 검사한다.
	for (const FVector Direction : {FVector::RightVector, FVector::ForwardVector})
	{
		for (const int32 FPS : {15, 30, 60, 120})
		{
			FVector Position = Feet;
			for (int32 Frame = 0; Frame < FPS; ++Frame)
			{
				const FVector Expected = Position + Direction * (300.0 / FPS);
				FVector Next;
				if (!AUOUIllusionTraversalProbe::CalculateScreenPreservingFeet(Expected, Plane,
					FVector::UpVector, View, Clearance, Next)) { AddError(TEXT("연속 상승 좌표 계산 실패")); return false; }
				Position = Next;
			}
			TestTrue(FString::Printf(TEXT("%d FPS에서 방향 %s 화면 속도 유지"), FPS, *Direction.ToCompactString()),
				FVector::VectorPlaneProject(Position - Feet - Direction * 300, View).IsNearlyZero(0.001));
		}
	}
	TestFalse(TEXT("표면과 평행한 시선은 나눗셈 대신 보류"), AUOUIllusionTraversalProbe::CalculateScreenPreservingFeet(
		Start, Plane, FVector::UpVector, FVector::RightVector, Clearance, Feet));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUOUIllusionVirtualReturnTest,
	"UnderOneUmbrella.IllusionProbe.RotationCancellation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUIllusionVirtualReturnTest::RunTest(const FString& Parameters)
{
	const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("IllusionReturnTest"),
		nullptr, false, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("회전 취소 테스트 월드"), World)) return false;
	auto* Character = World->SpawnActor<ACharacter>();
	auto* Probe = World->SpawnActor<AUOUIllusionTraversalProbe>();
	if (!Character || !Probe) { World->DestroyWorld(false); return false; }
	auto* Movement = Character->GetCharacterMovement();
	const FVector Entry(100, 200, 300), Floating(500, 600, 900);
	Probe->EntryLocation = Entry;
	Probe->VirtualCharacter = Character;
	Probe->bVirtualWalking = true;
	Probe->bEnableTextureAutoTraversal = true;
	Probe->SavedMovementMode = MOVE_Walking;
	Probe->bSavedMovementTick = true;
	Probe->SavedCollision = ECollisionEnabled::QueryAndPhysics;
	Movement->SetMovementMode(MOVE_Custom, 201);
	Movement->SetComponentTickEnabled(false);
	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Character->SetActorLocation(Floating);
	Movement->Velocity = FVector(100, 0, 0);
	// 표시 보정은 출발 복귀 때 누적되지 않고 정확히 원래 상대 위치로 돌아와야 한다.
	const FVector OriginalMeshLocation = Character->GetMesh()->GetRelativeLocation();
	Probe->VisualMesh = Character->GetMesh();
	Probe->AppliedVisualOffset = FVector(30, -20, 40);
	Character->GetMesh()->SetRelativeLocation(OriginalMeshLocation + Probe->AppliedVisualOffset);
	// 회전 취소는 명시적인 기능 끄기와 달리 자동 이동 설정을 보존한다.
	Probe->CancelVirtualWalkingForRotation();
	TestTrue(TEXT("출발 위치로 복귀"), Character->GetActorLocation().Equals(Entry));
	TestFalse(TEXT("가상 보행 해제"), Probe->bVirtualWalking);
	TestTrue(TEXT("자동 이동 설정 유지"), Probe->bEnableTextureAutoTraversal);
	TestTrue(TEXT("일반 이동 틱 복구"), Movement->IsComponentTickEnabled());
	TestTrue(TEXT("이전 속도 제거"), Movement->Velocity.IsNearlyZero());
	TestTrue(TEXT("캡슐 충돌 복구"), Character->GetCapsuleComponent()->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics);
	TestTrue(TEXT("표시 깊이 보정 복구"), Character->GetMesh()->GetRelativeLocation().Equals(OriginalMeshLocation));
	// 이미 확정된 일반 보행 상태에서는 회전 취소가 위치를 바꾸지 않는다.
	Character->SetActorLocation(Floating);
	Probe->CancelVirtualWalkingForRotation();
	TestTrue(TEXT("가상 보행 종료 후에는 복귀하지 않음"), Character->GetActorLocation().Equals(Floating));
	// 텍스처 없이도 검증된 선분을 되짚는다. 시선 방향 깊이 차이는 보행 거리에서 제외한다.
	Probe->VirtualCharacter = Character;
	Probe->bVirtualWalking = true;
	Probe->EntryLocation = Entry;
	Probe->EntryViewRotation = FRotator::ZeroRotator;
	Probe->EntryTravelDirection = FVector::RightVector;
	const FVector First = Entry + FVector(100, 0, 0);
	const FVector Last = First + FVector(0, 100, 0);
	Probe->VirtualTrail = {Entry, First, Last};
	Movement->SetMovementMode(MOVE_Custom, 201);
	Movement->SetComponentTickEnabled(false);
	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Character->SetActorLocation(Last);
	TestFalse(TEXT("전진을 후퇴로 처리하지 않음"), Probe->RetreatVirtualWalking(FVector::RightVector, 20, 120));
	// 화면에서 대각선으로 빠지는 입력까지 이전 경로로 강제하지 않는다.
	Probe->EntryViewRotation = FRotator(-30, 0, 0);
	TestFalse(TEXT("대각선 입력을 후퇴 경로에 붙잡지 않음"),
		Probe->RetreatVirtualWalking(FVector(1, -1, 0).GetSafeNormal(), 20, 120));
	TestTrue(TEXT("후퇴 처리 거부 시 다른 이동 판정에 위치를 그대로 넘김"), Character->GetActorLocation().Equals(Last));
	Probe->EntryViewRotation = FRotator::ZeroRotator;
	TestTrue(TEXT("프레임 없이 후퇴 가능"), Probe->RetreatVirtualWalking(-FVector::RightVector, 20, 120));
	TestTrue(TEXT("걸어온 선분에서 이동"), Character->GetActorLocation().Equals(Last - FVector(0, 20, 0)));
	Probe->RetreatVirtualWalking(-FVector::RightVector, 100, 120);
	TestFalse(TEXT("출발점 후퇴 완료 후 일반 이동"), Probe->bVirtualWalking);
	TestTrue(TEXT("시선 깊이 구간을 통과해 출발점 복귀"), Character->GetActorLocation().Equals(Entry));
	TestTrue(TEXT("후퇴 속도 유지"), Movement->Velocity.Equals(FVector(0, -120, 0)));
	// 연결 실패는 이동 컴포넌트가 꺼진 상태로 남기지 않고 일반 낙하에 넘긴다.
	Probe->VirtualCharacter = Character;
	Probe->bVirtualWalking = true;
	Movement->SetMovementMode(MOVE_Custom, 201);
	Movement->SetComponentTickEnabled(false);
	Probe->ReleaseVirtualWalking(FVector::RightVector, 150, TEXT("검증 실패 테스트"));
	TestTrue(TEXT("실패 후 일반 낙하로 전환"), Movement->IsFalling());
	TestTrue(TEXT("실패 후 입력 처리 틱 복구"), Movement->IsComponentTickEnabled());
	TestTrue(TEXT("실패 후 이동 속도 보존"), Movement->Velocity.Equals(FVector(0, 150, 0)));
	World->DestroyWorld(false);
	return true;
}

#endif
