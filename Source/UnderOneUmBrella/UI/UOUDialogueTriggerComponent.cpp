// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUDialogueTriggerComponent.h"

#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/UOUCameraControllerComponent.h"
#include "Player/UOUUmbrellaComponent.h"
#include "Player/UOUUmbrellaCoverVolumeComponent.h"
#include "TimerManager.h"
#include "UI/UOUDialogueCoverTargetComponent.h"
#include "UI/UOUDialogueSourceComponent.h"
#include "UI/UOUUISubsystem.h"

namespace
{
	// WBP_NPCSpeechBubble의 ShowBubble 함수 입력 구조와 맞춘 임시 파라미터입니다.
	struct FUOUDialogueHintBubbleParams
	{
		FText BubbleText;
		double Duration = 0.0;
	};
}

UUOUDialogueTriggerComponent::UUOUDialogueTriggerComponent()
{
	InitSphereRadius(180.0f);
	SetCollisionProfileName(TEXT("Trigger"));
	SetGenerateOverlapEvents(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	HintText = FText::FromString(TEXT("?"));
}

void UUOUDialogueTriggerComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddDynamic(this, &UUOUDialogueTriggerComponent::HandleBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &UUOUDialogueTriggerComponent::HandleEndOverlap);
	SetComponentTickEnabled(bRequireUmbrellaCoverHold);

	ResolveHintWidgetComponent();
	if (bEnableInteractionHint && bHideHintOnBeginPlay)
	{
		HideInteractionHint();
	}

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

	// 디버그 문구에 우산 보유 여부를 보여주기 위해 우산 컴포넌트를 찾습니다.
	const UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent(CurrentInstigatorActor);

	// 플레이어인지, 우산을 가지고 있는지, 우산 상태가 맞는지 같은 기본 조건을 먼저 봅니다.
	const bool bCanCheckCover = PassesInstigatorRules(CurrentInstigatorActor);

	// 실제 커버 판정입니다. 대화 대상의 커버 타겟 스피어가 플레이어의 대화용 우산 커버 박스와 겹치면 성공입니다.
	FString CoverDebugDetails;
	const bool bCovered = bCanCheckCover && IsOwnerCoveredByDialogueCover(CurrentInstigatorActor, &CoverDebugDetails);

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
		FString CoverDebugDetails;
		if (!IsOwnerCoveredByDialogueCover(InstigatorActor, &CoverDebugDetails)
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

void UUOUDialogueTriggerComponent::ShowInteractionHint()
{
	if (!bEnableInteractionHint)
	{
		return;
	}

	UUserWidget* UserWidget = GetHintUserWidget();
	if (UserWidget == nullptr)
	{
		return;
	}

	FText DisplayHintText = HintText;
	if (const UUOUDialogueSourceComponent* Source = ResolveDialogueSource())
	{
		if (Source->IsUsingDialogueTable())
		{
			DisplayHintText = Source->GetProximityBubbleText();
		}
	}

	if (DisplayHintText.IsEmpty())
	{
		return;
	}

	SetHintWidgetComponentVisible(true);
	CallHintWidgetShowFunction(UserWidget, DisplayHintText);
	bHintVisible = true;
}

void UUOUDialogueTriggerComponent::HideInteractionHint()
{
	if (!bEnableInteractionHint)
	{
		return;
	}

	UUserWidget* UserWidget = GetHintUserWidget();
	if (UserWidget != nullptr)
	{
		CallHintWidgetHideFunction(UserWidget);
	}

	SetHintWidgetComponentVisible(false);
	bHintVisible = false;
}

UWidgetComponent* UUOUDialogueTriggerComponent::ResolveHintWidgetComponent()
{
	if (HintWidgetComponent != nullptr)
	{
		return HintWidgetComponent;
	}

	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return nullptr;
	}

	TArray<UWidgetComponent*> WidgetComponents;
	OwnerActor->GetComponents<UWidgetComponent>(WidgetComponents);

	for (UWidgetComponent* WidgetComponent : WidgetComponents)
	{
		if (WidgetComponent != nullptr && WidgetComponent->GetFName() == HintWidgetComponentName)
		{
			HintWidgetComponent = WidgetComponent;
			return HintWidgetComponent;
		}
	}

	if (bAutoFindFirstHintWidgetComponent && WidgetComponents.Num() > 0)
	{
		HintWidgetComponent = WidgetComponents[0];
	}

	return HintWidgetComponent;
}

void UUOUDialogueTriggerComponent::ResetTrigger()
{
	bHasTriggered = false;
	ClearCoverProgress();
}

void UUOUDialogueTriggerComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!CanTrackOverlapActor(OtherActor))
	{
		if (bRequireUmbrellaCoverHold)
		{
			ShowCoverDebugMessage(TEXT("Dialogue Trigger Overlap Failed: not a valid actor"), FColor::Red, 1.5f);
		}
		return;
	}

	// 같은 캐릭터 안의 여러 컴포넌트가 닿아도 액터 기준 첫 진입만 처리합니다.
	if (!RegisterActorOverlap(OtherActor))
	{
		return;
	}

	if (bRequireUmbrellaCoverHold)
	{
		ShowCoverDebugMessage(FString::Printf(TEXT("Dialogue Trigger Overlap: %s"), *GetNameSafe(OtherActor)), FColor::White, 1.5f);

		// 커버형 대화는 트리거에 들어온 플레이어를 일단 추적합니다.
		// 그래야 트리거 안에 먼저 들어온 뒤 우산을 펼치는 흐름도 Tick에서 다시 검사할 수 있습니다.
		if (CanTrackOverlapActor(OtherActor))
		{
			ShowInteractionHint();
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

	if (CanTrackOverlapActor(OtherActor))
	{
		ShowInteractionHint();
	}
	TryStartDialogue(OtherActor);
}

void UUOUDialogueTriggerComponent::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!CanTrackOverlapActor(OtherActor))
	{
		return;
	}

	// 아직 같은 액터의 다른 컴포넌트가 트리거 안에 남아 있으면 실제 이탈로 보지 않습니다.
	if (!UnregisterActorOverlap(OtherActor))
	{
		return;
	}

	if (OtherActor == CurrentInstigatorActor)
	{
		ShowCoverDebugMessage(FString::Printf(TEXT("Dialogue Trigger EndOverlap: %s"), *GetNameSafe(OtherActor)), FColor::Orange, 1.5f);
		ClearCoverProgress();
	}

	if (CanTrackOverlapActor(OtherActor))
	{
		HideInteractionHint();
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

bool UUOUDialogueTriggerComponent::RegisterActorOverlap(AActor* InstigatorActor)
{
	if (InstigatorActor == nullptr)
	{
		return false;
	}

	const TWeakObjectPtr<AActor> ActorKey(InstigatorActor);
	int32& OverlapCount = ActiveOverlapCounts.FindOrAdd(ActorKey);
	++OverlapCount;

	// 1이면 이 액터가 처음 들어온 순간입니다.
	return OverlapCount == 1;
}

bool UUOUDialogueTriggerComponent::UnregisterActorOverlap(AActor* InstigatorActor)
{
	if (InstigatorActor == nullptr)
	{
		return false;
	}

	const TWeakObjectPtr<AActor> ActorKey(InstigatorActor);
	int32* OverlapCount = ActiveOverlapCounts.Find(ActorKey);
	if (OverlapCount == nullptr)
	{
		return true;
	}

	--(*OverlapCount);
	if (*OverlapCount > 0)
	{
		return false;
	}

	// 0이면 이 액터의 모든 겹침이 끝난 순간입니다.
	ActiveOverlapCounts.Remove(ActorKey);
	return true;
}

bool UUOUDialogueTriggerComponent::IsOwnerCoveredByDialogueCover(AActor* InstigatorActor, FString* OutDebugDetails) const
{
	TArray<UUOUDialogueCoverTargetComponent*> CoverTargets;
	ResolveCoverTargets(CoverTargets);
	if (CoverTargets.Num() <= 0)
	{
		if (OutDebugDetails != nullptr)
		{
			*OutDebugDetails = TEXT("CoverTarget:None");
		}
		return false;
	}

	TArray<UUOUUmbrellaCoverVolumeComponent*> CoverVolumes;
	ResolveUmbrellaCoverVolumes(InstigatorActor, CoverVolumes);
	if (CoverVolumes.Num() <= 0)
	{
		if (OutDebugDetails != nullptr)
		{
			*OutDebugDetails = TEXT("UmbrellaCoverVolume:None");
		}
		return false;
	}

	// 대화 커버 판정은 이제 RainBlocker 좌표 계산을 쓰지 않습니다.
	// NPC 쪽 DialogueCoverTarget 기준점 반경과 플레이어 쪽 UmbrellaCoverVolume 박스가 실제로 닿았는지 확인합니다.
	FString LastDebugDetails;
	for (const UUOUDialogueCoverTargetComponent* Target : CoverTargets)
	{
		if (Target == nullptr)
		{
			continue;
		}

		for (const UUOUUmbrellaCoverVolumeComponent* CoverVolume : CoverVolumes)
		{
			if (CoverVolume == nullptr)
			{
				continue;
			}

			const FVector TargetCenter = Target->GetComponentLocation();
			const float TargetRadius = FMath::Max(0.0f, Target->GetScaledSphereRadius());
			const float TouchTolerance = FMath::Max(0.0f, Target->CoverTouchTolerance);
			const FBox CoverWorldBox = CoverVolume->Bounds.GetBox();
			const FVector CoverBoxCenter = CoverWorldBox.GetCenter();
			const FVector CoverBoxExtent = CoverWorldBox.GetExtent();
			const FVector ClosestPoint = CoverWorldBox.IsValid ? CoverWorldBox.GetClosestPointTo(TargetCenter) : TargetCenter;
			const float DistanceToBox = CoverWorldBox.IsValid ? FVector::Distance(ClosestPoint, TargetCenter) : TNumericLimits<float>::Max();
			const float RequiredTouchDistance = TargetRadius + TouchTolerance;
			const float GapToTouch = CoverWorldBox.IsValid ? FMath::Max(0.0f, DistanceToBox - RequiredTouchDistance) : -1.0f;

			// 디버그에 표시하는 Gap 계산과 실제 성공 판정을 반드시 같은 값에서 뽑습니다.
			// 이전처럼 별도 함수 결과를 섞으면 화면상 Gap 0인데 Touch No가 나와 원인을 헷갈리게 만듭니다.
			const bool bTouching = CoverWorldBox.IsValid && DistanceToBox <= RequiredTouchDistance;

			LastDebugDetails = FString::Printf(
				TEXT("Targets:%d Volumes:%d | Target:%s Volume:%s Touch:%s | TargetCenter %.1f %.1f %.1f BoxCenter %.1f %.1f %.1f BoxExt %.1f %.1f %.1f R %.1f Tol %.1f Gap %.2f"),
				CoverTargets.Num(),
				CoverVolumes.Num(),
				*GetNameSafe(Target),
				*GetNameSafe(CoverVolume),
				bTouching ? TEXT("Yes") : TEXT("No"),
				TargetCenter.X,
				TargetCenter.Y,
				TargetCenter.Z,
				CoverBoxCenter.X,
				CoverBoxCenter.Y,
				CoverBoxCenter.Z,
				CoverBoxExtent.X,
				CoverBoxExtent.Y,
				CoverBoxExtent.Z,
				TargetRadius,
				TouchTolerance,
				GapToTouch);

			if (bTouching)
			{
				if (OutDebugDetails != nullptr)
				{
					*OutDebugDetails = LastDebugDetails;
				}
				return true;
			}
		}
	}

	if (OutDebugDetails != nullptr)
	{
		*OutDebugDetails = LastDebugDetails.IsEmpty() ? TEXT("CoverOverlap:No") : LastDebugDetails;
	}
	return false;
}

void UUOUDialogueTriggerComponent::ResolveCoverTargets(TArray<UUOUDialogueCoverTargetComponent*>& OutCoverTargets) const
{
	OutCoverTargets.Reset();

	if (CoverTarget != nullptr)
	{
		OutCoverTargets.Add(CoverTarget.Get());
		return;
	}

	if (!bAutoFindCoverTargets)
	{
		return;
	}

	const AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	OwnerActor->GetComponents<UUOUDialogueCoverTargetComponent>(OutCoverTargets);
}

void UUOUDialogueTriggerComponent::ResolveUmbrellaCoverVolumes(AActor* InstigatorActor, TArray<UUOUUmbrellaCoverVolumeComponent*>& OutCoverVolumes) const
{
	OutCoverVolumes.Reset();

	if (InstigatorActor == nullptr)
	{
		return;
	}

	InstigatorActor->GetComponents<UUOUUmbrellaCoverVolumeComponent>(OutCoverVolumes);
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
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DialogueFocusEndDelayTimerHandle);
	}

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
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DialogueFocusEndDelayTimerHandle);
	}

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
	if (DialogueFocusEndDelay <= 0.0f)
	{
		StopDialogueCameraFocus();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DialogueFocusEndDelayTimerHandle,
			this,
			&UUOUDialogueTriggerComponent::StopDialogueCameraFocus,
			DialogueFocusEndDelay,
			false);
		return;
	}

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

void UUOUDialogueTriggerComponent::SetHintWidgetComponentVisible(bool bNewVisible) const
{
	if (!bControlHintWidgetComponentVisibility || HintWidgetComponent == nullptr)
	{
		return;
	}

	HintWidgetComponent->SetVisibility(bNewVisible, true);
	HintWidgetComponent->SetHiddenInGame(!bNewVisible, true);
}

void UUOUDialogueTriggerComponent::CallHintWidgetShowFunction(UUserWidget* UserWidget, const FText& DisplayHintText) const
{
	if (UserWidget == nullptr || HintShowFunctionName.IsNone())
	{
		return;
	}

	UFunction* ShowFunction = UserWidget->FindFunction(HintShowFunctionName);
	if (ShowFunction == nullptr)
	{
		return;
	}

	FUOUDialogueHintBubbleParams Params;
	Params.BubbleText = DisplayHintText;
	Params.Duration = HintDuration > 0.0 ? HintDuration : 3.0;
	UserWidget->ProcessEvent(ShowFunction, &Params);
}

void UUOUDialogueTriggerComponent::CallHintWidgetHideFunction(UUserWidget* UserWidget) const
{
	if (UserWidget == nullptr || HintHideFunctionName.IsNone())
	{
		return;
	}

	UFunction* HideFunction = UserWidget->FindFunction(HintHideFunctionName);
	if (HideFunction != nullptr)
	{
		UserWidget->ProcessEvent(HideFunction, nullptr);
	}
}

UUserWidget* UUOUDialogueTriggerComponent::GetHintUserWidget()
{
	UWidgetComponent* WidgetComponent = ResolveHintWidgetComponent();
	return WidgetComponent != nullptr ? WidgetComponent->GetUserWidgetObject() : nullptr;
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
