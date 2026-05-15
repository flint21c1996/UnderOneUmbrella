// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUPuzzleConditionComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "World/WaterTarget/UOUWaterBasinPlatformComponent.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"

UUOUPuzzleConditionComponent::UUOUPuzzleConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUOUPuzzleConditionComponent::BeginPlay()
{
	Super::BeginPlay();

	BindToWaterBasinTarget();

	if (bEvaluateOnBeginPlay)
	{
		EvaluateCondition();
	}
}

void UUOUPuzzleConditionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromWaterBasinTarget();

	Super::EndPlay(EndPlayReason);
}

void UUOUPuzzleConditionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bEvaluateEveryTick || ValueSource == EUOUPuzzleConditionValueSource::PlatformWorldZ)
	{
		EvaluateCondition();
	}
}

void UUOUPuzzleConditionComponent::EvaluateCondition(bool bForceNotify)
{
	BindToWaterBasinTarget();

	UUOUWaterBasinTargetComponent* WaterBasinTarget = ResolveWaterBasinTarget();
	if (!WaterBasinTarget && ValueSource != EUOUPuzzleConditionValueSource::PlatformWorldZ)
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

bool UUOUPuzzleConditionComponent::IsConditionSatisfied() const
{
	return bIsSatisfied;
}

FUOUPuzzleConditionContext UUOUPuzzleConditionComponent::GetLastContext() const
{
	return LastContext;
}

void UUOUPuzzleConditionComponent::HandleWaterStateChanged(UUOUWaterBasinTargetComponent* ChangedTarget)
{
	EvaluateCondition();
}

void UUOUPuzzleConditionComponent::BindToWaterBasinTarget()
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
		BoundWaterBasinTarget->OnWaterStateChanged.AddUniqueDynamic(this, &UUOUPuzzleConditionComponent::HandleWaterStateChanged);
	}
}

void UUOUPuzzleConditionComponent::UnbindFromWaterBasinTarget()
{
	if (BoundWaterBasinTarget)
	{
		BoundWaterBasinTarget->OnWaterStateChanged.RemoveDynamic(this, &UUOUPuzzleConditionComponent::HandleWaterStateChanged);
		BoundWaterBasinTarget = nullptr;
	}
}

UUOUWaterBasinTargetComponent* UUOUPuzzleConditionComponent::ResolveWaterBasinTarget() const
{
	if (IsValid(TargetWaterTileActor))
	{
		return TargetWaterTileActor->FindComponentByClass<UUOUWaterBasinTargetComponent>();
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (bAutoResolveWaterTileFromPlatform)
	{
		if (const UUOUWaterBasinPlatformComponent* PlatformComponent = ResolvePlatformComponent())
		{
			if (IsValid(PlatformComponent->TargetActor))
			{
				if (UUOUWaterBasinTargetComponent* TargetComponent = PlatformComponent->TargetActor->FindComponentByClass<UUOUWaterBasinTargetComponent>())
				{
					return TargetComponent;
				}
			}
		}
	}

	if (UUOUWaterBasinTargetComponent* OwnerTargetComponent = Owner->FindComponentByClass<UUOUWaterBasinTargetComponent>())
	{
		return OwnerTargetComponent;
	}

	AActor* ParentActor = Owner->GetAttachParentActor();
	while (IsValid(ParentActor))
	{
		if (UUOUWaterBasinTargetComponent* ParentTargetComponent = ParentActor->FindComponentByClass<UUOUWaterBasinTargetComponent>())
		{
			return ParentTargetComponent;
		}

		ParentActor = ParentActor->GetAttachParentActor();
	}

	return nullptr;
}

UUOUWaterBasinPlatformComponent* UUOUPuzzleConditionComponent::ResolvePlatformComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UUOUWaterBasinPlatformComponent>() : nullptr;
}

FUOUPuzzleConditionContext UUOUPuzzleConditionComponent::BuildConditionContext(UUOUWaterBasinTargetComponent* WaterBasinTarget) const
{
	FUOUPuzzleConditionContext Context;
	Context.ConditionOwner = GetOwner();
	Context.ConditionComponent = const_cast<UUOUPuzzleConditionComponent*>(this);
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

float UUOUPuzzleConditionComponent::ResolveCurrentValue(const FUOUPuzzleConditionContext& Context) const
{
	switch (ValueSource)
	{
	case EUOUPuzzleConditionValueSource::WaterSurfaceWorldZ:
		return Context.WaterSurfaceWorldZ;
	case EUOUPuzzleConditionValueSource::WaterDepth:
		return Context.WaterDepth;
	case EUOUPuzzleConditionValueSource::WaterDepthWorld:
		return Context.WaterDepthWorld;
	case EUOUPuzzleConditionValueSource::WaterFillRatio:
		return Context.WaterFillRatio;
	case EUOUPuzzleConditionValueSource::WaterVolume:
		return Context.WaterVolume;
	case EUOUPuzzleConditionValueSource::PlatformWorldZ:
		return Context.PlatformWorldZ;
	default:
		return 0.0f;
	}
}

bool UUOUPuzzleConditionComponent::DoesValueSatisfyCondition(float CurrentValue) const
{
	switch (CompareMode)
	{
	case EUOUPuzzleConditionCompareMode::GreaterOrEqual:
		return CurrentValue >= ThresholdValue - Tolerance;
	case EUOUPuzzleConditionCompareMode::Greater:
		return CurrentValue > ThresholdValue;
	case EUOUPuzzleConditionCompareMode::LessOrEqual:
		return CurrentValue <= ThresholdValue + Tolerance;
	case EUOUPuzzleConditionCompareMode::Less:
		return CurrentValue < ThresholdValue;
	case EUOUPuzzleConditionCompareMode::Equal:
		return FMath::Abs(CurrentValue - ThresholdValue) <= Tolerance;
	case EUOUPuzzleConditionCompareMode::NotEqual:
		return FMath::Abs(CurrentValue - ThresholdValue) > Tolerance;
	case EUOUPuzzleConditionCompareMode::BetweenInclusive:
	{
		const float MinValue = FMath::Min(ThresholdValue, UpperThresholdValue);
		const float MaxValue = FMath::Max(ThresholdValue, UpperThresholdValue);
		return CurrentValue >= MinValue - Tolerance && CurrentValue <= MaxValue + Tolerance;
	}
	default:
		return false;
	}
}

void UUOUPuzzleConditionComponent::BroadcastConditionResult(const FUOUPuzzleConditionContext& Context, bool bWasSatisfied)
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

void UUOUPuzzleConditionComponent::NotifyReactionTargets(const FUOUPuzzleConditionContext& Context, bool bWasSatisfied)
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

void UUOUPuzzleConditionComponent::NotifyActivatableObject(UObject* Object, const FUOUPuzzleConditionContext& Context, bool bWasSatisfied, TSet<UObject*>& NotifiedObjects)
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
