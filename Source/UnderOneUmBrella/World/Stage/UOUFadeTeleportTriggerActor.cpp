// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Stage/UOUFadeTeleportTriggerActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "Player/UOUCameraControllerComponent.h"
#if WITH_EDITOR
#include "Editor.h"
#include "LevelEditorViewport.h"
#include "ScopedTransaction.h"
#endif

AUOUFadeTeleportTriggerActor::AUOUFadeTeleportTriggerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(RootScene);
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);
	TriggerVolume->SetBoxExtent(TriggerExtent);
}

void AUOUFadeTeleportTriggerActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyTriggerSettings();

	if (TriggerVolume != nullptr)
	{
		TriggerVolume->OnComponentBeginOverlap.RemoveDynamic(this, &AUOUFadeTeleportTriggerActor::HandleTriggerBeginOverlap);
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AUOUFadeTeleportTriggerActor::HandleTriggerBeginOverlap);
		TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &AUOUFadeTeleportTriggerActor::HandleTriggerEndOverlap);
	}
}

void AUOUFadeTeleportTriggerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FadeOutTimerHandle);
		World->GetTimerManager().ClearTimer(BlackHoldTimerHandle);
		World->GetTimerManager().ClearTimer(FadeInTimerHandle);
	}

	HideTransitionMessage();

	Super::EndPlay(EndPlayReason);
}

void AUOUFadeTeleportTriggerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyTriggerSettings();
}

bool AUOUFadeTeleportTriggerActor::TriggerTransition(AActor* InstigatorActor)
{
	if (ArrivalBlockedActors.Contains(InstigatorActor) || TeleportingActors.Contains(InstigatorActor))
	{
		return false;
	}
	if (bIsTransitioning || (bTriggerOnce && bHasTriggered))
	{
		return false;
	}

	if (!ShouldAcceptTriggerActor(InstigatorActor))
	{
		return false;
	}
	if (bRestrictCameraAngle)
	{
		const APlayerController* CameraController = ResolvePlayerController(InstigatorActor);
		if (!CameraController || !CameraController->PlayerCameraManager
			|| !IsCameraRotationAllowed(CameraController->PlayerCameraManager->GetCameraRotation()))
		{
			return false;
		}
	}

	const bool bUsesTargetLocation = TeleportTargetActor != nullptr && InstigatorActor == TeleportTargetActor;
	if (!bUsesTargetLocation && DestinationActor == nullptr)
	{
		return false;
	}

	if (InstigatorActor == nullptr || InstigatorActor == this)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	const bool bShouldUseCameraFade = bUseCameraFade && TeleportTargetActor == nullptr;
	APlayerController* PlayerController = nullptr;
	if (bShouldUseCameraFade)
	{
		PlayerController = ResolvePlayerController(InstigatorActor);
		if (PlayerController == nullptr || PlayerController->PlayerCameraManager == nullptr)
		{
			return false;
		}
	}

	bHasTriggered = true;
	bIsTransitioning = true;
	PendingTransitionActor = InstigatorActor;

	if (!bShouldUseCameraFade)
	{
		const bool bTeleported = TeleportPendingActor();
		FinishTransition();
		return bTeleported;
	}

	PendingPlayerController = PlayerController;

	const float SafeFadeOutDuration = FMath::Max(0.0f, FadeOutDuration);
	PlayerController->PlayerCameraManager->StartCameraFade(
		0.0f,
		1.0f,
		SafeFadeOutDuration,
		FadeColor,
		false,
		true);

	if (SafeFadeOutDuration <= 0.0f)
	{
		FinishFadeOut();
	}
	else
	{
		World->GetTimerManager().SetTimer(FadeOutTimerHandle, this, &AUOUFadeTeleportTriggerActor::FinishFadeOut, SafeFadeOutDuration, false);
	}

	return true;
}

void AUOUFadeTeleportTriggerActor::ResetTrigger()
{
	bHasTriggered = false;
}

void AUOUFadeTeleportTriggerActor::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	TriggerTransition(OtherActor);
}

void AUOUFadeTeleportTriggerActor::HandleTriggerEndOverlap(
	UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ReleaseArrivalLockIfOutside(OtherActor);
	// 다른 컴포넌트가 아직 안에 있을 수 있으므로 겹침 목록 갱신 후 다시 확인한다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(
			this, [this, Actor = TWeakObjectPtr<AActor>(OtherActor)]()
			{ ReleaseArrivalLockIfOutside(Actor); }));
	}
}

bool AUOUFadeTeleportTriggerActor::TeleportForIllusion(AActor* Actor, const FVector& Location, bool bPreserveCamera)
{
	if (!IsValid(Actor) || !Actor->GetWorld() || Location.ContainsNaN()) return false;
	TArray<TWeakObjectPtr<AUOUFadeTeleportTriggerActor>> Guards;
	for (TActorIterator<AUOUFadeTeleportTriggerActor> It(Actor->GetWorld()); It; ++It)
	{
		It->TeleportingActors.Add(Actor);
		Guards.Add(*It);
	}
	const FVector Before = Actor->GetActorLocation();
	const bool bMoved = Actor->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	// 하강은 트리거 재진입만 막고 카메라의 영구 오프셋을 더하지 않는다. 기존 호출은 동작을 유지한다.
	if (bMoved && bPreserveCamera)
	{
		if (auto* Camera = Actor->FindComponentByClass<UUOUCameraControllerComponent>())
			Camera->PreserveCameraAcrossTeleport(Actor->GetActorLocation() - Before);
	}
	for (const auto& Weak : Guards)
	{
		if (auto* Trigger = Weak.Get())
		{
			if (bMoved && IsValid(Actor) && Trigger->TriggerVolume->IsOverlappingActor(Actor))
				Trigger->ArrivalBlockedActors.Add(Actor);
			Trigger->TeleportingActors.Remove(Actor);
			Trigger->ReleaseArrivalLockIfOutside(Actor);
		}
	}
	return bMoved;
}

bool AUOUFadeTeleportTriggerActor::IsCameraRotationAllowed(FRotator CameraRotation) const
{
	if (!bRestrictCameraAngle) return true;
	if (CameraRotation.ContainsNaN() || AllowedCameraRotation.ContainsNaN()) return false;
	const float Tolerance = FMath::Clamp(CameraAngleTolerance, 0.0f, 180.0f);
	return FMath::Abs(FMath::FindDeltaAngleDegrees(CameraRotation.Yaw, AllowedCameraRotation.Yaw)) <= Tolerance
		&& (!bCheckCameraPitch || FMath::Abs(FMath::FindDeltaAngleDegrees(CameraRotation.Pitch, AllowedCameraRotation.Pitch)) <= Tolerance);
}

void AUOUFadeTeleportTriggerActor::CaptureCurrentCameraAngle()
{
	FRotator Rotation;
	bool bFound = false;
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		if (const APlayerController* PC = ResolvePlayerController(nullptr))
		{
			if (PC->PlayerCameraManager)
			{
				Rotation = PC->PlayerCameraManager->GetCameraRotation();
				bFound = true;
			}
		}
	}
#if WITH_EDITOR
	else if (GCurrentLevelEditingViewportClient)
	{
		Rotation = GCurrentLevelEditingViewportClient->GetViewRotation();
		bFound = true;
	}
#endif
	if (!bFound)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: No active camera available to capture."), *GetName());
		return;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(NSLOCTEXT("UOUTeleport", "CaptureAngle", "Capture teleport camera angle"));
	Modify();
#endif
	AllowedCameraRotation = Rotation.GetNormalized();
	bRestrictCameraAngle = true;
#if WITH_EDITOR
	MarkPackageDirty();
#endif
}

void AUOUFadeTeleportTriggerActor::ReleaseArrivalLockIfOutside(TWeakObjectPtr<AActor> Actor)
{
	if (!TeleportingActors.Contains(Actor)
		&& (!Actor.IsValid() || !TriggerVolume || !TriggerVolume->IsOverlappingActor(Actor.Get())))
	{
		ArrivalBlockedActors.Remove(Actor);
	}
	for (auto It = ArrivalBlockedActors.CreateIterator(); It; ++It)
	{
		if (!It->IsValid()) It.RemoveCurrent();
	}
}

void AUOUFadeTeleportTriggerActor::ApplyTriggerSettings()
{
	TriggerExtent.X = FMath::Max(0.0f, TriggerExtent.X);
	TriggerExtent.Y = FMath::Max(0.0f, TriggerExtent.Y);
	TriggerExtent.Z = FMath::Max(0.0f, TriggerExtent.Z);
	FadeOutDuration = FMath::Max(0.0f, FadeOutDuration);
	BlackHoldDuration = FMath::Max(0.0f, BlackHoldDuration);
	FadeInDuration = FMath::Max(0.0f, FadeInDuration);

	if (TriggerVolume == nullptr)
	{
		return;
	}

	TriggerVolume->SetBoxExtent(TriggerExtent);
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);
}

bool AUOUFadeTeleportTriggerActor::ShouldAcceptTriggerActor(const AActor* OtherActor) const
{
	if (OtherActor == nullptr || OtherActor == this)
	{
		return false;
	}

	if (TeleportTargetActor != nullptr)
	{
		return OtherActor == TeleportTargetActor;
	}

	if (!bPlayerOnly)
	{
		return true;
	}

	const APawn* Pawn = Cast<APawn>(OtherActor);
	return Pawn != nullptr && Pawn->IsPlayerControlled();
}

APlayerController* AUOUFadeTeleportTriggerActor::ResolvePlayerController(AActor* InstigatorActor) const
{
	APlayerController* PlayerController = Cast<APlayerController>(InstigatorActor);
	if (PlayerController != nullptr)
	{
		return PlayerController;
	}

	const APawn* Pawn = Cast<APawn>(InstigatorActor);
	if (Pawn != nullptr)
	{
		PlayerController = Cast<APlayerController>(Pawn->GetController());
	}

	if (PlayerController == nullptr && GetWorld() != nullptr)
	{
		PlayerController = GetWorld()->GetFirstPlayerController();
	}

	return PlayerController;
}

void AUOUFadeTeleportTriggerActor::FinishFadeOut()
{
	TeleportPendingActor();
	ShowTransitionMessage(FadeOutMessageSettings);

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		FinishTransition();
		return;
	}

	const float SafeBlackHoldDuration = FMath::Max(0.0f, BlackHoldDuration);
	if (SafeBlackHoldDuration <= 0.0f)
	{
		StartFadeIn();
	}
	else
	{
		World->GetTimerManager().SetTimer(BlackHoldTimerHandle, this, &AUOUFadeTeleportTriggerActor::StartFadeIn, SafeBlackHoldDuration, false);
	}
}

void AUOUFadeTeleportTriggerActor::StartFadeIn()
{
	APlayerController* PlayerController = PendingPlayerController.Get();
	if (PlayerController == nullptr || PlayerController->PlayerCameraManager == nullptr)
	{
		FinishTransition();
		return;
	}

	const float SafeFadeInDuration = FMath::Max(0.0f, FadeInDuration);
	ShowTransitionMessage(FadeInMessageSettings);

	PlayerController->PlayerCameraManager->StartCameraFade(
		1.0f,
		0.0f,
		SafeFadeInDuration,
		FadeColor,
		false,
		false);

	UWorld* World = GetWorld();
	if (World == nullptr || SafeFadeInDuration <= 0.0f)
	{
		FinishTransition();
	}
	else
	{
		World->GetTimerManager().SetTimer(FadeInTimerHandle, this, &AUOUFadeTeleportTriggerActor::FinishTransition, SafeFadeInDuration, false);
	}
}

void AUOUFadeTeleportTriggerActor::FinishTransition()
{
	HideTransitionMessage();

	PendingTransitionActor = nullptr;
	PendingPlayerController = nullptr;
	bIsTransitioning = false;
}

void AUOUFadeTeleportTriggerActor::ShowTransitionMessage(const FUOUTransitionMessageSettings& MessageSettings)
{
	TransitionMessagePresenter.Show(GetWorld(), MessageSettings);
}

void AUOUFadeTeleportTriggerActor::HideTransitionMessage()
{
	TransitionMessagePresenter.Hide();
}

bool AUOUFadeTeleportTriggerActor::TeleportPendingActor()
{
	AActor* TargetActor = PendingTransitionActor.Get();
	if (TargetActor == nullptr)
	{
		return false;
	}

	const bool bUsesTargetLocation = TargetActor == TeleportTargetActor;
	AActor* Destination = DestinationActor.Get();
	if (!bUsesTargetLocation && Destination == nullptr)
	{
		return false;
	}

	if (bStopMovementOnTeleport)
	{
		StopActorMovement(TargetActor);
	}

	const FVector DestinationLocation = bUsesTargetLocation
		? TeleportTargetLocation
		: Destination->GetActorLocation();
	const FRotator TargetRotation = bUsesTargetLocation
		? TargetActor->GetActorRotation()
		: Destination->GetActorRotation();
	const FRotator DestinationRotation = bUseDestinationRotation ? TargetRotation : TargetActor->GetActorRotation();
	// 위치 변경 중 겹침 이벤트가 즉시 발생할 수 있으므로 먼저 모든 이동 영역을 잠근다.
	// 도착 액터가 별도 위치 마커인 경우에도 그 위치를 포함하는 영역까지 보호한다.
	TArray<TWeakObjectPtr<AUOUFadeTeleportTriggerActor>> GuardedTriggers;
	for (TActorIterator<AUOUFadeTeleportTriggerActor> It(GetWorld()); It; ++It)
	{
		It->TeleportingActors.Add(TargetActor);
		GuardedTriggers.Add(*It);
	}
	const FVector PreviousLocation = TargetActor->GetActorLocation();
	const bool bTeleported = TargetActor->SetActorLocationAndRotation(
		DestinationLocation,
		DestinationRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	if (bTeleported && bPreserveCameraOnInstantTeleport && !bUseCameraFade
		&& !bUseDestinationRotation && TeleportTargetActor == nullptr)
	{
		if (auto* CameraController = TargetActor->FindComponentByClass<UUOUCameraControllerComponent>())
		{
			CameraController->PreserveCameraAcrossTeleport(TargetActor->GetActorLocation() - PreviousLocation);
		}
	}
	for (const auto& WeakTrigger : GuardedTriggers)
	{
		if (AUOUFadeTeleportTriggerActor* Trigger = WeakTrigger.Get())
		{
			if (bTeleported && IsValid(TargetActor) && Trigger->TriggerVolume
				&& Trigger->TriggerVolume->IsOverlappingActor(TargetActor))
			{
				Trigger->ArrivalBlockedActors.Add(TargetActor);
			}
			Trigger->TeleportingActors.Remove(TargetActor);
			Trigger->ReleaseArrivalLockIfOutside(TargetActor);
		}
	}

	if (bTeleported && TeleportTargetActor != nullptr)
	{
		TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(TargetActor);
		for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
		{
			if (PrimitiveComponent == nullptr || !PrimitiveComponent->IsSimulatingPhysics())
			{
				continue;
			}

			// Push/Pull 이동 잠금의 기준점을 텔레포트된 새 위치로 다시 설정합니다.
			PrimitiveComponent->RecreatePhysicsState();
			PrimitiveComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
			PrimitiveComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
	}

	if (bTeleported && bUseDestinationRotation)
	{
		if (APlayerController* PlayerController = PendingPlayerController.Get())
		{
			PlayerController->SetControlRotation(DestinationRotation);
		}
	}

	return bTeleported;
}

void AUOUFadeTeleportTriggerActor::StopActorMovement(AActor* TargetActor) const
{
	ACharacter* Character = Cast<ACharacter>(TargetActor);
	if (Character != nullptr)
	{
		if (UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement())
		{
			CharacterMovement->StopMovementImmediately();
		}
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(TargetActor);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent != nullptr && PrimitiveComponent->IsSimulatingPhysics())
		{
			PrimitiveComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
			PrimitiveComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
	}
}
