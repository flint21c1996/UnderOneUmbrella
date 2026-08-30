// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUDialogueTriggerComponent.h"

#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/UOUCameraControllerComponent.h"
#include "Player/UOUPlayerInteractionExecutorComponent.h"
#include "Player/UOUUmbrellaComponent.h"
#include "Player/UOUUmbrellaCoverVolumeComponent.h"
#include "TimerManager.h"
#include "UI/UOUDialogueCoverTargetComponent.h"
#include "UI/UOUDialogueSourceComponent.h"
#include "UI/UOUSpeechBubbleWidget.h"
#include "UI/UOUUISubsystem.h"

namespace
{
	// WBP_NPCSpeechBubble??ShowBubble ?⑥닔 ?낅젰 援ъ“? 留욎텣 ?꾩떆 ?뚮씪誘명꽣?낅땲??
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
	bDrawOnlyIfSelected = !bShowTriggerShapeInGame;
	SetHiddenInGame(!bShowTriggerShapeInGame);
	SetVisibility(bShowTriggerShapeInGame);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	HintText = FText::FromString(TEXT("?"));
}

void UUOUDialogueTriggerComponent::BeginPlay()
{
	Super::BeginPlay();

	bDrawOnlyIfSelected = !bShowTriggerShapeInGame;
	SetHiddenInGame(!bShowTriggerShapeInGame);
	SetVisibility(bShowTriggerShapeInGame);
	MarkRenderStateDirty();

	OnComponentBeginOverlap.AddDynamic(this, &UUOUDialogueTriggerComponent::HandleBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &UUOUDialogueTriggerComponent::HandleEndOverlap);
	SetComponentTickEnabled(bDialogueInteractionEnabled && bRequireUmbrellaCoverHold);

	if (UUOUDialogueSourceComponent* Source = ResolveDialogueSource())
	{
		Source->SetDialogueAvailable(bDialogueInteractionEnabled);
	}

	UWidgetComponent* ResolvedHintWidgetComponent = ResolveHintWidgetComponent();
	if (ResolvedHintWidgetComponent != nullptr)
	{
		// 월드 위젯은 첫 표시 순간에 생성하면 NativeConstruct 초기화와 ShowBubble 호출이 겹칠 수 있습니다.
		// BeginPlay에서 미리 만들어 둔 뒤 숨겨 두면, 첫 접근 때도 말풍선 표시만 안정적으로 실행됩니다.
		ResolvedHintWidgetComponent->InitWidget();
	}
	if (bEnableInteractionHint && bHideHintOnBeginPlay)
	{
		HideInteractionHint();
	}

	// 이벤트 바인딩 전에 이미 형성된 overlap도 첫 접근으로 처리합니다.
	RefreshOverlappingInteraction();

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

	if (!bDialogueInteractionEnabled)
	{
		return;
	}

	// ?곗궛 而ㅻ쾭 ??붾뒗 ?몃━嫄??덉뿉 ?ㅼ뼱???뚮젅?댁뼱瑜?湲곗뼲???먭퀬 留??깅쭏???곹깭瑜??ㅼ떆 寃?ы빀?덈떎.
	// ?뚮젅?댁뼱媛 癒쇱? ?ㅼ뼱?????곗궛???쇱퀜?? 議곌굔??留욌뒗 ?쒓컙遺??而ㅻ쾭 ?좎? ?쒓컙???볦엯?덈떎.
	// ?좎? ?쒓컙??RequiredCoverDuration???섏쑝硫??ㅼ젣 ??붾? ?쒖옉?⑸땲??
	if (!bRequireUmbrellaCoverHold || CurrentInstigatorActor == nullptr)
	{
		return;
	}

	ShowProximityDebugStatus(
		FString::Printf(
			TEXT("Proximity: IN | Actor:%s | Hint:%s | OverlapActors:%d | HintStatus:%s"),
			*GetNameSafe(CurrentInstigatorActor),
			bHintVisible ? TEXT("Visible") : TEXT("Hidden"),
			ActiveOverlapCounts.Num(),
			*LastHintDebugStatus),
		FColor::Green);

	// ?붾쾭洹?臾멸뎄???곗궛 蹂댁쑀 ?щ?瑜?蹂댁뿬二쇨린 ?꾪빐 ?곗궛 而댄룷?뚰듃瑜?李얠뒿?덈떎.
	const UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent(CurrentInstigatorActor);

	// ?뚮젅?댁뼱?몄?, ?곗궛??媛吏怨??덈뒗吏, ?곗궛 ?곹깭媛 留욌뒗吏 媛숈? 湲곕낯 議곌굔??癒쇱? 遊낅땲??
	const bool bCanCheckCover = PassesInstigatorRules(CurrentInstigatorActor);

	// ?ㅼ젣 而ㅻ쾭 ?먯젙?낅땲?? ?????곸쓽 而ㅻ쾭 ?寃??ㅽ뵾?닿? ?뚮젅?댁뼱????붿슜 ?곗궛 而ㅻ쾭 諛뺤뒪? 寃뱀튂硫??깃났?낅땲??
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
	if (!bDialogueInteractionEnabled)
	{
		ShowProximityDebugStatus(TEXT("Dialogue Start Blocked: interaction disabled"), FColor::Yellow);
		return false;
	}

	if (bTriggerOnce && bHasTriggered)
	{
		ShowProximityDebugStatus(TEXT("Dialogue Start Blocked: already triggered"), FColor::Yellow);
		return false;
	}

	if (!PassesInstigatorRules(InstigatorActor))
	{
		ShowProximityDebugStatus(TEXT("Dialogue Start Blocked: rule check failed"), FColor::Yellow);
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
	if (Source == nullptr || !Source->CanStartDialogue())
	{
		ShowCoverDebugMessage(TEXT("Dialogue Start Failed: source missing or blocked"), FColor::Red, 1.5f);
		return false;
	}

	HideInteractionHint();
	if (!Source->StartDialogue(InstigatorActor))
	{
		ShowInteractionHint();
		ShowCoverDebugMessage(TEXT("Dialogue Start Failed: UI subsystem missing"), FColor::Red, 1.5f);
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
		LastHintDebugStatus = TEXT("Failed:Disabled");
		return;
	}

	if (const UUOUUISubsystem* UISubsystem = ResolveUISubsystem())
	{
		if (UISubsystem->IsDialoguePlaying())
		{
			LastHintDebugStatus = TEXT("Failed:DialoguePlaying");
			return;
		}
	}

	FText DisplayHintText = HintText;
	double DisplayDuration = HintDuration > 0.0 ? HintDuration : 3.0;
	FName PresentationStyle = NAME_None;
	if (const UUOUDialogueSourceComponent* Source = ResolveDialogueSource())
	{
		// ????뚯뒪媛 ?덈뒗 ??곸? CSV??洹쇱젒 留먰뭾?좊쭔 ?ъ슜?⑸땲??
		// 媛믪씠 鍮꾩뼱 ?덉쑝硫?湲곕낯 臾쇱쓬?쒕줈 ?섎룎?꾧?吏 ?딄퀬 ?쒖떆?섏? ?딆뒿?덈떎.
		DisplayHintText = Source->GetProximityBubbleText();
		DisplayDuration = Source->GetProximityBubbleDuration();
		PresentationStyle = Source->GetProximityBubbleStyle();
	}

	if (DisplayHintText.IsEmpty())
	{
		LastHintDebugStatus = TEXT("Failed:TextEmpty");
		return;
	}

	UWidgetComponent* WidgetComponent = ResolveHintWidgetComponent();
	if (WidgetComponent == nullptr)
	{
		LastHintDebugStatus = TEXT("Failed:WidgetComponentMissing");
		ShowCoverDebugMessage(TEXT("Hint Show Failed: widget component missing"), FColor::Red, 1.5f);
		return;
	}

	SetHintWidgetComponentVisible(true);
	WidgetComponent->InitWidget();

	UUserWidget* UserWidget = WidgetComponent->GetUserWidgetObject();
	if (UserWidget == nullptr)
	{
		SetHintWidgetComponentVisible(false);
		LastHintDebugStatus = FString::Printf(TEXT("Failed:UserWidgetMissing Component:%s"), *GetNameSafe(WidgetComponent));
		ShowCoverDebugMessage(
			FString::Printf(TEXT("Hint Show Failed: user widget missing | Component:%s"), *GetNameSafe(WidgetComponent)),
			FColor::Red,
			1.5f);
		return;
	}

	if (!CallHintWidgetShowFunction(UserWidget, DisplayHintText, DisplayDuration, PresentationStyle))
	{
		SetHintWidgetComponentVisible(false);
		LastHintDebugStatus = FString::Printf(TEXT("Failed:ShowFunction Widget:%s Function:%s"),
			*GetNameSafe(UserWidget),
			*HintShowFunctionName.ToString());
		ShowCoverDebugMessage(
			FString::Printf(TEXT("Hint Show Failed: show function failed | Widget:%s | Function:%s"),
				*GetNameSafe(UserWidget),
				*HintShowFunctionName.ToString()),
			FColor::Red,
			1.5f);
		return;
	}

	SetHintWidgetComponentVisible(true);
	bHintVisible = true;
	LastHintDebugStatus = FString::Printf(TEXT("Success:%s"), *DisplayHintText.ToString());
	ShowCoverDebugMessage(
		FString::Printf(TEXT("Hint Show Success: %s"), *DisplayHintText.ToString()),
		FColor::Green,
		1.5f);
}

void UUOUDialogueTriggerComponent::HideInteractionHint()
{
	if (!bEnableInteractionHint)
	{
		LastHintDebugStatus = TEXT("HideSkipped:Disabled");
		return;
	}

	UUserWidget* UserWidget = GetHintUserWidget();
	bool bHideHandledByWidget = false;
	if (UserWidget != nullptr)
	{
		bHideHandledByWidget = CallHintWidgetHideFunction(UserWidget);
	}

	if (!bHideHandledByWidget)
	{
		SetHintWidgetComponentVisible(false);
	}
	bHintVisible = false;
	LastHintDebugStatus = TEXT("Hidden");
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

void UUOUDialogueTriggerComponent::SetDialogueInteractionEnabled(bool bNewEnabled)
{
	bDialogueInteractionEnabled = bNewEnabled;

	if (UUOUDialogueSourceComponent* Source = ResolveDialogueSource())
	{
		Source->SetDialogueAvailable(bDialogueInteractionEnabled);
	}

	if (!bDialogueInteractionEnabled)
	{
		ClearCoverProgress();
		if (!bShowHintWhenInteractionDisabled)
		{
			HideInteractionHint();
		}
		return;
	}

	// 같은 Activate 결과가 다시 들어온 경우에도 현재 overlap과 우산 상태는 달라질 수 있으므로 항상 재평가합니다.
	RefreshOverlappingInteraction();
}

void UUOUDialogueTriggerComponent::RefreshOverlappingInteraction()
{
	TArray<AActor*> InteractionCandidates;
	for (const TPair<TWeakObjectPtr<AActor>, int32>& OverlapEntry : ActiveOverlapCounts)
	{
		if (OverlapEntry.Value > 0)
		{
			if (AActor* TrackedActor = OverlapEntry.Key.Get())
			{
				InteractionCandidates.AddUnique(TrackedActor);
			}
		}
	}

	TArray<AActor*> PhysicallyOverlappingActors;
	GetOverlappingActors(PhysicallyOverlappingActors);
	for (AActor* PhysicallyOverlappingActor : PhysicallyOverlappingActors)
	{
		if (PhysicallyOverlappingActor != nullptr)
		{
			InteractionCandidates.AddUnique(PhysicallyOverlappingActor);
		}
	}

	for (AActor* OverlappingActor : InteractionCandidates)
	{
		if (!CanTrackOverlapActor(OverlappingActor))
		{
			continue;
		}

		if (bDialogueInteractionEnabled || bShowHintWhenInteractionDisabled)
		{
			ShowInteractionHint();
		}
		if (!bDialogueInteractionEnabled)
		{
			return;
		}

		if (bRequireUmbrellaCoverHold)
		{
			CurrentInstigatorActor = OverlappingActor;
			CurrentCoverHoldTime = 0.0f;
			bIsCurrentlyCoveredByUmbrella = false;
			SetComponentTickEnabled(true);

			const bool bPassesRules = PassesInstigatorRules(OverlappingActor);
			const bool bCovered = IsOwnerCoveredByDialogueCover(OverlappingActor);

			if (bPassesRules && bCovered)
			{
				bIsCurrentlyCoveredByUmbrella = true;
				CurrentCoverHoldTime = FMath::Max(0.0f, RequiredCoverDuration);
				TryStartDialogue(OverlappingActor);
			}
			return;
		}

		if (TryStartDialogue(OverlappingActor))
		{
			return;
		}
	}
}

void UUOUDialogueTriggerComponent::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
	case EOUUPuzzleResultAction::Resume:
		SetDialogueInteractionEnabled(true);
		break;
	case EOUUPuzzleResultAction::Deactivate:
	case EOUUPuzzleResultAction::Pause:
		SetDialogueInteractionEnabled(false);
		break;
	case EOUUPuzzleResultAction::Toggle:
		SetDialogueInteractionEnabled(!bDialogueInteractionEnabled);
		break;
	case EOUUPuzzleResultAction::None:
	default:
		break;
	}
}

void UUOUDialogueTriggerComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	HandleTrackedActorEnter(OtherActor, OtherComp);
}

void UUOUDialogueTriggerComponent::HandleTrackedActorEnter(AActor* OtherActor, UPrimitiveComponent* OtherComp)
{
	if (!CanTrackOverlapActor(OtherActor))
	{
		if (bRequireUmbrellaCoverHold)
		{
			ShowCoverDebugMessage(TEXT("Dialogue Trigger Overlap Failed: not a valid actor"), FColor::Red, 1.5f);
		}
		return;
	}

	// 媛숈? 罹먮┃???덉쓽 ?щ윭 而댄룷?뚰듃媛 ?우븘???≫꽣 湲곗? 泥?吏꾩엯留?泥섎━?⑸땲??
	if (!RegisterActorOverlap(OtherActor))
	{
		return;
	}

	if (!bDialogueInteractionEnabled)
	{
		if (bShowHintWhenInteractionDisabled)
		{
			ShowInteractionHint();
		}
		return;
	}

	// 플랫폼 이동 중에는 트리거 겹침이 반복될 수 있으므로 근접 진입은 고정 상태 줄에서만 보여줍니다.
	ShowProximityDebugStatus(
		FString::Printf(TEXT("Dialogue Proximity Accepted: %s"), *GetNameSafe(OtherActor)),
		FColor::Green);

	if (bRequireUmbrellaCoverHold)
	{
		ShowCoverDebugMessage(FString::Printf(TEXT("Dialogue Trigger Overlap: %s"), *GetNameSafe(OtherActor)), FColor::White, 1.5f);

		// 而ㅻ쾭????붾뒗 ?몃━嫄곗뿉 ?ㅼ뼱???뚮젅?댁뼱瑜??쇰떒 異붿쟻?⑸땲??
		// 洹몃옒???몃━嫄??덉뿉 癒쇱? ?ㅼ뼱?????곗궛???쇱튂???먮쫫??Tick?먯꽌 ?ㅼ떆 寃?ы븷 ???덉뒿?덈떎.
		if (CanTrackOverlapActor(OtherActor))
		{
			CurrentInstigatorActor = OtherActor;
			CurrentCoverHoldTime = 0.0f;
			bIsCurrentlyCoveredByUmbrella = false;
			SetComponentTickEnabled(true);

			ShowInteractionHint();

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

	// ?꾩쭅 媛숈? ?≫꽣???ㅻⅨ 而댄룷?뚰듃媛 ?몃━嫄??덉뿉 ?⑥븘 ?덉쑝硫??ㅼ젣 ?댄깉濡?蹂댁? ?딆뒿?덈떎.
	if (!UnregisterActorOverlap(OtherActor))
	{
		return;
	}

	if (OtherActor == CurrentInstigatorActor)
	{
		ShowCoverDebugMessage(FString::Printf(TEXT("Dialogue Trigger EndOverlap: %s"), *GetNameSafe(OtherActor)), FColor::Orange, 1.5f);
		ClearCoverProgress();
	}

	ShowProximityDebugStatus(
		FString::Printf(TEXT("Dialogue Proximity Exit: %s"), *GetNameSafe(OtherActor)),
		FColor::Orange);

	HideInteractionHint();
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

	if (bRequireOpenUmbrella && !UmbrellaComponent->IsOpen())
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

	// 而ㅻ쾭????붾뒗 ?곗궛???섏쨷???쇱튂??寃쎌슦???덉슜?댁빞 ?섎?濡?
	// ?ш린?쒕뒗 Pawn 媛숈? 理쒖냼 異붿쟻 議곌굔留?寃?ы븯怨??곗궛 ?곹깭 寃?щ뒗 Tick?먯꽌 ?곕줈 泥섎━?⑸땲??
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

	// 1?대㈃ ???≫꽣媛 泥섏쓬 ?ㅼ뼱???쒓컙?낅땲??
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

	// 0?대㈃ ???≫꽣??紐⑤뱺 寃뱀묠???앸궃 ?쒓컙?낅땲??
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

	// ???而ㅻ쾭 ?먯젙? ?댁젣 RainBlocker 醫뚰몴 怨꾩궛???곗? ?딆뒿?덈떎.
	// NPC 履?DialogueCoverTarget 湲곗???諛섍꼍怨??뚮젅?댁뼱 履?UmbrellaCoverVolume 諛뺤뒪媛 ?ㅼ젣濡??우븯?붿? ?뺤씤?⑸땲??
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

			// ?붾쾭洹몄뿉 ?쒖떆?섎뒗 Gap 怨꾩궛怨??ㅼ젣 ?깃났 ?먯젙??諛섎뱶??媛숈? 媛믪뿉??戮묒뒿?덈떎.
			// ?댁쟾泥섎읆 蹂꾨룄 ?⑥닔 寃곌낵瑜??욎쑝硫??붾㈃??Gap 0?몃뜲 Touch No媛 ?섏? ?먯씤???룰컝由ш쾶 留뚮벊?덈떎.
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
	if (!bHasSavedCameraOcclusionEnabled)
	{
		bSavedCameraOcclusionEnabled = ActiveDialogueCameraController->IsCameraOcclusionEnabled();
		bHasSavedCameraOcclusionEnabled = true;
	}

	if (bEnableCameraOcclusionDuringDialogueFocus)
	{
		ActiveDialogueCameraController->SetCameraOcclusionEnabled(true);
	}

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
		if (bHasSavedCameraOcclusionEnabled)
		{
			ActiveDialogueCameraController->SetCameraOcclusionEnabled(bSavedCameraOcclusionEnabled);
		}
		ActiveDialogueCameraController = nullptr;
	}

	bSavedCameraOcclusionEnabled = false;
	bHasSavedCameraOcclusionEnabled = false;

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

	UUOUPlayerInteractionExecutorComponent* InputExecutor =
		UUOUPlayerInteractionExecutorComponent::FindLocalPlayerExecutor(this);
	if (InputExecutor == nullptr)
	{
		return;
	}

	InputExecutor->RequestPlayerInputBlock(this, true);
	LockedInputExecutorComponent = InputExecutor;
	bDialogueMovementLocked = true;
}

void UUOUDialogueTriggerComponent::UnlockMovementForDialogue()
{
	if (!bDialogueMovementLocked)
	{
		return;
	}

	if (LockedInputExecutorComponent != nullptr)
	{
		LockedInputExecutorComponent->ReleasePlayerInputBlock(this);
		LockedInputExecutorComponent = nullptr;
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

bool UUOUDialogueTriggerComponent::CallHintWidgetShowFunction(
	UUserWidget* UserWidget,
	const FText& DisplayHintText,
	double DisplayDuration,
	FName PresentationStyle) const
{
	if (UserWidget == nullptr)
	{
		return false;
	}

	if (UUOUSpeechBubbleWidget* SpeechBubbleWidget = Cast<UUOUSpeechBubbleWidget>(UserWidget))
	{
		SpeechBubbleWidget->ShowBubbleStyled(DisplayHintText, DisplayDuration, PresentationStyle);
		return true;
	}

	if (HintShowFunctionName.IsNone())
	{
		return false;
	}

	UFunction* ShowFunction = UserWidget->FindFunction(HintShowFunctionName);
	if (ShowFunction == nullptr)
	{
		return false;
	}

	FUOUDialogueHintBubbleParams Params;
	Params.BubbleText = DisplayHintText;
	Params.Duration = DisplayDuration;
	UserWidget->ProcessEvent(ShowFunction, &Params);
	return true;
}

bool UUOUDialogueTriggerComponent::CallHintWidgetHideFunction(UUserWidget* UserWidget) const
{
	if (UserWidget == nullptr)
	{
		return false;
	}

	if (UUOUSpeechBubbleWidget* SpeechBubbleWidget = Cast<UUOUSpeechBubbleWidget>(UserWidget))
	{
		SpeechBubbleWidget->HideBubble();
		return true;
	}

	if (HintHideFunctionName.IsNone())
	{
		return false;
	}

	UFunction* HideFunction = UserWidget->FindFunction(HintHideFunctionName);
	if (HideFunction != nullptr)
	{
		UserWidget->ProcessEvent(HideFunction, nullptr);
		return true;
	}

	return false;
}

UUserWidget* UUOUDialogueTriggerComponent::GetHintUserWidget()
{
	UWidgetComponent* WidgetComponent = ResolveHintWidgetComponent();
	return WidgetComponent != nullptr ? WidgetComponent->GetUserWidgetObject() : nullptr;
}

void UUOUDialogueTriggerComponent::ShowCoverDebugMessage(const FString&, const FColor&, float) const
{
}

void UUOUDialogueTriggerComponent::ShowCoverDebugStatus(const FString&, const FColor&) const
{
}

void UUOUDialogueTriggerComponent::ShowProximityDebugStatus(const FString&, const FColor&) const
{
}
