// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOULightCountBulbActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "World/Light/UOULightExposureReceiverComponent.h"

namespace UOULightCountBulbPrivate
{
	AActor* ResolveSourceActor(UObject* SourceObject)
	{
		if (AActor* SourceActor = Cast<AActor>(SourceObject))
		{
			return SourceActor;
		}

		const UActorComponent* SourceComponent = Cast<UActorComponent>(SourceObject);
		return SourceComponent != nullptr ? SourceComponent->GetOwner() : nullptr;
	}

	FString GetSourceDebugName(UObject* SourceObject)
	{
		const AActor* SourceActor = ResolveSourceActor(SourceObject);
		if (SourceActor == nullptr)
		{
			return GetNameSafe(SourceObject);
		}

#if WITH_EDITOR
		return SourceActor->GetActorLabel();
#else
		return SourceActor->GetName();
#endif
	}
}

AUOULightCountBulbActor::AUOULightCountBulbActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	BulbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulbMesh"));
	BulbMesh->SetupAttachment(RootScene);
	BulbMesh->SetMobility(EComponentMobility::Movable);
	BulbMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		BulbMesh->SetStaticMesh(SphereMeshFinder.Object);
	}

	LightReceiverVolume = CreateDefaultSubobject<USphereComponent>(TEXT("LightReceiverVolume"));
	LightReceiverVolume->SetupAttachment(RootScene);
	LightReceiverVolume->InitSphereRadius(55.0f);
	LightReceiverVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LightReceiverVolume->SetCollisionObjectType(ECC_WorldDynamic);
	LightReceiverVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
	LightReceiverVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	LightReceiverVolume->SetGenerateOverlapEvents(false);

	LightReceiver = CreateDefaultSubobject<UUOULightExposureReceiverComponent>(TEXT("LightReceiver"));
	LightReceiver->bAutoFindReceiverTransform = false;
	LightReceiver->bUseReceiverVolumeSampling = false;
	LightReceiver->bUseBeamVolumeOverlap = true;
	LightReceiver->MinimumBeamOverlapDepth = 10.0f;
	LightReceiver->TemperatureRisePerIntensity = 0.0f;
	LightReceiver->bRecoverToAmbientWhenNotExposed = false;
	LightReceiver->ReceiverTransformReference.ComponentProperty =
		GET_MEMBER_NAME_CHECKED(AUOULightCountBulbActor, LightReceiverVolume);
	LightReceiver->ReceiverVolumeReference.ComponentProperty =
		GET_MEMBER_NAME_CHECKED(AUOULightCountBulbActor, LightReceiverVolume);
}

void AUOULightCountBulbActor::BeginPlay()
{
	Super::BeginPlay();

	RequiredLightCount = FMath::Max(1, RequiredLightCount);
	LightSourceLossGraceTime = FMath::Max(0.0f, LightSourceLossGraceTime);
	OffEmissiveIntensity = FMath::Max(0.0f, OffEmissiveIntensity);
	InsufficientEmissiveIntensity = FMath::Max(0.0f, InsufficientEmissiveIntensity);
	SatisfiedEmissiveIntensity = FMath::Max(0.0f, SatisfiedEmissiveIntensity);
	OverheatedEmissiveIntensity = FMath::Max(0.0f, OverheatedEmissiveIntensity);

	if (LightReceiver != nullptr)
	{
		LightReceiver->OnLightExposureReceived.RemoveDynamic(
			this,
			&AUOULightCountBulbActor::HandleLightExposureReceived);
		LightReceiver->OnLightExposureReceived.AddDynamic(
			this,
			&AUOULightCountBulbActor::HandleLightExposureReceived);
	}

	EnsureRuntimeMaterials();
	RefreshBulbState();
	ApplyStateVisual();
}

void AUOULightCountBulbActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (LightReceiver != nullptr)
	{
		LightReceiver->OnLightExposureReceived.RemoveDynamic(
			this,
			&AUOULightCountBulbActor::HandleLightExposureReceived);
	}

	ActiveLightExpirationTimes.Reset();
	Super::EndPlay(EndPlayReason);
}

void AUOULightCountBulbActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	RefreshBulbState();
}

EUOUDebugCategory AUOULightCountBulbActor::GetDebugCategory_Implementation() const
{
	return EUOUDebugCategory::Puzzle;
}

FText AUOULightCountBulbActor::GetDebugSummaryText_Implementation() const
{
	TArray<FString> SourceNames;
	for (const TPair<TWeakObjectPtr<UObject>, float>& ActiveSource : ActiveLightExpirationTimes)
	{
		if (UObject* SourceObject = ActiveSource.Key.Get())
		{
			SourceNames.Add(UOULightCountBulbPrivate::GetSourceDebugName(SourceObject));
		}
	}
	SourceNames.Sort();

	const TArray<FString> DebugLines = {
		FString::Printf(TEXT("Bulb State: %s"), *GetDebugStateName()),
		FString::Printf(TEXT("Lights: %d / %d"), ActiveLightCount, RequiredLightCount),
		FString::Printf(
			TEXT("Sources: %s"),
			SourceNames.IsEmpty() ? TEXT("None") : *FString::Join(SourceNames, TEXT(", ")))
	};
	return FText::FromString(FString::Join(DebugLines, LINE_TERMINATOR));
}

void AUOULightCountBulbActor::GetDebugConnections_Implementation(
	TArray<FUOUDebugConnection>& OutConnections) const
{
	for (const TPair<TWeakObjectPtr<UObject>, float>& ActiveSource : ActiveLightExpirationTimes)
	{
		UObject* SourceObject = ActiveSource.Key.Get();
		if (!IsValid(SourceObject))
		{
			continue;
		}

		AActor* SourceActor = UOULightCountBulbPrivate::ResolveSourceActor(SourceObject);
		FUOUDebugConnection& Connection = OutConnections.AddDefaulted_GetRef();
		Connection.SourceObject = SourceActor != nullptr ? SourceActor : SourceObject;
		Connection.TargetObject = const_cast<AUOULightCountBulbActor*>(this);
		Connection.ConnectionType = EUOUDebugConnectionType::PuzzleInput;
		Connection.Label = FText::FromString(
			UOULightCountBulbPrivate::GetSourceDebugName(SourceObject));
		Connection.Color = GetDebugStateColor();
		Connection.Thickness = 2.0f;
	}
}

EUOULightCountBulbState AUOULightCountBulbActor::EvaluateState(
	int32 LightCount,
	int32 RequiredCount)
{
	const int32 SafeLightCount = FMath::Max(0, LightCount);
	const int32 SafeRequiredCount = FMath::Max(1, RequiredCount);
	if (SafeLightCount == 0)
	{
		return EUOULightCountBulbState::Off;
	}
	if (SafeLightCount < SafeRequiredCount)
	{
		return EUOULightCountBulbState::Insufficient;
	}
	if (SafeLightCount == SafeRequiredCount)
	{
		return EUOULightCountBulbState::Satisfied;
	}
	return EUOULightCountBulbState::Overheated;
}

void AUOULightCountBulbActor::RefreshBulbState()
{
	const UWorld* World = GetWorld();
	const float CurrentTime = World != nullptr ? World->GetTimeSeconds() : 0.0f;
	for (auto SourceIt = ActiveLightExpirationTimes.CreateIterator(); SourceIt; ++SourceIt)
	{
		const bool bSourceExpired = !SourceIt.Key().IsValid() || CurrentTime > SourceIt.Value();
		if (bSourceExpired)
		{
			SourceIt.RemoveCurrent();
		}
	}

	ActiveLightCount = ActiveLightExpirationTimes.Num();
	SetBulbState(EvaluateState(ActiveLightCount, RequiredLightCount));
	SetActorTickEnabled(ActiveLightCount > 0);
}

void AUOULightCountBulbActor::HandleLightExposureReceived(
	const FUOULightExposureData& ExposureData)
{
	if (!IsValid(ExposureData.Source.Get()) || ExposureData.Intensity <= 0.0f)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World != nullptr ? World->GetTimeSeconds() : 0.0f;
	// 같은 광원의 직접광과 반사광, 반복 샘플은 하나의 고유 광원으로 유지합니다.
	const TWeakObjectPtr<UObject> SourceKey(ExposureData.Source.Get());
	const float EffectiveGraceTime = FMath::Max(
		LightSourceLossGraceTime,
		FMath::Max(0.0f, ExposureData.DeltaTime) * 1.5f);
	ActiveLightExpirationTimes.FindOrAdd(SourceKey) = CurrentTime + EffectiveGraceTime;
	RefreshBulbState();
}

void AUOULightCountBulbActor::SetBulbState(EUOULightCountBulbState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	const EUOULightCountBulbState PreviousState = CurrentState;
	const bool bWasSatisfied = PreviousState == EUOULightCountBulbState::Satisfied;
	CurrentState = NewState;
	ApplyStateVisual();
	OnBulbStateChanged.Broadcast(CurrentState, PreviousState, ActiveLightCount);

	const bool bIsNowSatisfied = CurrentState == EUOULightCountBulbState::Satisfied;
	if (bWasSatisfied != bIsNowSatisfied)
	{
		OnBulbSatisfiedChanged.Broadcast(bIsNowSatisfied);
	}
}

void AUOULightCountBulbActor::EnsureRuntimeMaterials()
{
	if (BulbMesh == nullptr || RuntimeMaterialInstances.Num() == BulbMesh->GetNumMaterials())
	{
		return;
	}

	RuntimeMaterialInstances.Reset();
	for (int32 MaterialIndex = 0; MaterialIndex < BulbMesh->GetNumMaterials(); ++MaterialIndex)
	{
		RuntimeMaterialInstances.Add(
			BulbMesh->CreateAndSetMaterialInstanceDynamic(MaterialIndex));
	}
}

void AUOULightCountBulbActor::ApplyStateVisual()
{
	EnsureRuntimeMaterials();
	const FLinearColor StateColor = GetStateColor();
	const float EmissiveIntensity = GetStateEmissiveIntensity();
	for (UMaterialInstanceDynamic* MaterialInstance : RuntimeMaterialInstances)
	{
		if (MaterialInstance == nullptr)
		{
			continue;
		}

		MaterialInstance->SetVectorParameterValue(PrimaryColorParameterName, StateColor);
		MaterialInstance->SetVectorParameterValue(SecondaryColorParameterName, StateColor);
		MaterialInstance->SetVectorParameterValue(EmissiveColorParameterName, StateColor);
		MaterialInstance->SetScalarParameterValue(EmissiveIntensityParameterName, EmissiveIntensity);
	}
}

FLinearColor AUOULightCountBulbActor::GetStateColor() const
{
	switch (CurrentState)
	{
	case EUOULightCountBulbState::Insufficient:
		return InsufficientColor;
	case EUOULightCountBulbState::Satisfied:
		return SatisfiedColor;
	case EUOULightCountBulbState::Overheated:
		return OverheatedColor;
	case EUOULightCountBulbState::Off:
	default:
		return OffColor;
	}
}

float AUOULightCountBulbActor::GetStateEmissiveIntensity() const
{
	switch (CurrentState)
	{
	case EUOULightCountBulbState::Insufficient:
		return InsufficientEmissiveIntensity;
	case EUOULightCountBulbState::Satisfied:
		return SatisfiedEmissiveIntensity;
	case EUOULightCountBulbState::Overheated:
		return OverheatedEmissiveIntensity;
	case EUOULightCountBulbState::Off:
	default:
		return OffEmissiveIntensity;
	}
}

FString AUOULightCountBulbActor::GetDebugStateName() const
{
	switch (CurrentState)
	{
	case EUOULightCountBulbState::Insufficient:
		return TEXT("Insufficient");
	case EUOULightCountBulbState::Satisfied:
		return TEXT("Satisfied");
	case EUOULightCountBulbState::Overheated:
		return TEXT("Overheated");
	case EUOULightCountBulbState::Off:
	default:
		return TEXT("Off");
	}
}

FColor AUOULightCountBulbActor::GetDebugStateColor() const
{
	switch (CurrentState)
	{
	case EUOULightCountBulbState::Insufficient:
		return FColor::Orange;
	case EUOULightCountBulbState::Satisfied:
		return FColor::Green;
	case EUOULightCountBulbState::Overheated:
		return FColor::Red;
	case EUOULightCountBulbState::Off:
	default:
		return FColor::Silver;
	}
}
