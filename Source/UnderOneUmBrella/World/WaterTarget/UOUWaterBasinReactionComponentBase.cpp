// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUWaterBasinReactionComponentBase.h"

#include "Debug/UOUDevelopmentDebugDrawContext.h"
#include "GameFramework/Actor.h"
#include "World/WaterTarget/UOUWaterBasinPlatformComponent.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"

namespace
{
	// ValueSource를 해석할 수 없을 때 사용하는 안전한 기본값입니다.
	constexpr float InvalidReactionValue = 0.0f;

	// DrawDebugString에서 0초는 한 프레임 표시를 의미합니다.
	// 이 컴포넌트는 Tick마다 다시 그리므로 지속 시간을 길게 주면 이전 텍스트가 겹쳐 보일 수 있습니다.

	FString GetReactionValueSourceDebugName(EUOUWaterBasinReactionValueSource InValueSource)
	{
		if (const UEnum* ValueSourceEnum = StaticEnum<EUOUWaterBasinReactionValueSource>())
		{
			const FText DisplayName = ValueSourceEnum->GetDisplayNameTextByValue(static_cast<int64>(InValueSource));
			if (!DisplayName.IsEmpty())
			{
				return DisplayName.ToString();
			}

			return ValueSourceEnum->GetNameStringByValue(static_cast<int64>(InValueSource));
		}

		return TEXT("Unknown");
	}
}
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
	if (!WaterBasinTarget
		&& ValueSource != EUOUWaterBasinReactionValueSource::PlatformWorldZ
		&& ValueSource != EUOUWaterBasinReactionValueSource::RotationAngleDegrees
		&& ValueSource != EUOUWaterBasinReactionValueSource::SignedRotationAngleDegrees)
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

TArray<FString> UUOUWaterBasinReactionComponentBase::GetPuzzleDebugInfo_Implementation() const
{
	TArray<FString> DebugInfo = Super::GetPuzzleDebugInfo_Implementation();
	const EUOUWaterBasinReactionValueSource DebugValueSource = bHasEvaluated ? LastContext.ValueSource : ValueSource;
	DebugInfo.Add(FString::Printf(TEXT("Value Source: %s"), *GetReactionValueSourceDebugName(DebugValueSource)));
	DebugInfo.Add(FString::Printf(TEXT("Reaction Value: %.3f / %.3f"), LastContext.CurrentValue, LastContext.ThresholdValue));
	DebugInfo.Add(FString::Printf(TEXT("Water Fill: %.3f"), LastContext.WaterFillRatio));
	DebugInfo.Add(FString::Printf(TEXT("Rotation Angle: %.2f"), LastContext.RotationAngleDegrees));
	DebugInfo.Add(FString::Printf(TEXT("Signed Rotation Angle: %.2f"), LastContext.SignedRotationAngleDegrees));
	DebugInfo.Add(FString::Printf(TEXT("Reaction Events: +%d / -%d"), SatisfiedEventCount, UnsatisfiedEventCount));
	return DebugInfo;
}

EUOUDebugCategory UUOUWaterBasinReactionComponentBase::GetDebugCategory_Implementation() const
{
	return EUOUDebugCategory::Puzzle;
}

#if UOU_WITH_DEVELOPMENT_TOOLS
void UUOUWaterBasinReactionComponentBase::GatherDevelopmentDebugDraw(
	IUOUDevelopmentDebugDrawContext& Context) const
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	TArray<UUOUWaterBasinReactionComponentBase*> ReactionComponents;
	Owner->GetComponents<UUOUWaterBasinReactionComponentBase>(ReactionComponents);
	const int32 FoundReactionIndex = ReactionComponents.IndexOfByPredicate(
		[this](const UUOUWaterBasinReactionComponentBase* ReactionComponent)
		{
			return ReactionComponent == this;
		});
	const int32 ReactionIndex = FMath::Max(0, FoundReactionIndex);
	const TArray<FString> DebugLines = GetPuzzleDebugInfo_Implementation();
	const FString DebugText = FString::Join(DebugLines, LINE_TERMINATOR);
	const FVector DebugLocation = Owner->GetActorLocation()
		+ FVector(0.0f, 0.0f, 280.0f + static_cast<float>(ReactionIndex) * 140.0f);
	const FColor DebugColor = bHasEvaluated
		? (bIsConditionSatisfied ? FColor::Green : FColor::Red)
		: FColor::Yellow;

	Context.DrawString(DebugLocation, DebugText, DebugColor, 0.85f);
}
#endif

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
	Context.ValueSource = ValueSource;
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
	case EUOUWaterBasinReactionValueSource::RotationAngleDegrees:
		return Context.RotationAngleDegrees;
	case EUOUWaterBasinReactionValueSource::SignedRotationAngleDegrees:
		return Context.SignedRotationAngleDegrees;
	default:
		return InvalidReactionValue;
	}
}

bool UUOUWaterBasinReactionComponentBase::DoesValueSatisfyCondition(float CurrentValue) const
{
	// 수면과 플랫폼 위치는 Tick 보간과 float 계산을 거치므로 정확히 같은 값이 되지 않을 수 있습니다.
	// Equal/NotEqual/포함 범위와 같은 경계 비교에는 Tolerance를 적용하고, Greater/Less는 의도한 엄격 비교를 유지합니다.
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
	if (bExposeAsPuzzleCondition)
	{
		// Reaction 조건 결과를 퍼즐 조건 소스 상태에도 동기화합니다.
		SetSatisfiedState(Context.bIsSatisfied, true);
	}
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

