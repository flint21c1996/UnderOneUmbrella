// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUWaterBasinConditionComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "World/WaterTarget/UOUWaterBasinPlatformComponent.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"

UUOUWaterBasinConditionComponent::UUOUWaterBasinConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUOUWaterBasinConditionComponent::OnRegister()
{
	Super::OnRegister();

	ResolveWaterBasinTarget();
}

void UUOUWaterBasinConditionComponent::BeginPlay()
{
	Super::BeginPlay();

	BindToWaterBasinTarget();

	if (bEvaluateOnBeginPlay)
	{
		EvaluateCondition();
	}
}

void UUOUWaterBasinConditionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromWaterBasinTarget();

	Super::EndPlay(EndPlayReason);
}

void UUOUWaterBasinConditionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bEvaluateEveryTick || ValueSource == EUOUWaterBasinConditionValueSource::PlatformWorldZ)
	{
		EvaluateCondition();
	}
}

#if WITH_EDITOR
void UUOUWaterBasinConditionComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName ChangedPropertyName = PropertyChangedEvent.Property
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UUOUWaterBasinConditionComponent, TargetWaterTileActor))
	{
		bTargetWaterTileActorAutoResolved = false;
		UnbindFromWaterBasinTarget();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UUOUWaterBasinConditionComponent::EvaluateCondition(bool bForceNotify)
{
	BindToWaterBasinTarget();

	UUOUWaterBasinTargetComponent* WaterBasinTarget = ResolveWaterBasinTarget();
	if (!WaterBasinTarget && ValueSource != EUOUWaterBasinConditionValueSource::PlatformWorldZ)
	{
		return;
	}

	FUOUPuzzleConditionContext NewContext = BuildConditionContext(WaterBasinTarget);
	NewContext.CurrentValue = ResolveCurrentValue(NewContext);
	NewContext.bIsSatisfied = DoesValueSatisfyCondition(NewContext.CurrentValue);

	const bool bWasSatisfied = bIsSatisfied;
	const bool bShouldNotify = bForceNotify
		|| (bHasEvaluated && bWasSatisfied != NewContext.bIsSatisfied)
		|| (!bHasEvaluated && NewContext.bIsSatisfied);

	bIsSatisfied = NewContext.bIsSatisfied;
	bHasEvaluated = true;
	LastContext = NewContext;

	if (bShouldNotify)
	{
		BroadcastConditionResult(NewContext, bWasSatisfied);
	}
}

bool UUOUWaterBasinConditionComponent::IsConditionSatisfied() const
{
	return bIsSatisfied;
}

FUOUPuzzleConditionContext UUOUWaterBasinConditionComponent::GetLastContext() const
{
	return LastContext;
}

void UUOUWaterBasinConditionComponent::HandleWaterStateChanged(UUOUWaterBasinTargetComponent* ChangedTarget)
{
	if (ChangedTarget != BoundWaterBasinTarget)
	{
		BindToWaterBasinTarget();
	}

	EvaluateCondition();
}

void UUOUWaterBasinConditionComponent::BindToWaterBasinTarget()
{
	UUOUWaterBasinTargetComponent* ResolvedTarget = ResolveWaterBasinTarget();
	if (BoundWaterBasinTarget == ResolvedTarget)
	{
		return;
	}

	UnbindFromWaterBasinTarget();

	BoundWaterBasinTarget = ResolvedTarget;
	if (BoundWaterBasinTarget)
	{
		BoundWaterBasinTarget->OnWaterStateChanged.AddUniqueDynamic(this, &UUOUWaterBasinConditionComponent::HandleWaterStateChanged);
	}
}

void UUOUWaterBasinConditionComponent::UnbindFromWaterBasinTarget()
{
	if (BoundWaterBasinTarget)
	{
		BoundWaterBasinTarget->OnWaterStateChanged.RemoveDynamic(this, &UUOUWaterBasinConditionComponent::HandleWaterStateChanged);
		BoundWaterBasinTarget = nullptr;
	}
}

UUOUWaterBasinTargetComponent* UUOUWaterBasinConditionComponent::ResolveWaterBasinTarget()
{
	if (IsValid(TargetWaterTileActor) && !bTargetWaterTileActorAutoResolved)
	{
		return TargetWaterTileActor->FindComponentByClass<UUOUWaterBasinTargetComponent>();
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (bAutoResolveTargetWaterTileActor)
	{
		for (AActor* ParentActor = Owner->GetAttachParentActor(); IsValid(ParentActor); ParentActor = ParentActor->GetAttachParentActor())
		{
			if (UUOUWaterBasinTargetComponent* ParentTargetComponent = ParentActor->FindComponentByClass<UUOUWaterBasinTargetComponent>())
			{
				TargetWaterTileActor = ParentActor;
				bTargetWaterTileActorAutoResolved = true;
				return ParentTargetComponent;
			}
		}

		if (const UUOUWaterBasinPlatformComponent* PlatformComponent = ResolvePlatformComponent())
		{
			if (IsValid(PlatformComponent->TargetActor))
			{
				if (UUOUWaterBasinTargetComponent* TargetComponent = PlatformComponent->TargetActor->FindComponentByClass<UUOUWaterBasinTargetComponent>())
				{
					TargetWaterTileActor = PlatformComponent->TargetActor;
					bTargetWaterTileActorAutoResolved = true;
					return TargetComponent;
				}
			}
		}

		if (UUOUWaterBasinTargetComponent* OwnerTargetComponent = Owner->FindComponentByClass<UUOUWaterBasinTargetComponent>())
		{
			TargetWaterTileActor = Owner;
			bTargetWaterTileActorAutoResolved = true;
			return OwnerTargetComponent;
		}
	}

	return IsValid(TargetWaterTileActor)
		? TargetWaterTileActor->FindComponentByClass<UUOUWaterBasinTargetComponent>()
		: nullptr;
}

UUOUWaterBasinPlatformComponent* UUOUWaterBasinConditionComponent::ResolvePlatformComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UUOUWaterBasinPlatformComponent>() : nullptr;
}

FUOUPuzzleConditionContext UUOUWaterBasinConditionComponent::BuildConditionContext(UUOUWaterBasinTargetComponent* WaterBasinTarget) const
{
	FUOUPuzzleConditionContext Context;
	Context.ConditionOwner = GetOwner();
	Context.ConditionComponent = const_cast<UUOUWaterBasinConditionComponent*>(this);
	Context.ConditionId = ConditionId;
	Context.ThresholdValue = ThresholdValue;
	Context.PlatformComponent = ResolvePlatformComponent();

	if (WaterBasinTarget)
	{
		Context.WaterBasinTarget = WaterBasinTarget;
		Context.WaterTileActor = WaterBasinTarget->GetOwner();
		Context.WaterSurfaceWorldZ = WaterBasinTarget->WaterSurfaceWorldZ;
		Context.WaterDepth = WaterBasinTarget->CurrentWaterDepth;
		Context.WaterDepthWorld = WaterBasinTarget->GetWaterDepthWorld();
		Context.WaterFillRatio = WaterBasinTarget->CurrentFillRatio;
		Context.WaterVolume = WaterBasinTarget->CurrentWaterVolume;
	}

	if (Context.PlatformComponent)
	{
		if (IsValid(Context.PlatformComponent->PlatformComponent))
		{
			Context.PlatformWorldZ = Context.PlatformComponent->PlatformComponent->GetComponentLocation().Z;
		}
		else
		{
			Context.PlatformWorldZ = Context.PlatformComponent->CurrentTargetWorldZ;
		}
	}
	else if (const AActor* Owner = GetOwner())
	{
		Context.PlatformWorldZ = Owner->GetActorLocation().Z;
	}

	return Context;
}

float UUOUWaterBasinConditionComponent::ResolveCurrentValue(const FUOUPuzzleConditionContext& Context) const
{
	switch (ValueSource)
	{
	case EUOUWaterBasinConditionValueSource::WaterSurfaceWorldZ:
		return Context.WaterSurfaceWorldZ;
	case EUOUWaterBasinConditionValueSource::WaterDepth:
		return Context.WaterDepth;
	case EUOUWaterBasinConditionValueSource::WaterDepthWorld:
		return Context.WaterDepthWorld;
	case EUOUWaterBasinConditionValueSource::WaterFillRatio:
		return Context.WaterFillRatio;
	case EUOUWaterBasinConditionValueSource::WaterVolume:
		return Context.WaterVolume;
	case EUOUWaterBasinConditionValueSource::PlatformWorldZ:
		return Context.PlatformWorldZ;
	default:
		return 0.0f;
	}
}

bool UUOUWaterBasinConditionComponent::DoesValueSatisfyCondition(float CurrentValue) const
{
	switch (CompareMode)
	{
	case EUOUWaterBasinConditionCompareMode::GreaterOrEqual:
		return CurrentValue >= ThresholdValue - Tolerance;
	case EUOUWaterBasinConditionCompareMode::Greater:
		return CurrentValue > ThresholdValue;
	case EUOUWaterBasinConditionCompareMode::LessOrEqual:
		return CurrentValue <= ThresholdValue + Tolerance;
	case EUOUWaterBasinConditionCompareMode::Less:
		return CurrentValue < ThresholdValue;
	case EUOUWaterBasinConditionCompareMode::Equal:
		return FMath::Abs(CurrentValue - ThresholdValue) <= Tolerance;
	case EUOUWaterBasinConditionCompareMode::NotEqual:
		return FMath::Abs(CurrentValue - ThresholdValue) > Tolerance;
	case EUOUWaterBasinConditionCompareMode::BetweenInclusive:
	{
		const float MinValue = FMath::Min(ThresholdValue, UpperThresholdValue);
		const float MaxValue = FMath::Max(ThresholdValue, UpperThresholdValue);
		return CurrentValue >= MinValue - Tolerance && CurrentValue <= MaxValue + Tolerance;
	}
	default:
		return false;
	}
}

void UUOUWaterBasinConditionComponent::BroadcastConditionResult(const FUOUPuzzleConditionContext& Context, bool bWasSatisfied)
{
	const bool bCanTriggerSatisfied = !bTriggerOnce || !bHasTriggeredOnce;
	const bool bBecameSatisfied = !bWasSatisfied && Context.bIsSatisfied;
	const bool bBecameUnsatisfied = bWasSatisfied && !Context.bIsSatisfied;

	OnConditionChanged.Broadcast(Context);
	NotifyReactionTargets(Context, bWasSatisfied);

	if (bBecameSatisfied && bCanTriggerSatisfied)
	{
		OnConditionSatisfied.Broadcast(Context);
		bHasTriggeredOnce = true;
	}
	else if (bBecameUnsatisfied)
	{
		OnConditionUnsatisfied.Broadcast(Context);
	}
}

void UUOUWaterBasinConditionComponent::NotifyReactionTargets(const FUOUPuzzleConditionContext& Context, bool bWasSatisfied)
{
	TSet<UObject*> NotifiedObjects;

	AActor* Owner = GetOwner();
	if (bNotifyOwnerActor && Owner)
	{
		NotifyActivatableObject(Owner, Context, bWasSatisfied, NotifiedObjects);
	}

	if (bNotifyOwnerComponents && Owner)
	{
		TArray<UActorComponent*> OwnerComponents;
		Owner->GetComponents<UActorComponent>(OwnerComponents);
		for (UActorComponent* OwnerComponent : OwnerComponents)
		{
			NotifyActivatableObject(OwnerComponent, Context, bWasSatisfied, NotifiedObjects);
		}
	}

	for (AActor* ReactionActor : ExplicitReactionActors)
	{
		if (!IsValid(ReactionActor))
		{
			continue;
		}

		NotifyActivatableObject(ReactionActor, Context, bWasSatisfied, NotifiedObjects);

		if (bNotifyComponentsOnExplicitActors)
		{
			TArray<UActorComponent*> ReactionComponents;
			ReactionActor->GetComponents<UActorComponent>(ReactionComponents);
			for (UActorComponent* ReactionComponent : ReactionComponents)
			{
				NotifyActivatableObject(ReactionComponent, Context, bWasSatisfied, NotifiedObjects);
			}
		}
	}

	for (UActorComponent* ReactionComponent : ExplicitReactionComponents)
	{
		NotifyActivatableObject(ReactionComponent, Context, bWasSatisfied, NotifiedObjects);
	}
}

void UUOUWaterBasinConditionComponent::NotifyActivatableObject(UObject* Object, const FUOUPuzzleConditionContext& Context, bool bWasSatisfied, TSet<UObject*>& NotifiedObjects)
{
	if (!IsValid(Object) || Object == this || NotifiedObjects.Contains(Object))
	{
		return;
	}

	NotifiedObjects.Add(Object);

	if (!Object->GetClass()->ImplementsInterface(UUOUPuzzleConditionActivatable::StaticClass()))
	{
		return;
	}

	IUOUPuzzleConditionActivatable::Execute_OnPuzzleConditionStateChanged(Object, Context);

	const bool bBecameSatisfied = !bWasSatisfied && Context.bIsSatisfied;
	const bool bBecameUnsatisfied = bWasSatisfied && !Context.bIsSatisfied;
	const bool bCanTriggerSatisfied = !bTriggerOnce || !bHasTriggeredOnce;

	if (bBecameSatisfied && bCanTriggerSatisfied)
	{
		if (IUOUPuzzleConditionActivatable::Execute_CanActivateByPuzzleCondition(Object, Context))
		{
			IUOUPuzzleConditionActivatable::Execute_ActivateByPuzzleCondition(Object, Context);
		}
	}
	else if (bBecameUnsatisfied)
	{
		IUOUPuzzleConditionActivatable::Execute_DeactivateByPuzzleCondition(Object, Context);
	}
}
