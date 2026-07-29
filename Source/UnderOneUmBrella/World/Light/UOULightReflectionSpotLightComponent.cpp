// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOULightReflectionSpotLightComponent.h"

#include "Components/SpotLightComponent.h"
#include "GameFramework/Actor.h"
#include "World/Light/UOULightExposureSourceComponent.h"

UUOULightReflectionSpotLightComponent::UUOULightReflectionSpotLightComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOULightReflectionSpotLightComponent::BeginPlay()
{
	Super::BeginPlay();

	BoundSourceComponent = ResolveSourceComponent();
	if (BoundSourceComponent == nullptr)
	{
		HideUnusedSpotLights(0);
		return;
	}

	BoundSourceComponent->OnReflectionPathsUpdated.AddDynamic(
		this,
		&UUOULightReflectionSpotLightComponent::HandleReflectionPathsUpdated);
	RefreshSpotLights();
}

void UUOULightReflectionSpotLightComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundSourceComponent != nullptr)
	{
		BoundSourceComponent->OnReflectionPathsUpdated.RemoveDynamic(
			this,
			&UUOULightReflectionSpotLightComponent::HandleReflectionPathsUpdated);
	}

	for (USpotLightComponent* SpotLight : SpotLightPool)
	{
		if (SpotLight != nullptr)
		{
			SpotLight->DestroyComponent();
		}
	}
	SpotLightPool.Reset();
	ActiveSpotLightCount = 0;
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
		HideUnusedSpotLights(0);
		return;
	}

	HandleReflectionPathsUpdated(BoundSourceComponent->GetReflectionPaths());
}

void UUOULightReflectionSpotLightComponent::HandleReflectionPathsUpdated(
	const TArray<FUOULightReflectionPathData>& ReflectionPaths)
{
	if (!bEnabled)
	{
		HideUnusedSpotLights(0);
		return;
	}

	const FLinearColor LightColor = ResolveLightColor();
	int32 SpotLightIndex = 0;
	for (const FUOULightReflectionPathData& PathData : ReflectionPaths)
	{
		for (const FUOULightReflectionSegmentData& SegmentData : PathData.Segments)
		{
			if (SpotLightIndex >= MaxSpotLightCount)
			{
				break;
			}
			if (SegmentData.SegmentLength < MinimumSegmentLength ||
				SegmentData.ReflectedIntensity <= 0.0f ||
				SegmentData.ReflectedDirection.IsNearlyZero())
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
	const FUOULightReflectionSegmentData& SegmentData,
	const FLinearColor& LightColor) const
{
	if (SpotLight == nullptr)
	{
		return;
	}

	const FVector Direction = SegmentData.ReflectedDirection.GetSafeNormal();
	const float SegmentLength = FMath::Max(MinimumSegmentLength, SegmentData.SegmentLength);
	const float EndRadius = FMath::Max(SegmentData.BeamStartRadius, SegmentData.BeamEndRadius);
	const float OuterConeAngle = FMath::Clamp(
		FMath::RadiansToDegrees(FMath::Atan2(EndRadius, SegmentLength)),
		1.0f,
		89.0f);

	SpotLight->SetWorldLocationAndRotation(
		SegmentData.ReflectionStart,
		Direction.Rotation());
	SpotLight->SetAttenuationRadius(SegmentLength);
	SpotLight->SetOuterConeAngle(OuterConeAngle);
	SpotLight->SetInnerConeAngle(OuterConeAngle * FMath::Clamp(InnerConeRatio, 0.0f, 1.0f));
	SpotLight->SetIntensity(FMath::Max(0.0f, SegmentData.ReflectedIntensity * IntensityScale));
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
