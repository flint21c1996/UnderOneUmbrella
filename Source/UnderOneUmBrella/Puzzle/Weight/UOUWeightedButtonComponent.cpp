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
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	SetAutoActivate(true);
}

void UUOUWeightedButtonComponent::BeginPlay()
{
	Super::BeginPlay();

	Activate(true);
	SetComponentTickEnabled(true);

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
	const USceneComponent* TargetPoint = bIsSatisfied ? PressedPoint : ReleasedPoint;
	const float VisualRelativeZ = ButtonVisual != nullptr ? ButtonVisual->GetRelativeLocation().Z : 0.0f;
	const float TargetRelativeZ = TargetPoint != nullptr ? TargetPoint->GetRelativeLocation().Z : 0.0f;

	return {
		FString::Printf(
			TEXT("Weighted Button: %s"),
			IsPressed() ? TEXT("Pressed") : TEXT("Released")),
		FString::Printf(
			TEXT("Weight: %.2f (Press %.2f / Release %.2f)"),
			CurrentWeight,
			PressWeight,
			ReleaseWeight),
		FString::Printf(TEXT("Sensor Overlaps: %d"), OverlapCount),
		FString::Printf(
			TEXT("Motion: Tick %s / Active %s / Speed %.1f / %s -> %s / RelZ %.1f -> %.1f"),
			IsComponentTickEnabled() ? TEXT("On") : TEXT("Off"),
			IsActive() ? TEXT("On") : TEXT("Off"),
			MoveSpeed,
			*GetNameSafe(ButtonVisual),
			*GetNameSafe(TargetPoint),
			VisualRelativeZ,
			TargetRelativeZ)
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

	auto IsOwnedComponent = [Owner](const UActorComponent* Component) -> bool
	{
		return Component != nullptr && Component->GetOwner() == Owner;
	};

	auto IsUsableMotionComponent = [Owner, &IsOwnedComponent](const USceneComponent* Component) -> bool
	{
		if (!IsOwnedComponent(Component))
		{
			return false;
		}

		// 컴포넌트 이름이 비어 있는 수동 참조는 RootScene으로 풀릴 수 있습니다.
		// 루트는 버튼 표면이나 눌림 지점이 아니므로 자동 탐색 대상으로 넘깁니다.
		return Component != Owner->GetRootComponent();
	};

	auto ResolveMotionComponent = [Owner, &IsUsableMotionComponent](const FComponentReference& Reference) -> USceneComponent*
	{
		USceneComponent* ResolvedComponent = Cast<USceneComponent>(Reference.GetComponent(Owner));
		return IsUsableMotionComponent(ResolvedComponent) ? ResolvedComponent : nullptr;
	};

	// BP 상속이나 복제 과정에서 CDO/템플릿 컴포넌트 포인터가 남아 있으면 실제 월드 인스턴스가 움직이지 않습니다.
	// 그래서 BeginPlay마다 현재 Owner가 가진 컴포넌트인지 먼저 확인하고, 아니면 이름 기반 자동 탐색으로 다시 잡습니다.
	if (!IsOwnedComponent(Sensor))
	{
		Sensor = nullptr;
	}

	if (!IsUsableMotionComponent(ButtonVisual))
	{
		ButtonVisual = nullptr;
	}

	if (!IsUsableMotionComponent(ReleasedPoint))
	{
		ReleasedPoint = nullptr;
	}

	if (!IsUsableMotionComponent(PressedPoint))
	{
		PressedPoint = nullptr;
	}

	if (UActorComponent* SensorComponent = SensorReference.GetComponent(Owner))
	{
		Sensor = Cast<UUOUWeightSensorComponent>(SensorComponent);
	}

	if (USceneComponent* ResolvedButtonVisual = ResolveMotionComponent(ButtonVisualReference))
	{
		ButtonVisual = ResolvedButtonVisual;
	}

	if (USceneComponent* ResolvedReleasedPoint = ResolveMotionComponent(ReleasedPointReference))
	{
		ReleasedPoint = ResolvedReleasedPoint;
	}

	if (USceneComponent* ResolvedPressedPoint = ResolveMotionComponent(PressedPointReference))
	{
		PressedPoint = ResolvedPressedPoint;
	}

	if (!IsOwnedComponent(Sensor))
	{
		Sensor = nullptr;
	}

	if (!IsUsableMotionComponent(ButtonVisual))
	{
		ButtonVisual = nullptr;
	}

	if (!IsUsableMotionComponent(ReleasedPoint))
	{
		ReleasedPoint = nullptr;
	}

	if (!IsUsableMotionComponent(PressedPoint))
	{
		PressedPoint = nullptr;
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

	if (ButtonVisual == nullptr && !bAutoFindMotionReferences)
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

	const USceneComponent* AttachParent = ButtonVisual->GetAttachParent();
	const FTransform ParentTransform = AttachParent != nullptr
		? AttachParent->GetComponentTransform()
		: FTransform::Identity;
	const FVector CurrentRelativeLocation = ButtonVisual->GetRelativeLocation();
	const FVector TargetRelativeLocation = ParentTransform.InverseTransformPosition(TargetPoint->GetComponentLocation());
	const FVector NextRelativeLocation = FMath::VInterpConstantTo(
		CurrentRelativeLocation,
		TargetRelativeLocation,
		DeltaTime,
		MoveSpeed);

	ButtonVisual->SetRelativeLocation(NextRelativeLocation, false, nullptr, ETeleportType::TeleportPhysics);
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
		const USceneComponent* AttachParent = ButtonVisual->GetAttachParent();
		const FTransform ParentTransform = AttachParent != nullptr
			? AttachParent->GetComponentTransform()
			: FTransform::Identity;
		const FVector TargetRelativeLocation = ParentTransform.InverseTransformPosition(TargetPoint->GetComponentLocation());
		ButtonVisual->SetRelativeLocation(TargetRelativeLocation, false, nullptr, ETeleportType::TeleportPhysics);
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
