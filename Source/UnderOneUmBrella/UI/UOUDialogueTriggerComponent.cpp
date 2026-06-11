// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUDialogueTriggerComponent.h"

#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/UOUCameraControllerComponent.h"
#include "Player/UOUUmbrellaComponent.h"
#include "UI/UOUDialogueSourceComponent.h"
#include "UI/UOUUISubsystem.h"

UUOUDialogueTriggerComponent::UUOUDialogueTriggerComponent()
{
	InitSphereRadius(180.0f);
	SetCollisionProfileName(TEXT("Trigger"));
	SetGenerateOverlapEvents(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UUOUDialogueTriggerComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddDynamic(this, &UUOUDialogueTriggerComponent::HandleBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &UUOUDialogueTriggerComponent::HandleEndOverlap);
	SetComponentTickEnabled(bRequireUmbrellaCoverHold);

	if (bRequireUmbrellaCoverHold)
	{
		ShowCoverDebugMessage(FString::Printf(TEXT("DialogueTrigger Ready: %s"), *GetNameSafe(GetOwner())), FColor::White, 2.0f);
	}
}

void UUOUDialogueTriggerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnComponentBeginOverlap.RemoveDynamic(this, &UUOUDialogueTriggerComponent::HandleBeginOverlap);
	OnComponentEndOverlap.RemoveDynamic(this, &UUOUDialogueTriggerComponent::HandleEndOverlap);
	StopDialogueCameraFocus();

	Super::EndPlay(EndPlayReason);
}

void UUOUDialogueTriggerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 우산 커버 대화는 트리거 안에 들어온 플레이어를 기억해 두고 매 틱마다 상태를 다시 검사합니다.
	// 플레이어가 먼저 들어온 뒤 우산을 펼쳐도, 조건이 맞는 순간부터 커버 유지 시간이 쌓입니다.
	// 유지 시간이 RequiredCoverDuration을 넘으면 실제 대화를 시작합니다.
	if (!bRequireUmbrellaCoverHold || CurrentInstigatorActor == nullptr)
	{
		return;
	}

	// 현재 트리거 안에 있는 플레이어가 가진 우산 컴포넌트를 찾습니다.
	const UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent(CurrentInstigatorActor);

	// 플레이어인지, 우산을 가지고 있는지, 우산 상태가 맞는지 같은 기본 조건을 먼저 봅니다.
	const bool bCanCheckCover = (UmbrellaComponent != nullptr) && PassesInstigatorRules(CurrentInstigatorActor);

	// 실제 커버 판정입니다. 대화 대상의 여러 기준점 중 하나라도 우산 비 차단 박스 안에 있으면 성공입니다.
	FString CoverDebugDetails;
	const bool bCovered = bCanCheckCover && IsOwnerCoveredByUmbrella(*UmbrellaComponent, &CoverDebugDetails);

	if (!bCanCheckCover && CoverDebugDetails.IsEmpty())
	{
		CoverDebugDetails = FString::Printf(
			TEXT("RuleFailed Umbrella:%s Rules:%s"),
			UmbrellaComponent != nullptr ? TEXT("Yes") : TEXT("None"),
			PassesInstigatorRules(CurrentInstigatorActor) ? TEXT("Pass") : TEXT("Fail"));
	}

	if (!bCovered)
	{
		if (bIsCurrentlyCoveredByUmbrella)
		{
			ShowCoverDebugMessage(TEXT("Umbrella Cover Exit"), FColor::Orange, 1.5f);
		}

		bIsCurrentlyCoveredByUmbrella = false;
		CurrentCoverHoldTime = 0.0f;
		ShowCoverDebugStatus(
			FString::Printf(TEXT("Cover: OUT  %.1f / %.1f | %s"), CurrentCoverHoldTime, RequiredCoverDuration, *CoverDebugDetails),
			FColor::Orange);
		return;
	}

	if (!bIsCurrentlyCoveredByUmbrella)
	{
		ShowCoverDebugMessage(TEXT("Umbrella Cover Enter"), FColor::Green, 1.5f);
	}

	bIsCurrentlyCoveredByUmbrella = true;
	CurrentCoverHoldTime += FMath::Max(0.0f, DeltaTime);
	ShowCoverDebugStatus(
		FString::Printf(TEXT("Cover: IN  %.1f / %.1f | %s"), CurrentCoverHoldTime, RequiredCoverDuration, *CoverDebugDetails),
		FColor::Green);
	if (CurrentCoverHoldTime >= RequiredCoverDuration)
	{
		TryStartDialogue(CurrentInstigatorActor);
	}
}

bool UUOUDialogueTriggerComponent::TryStartDialogue(AActor* InstigatorActor)
{
	if (bTriggerOnce && bHasTriggered)
	{
		ShowCoverDebugMessage(TEXT("Dialogue Start Failed: already triggered"), FColor::Red, 1.5f);
		return false;
	}

	if (!PassesInstigatorRules(InstigatorActor))
	{
		ShowCoverDebugMessage(TEXT("Dialogue Start Failed: rule check failed"), FColor::Red, 1.5f);
		return false;
	}

	if (bRequireUmbrellaCoverHold)
	{
		const UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent(InstigatorActor);
		FString CoverDebugDetails;
		if (UmbrellaComponent == nullptr
			|| !IsOwnerCoveredByUmbrella(*UmbrellaComponent, &CoverDebugDetails)
			|| CurrentCoverHoldTime < RequiredCoverDuration)
		{
			ShowCoverDebugMessage(FString::Printf(TEXT("Dialogue Start Failed: cover hold not ready | %s"), *CoverDebugDetails), FColor::Red, 1.5f);
			return false;
		}
	}

	UUOUDialogueSourceComponent* Source = ResolveDialogueSource();
	if (Source == nullptr || !Source->StartDialogue(InstigatorActor))
	{
		ShowCoverDebugMessage(TEXT("Dialogue Start Failed: source missing or blocked"), FColor::Red, 1.5f);
		return false;
	}

	ShowCoverDebugMessage(TEXT("Dialogue Started"), FColor::Cyan, 2.0f);
	StartDialogueCameraFocus(InstigatorActor, Source->GetSpeakerActor());

	bHasTriggered = true;
	ClearCoverProgress();
	return true;
}

void UUOUDialogueTriggerComponent::ResetTrigger()
{
	bHasTriggered = false;
	ClearCoverProgress();
}

void UUOUDialogueTriggerComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bRequireUmbrellaCoverHold)
	{
		ShowCoverDebugMessage(FString::Printf(TEXT("Dialogue Trigger Overlap: %s"), *GetNameSafe(OtherActor)), FColor::White, 1.5f);

		// 커버형 대화는 트리거에 들어온 플레이어를 일단 추적합니다.
		// 그래야 트리거 안에 먼저 들어온 뒤 우산을 펼치는 흐름도 Tick에서 다시 검사할 수 있습니다.
		if (CanTrackOverlapActor(OtherActor))
		{
			CurrentInstigatorActor = OtherActor;
			CurrentCoverHoldTime = 0.0f;
			bIsCurrentlyCoveredByUmbrella = false;
			SetComponentTickEnabled(true);

			if (!PassesInstigatorRules(OtherActor))
			{
				ShowCoverDebugMessage(TEXT("Dialogue Trigger Tracking: waiting for umbrella rules"), FColor::Orange, 1.5f);
			}
		}
		else
		{
			ShowCoverDebugMessage(TEXT("Dialogue Trigger Overlap Failed: not a valid actor"), FColor::Red, 1.5f);
		}
		return;
	}

	TryStartDialogue(OtherActor);
}

void UUOUDialogueTriggerComponent::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == CurrentInstigatorActor)
	{
		ShowCoverDebugMessage(FString::Printf(TEXT("Dialogue Trigger EndOverlap: %s"), *GetNameSafe(OtherActor)), FColor::Orange, 1.5f);
		ClearCoverProgress();
	}
}

UUOUDialogueSourceComponent* UUOUDialogueTriggerComponent::ResolveDialogueSource() const
{
	if (DialogueSource != nullptr)
	{
		return DialogueSource;
	}

	AActor* OwnerActor = GetOwner();
	return OwnerActor != nullptr ? OwnerActor->FindComponentByClass<UUOUDialogueSourceComponent>() : nullptr;
}

bool UUOUDialogueTriggerComponent::PassesInstigatorRules(AActor* InstigatorActor) const
{
	if (InstigatorActor == nullptr)
	{
		return false;
	}

	if (bOnlyPawn && Cast<APawn>(InstigatorActor) == nullptr)
	{
		return false;
	}

	if (!bRequireUmbrella)
	{
		return true;
	}

	const UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent(InstigatorActor);
	if (UmbrellaComponent == nullptr || !UmbrellaComponent->HasUmbrella())
	{
		return false;
	}

	if (bRequireOpenUmbrella && !UmbrellaComponent->IsOpen() && !UmbrellaComponent->IsUpsideDown())
	{
		return false;
	}

	if (bRequireBlockingRain && !UmbrellaComponent->IsBlockingRain())
	{
		return false;
	}

	return true;
}

UUOUUmbrellaComponent* UUOUDialogueTriggerComponent::FindUmbrellaComponent(AActor* InstigatorActor) const
{
	if (InstigatorActor == nullptr)
	{
		return nullptr;
	}

	return InstigatorActor->FindComponentByClass<UUOUUmbrellaComponent>();
}

bool UUOUDialogueTriggerComponent::CanTrackOverlapActor(AActor* InstigatorActor) const
{
	if (InstigatorActor == nullptr)
	{
		return false;
	}

	// 커버형 대화는 우산을 나중에 펼치는 경우도 허용해야 하므로,
	// 여기서는 Pawn 같은 최소 추적 조건만 검사하고 우산 상태 검사는 Tick에서 따로 처리합니다.
	if (bOnlyPawn && Cast<APawn>(InstigatorActor) == nullptr)
	{
		return false;
	}

	return true;
}

bool UUOUDialogueTriggerComponent::IsOwnerCoveredByUmbrella(const UUOUUmbrellaComponent& UmbrellaComponent, FString* OutDebugDetails) const
{
	if (!UmbrellaComponent.IsOpen())
	{
		if (OutDebugDetails != nullptr)
		{
			*OutDebugDetails = TEXT("UmbrellaOpen:No");
		}
		return false;
	}

	FVector BlockerCenter = FVector::ZeroVector;
	FRotator BlockerRotation = FRotator::ZeroRotator;
	FVector BlockerHalfExtent = FVector::ZeroVector;
	if (!UmbrellaComponent.TryGetRainBlockerVolumeData(BlockerCenter, BlockerRotation, BlockerHalfExtent))
	{
		if (OutDebugDetails != nullptr)
		{
			*OutDebugDetails = TEXT("RainBlockerData:Missing");
		}
		return false;
	}

	const AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		if (OutDebugDetails != nullptr)
		{
			*OutDebugDetails = TEXT("Owner:None");
		}
		return false;
	}

	const FTransform BlockerTransform(BlockerRotation, BlockerCenter);

	struct FCoverCheckPoint
	{
		FString Name;
		FVector WorldLocation = FVector::ZeroVector;
	};

	TArray<FCoverCheckPoint> CheckPoints;

	// 디테일 창에서 조정하는 대표 검사점입니다. NPC 머리 위나 상자 중심처럼 원하는 지점을 맞출 때 씁니다.
	CheckPoints.Add({ TEXT("Offset"), OwnerActor->GetActorLocation() + CoverTargetOffset });

	// 액터 전체 바운드의 중심입니다. 메쉬 피벗이 바닥이나 한쪽에 치우친 액터도 어느 정도 안정적으로 잡기 위한 보조점입니다.
	const FBox OwnerBounds = OwnerActor->GetComponentsBoundingBox(true);
	if (OwnerBounds.IsValid)
	{
		CheckPoints.Add({ TEXT("BoundsCenter"), OwnerBounds.GetCenter() });
	}

	// 마지막 보조점입니다. 피벗 기준으로 판정하고 싶거나 바운드가 비정상일 때 확인용으로 사용합니다.
	CheckPoints.Add({ TEXT("ActorLocation"), OwnerActor->GetActorLocation() });

	FString LastDebugDetails;
	for (const FCoverCheckPoint& CheckPoint : CheckPoints)
	{
		const FVector LocalTarget = BlockerTransform.InverseTransformPosition(CheckPoint.WorldLocation);
		const bool bInsideX = FMath::Abs(LocalTarget.X) <= BlockerHalfExtent.X;
		const bool bInsideY = FMath::Abs(LocalTarget.Y) <= BlockerHalfExtent.Y;
		const bool bInsideZ = FMath::Abs(LocalTarget.Z) <= BlockerHalfExtent.Z;

		LastDebugDetails = FString::Printf(
			TEXT("%s Local %.1f %.1f %.1f Half %.1f %.1f %.1f Axis %s/%s/%s"),
			*CheckPoint.Name,
			LocalTarget.X,
			LocalTarget.Y,
			LocalTarget.Z,
			BlockerHalfExtent.X,
			BlockerHalfExtent.Y,
			BlockerHalfExtent.Z,
			bInsideX ? TEXT("XIn") : TEXT("XOut"),
			bInsideY ? TEXT("YIn") : TEXT("YOut"),
			bInsideZ ? TEXT("ZIn") : TEXT("ZOut"));

		if (bInsideX && bInsideY && bInsideZ)
		{
			if (OutDebugDetails != nullptr)
			{
				*OutDebugDetails = LastDebugDetails;
			}
			return true;
		}
	}

	if (OutDebugDetails != nullptr)
	{
		*OutDebugDetails = LastDebugDetails;
	}
	return false;
}

UUOUUISubsystem* UUOUDialogueTriggerComponent::ResolveUISubsystem() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (PlayerController == nullptr)
	{
		return nullptr;
	}

	const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	return LocalPlayer != nullptr ? LocalPlayer->GetSubsystem<UUOUUISubsystem>() : nullptr;
}

UUOUCameraControllerComponent* UUOUDialogueTriggerComponent::FindCameraControllerComponent(AActor* InstigatorActor) const
{
	if (InstigatorActor == nullptr)
	{
		return nullptr;
	}

	return InstigatorActor->FindComponentByClass<UUOUCameraControllerComponent>();
}

void UUOUDialogueTriggerComponent::StartDialogueCameraFocus(AActor* InstigatorActor, AActor* SpeakerActor)
{
	UUOUCameraControllerComponent* CameraController = FindCameraControllerComponent(InstigatorActor);
	if (CameraController == nullptr || SpeakerActor == nullptr)
	{
		return;
	}

	LockMovementForDialogue(InstigatorActor);

	ActiveDialogueCameraController = CameraController;
	ActiveDialogueCameraController->StartDialogueFocus(SpeakerActor);

	if (UUOUUISubsystem* UISubsystem = ResolveUISubsystem())
	{
		UISubsystem->OnDialogueEnded.RemoveDynamic(this, &UUOUDialogueTriggerComponent::HandleDialogueEnded);
		UISubsystem->OnDialogueEnded.AddDynamic(this, &UUOUDialogueTriggerComponent::HandleDialogueEnded);
		BoundUISubsystem = UISubsystem;
	}
}

void UUOUDialogueTriggerComponent::StopDialogueCameraFocus()
{
	if (BoundUISubsystem != nullptr)
	{
		BoundUISubsystem->OnDialogueEnded.RemoveDynamic(this, &UUOUDialogueTriggerComponent::HandleDialogueEnded);
		BoundUISubsystem = nullptr;
	}

	if (ActiveDialogueCameraController != nullptr)
	{
		ActiveDialogueCameraController->EndDialogueFocus();
		ActiveDialogueCameraController = nullptr;
	}

	UnlockMovementForDialogue();
}

void UUOUDialogueTriggerComponent::HandleDialogueEnded()
{
	StopDialogueCameraFocus();
}

void UUOUDialogueTriggerComponent::LockMovementForDialogue(AActor* InstigatorActor)
{
	if (!bLockMovementDuringDialogueFocus || bDialogueMovementLocked)
	{
		return;
	}

	APawn* InstigatorPawn = Cast<APawn>(InstigatorActor);
	if (InstigatorPawn == nullptr)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(InstigatorPawn->GetController());
	if (PlayerController == nullptr)
	{
		PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	}

	if (PlayerController == nullptr)
	{
		return;
	}

	// 대화 카메라가 NPC와 플레이어 사이로 들어가는 동안에는 이동 입력만 잠급니다.
	// 룩 입력은 잠그지 않습니다. 카메라 컴포넌트가 대화용 위치로 직접 보간하고 있기 때문입니다.
	PlayerController->SetIgnoreMoveInput(true);
	LockedMovementPlayerController = PlayerController;
	bDialogueMovementLocked = true;

	// 이미 이동 중이던 속도를 즉시 끊어서 줌 시작 직후 캐릭터가 미끄러지는 느낌을 줄입니다.
	if (ACharacter* Character = Cast<ACharacter>(InstigatorPawn))
	{
		if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}
	}
}

void UUOUDialogueTriggerComponent::UnlockMovementForDialogue()
{
	if (!bDialogueMovementLocked)
	{
		return;
	}

	if (LockedMovementPlayerController != nullptr)
	{
		// LockMovementForDialogue에서 올린 이동 입력 잠금을 정확히 한 번만 되돌립니다.
		LockedMovementPlayerController->SetIgnoreMoveInput(false);
		LockedMovementPlayerController = nullptr;
	}

	bDialogueMovementLocked = false;
}

void UUOUDialogueTriggerComponent::ClearCoverProgress()
{
	CurrentCoverHoldTime = 0.0f;
	CurrentInstigatorActor = nullptr;
	bIsCurrentlyCoveredByUmbrella = false;

	if (bRequireUmbrellaCoverHold)
	{
		SetComponentTickEnabled(false);
	}
}

void UUOUDialogueTriggerComponent::ShowCoverDebugMessage(const FString& Message, const FColor& Color, float Duration) const
{
	if (!bShowUmbrellaCoverDebug || GEngine == nullptr)
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, Duration, Color, Message);
}

void UUOUDialogueTriggerComponent::ShowCoverDebugStatus(const FString& Message, const FColor& Color) const
{
	if (!bShowUmbrellaCoverDebug || GEngine == nullptr)
	{
		return;
	}

	const uint64 DebugKey = static_cast<uint64>(GetUniqueID());
	GEngine->AddOnScreenDebugMessage(DebugKey, 0.0f, Color, Message);
}
