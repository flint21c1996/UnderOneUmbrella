// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUWaterBasinReactionComponentBase.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "World/WaterTarget/UOUWaterBasinPlatformComponent.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"

UUOUWaterBasinReactionComponentBase::UUOUWaterBasinReactionComponentBase()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUOUWaterBasinReactionComponentBase::OnRegister()
{
	Super::OnRegister();

	ResolveWaterBasinTarget();
}

void UUOUWaterBasinReactionComponentBase::BeginPlay()
{
	Super::BeginPlay();

	BindToWaterBasinTarget();

	if (bEvaluateOnBeginPlay)
	{
		EvaluateReaction();
	}
}

void UUOUWaterBasinReactionComponentBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromWaterBasinTarget();

	Super::EndPlay(EndPlayReason);
}

void UUOUWaterBasinReactionComponentBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bEvaluateEveryTick || ValueSource == EUOUWaterBasinReactionValueSource::PlatformWorldZ)
	{
		EvaluateReaction();
	}

	DrawReactionDebugText();
}

#if WITH_EDITOR
void UUOUWaterBasinReactionComponentBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName ChangedPropertyName = PropertyChangedEvent.Property
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UUOUWaterBasinReactionComponentBase, TargetWaterTileActor))
	{
		bTargetWaterTileActorAutoResolved = false;
		UnbindFromWaterBasinTarget();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UUOUWaterBasinReactionComponentBase::EvaluateReaction(bool bForceNotify)
{
	BindToWaterBasinTarget();

	UUOUWaterBasinTargetComponent* WaterBasinTarget = ResolveWaterBasinTarget();
	if (!WaterBasinTarget && ValueSource != EUOUWaterBasinReactionValueSource::PlatformWorldZ)
	{
		return;
	}

	FUOUWaterBasinReactionContext NewContext = BuildReactionContext(WaterBasinTarget);
	NewContext.bIsSatisfied = EvaluateReactionCondition(NewContext);

	const bool bWasSatisfied = bIsConditionSatisfied;
	bIsConditionSatisfied = NewContext.bIsSatisfied;
	bHasEvaluated = true;
	LastContext = NewContext;

	OnWaterBasinReactionStateUpdated(NewContext);
	NotifyReactionResult(NewContext, bWasSatisfied, bForceNotify);
}

bool UUOUWaterBasinReactionComponentBase::IsReactionConditionSatisfied() const
{
	return bIsConditionSatisfied;
}

FUOUWaterBasinReactionContext UUOUWaterBasinReactionComponentBase::GetLastReactionContext() const
{
	return LastContext;
}

void UUOUWaterBasinReactionComponentBase::OnWaterBasinReactionStateUpdated_Implementation(const FUOUWaterBasinReactionContext& /*Context*/)
{
}

void UUOUWaterBasinReactionComponentBase::OnWaterBasinReactionSatisfied_Implementation(const FUOUWaterBasinReactionContext& /*Context*/)
{
}

void UUOUWaterBasinReactionComponentBase::OnWaterBasinReactionUnsatisfied_Implementation(const FUOUWaterBasinReactionContext& /*Context*/)
{
}

UUOUWaterBasinTargetComponent* UUOUWaterBasinReactionComponentBase::ResolveWaterBasinTarget()
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

UUOUWaterBasinPlatformComponent* UUOUWaterBasinReactionComponentBase::ResolvePlatformComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UUOUWaterBasinPlatformComponent>() : nullptr;
}

FUOUWaterBasinReactionContext UUOUWaterBasinReactionComponentBase::BuildReactionContext(UUOUWaterBasinTargetComponent* WaterBasinTarget)
{
	FUOUWaterBasinReactionContext Context;
	Context.ReactionOwner = GetOwner();
	Context.ReactionComponent = this;
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

bool UUOUWaterBasinReactionComponentBase::EvaluateReactionCondition(FUOUWaterBasinReactionContext& Context)
{
	Context.CurrentValue = ResolveCurrentValue(Context);
	Context.ThresholdValue = ThresholdValue;
	return DoesValueSatisfyCondition(Context.CurrentValue);
}

float UUOUWaterBasinReactionComponentBase::ResolveCurrentValue(const FUOUWaterBasinReactionContext& Context) const
{
	switch (ValueSource)
	{
	case EUOUWaterBasinReactionValueSource::WaterSurfaceWorldZ:
		return Context.WaterSurfaceWorldZ;
	case EUOUWaterBasinReactionValueSource::WaterDepth:
		return Context.WaterDepth;
	case EUOUWaterBasinReactionValueSource::WaterDepthWorld:
		return Context.WaterDepthWorld;
	case EUOUWaterBasinReactionValueSource::WaterFillRatio:
		return Context.WaterFillRatio;
	case EUOUWaterBasinReactionValueSource::WaterVolume:
		return Context.WaterVolume;
	case EUOUWaterBasinReactionValueSource::PlatformWorldZ:
		return Context.PlatformWorldZ;
	default:
		return 0.0f;
	}
}

bool UUOUWaterBasinReactionComponentBase::DoesValueSatisfyCondition(float CurrentValue) const
{
	switch (CompareMode)
	{
	case EUOUWaterBasinReactionCompareMode::GreaterOrEqual:
		return CurrentValue >= ThresholdValue - Tolerance;
	case EUOUWaterBasinReactionCompareMode::Greater:
		return CurrentValue > ThresholdValue;
	case EUOUWaterBasinReactionCompareMode::LessOrEqual:
		return CurrentValue <= ThresholdValue + Tolerance;
	case EUOUWaterBasinReactionCompareMode::Less:
		return CurrentValue < ThresholdValue;
	case EUOUWaterBasinReactionCompareMode::Equal:
		return FMath::Abs(CurrentValue - ThresholdValue) <= Tolerance;
	case EUOUWaterBasinReactionCompareMode::NotEqual:
		return FMath::Abs(CurrentValue - ThresholdValue) > Tolerance;
	case EUOUWaterBasinReactionCompareMode::BetweenInclusive:
	{
		const float MinValue = FMath::Min(ThresholdValue, UpperThresholdValue);
		const float MaxValue = FMath::Max(ThresholdValue, UpperThresholdValue);
		return CurrentValue >= MinValue - Tolerance && CurrentValue <= MaxValue + Tolerance;
	}
	default:
		return false;
	}
}

void UUOUWaterBasinReactionComponentBase::HandleWaterStateChanged(UUOUWaterBasinTargetComponent* ChangedTarget)
{
	if (ChangedTarget != BoundWaterBasinTarget)
	{
		BindToWaterBasinTarget();
	}

	EvaluateReaction();
}

void UUOUWaterBasinReactionComponentBase::BindToWaterBasinTarget()
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
		BoundWaterBasinTarget->OnWaterStateChanged.AddUniqueDynamic(this, &UUOUWaterBasinReactionComponentBase::HandleWaterStateChanged);
	}
}

void UUOUWaterBasinReactionComponentBase::UnbindFromWaterBasinTarget()
{
	if (BoundWaterBasinTarget)
	{
		BoundWaterBasinTarget->OnWaterStateChanged.RemoveDynamic(this, &UUOUWaterBasinReactionComponentBase::HandleWaterStateChanged);
		BoundWaterBasinTarget = nullptr;
	}
}

void UUOUWaterBasinReactionComponentBase::NotifyReactionResult(const FUOUWaterBasinReactionContext& Context, bool bWasSatisfied, bool bForceNotify)
{
	const bool bBecameSatisfied = !bWasSatisfied && Context.bIsSatisfied;
	const bool bBecameUnsatisfied = bWasSatisfied && !Context.bIsSatisfied;
	const bool bCanTriggerSatisfied = !bTriggerOnce || !bHasTriggeredOnce;

	if (bForceNotify || bBecameSatisfied || bBecameUnsatisfied)
	{
		OnReactionConditionChanged.Broadcast(Context);
	}

	if ((bForceNotify || bBecameSatisfied) && Context.bIsSatisfied && bCanTriggerSatisfied)
	{
		OnReactionSatisfied.Broadcast(Context);
		OnWaterBasinReactionSatisfied(Context);
		++SatisfiedEventCount;
		bHasTriggeredOnce = true;
	}
	else if ((bForceNotify || bBecameUnsatisfied) && !Context.bIsSatisfied)
	{
		OnReactionUnsatisfied.Broadcast(Context);
		OnWaterBasinReactionUnsatisfied(Context);
		++UnsatisfiedEventCount;
	}
}

void UUOUWaterBasinReactionComponentBase::DrawReactionDebugText()
{
	if (!bDrawDebugText || !GetWorld() || !GetOwner())
	{
		return;
	}

	const FVector DrawLocation = GetOwner()->GetActorLocation() + DrawDebugOffset;
	const FColor TextColor = bHasEvaluated
		? (bIsConditionSatisfied ? DebugSatisfiedColor : DebugUnsatisfiedColor)
		: DebugWaitingColor;

	const FString DebugText = FString::Printf(
		TEXT("%s\nSatisfied: %s\nValue: %.3f / %.3f\nWater Z: %.1f\nDepth: %.3f\nFill: %.3f\nVolume: %.3f\nEvents: +%d / -%d"),
		*GetName(),
		bIsConditionSatisfied ? TEXT("TRUE") : TEXT("FALSE"),
		LastContext.CurrentValue,
		LastContext.ThresholdValue,
		LastContext.WaterSurfaceWorldZ,
		LastContext.WaterDepth,
		LastContext.WaterFillRatio,
		LastContext.WaterVolume,
		SatisfiedEventCount,
		UnsatisfiedEventCount);

	DrawDebugString(GetWorld(), DrawLocation, DebugText, nullptr, TextColor, 0.0f, true);
}
