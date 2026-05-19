// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUWeightedButtonComponent.h"

#include "Components/SceneComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "UOUWeightSensorComponent.h"

UUOUWeightedButtonComponent::UUOUWeightedButtonComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UUOUWeightedButtonComponent::BeginPlay()
{
	Super::BeginPlay();

	PressWeight = FMath::Max(0.0f, PressWeight);
	ReleaseWeight = FMath::Clamp(ReleaseWeight, 0.0f, PressWeight);
	MoveSpeed = FMath::Max(0.0f, MoveSpeed);

	ResolveReferences();
	RefreshPressedState();
	SnapVisualToCurrentState();
}

void UUOUWeightedButtonComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RefreshPressedState();
	MoveButtonVisual(DeltaTime);
	DrawScreenDebug();
}

float UUOUWeightedButtonComponent::GetPuzzleWeight() const
{
	return CurrentWeight;
}

TArray<FString> UUOUWeightedButtonComponent::GetPuzzleDebugInfo_Implementation() const
{
	const int32 OverlapCount = Sensor != nullptr ? Sensor->OverlappingActorCount : 0;
	return {
		FString::Printf(
			TEXT("Weighted Button: %s"),
			IsPressed() ? TEXT("Pressed") : TEXT("Released")),
		FString::Printf(
			TEXT("Weight: %.2f (Press %.2f / Release %.2f)"),
			CurrentWeight,
			PressWeight,
			ReleaseWeight),
		FString::Printf(TEXT("Sensor Overlaps: %d"), OverlapCount)
	};
}

void UUOUWeightedButtonComponent::GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const
{
	if (Sensor != nullptr)
	{
		Sensor->GetOverlappingActors(OutInputActors);
	}
}

bool UUOUWeightedButtonComponent::IsPressed() const
{
	return IsSatisfied();
}

void UUOUWeightedButtonComponent::ResolveReferences()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	if (UActorComponent* SensorComponent = SensorReference.GetComponent(Owner))
	{
		Sensor = Cast<UUOUWeightSensorComponent>(SensorComponent);
	}

	if (UActorComponent* ButtonVisualComponent = ButtonVisualReference.GetComponent(Owner))
	{
		ButtonVisual = Cast<USceneComponent>(ButtonVisualComponent);
	}

	if (UActorComponent* ReleasedPointComponent = ReleasedPointReference.GetComponent(Owner))
	{
		ReleasedPoint = Cast<USceneComponent>(ReleasedPointComponent);
	}

	if (UActorComponent* PressedPointComponent = PressedPointReference.GetComponent(Owner))
	{
		PressedPoint = Cast<USceneComponent>(PressedPointComponent);
	}

	if (bAutoFindSensor && Sensor == nullptr)
	{
		Sensor = Owner->FindComponentByClass<UUOUWeightSensorComponent>();
	}

	if (bAutoFindMotionReferences)
	{
		TInlineComponentArray<USceneComponent*> SceneComponents(Owner);

		auto FindSceneComponentByName = [&SceneComponents](const FName PreferredName) -> USceneComponent*
		{
			for (USceneComponent* SceneComponent : SceneComponents)
			{
				if (SceneComponent != nullptr && SceneComponent->GetFName() == PreferredName)
				{
					return SceneComponent;
				}
			}

			return nullptr;
		};

		if (ButtonVisual == nullptr)
		{
			ButtonVisual = FindSceneComponentByName(PreferredButtonVisualName);
		}

		if (ReleasedPoint == nullptr)
		{
			ReleasedPoint = FindSceneComponentByName(PreferredReleasedPointName);
		}

		if (PressedPoint == nullptr)
		{
			PressedPoint = FindSceneComponentByName(PreferredPressedPointName);
		}
	}

	if (ButtonVisual == nullptr)
	{
		ButtonVisual = Owner->GetRootComponent();
	}
}

void UUOUWeightedButtonComponent::RefreshPressedState()
{
	CurrentWeight = Sensor != nullptr ? Sensor->CurrentWeight : 0.0f;

	if (!bIsSatisfied && CurrentWeight >= PressWeight)
	{
		SetSatisfiedState(true, true);
		return;
	}

	if (bIsSatisfied && CurrentWeight <= ReleaseWeight)
	{
		SetSatisfiedState(false, true);
	}
}

void UUOUWeightedButtonComponent::MoveButtonVisual(float DeltaTime)
{
	if (ButtonVisual == nullptr)
	{
		return;
	}

	USceneComponent* TargetPoint = bIsSatisfied ? PressedPoint : ReleasedPoint;
	if (TargetPoint == nullptr)
	{
		return;
	}

	const FVector CurrentLocation = ButtonVisual->GetComponentLocation();
	const FVector TargetLocation = TargetPoint->GetComponentLocation();
	const FVector NextLocation = FMath::VInterpConstantTo(CurrentLocation, TargetLocation, DeltaTime, MoveSpeed);
	ButtonVisual->SetWorldLocation(NextLocation);
}

void UUOUWeightedButtonComponent::SnapVisualToCurrentState()
{
	if (ButtonVisual == nullptr)
	{
		return;
	}

	USceneComponent* TargetPoint = bIsSatisfied ? PressedPoint : ReleasedPoint;
	if (TargetPoint != nullptr)
	{
		ButtonVisual->SetWorldLocation(TargetPoint->GetComponentLocation());
	}
}

void UUOUWeightedButtonComponent::DrawScreenDebug() const
{
	if (!bShowScreenDebug
		|| !UUOUDebugSubsystem::IsDebugScreenMessageEnabled(this, EUOUDebugCategory::Puzzle)
		|| GEngine == nullptr)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (World == nullptr || !World->IsGameWorld())
	{
		return;
	}

	const AActor* Owner = GetOwner();
	const int32 OverlapCount = Sensor != nullptr ? Sensor->OverlappingActorCount : 0;
	const FString SensorName = GetNameSafe(Sensor);
	const FString SensorVolumeName = Sensor != nullptr ? GetNameSafe(Sensor->SensorVolume) : TEXT("None");
	const FString DebugText = FString::Printf(
		TEXT("Button: %s\nPressed: %s\nCurrent Weight: %.2f\nPress / Release: %.2f / %.2f\nOverlap Count: %d\nSensor: %s\nSensor Volume: %s"),
		*GetNameSafe(Owner),
		IsPressed() ? TEXT("Yes") : TEXT("No"),
		CurrentWeight,
		PressWeight,
		ReleaseWeight,
		OverlapCount,
		*SensorName,
		*SensorVolumeName);

	const uint64 OwnerId = reinterpret_cast<uint64>(Owner);
	const int32 MessageKey = static_cast<int32>(0x554F4200u + (OwnerId & 0xFFu));
	GEngine->AddOnScreenDebugMessage(
		MessageKey,
		0.0f,
		UUOUDebugSubsystem::GetDebugCategoryColor(this, EUOUDebugCategory::Puzzle, IsPressed() ? FColor::Green : FColor::Yellow),
		DebugText,
		false,
		FVector2D(1.0f, 1.0f));
}
