// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUInteractionComponent.h"

#include "Components/ArrowComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UUOUInteractionComponent::UUOUInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	InteractionRange = FMath::Max(0.0f, InteractionRange);
	InteractionProbeRadius = FMath::Max(0.0f, InteractionProbeRadius);

	if (DetectionOrigin == nullptr)
	{
		if (AActor* Owner = GetOwner())
		{
			TInlineComponentArray<UArrowComponent*> ArrowComponents(Owner);
			for (UArrowComponent* ArrowComponent : ArrowComponents)
			{
				if (ArrowComponent != nullptr && ArrowComponent->GetFName() == TEXT("InteractionOrigin"))
				{
					DetectionOrigin = ArrowComponent;
					break;
				}
			}
		}
	}
}

void UUOUInteractionComponent::SetInteractionEnabled(bool bEnabled)
{
	bInteractionEnabled = bEnabled;
	if (!bInteractionEnabled)
	{
		CurrentCandidateComponent = nullptr;
	}
}

void UUOUInteractionComponent::RefreshCandidate()
{
	CurrentCandidateComponent = nullptr;

	if (!bInteractionEnabled)
	{
		return;
	}

	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (World == nullptr || Owner == nullptr)
	{
		return;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(InteractionProbe), false, Owner);
	QueryParams.AddIgnoredActor(Owner);

	if (World->SweepSingleByChannel(
		HitResult,
		GetDetectionStart(),
		GetDetectionEnd(),
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(InteractionProbeRadius),
		QueryParams))
	{
		CurrentCandidateComponent = HitResult.GetComponent();
	}
}

FVector UUOUInteractionComponent::GetDetectionStart() const
{
	if (DetectionOrigin != nullptr)
	{
		return DetectionOrigin->GetComponentLocation();
	}

	if (const AActor* Owner = GetOwner())
	{
		return Owner->GetActorLocation() + DetectionOffset;
	}

	return FVector::ZeroVector;
}

FVector UUOUInteractionComponent::GetDetectionEnd() const
{
	if (DetectionOrigin != nullptr)
	{
		return DetectionOrigin->GetComponentLocation() + DetectionOrigin->GetForwardVector() * InteractionRange;
	}

	if (const AActor* Owner = GetOwner())
	{
		return Owner->GetActorLocation() + DetectionOffset + Owner->GetActorForwardVector() * InteractionRange;
	}

	return FVector::ZeroVector;
}
