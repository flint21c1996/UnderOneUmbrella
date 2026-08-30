// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOULightReflectionSpotLightComponent.h"

#include "Components/SpotLightComponent.h"
#include "GameFramework/Actor.h"
#include "World/Light/UOULightExposureSourceComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogUOULightReflectionSpotLight, Log, All);

UUOULightReflectionSpotLightComponent::UUOULightReflectionSpotLightComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOULightReflectionSpotLightComponent::BeginPlay()
{
	Super::BeginPlay();

	BoundSourceComponent = ResolveSourceComponent();
	if (AActor* Owner = GetOwner())
	{
		// 반사 보조 SpotLight 풀을 만들기 전에 원본 컴포넌트를 고정해 둡니다.
		ManagedSourceSpotLight = Owner->FindComponentByClass<USpotLightComponent>();
	}
	if (BoundSourceComponent == nullptr)
	{
		HideUnusedSpotLights(0);
		return;
	}

	BoundSourceComponent->OnLightPathsUpdated.AddDynamic(
		this,
		&UUOULightReflectionSpotLightComponent::HandleLightPathsUpdated);
	RefreshSpotLights();
}

void UUOULightReflectionSpotLightComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundSourceComponent != nullptr)
	{
		BoundSourceComponent->OnLightPathsUpdated.RemoveDynamic(
			this,
			&UUOULightReflectionSpotLightComponent::HandleLightPathsUpdated);
	}

	for (USpotLightComponent* SpotLight : SpotLightPool)
	{
		if (SpotLight != nullptr)
		{
			SpotLight->DestroyComponent();
		}
	}
	SpotLightPool.Reset();
	SetSourceSpotLightSuppressed(false);
	ManagedSourceSpotLight = nullptr;
	ActiveSpotLightCount = 0;
	bHasWarnedSpotLightLimit = false;
	BoundSourceComponent = nullptr;

	Super::EndPlay(EndPlayReason);
}

void UUOULightReflectionSpotLightComponent::RefreshSpotLights()
{
	if (BoundSourceComponent == nullptr)
	{
		BoundSourceComponent = ResolveSourceComponent();
	}

	if (!bEnabled || BoundSourceComponent == nullptr || MaxSpotLightCount <= 0)
	{
		SetSourceSpotLightSuppressed(false);
		HideUnusedSpotLights(0);
		return;
	}

	HandleLightPathsUpdated(BoundSourceComponent->GetLightPaths());
}

void UUOULightReflectionSpotLightComponent::HandleLightPathsUpdated(
	const TArray<FUOULightPathData>& LightPaths)
{
	if (!bEnabled)
	{
		SetSourceSpotLightSuppressed(false);
		HideUnusedSpotLights(0);
		return;
	}

	int32 EligibleSegmentCount = 0;
	for (const FUOULightPathData& PathData : LightPaths)
	{
		for (const FUOULightPathSegmentData& SegmentData : PathData.Segments)
		{
			if (SegmentData.bReflected &&
				SegmentData.Length >= MinimumSegmentLength &&
				SegmentData.Intensity > 0.0f &&
				!SegmentData.Direction.IsNearlyZero())
			{
				++EligibleSegmentCount;
			}
		}
	}

	// 실제 원본 SpotLight는 커스텀 반사 경로를 알지 못해 거울 뒤까지 직진합니다.
	// 반사가 성립한 동안에는 원본을 숨기고, 아래의 경로 기반 보조 조명만 사용합니다.
	SetSourceSpotLightSuppressed(
		bSuppressSourceSpotLightWhileReflecting && EligibleSegmentCount > 0);

	if (EligibleSegmentCount > MaxSpotLightCount)
	{
		if (!bHasWarnedSpotLightLimit)
		{
			UE_LOG(
				LogUOULightReflectionSpotLight,
				Warning,
				TEXT("%s: 반사 보조 SpotLight 구간 %d개 중 최대 %d개만 표시합니다."),
				*GetNameSafe(GetOwner()),
				EligibleSegmentCount,
				MaxSpotLightCount);
			bHasWarnedSpotLightLimit = true;
		}
	}
	else
	{
		bHasWarnedSpotLightLimit = false;
	}

	const FLinearColor LightColor = ResolveLightColor();
	int32 SpotLightIndex = 0;
	for (const FUOULightPathData& PathData : LightPaths)
	{
		for (const FUOULightPathSegmentData& SegmentData : PathData.Segments)
		{
			if (SpotLightIndex >= MaxSpotLightCount)
			{
				break;
			}
			if (!SegmentData.bReflected ||
				SegmentData.Length < MinimumSegmentLength ||
				SegmentData.Intensity <= 0.0f ||
				SegmentData.Direction.IsNearlyZero())
			{
				continue;
			}

			USpotLightComponent* SpotLight = AcquireSpotLight(SpotLightIndex);
			if (SpotLight == nullptr)
			{
				continue;
			}

			UpdateSpotLight(SpotLight, SegmentData, LightColor);
			++SpotLightIndex;
		}

		if (SpotLightIndex >= MaxSpotLightCount)
		{
			break;
		}
	}

	HideUnusedSpotLights(SpotLightIndex);
	ActiveSpotLightCount = SpotLightIndex;
}

UUOULightExposureSourceComponent* UUOULightReflectionSpotLightComponent::ResolveSourceComponent() const
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return nullptr;
	}

	if (UActorComponent* ReferencedComponent = SourceComponentReference.GetComponent(Owner))
	{
		return Cast<UUOULightExposureSourceComponent>(ReferencedComponent);
	}

	return bAutoFindSourceComponent
		? Owner->FindComponentByClass<UUOULightExposureSourceComponent>()
		: nullptr;
}

USpotLightComponent* UUOULightReflectionSpotLightComponent::AcquireSpotLight(int32 PoolIndex)
{
	if (SpotLightPool.IsValidIndex(PoolIndex))
	{
		return SpotLightPool[PoolIndex];
	}

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return nullptr;
	}

	USpotLightComponent* SpotLight = NewObject<USpotLightComponent>(Owner, NAME_None, RF_Transient);
	if (SpotLight == nullptr)
	{
		return nullptr;
	}

	SpotLight->CreationMethod = EComponentCreationMethod::Instance;
	SpotLight->SetMobility(EComponentMobility::Movable);
	SpotLight->bUseInverseSquaredFalloff = false;
	SpotLight->SetCastShadows(bCastShadows);
	SpotLight->SetVisibility(false);
	if (USceneComponent* RootComponent = Owner->GetRootComponent())
	{
		SpotLight->SetupAttachment(RootComponent);
	}

	Owner->AddInstanceComponent(SpotLight);
	SpotLight->RegisterComponent();
	SpotLightPool.Add(SpotLight);
	return SpotLight;
}

void UUOULightReflectionSpotLightComponent::UpdateSpotLight(
	USpotLightComponent* SpotLight,
	const FUOULightPathSegmentData& SegmentData,
	const FLinearColor& LightColor) const
{
	if (SpotLight == nullptr)
	{
		return;
	}

	const FVector Direction = SegmentData.Direction.GetSafeNormal();
	const float SegmentLength = FMath::Max(MinimumSegmentLength, SegmentData.Length);
	const float OuterConeAngle = FMath::Clamp(SegmentData.ConeAngle, 1.0f, 89.0f);

	SpotLight->SetWorldLocationAndRotation(
		SegmentData.Start,
		Direction.Rotation());
	SpotLight->SetAttenuationRadius(SegmentLength);
	SpotLight->SetOuterConeAngle(OuterConeAngle);
	SpotLight->SetInnerConeAngle(OuterConeAngle * FMath::Clamp(InnerConeRatio, 0.0f, 1.0f));
	SpotLight->SetIntensity(FMath::Max(0.0f, SegmentData.Intensity * IntensityScale));
	SpotLight->SetLightColor(LightColor);
	SpotLight->SetCastShadows(bCastShadows);
	SpotLight->SetVisibility(true);
}

void UUOULightReflectionSpotLightComponent::HideUnusedSpotLights(int32 FirstUnusedIndex)
{
	for (int32 PoolIndex = FMath::Max(0, FirstUnusedIndex); PoolIndex < SpotLightPool.Num(); ++PoolIndex)
	{
		if (USpotLightComponent* SpotLight = SpotLightPool[PoolIndex])
		{
			SpotLight->SetVisibility(false);
		}
	}

	ActiveSpotLightCount = FMath::Clamp(FirstUnusedIndex, 0, SpotLightPool.Num());
}

void UUOULightReflectionSpotLightComponent::SetSourceSpotLightSuppressed(const bool bSuppress)
{
	if (bSuppress)
	{
		if (bIsSourceSpotLightSuppressed)
		{
			if (ManagedSourceSpotLight != nullptr)
			{
				ManagedSourceSpotLight->SetVisibility(false);
			}
			return;
		}

		if (ManagedSourceSpotLight == nullptr)
		{
			return;
		}

		bSourceSpotLightWasVisible = ManagedSourceSpotLight->IsVisible();
		bIsSourceSpotLightSuppressed = true;
		ManagedSourceSpotLight->SetVisibility(false);
		return;
	}

	if (!bIsSourceSpotLightSuppressed)
	{
		return;
	}

	if (ManagedSourceSpotLight != nullptr)
	{
		const bool bSourceStillEmitsLight = BoundSourceComponent == nullptr ||
			BoundSourceComponent->bEmitLight;
		ManagedSourceSpotLight->SetVisibility(
			bSourceSpotLightWasVisible && bSourceStillEmitsLight);
	}

	bSourceSpotLightWasVisible = false;
	bIsSourceSpotLightSuppressed = false;
}

FLinearColor UUOULightReflectionSpotLightComponent::ResolveLightColor() const
{
	if (bUseSourceLightColor && BoundSourceComponent != nullptr)
	{
		if (const AActor* SourceOwner = BoundSourceComponent->GetOwner())
		{
			if (const USpotLightComponent* SourceSpotLight =
				SourceOwner->FindComponentByClass<USpotLightComponent>())
			{
				return SourceSpotLight->GetLightColor();
			}
		}
	}

	return ReflectionLightColor;
}
