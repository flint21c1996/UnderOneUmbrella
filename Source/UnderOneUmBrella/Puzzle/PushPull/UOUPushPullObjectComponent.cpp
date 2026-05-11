// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUPushPullObjectComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

UUOUPushPullObjectComponent::UUOUPushPullObjectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUPushPullObjectComponent::BeginPlay()
{
	Super::BeginPlay();

	MaxHorizontalSpeed = FMath::Max(0.0f, MaxHorizontalSpeed);
	ResolveTargetPrimitive();
}

bool UUOUPushPullObjectComponent::CanGrab(AActor* Interactor) const
{
	if (Interactor == nullptr || TargetPrimitive == nullptr || !TargetPrimitive->IsSimulatingPhysics())
	{
		return false;
	}

	return CurrentGrabber == nullptr || CurrentGrabber == Interactor;
}

bool UUOUPushPullObjectComponent::TryBeginGrab(AActor* Interactor)
{
	if (!CanGrab(Interactor))
	{
		return false;
	}

	CurrentGrabber = Interactor;
	bIsGrabbed = true;
	return true;
}

void UUOUPushPullObjectComponent::EndGrab(AActor* Interactor)
{
	if (CurrentGrabber != Interactor)
	{
		return;
	}

	CurrentGrabber = nullptr;
	bIsGrabbed = false;
	StopHorizontalMotion();
}

FVector UUOUPushPullObjectComponent::SetHorizontalVelocity(FVector HorizontalVelocity)
{
	if (!bIsGrabbed || TargetPrimitive == nullptr)
	{
		return FVector::ZeroVector;
	}

	HorizontalVelocity.Z = 0.0f;
	if (MaxHorizontalSpeed > 0.0f && HorizontalVelocity.Size() > MaxHorizontalSpeed)
	{
		HorizontalVelocity = HorizontalVelocity.GetSafeNormal() * MaxHorizontalSpeed;
	}

	const FVector CurrentVelocity = TargetPrimitive->GetPhysicsLinearVelocity();
	TargetPrimitive->SetPhysicsLinearVelocity(FVector(HorizontalVelocity.X, HorizontalVelocity.Y, CurrentVelocity.Z));
	return HorizontalVelocity;
}

void UUOUPushPullObjectComponent::ResolveTargetPrimitive()
{
	if (!bAutoFindTargetPrimitive || TargetPrimitive != nullptr)
	{
		return;
	}

	if (AActor* Owner = GetOwner())
	{
		TargetPrimitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
		if (TargetPrimitive != nullptr && TargetPrimitive->IsSimulatingPhysics())
		{
			return;
		}

		TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(Owner);
		for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
		{
			if (PrimitiveComponent != nullptr && PrimitiveComponent->IsSimulatingPhysics())
			{
				TargetPrimitive = PrimitiveComponent;
				return;
			}
		}
	}
}

void UUOUPushPullObjectComponent::StopHorizontalMotion()
{
	if (TargetPrimitive == nullptr)
	{
		return;
	}

	const FVector CurrentVelocity = TargetPrimitive->GetPhysicsLinearVelocity();
	TargetPrimitive->SetPhysicsLinearVelocity(FVector(0.0f, 0.0f, CurrentVelocity.Z));
	TargetPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
}
