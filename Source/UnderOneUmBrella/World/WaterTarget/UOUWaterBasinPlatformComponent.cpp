// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUWaterBasinPlatformComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"

UUOUWaterBasinPlatformComponent::UUOUWaterBasinPlatformComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUOUWaterBasinPlatformComponent::BeginPlay()
{
	Super::BeginPlay();

	RefreshPlatformPosition();
}

void UUOUWaterBasinPlatformComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	USceneComponent* ResolvedPlatformComponent = GetPlatformComponent();
	if (!ResolvedPlatformComponent)
	{
		return;
	}

	CurrentTargetWorldZ = GetTargetSurfaceWorldZ() + SurfaceOffsetZ;

	FVector NextLocation = ResolvedPlatformComponent->GetComponentLocation();
	if (!bMoveOnlyZ)
	{
		if (const AActor* ResolvedTargetActor = TargetActor ? TargetActor.Get() : GetOwner())
		{
			const FVector TargetLocation = ResolvedTargetActor->GetActorLocation();
			NextLocation.X = TargetLocation.X;
			NextLocation.Y = TargetLocation.Y;
		}
	}

	if (bUseInterpolation && InterpSpeed > KINDA_SMALL_NUMBER)
	{
		NextLocation.Z = FMath::FInterpTo(NextLocation.Z, CurrentTargetWorldZ, DeltaTime, InterpSpeed);
	}
	else
	{
		NextLocation.Z = CurrentTargetWorldZ;
	}

	ResolvedPlatformComponent->SetWorldLocation(NextLocation);
}

void UUOUWaterBasinPlatformComponent::SetTargetActor(AActor* NewTargetActor)
{
	TargetActor = NewTargetActor;

	RefreshPlatformPosition();
}

void UUOUWaterBasinPlatformComponent::RefreshPlatformPosition()
{
	USceneComponent* ResolvedPlatformComponent = GetPlatformComponent();
	if (!ResolvedPlatformComponent)
	{
		return;
	}

	CurrentTargetWorldZ = GetTargetSurfaceWorldZ() + SurfaceOffsetZ;

	FVector NewLocation = ResolvedPlatformComponent->GetComponentLocation();
	if (!bMoveOnlyZ)
	{
		if (const AActor* ResolvedTargetActor = TargetActor ? TargetActor.Get() : GetOwner())
		{
			const FVector TargetLocation = ResolvedTargetActor->GetActorLocation();
			NewLocation.X = TargetLocation.X;
			NewLocation.Y = TargetLocation.Y;
		}
	}

	NewLocation.Z = CurrentTargetWorldZ;
	ResolvedPlatformComponent->SetWorldLocation(NewLocation);
}

float UUOUWaterBasinPlatformComponent::GetTargetSurfaceWorldZ() const
{
	const UUOUWaterBasinTargetComponent* TargetComponent = GetTargetComponent();
	if (!TargetComponent)
	{
		return GetOwner() ? GetOwner()->GetActorLocation().Z : 0.0f;
	}

	return TargetComponent->GetBottomWorldZ() + TargetComponent->GetWaterDepthWorld();
}

UUOUWaterBasinTargetComponent* UUOUWaterBasinPlatformComponent::GetTargetComponent() const
{
	// TargetActor는 수면 정보를 제공하는 WaterTile Actor입니다.
	// 비어 있을 때만 Owner에서 찾는데, 이 경우는 플랫폼 Actor와 WaterTile Actor가 같은 특수한 구성입니다.
	if (IsValid(TargetActor))
	{
		return TargetActor->FindComponentByClass<UUOUWaterBasinTargetComponent>();
	}

	return GetOwner() ? GetOwner()->FindComponentByClass<UUOUWaterBasinTargetComponent>() : nullptr;
}

USceneComponent* UUOUWaterBasinPlatformComponent::GetPlatformComponent() const
{
	// PlatformComponent가 명시되어 있으면 그 컴포넌트만 움직입니다.
	if (IsValid(PlatformComponent))
	{
		return PlatformComponent.Get();
	}

	// 명시 참조가 없으면 플랫폼 Actor 내부에서 이름/태그로 플랫폼 컴포넌트를 찾습니다.
	if (USceneComponent* FoundPlatformComponent = FindPlatformComponent())
	{
		return FoundPlatformComponent;
	}

	// 플랫폼 Actor 자체가 움직여야 하는 구성에서는 RootComponent를 이동 대상으로 사용합니다.
	return bUseOwnerRootWhenPlatformMissing && GetOwner() ? GetOwner()->GetRootComponent() : nullptr;
}

USceneComponent* UUOUWaterBasinPlatformComponent::FindPlatformComponent() const
{
	const AActor* Owner = GetOwner();
	if (!Owner || PlatformComponentName.IsNone())
	{
		return nullptr;
	}

	const FString TargetName = PlatformComponentName.ToString();
	TArray<USceneComponent*> SceneComponents;
	Owner->GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (!IsValid(SceneComponent))
		{
			continue;
		}

		if (SceneComponent->GetFName() == PlatformComponentName
			|| SceneComponent->ComponentTags.Contains(PlatformComponentName)
			|| SceneComponent->GetName().Contains(TargetName, ESearchCase::IgnoreCase))
		{
			return SceneComponent;
		}
	}

	return nullptr;
}
