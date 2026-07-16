// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUUmbrellaComponent.h"
#include "Player/UOUUmbrellaRuntimeVisualPresenter.h"
#include "Player/UOUUmbrellaSkeletalVisualPresenter.h"
#include "Player/UOUUmbrellaVisualPolicy.h"
#include "Player/UOUWaterContainerComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUUmbrellaStateTransitionTest,
	"UnderOneUmBrella.Player.Umbrella.StateTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUUmbrellaStateTransitionTest::RunTest(const FString& Parameters)
{
	UUOUUmbrellaComponent* Umbrella = NewObject<UUOUUmbrellaComponent>();
	TestNotNull(TEXT("Umbrella component can be created"), Umbrella);
	if (Umbrella == nullptr)
	{
		return false;
	}

	Umbrella->AcquireUmbrella();
	TestTrue(TEXT("Acquire grants umbrella ownership"), Umbrella->HasUmbrella());
	TestTrue(TEXT("Acquired umbrella starts closed"), Umbrella->IsClosed());
	TestEqual(TEXT("Closed umbrella uses the closed visual state"), Umbrella->GetCurrentVisualState(), EUOUUmbrellaVisualState::Closed);

	Umbrella->OpenUmbrella();
	TestTrue(TEXT("Open command enters the open state"), Umbrella->IsOpen());
	TestEqual(TEXT("Open umbrella uses the open visual state"), Umbrella->GetCurrentVisualState(), EUOUUmbrellaVisualState::Open);

	Umbrella->TurnUmbrellaUpsideDown();
	TestTrue(TEXT("Invert command enters the upside-down state"), Umbrella->IsUpsideDown());
	TestEqual(TEXT("Upside-down umbrella uses the reversed-open visual state"), Umbrella->GetCurrentVisualState(), EUOUUmbrellaVisualState::OpenReversed);

	Umbrella->CloseUmbrella();
	TestTrue(TEXT("Close command returns to the closed state"), Umbrella->IsClosed());
	TestTrue(TEXT("Closing resets the umbrella direction"), Umbrella->IsNormalDirection());

	Umbrella->RemoveUmbrella();
	TestFalse(TEXT("Remove clears umbrella ownership"), Umbrella->HasUmbrella());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUUmbrellaPourVisualStateTest,
	"UnderOneUmBrella.Player.Umbrella.PourVisualState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUUmbrellaPourVisualStateTest::RunTest(const FString& Parameters)
{
	UUOUUmbrellaComponent* Umbrella = NewObject<UUOUUmbrellaComponent>();
	UUOUWaterContainerComponent* WaterContainer = NewObject<UUOUWaterContainerComponent>();
	TestNotNull(TEXT("Umbrella component can be created"), Umbrella);
	TestNotNull(TEXT("Water container can be created"), WaterContainer);
	if (Umbrella == nullptr || WaterContainer == nullptr)
	{
		return false;
	}

	Umbrella->StoredWaterContainer = WaterContainer;
	Umbrella->AcquireUmbrella();
	Umbrella->TurnUmbrellaUpsideDown();
	WaterContainer->SetAmount(1.0f);
	Umbrella->BeginPour();

	TestTrue(TEXT("Pour begins from the upside-down state when water is available"), Umbrella->IsPouring());
	TestEqual(TEXT("Pouring keeps the reversed-open visual state"), Umbrella->GetCurrentVisualState(), EUOUUmbrellaVisualState::OpenReversed);

	Umbrella->EndPour();
	TestTrue(TEXT("Ending pour returns to the upside-down state"), Umbrella->IsUpsideDown());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUUmbrellaVisualPolicyTest,
	"UnderOneUmBrella.Player.Umbrella.VisualPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUUmbrellaVisualPolicyTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Closed gameplay state resolves to the closed visual"),
		FUOUUmbrellaVisualPolicy::ResolveVisualState(EUOUUmbrellaState::Closed, false),
		EUOUUmbrellaVisualState::Closed);
	TestEqual(
		TEXT("Closed reversed override is preserved"),
		FUOUUmbrellaVisualPolicy::ResolveVisualState(EUOUUmbrellaState::Closed, true),
		EUOUUmbrellaVisualState::ClosedReversed);
	TestEqual(
		TEXT("Pouring resolves to the reversed-open visual"),
		FUOUUmbrellaVisualPolicy::ResolveVisualState(EUOUUmbrellaState::Pouring, false),
		EUOUUmbrellaVisualState::OpenReversed);

	const FUOUUmbrellaVisualVisibility DedicatedReversed = FUOUUmbrellaVisualPolicy::ResolveVisibility(
		true,
		EUOUUmbrellaVisualState::OpenReversed,
		true,
		true,
		true,
		true);
	TestFalse(TEXT("Dedicated reversed visual hides the normal open mesh"), DedicatedReversed.bShowOpen);
	TestTrue(TEXT("Dedicated reversed visual shows the upside-down mesh"), DedicatedReversed.bShowUpsideDown);
	TestFalse(TEXT("Dedicated reversed visual does not need the runtime fallback"), DedicatedReversed.bShowRuntime);

	const FUOUUmbrellaVisualVisibility RuntimeFallback = FUOUUmbrellaVisualPolicy::ResolveVisibility(
		true,
		EUOUUmbrellaVisualState::OpenReversed,
		true,
		false,
		true,
		true);
	TestFalse(TEXT("Runtime fallback hides the normal open mesh"), RuntimeFallback.bShowOpen);
	TestTrue(TEXT("Runtime fallback shows the copied pickup mesh"), RuntimeFallback.bShowRuntime);
	TestTrue(TEXT("Runtime fallback flips the copied pickup mesh"), RuntimeFallback.bFlipRuntime);

	const FUOUUmbrellaVisualVisibility MissingReversedFallback = FUOUUmbrellaVisualPolicy::ResolveVisibility(
		true,
		EUOUUmbrellaVisualState::OpenReversed,
		true,
		false,
		false,
		true);
	TestTrue(TEXT("Normal open mesh is the final fallback when no reversed visual exists"), MissingReversedFallback.bShowOpen);

	const FUOUUmbrellaVisualVisibility NotOwned = FUOUUmbrellaVisualPolicy::ResolveVisibility(
		false,
		EUOUUmbrellaVisualState::Open,
		true,
		true,
		true,
		true);
	TestFalse(TEXT("No dedicated visual is shown without umbrella ownership"), NotOwned.bShowOpen || NotOwned.bShowUpsideDown);
	TestFalse(TEXT("No runtime visual is shown without umbrella ownership"), NotOwned.bShowRuntime);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUUmbrellaSkeletalVisualPresenterTest,
	"UnderOneUmBrella.Player.Umbrella.SkeletalVisualPresenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUUmbrellaSkeletalVisualPresenterTest::RunTest(const FString& Parameters)
{
	FUOUUmbrellaSkeletalVisualVariants Variants;
	Variants.Closed.SocketName = TEXT("ClosedSocket");
	Variants.Open.SocketName = TEXT("OpenSocket");
	Variants.ClosedReversed.SocketName = TEXT("ClosedReversedSocket");
	Variants.OpenReversed.SocketName = TEXT("OpenReversedSocket");

	TestEqual(
		TEXT("Closed state selects the closed skeletal variant"),
		Variants.Resolve(EUOUUmbrellaVisualState::Closed).SocketName,
		FName(TEXT("ClosedSocket")));
	TestEqual(
		TEXT("Open state selects the open skeletal variant"),
		Variants.Resolve(EUOUUmbrellaVisualState::Open).SocketName,
		FName(TEXT("OpenSocket")));
	TestEqual(
		TEXT("Closed reversed state selects its dedicated skeletal variant"),
		Variants.Resolve(EUOUUmbrellaVisualState::ClosedReversed).SocketName,
		FName(TEXT("ClosedReversedSocket")));
	TestEqual(
		TEXT("Open reversed state selects its dedicated skeletal variant"),
		Variants.Resolve(EUOUUmbrellaVisualState::OpenReversed).SocketName,
		FName(TEXT("OpenReversedSocket")));

	USkeletalMeshComponent* Visual = NewObject<USkeletalMeshComponent>();
	TestNotNull(TEXT("Skeletal visual can be created"), Visual);
	if (Visual == nullptr)
	{
		return false;
	}

	Visual->SetVisibility(true);
	FUOUUmbrellaSkeletalVisualRequest Request;
	Request.Visual = Visual;
	Request.bHasUmbrella = false;
	Request.State = EUOUUmbrellaState::Closed;
	Request.DirectionState = EUOUUmbrellaDirectionState::Normal;
	Request.VisualState = EUOUUmbrellaVisualState::Closed;

	FUOUUmbrellaSkeletalVisualPlaybackState PlaybackState;
	PlaybackState.bHasAppliedAnimation = true;
	FUOUUmbrellaSkeletalVisualPresenter::Apply(Request, PlaybackState);

	TestFalse(TEXT("Presenter hides the skeletal visual when the umbrella is not owned"), Visual->IsVisible());
	TestEqual(TEXT("Presenter disables skeletal visual collision"), Visual->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestFalse(TEXT("Presenter disables skeletal visual overlap events"), Visual->GetGenerateOverlapEvents());
	TestFalse(TEXT("Hiding the umbrella resets direct animation playback state"), PlaybackState.bHasAppliedAnimation);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOUUmbrellaRuntimeVisualPresenterTest,
	"UnderOneUmBrella.Player.Umbrella.RuntimeVisualPresenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUUmbrellaRuntimeVisualPresenterTest::RunTest(const FString& Parameters)
{
	const FTransform AnchorTransform(
		FRotator(0.0f, 30.0f, 0.0f),
		FVector(10.0f, 20.0f, 30.0f),
		FVector(9.0f));
	const FTransform BaseTransform = FUOUUmbrellaRuntimeVisualPresenter::CalculateBaseRelativeTransform(
		AnchorTransform,
		FVector(2.0f, 3.0f, 4.0f),
		FVector(0.5f, 2.0f, 1.5f),
		true);

	TestEqual(TEXT("Runtime visual base keeps the anchor location"), BaseTransform.GetLocation(), AnchorTransform.GetLocation());
	TestTrue(TEXT("Runtime visual base keeps the anchor rotation"), BaseTransform.GetRotation().Equals(AnchorTransform.GetRotation()));
	TestEqual(TEXT("Runtime visual base combines held and pickup scale"), BaseTransform.GetScale3D(), FVector(1.0f, 6.0f, 6.0f));

	const FTransform ScaleIgnored = FUOUUmbrellaRuntimeVisualPresenter::CalculateBaseRelativeTransform(
		AnchorTransform,
		FVector(2.0f, 3.0f, 4.0f),
		FVector(0.5f, 2.0f, 1.5f),
		false);
	TestEqual(TEXT("Pickup scale can be ignored"), ScaleIgnored.GetScale3D(), FVector(2.0f, 3.0f, 4.0f));

	const FTransform ReversedTransform = FUOUUmbrellaRuntimeVisualPresenter::CalculateStateRelativeTransform(
		BaseTransform,
		true,
		EUOUUmbrellaVisualState::OpenReversed,
		FRotator(180.0f, 0.0f, 0.0f),
		FVector(0.0f, 0.0f, 150.0f));
	TestEqual(TEXT("Reversed visual adds its location offset"), ReversedTransform.GetLocation(), FVector(10.0f, 20.0f, 180.0f));
	TestFalse(TEXT("Reversed visual applies an additional rotation"), ReversedTransform.GetRotation().Equals(BaseTransform.GetRotation()));

	const FTransform OpenTransform = FUOUUmbrellaRuntimeVisualPresenter::CalculateStateRelativeTransform(
		BaseTransform,
		true,
		EUOUUmbrellaVisualState::Open,
		FRotator(180.0f, 0.0f, 0.0f),
		FVector(0.0f, 0.0f, 150.0f));
	TestTrue(TEXT("Normal open visual keeps the base transform"), OpenTransform.Equals(BaseTransform));

	UStaticMeshComponent* SourceVisual = NewObject<UStaticMeshComponent>();
	UStaticMeshComponent* TargetVisual = NewObject<UStaticMeshComponent>();
	UStaticMesh* Mesh = NewObject<UStaticMesh>();
	TestNotNull(TEXT("Source runtime visual can be created"), SourceVisual);
	TestNotNull(TEXT("Target runtime visual can be created"), TargetVisual);
	TestNotNull(TEXT("Runtime visual mesh can be created"), Mesh);
	if (SourceVisual == nullptr || TargetVisual == nullptr || Mesh == nullptr)
	{
		return false;
	}

	SourceVisual->SetStaticMesh(Mesh);
	SourceVisual->SetRelativeScale3D(FVector(1.5f, 2.0f, 2.5f));
	const FUOUUmbrellaRuntimeVisualAssets CapturedAssets = FUOUUmbrellaRuntimeVisualPresenter::CaptureAssets(SourceVisual);
	TestEqual(TEXT("Presenter captures the pickup static mesh"), CapturedAssets.Mesh, Mesh);
	TestEqual(TEXT("Presenter captures the pickup relative scale"), CapturedAssets.SourceRelativeScale, FVector(1.5f, 2.0f, 2.5f));

	FUOUUmbrellaRuntimeVisualPresenter::ApplyAssets(TargetVisual, CapturedAssets, nullptr);
	TestTrue(TEXT("Presenter applies the captured static mesh"), TargetVisual->GetStaticMesh() == Mesh);
	TestEqual(
		TEXT("Ensure returns an existing runtime visual without requiring an owner"),
		FUOUUmbrellaRuntimeVisualPresenter::EnsureVisual(nullptr, nullptr, TargetVisual, FTransform::Identity),
		TargetVisual);

	return true;
}

#endif
