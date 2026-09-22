#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "World/Stage/UOUFadeTeleportTriggerActor.h"
#include "Player/UOUCameraControllerComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUOUTeleportArrivalLockTest,
	"UnderOneUmbrella.Teleport.ArrivalLockUntilExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUTeleportArrivalLockTest::RunTest(const FString& Parameters)
{
	const auto Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
		TEXT("TeleportArrivalTestWorld"), nullptr, false, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* A = World->SpawnActor<AUOUFadeTeleportTriggerActor>(FVector(0, 0, 1000), FRotator::ZeroRotator, Spawn);
	auto* B = World->SpawnActor<AUOUFadeTeleportTriggerActor>(FVector(1000, 0, 1000), FRotator::ZeroRotator, Spawn);
	auto* Character = World->SpawnActor<ACharacter>(FVector(-1000, 0, 1000), FRotator::ZeroRotator, Spawn);
	if (!A || !B || !Character)
	{
		AddError(TEXT("Could not spawn test actors"));
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	for (auto* Trigger : {A, B})
	{
		Trigger->bPlayerOnly = false;
		Trigger->bTriggerOnce = false;
		Trigger->bUseCameraFade = false;
		Trigger->bUseDestinationRotation = false;
		Trigger->DispatchBeginPlay();
	}
	A->DestinationActor = B;
	B->DestinationActor = A;
	A->bStopMovementOnTeleport = false;
	A->bRestrictCameraAngle = true;
	A->AllowedCameraRotation = FRotator(-20.0f, 179.0f, 0.0f);
	A->CameraAngleTolerance = 2.0f;
	TestTrue(TEXT("Yaw wraps across -180/180"), A->IsCameraRotationAllowed(FRotator(0, -179, 0)));
	TestFalse(TEXT("Other yaw is rejected"), A->IsCameraRotationAllowed(FRotator(-20, 134, 0)));
	A->bCheckCameraPitch = true;
	TestFalse(TEXT("Pitch mismatch is rejected when enabled"), A->IsCameraRotationAllowed(FRotator(0, 179, 0)));
	TestTrue(TEXT("Matching pitch and yaw accepted"), A->IsCameraRotationAllowed(FRotator(-20, 179, 0)));
	TestFalse(TEXT("Missing gameplay camera fails closed"), A->TriggerTransition(Character));
	TestTrue(TEXT("Rejected transition leaves position unchanged"), Character->GetActorLocation().Equals(FVector(-1000, 0, 1000)));
	A->bRestrictCameraAngle = false;
	TestTrue(TEXT("Disabled restriction accepts all angles"), A->IsCameraRotationAllowed(FRotator(90, 0, 0)));
	Character->GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	auto* Boom = NewObject<USpringArmComponent>(Character);
	Boom->SetupAttachment(Character->GetRootComponent());
	Boom->RegisterComponent();
	auto* Camera = NewObject<UCameraComponent>(Character);
	Camera->SetupAttachment(Boom, USpringArmComponent::SocketName);
	Camera->RegisterComponent();
	auto* CameraController = NewObject<UUOUCameraControllerComponent>(Character);
	CameraController->SetCameraRigComponents(Boom, Camera);
	CameraController->RegisterComponent();
	Character->DispatchBeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();
	const FVector MovingVelocity(180.0f, 60.0f, 0.0f);
	Character->GetCharacterMovement()->Velocity = MovingVelocity;
	const EMovementMode OriginalMode = Character->GetCharacterMovement()->MovementMode;
	const FRotator OriginalRotation = Character->GetActorRotation();
	Character->SetActorLocation(A->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	TestTrue(TEXT("Position-only teleport preserves velocity"), Character->GetCharacterMovement()->Velocity.Equals(MovingVelocity));
	TestTrue(TEXT("Position-only teleport preserves rotation"), Character->GetActorRotation().Equals(OriginalRotation));
	TestTrue(TEXT("Position-only teleport preserves movement mode"), Character->GetCharacterMovement()->MovementMode == OriginalMode);
	TestTrue(TEXT("Entering A arrives at B without bouncing"), Character->GetActorLocation().Equals(B->GetActorLocation(), 1.0f));
	TestTrue(TEXT("Teleport preserves pre-teleport orbit pivot"),
		(Boom->GetComponentLocation() + Boom->TargetOffset).Equals(A->GetActorLocation(), 1.0f));
	TestTrue(TEXT("Camera compensates only teleport displacement"),
		CameraController->TeleportFollowOffset.Equals(A->GetActorLocation() - B->GetActorLocation()));
	TestFalse(TEXT("Arrival B is locked even for explicit activation"), B->TriggerTransition(Character));
	World->Tick(LEVELTICK_All, 0.016f);
	TestTrue(TEXT("Remaining inside B does not return to A"), B->TriggerVolume->IsOverlappingActor(Character));
	TestTrue(TEXT("Camera compensation does not interpolate away"),
		Boom->TargetOffset.Equals(CameraController->TeleportFollowOffset, 0.01f));
	CameraController->RotateCameraRight();
	World->Tick(LEVELTICK_All, 0.016f);
	TestTrue(TEXT("Q/E retains corrected orbit pivot"),
		Boom->TargetOffset.Equals(CameraController->TeleportFollowOffset, 0.01f));
	Character->SetActorLocation(FVector(1000, 500, 1000), false, nullptr, ETeleportType::TeleportPhysics);
	World->Tick(LEVELTICK_All, 0.016f);
	Character->SetActorLocation(B->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	TestTrue(TEXT("Leaving and reentering B returns to A"), Character->GetActorLocation().Equals(A->GetActorLocation(), 1.0f));
	TestTrue(TEXT("Round trip cancels camera compensation"), CameraController->TeleportFollowOffset.IsNearlyZero(0.01f));
	TestFalse(TEXT("Return arrival A is locked"), A->TriggerTransition(Character));
	// 하강용 이동은 수동 트리거를 다시 실행하지 않고 기존 카메라 보정값을 유지한다.
	const FVector OffsetBeforeDescent = CameraController->TeleportFollowOffset;
	const FVector TargetOffsetBeforeDescent = Boom->TargetOffset;
	TestTrue(TEXT("하강용 보호 이동 성공"), AUOUFadeTeleportTriggerActor::TeleportForIllusion(Character, B->GetActorLocation(), false));
	TestTrue(TEXT("하강 도착 트리거의 재이동 차단"), Character->GetActorLocation().Equals(B->GetActorLocation(), 1));
	TestTrue(TEXT("하강은 누적 카메라 보정을 추가하지 않음"), CameraController->TeleportFollowOffset.Equals(OffsetBeforeDescent));
	TestTrue(TEXT("하강은 추적 오프셋을 변경하지 않음"), Boom->TargetOffset.Equals(TargetOffsetBeforeDescent));
	TestFalse(TEXT("하강 도착 트리거 잠금 유지"), B->TriggerTransition(Character));
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
