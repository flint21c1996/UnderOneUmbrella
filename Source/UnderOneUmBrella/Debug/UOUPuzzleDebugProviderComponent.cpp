// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/UOUPuzzleDebugProviderComponent.h"

#include "Components/ActorComponent.h"
#include "Debug/UOUPuzzleDebugInfoProvider.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleConditionGroupActor.h"

namespace UOUPuzzleDebugProviderPrivate
{
	int32 CountValidActors(const TArray<TObjectPtr<AActor>>& Actors)
	{
		int32 Count = 0;
		for (const AActor* Actor : Actors)
		{
			if (Actor != nullptr)
			{
				++Count;
			}
		}

		return Count;
	}

	FString GetResultActionName(EOUUPuzzleResultAction Action)
	{
		const UEnum* ActionEnum = StaticEnum<EOUUPuzzleResultAction>();
		return ActionEnum != nullptr
			? ActionEnum->GetNameStringByValue(static_cast<int64>(Action))
			: TEXT("Unknown");
	}

	FString GetActorDebugName(const AActor* Actor)
	{
		if (Actor == nullptr)
		{
			return TEXT("None");
		}

#if WITH_EDITOR
		return Actor->GetActorLabel();
#else
		return Actor->GetName();
#endif
	}

	TArray<FString> GetPuzzleDebugInfo(const UObject* Object)
	{
		if (Object == nullptr || !Object->GetClass()->ImplementsInterface(UUOUPuzzleDebugInfoProvider::StaticClass()))
		{
			return {};
		}

		return IUOUPuzzleDebugInfoProvider::Execute_GetPuzzleDebugInfo(const_cast<UObject*>(Object));
	}

	void AppendObjectDebugInfo(const UObject* Object, TArray<FString>& OutLines, int32 MaxLineCount)
	{
		if (Object == nullptr || OutLines.Num() >= MaxLineCount)
		{
			return;
		}

		const TArray<FString> DebugLines = GetPuzzleDebugInfo(Object);
		for (const FString& DebugLine : DebugLines)
		{
			if (!DebugLine.IsEmpty())
			{
				OutLines.Add(DebugLine);
				if (OutLines.Num() >= MaxLineCount)
				{
					break;
				}
			}
		}
	}

	void GatherActorDebugInfo(const AActor* Actor, TArray<FString>& OutLines, int32 MaxLineCount)
	{
		if (Actor == nullptr || OutLines.Num() >= MaxLineCount)
		{
			return;
		}

		AppendObjectDebugInfo(Actor, OutLines, MaxLineCount);

		TInlineComponentArray<UActorComponent*> Components(Actor);
		for (const UActorComponent* Component : Components)
		{
			AppendObjectDebugInfo(Component, OutLines, MaxLineCount);
			if (OutLines.Num() >= MaxLineCount)
			{
				break;
			}
		}
	}

	void AppendActorDebugSection(
		TArray<FString>& SummaryLines,
		const FString& SectionName,
		const AActor* Actor,
		int32 MaxLineCount,
		int32& UsedLineCount)
	{
		if (Actor == nullptr || UsedLineCount + 1 >= MaxLineCount)
		{
			return;
		}

		TArray<FString> ActorDebugLines;
		GatherActorDebugInfo(Actor, ActorDebugLines, MaxLineCount - UsedLineCount - 1);
		if (ActorDebugLines.IsEmpty())
		{
			return;
		}

		SummaryLines.Add(FString::Printf(TEXT("%s: %s"), *SectionName, *GetActorDebugName(Actor)));
		++UsedLineCount;

		for (const FString& ActorDebugLine : ActorDebugLines)
		{
			if (UsedLineCount >= MaxLineCount)
			{
				break;
			}

			SummaryLines.Add(FString::Printf(TEXT("  - %s"), *ActorDebugLine));
			++UsedLineCount;
		}
	}

	void AddConnection(
		TArray<FUOUDebugConnection>& OutConnections,
		UObject* SourceObject,
		UObject* TargetObject,
		EUOUDebugConnectionType ConnectionType,
		const FText& Label,
		const FColor& Color,
		float Thickness)
	{
		if (SourceObject == nullptr || TargetObject == nullptr)
		{
			return;
		}

		FUOUDebugConnection Connection;
		Connection.SourceObject = SourceObject;
		Connection.TargetObject = TargetObject;
		Connection.ConnectionType = ConnectionType;
		Connection.Label = Label;
		Connection.Color = Color;
		Connection.Thickness = Thickness;
		OutConnections.Add(Connection);
	}
}

UUOUPuzzleDebugProviderComponent::UUOUPuzzleDebugProviderComponent()
{
	DebugCategory = EUOUDebugCategory::Puzzle;
	WorldLocationOffset = FVector(0.0f, 0.0f, 160.0f);
}

FVector UUOUPuzzleDebugProviderComponent::GetConditionGroupNodeWorldLocation() const
{
	const AActor* Owner = GetOwner();
	const FVector BaseLocation = Owner != nullptr ? Owner->GetActorLocation() : FVector::ZeroVector;
	return BaseLocation + ConditionGroupNodeOffset;
}

FText UUOUPuzzleDebugProviderComponent::GetDebugDisplayName_Implementation() const
{
	if (!DisplayName.IsEmpty())
	{
		return DisplayName;
	}

	const AActor* Owner = GetOwner();
	return Owner != nullptr
		? FText::FromString(Owner->GetName())
		: Super::GetDebugDisplayName_Implementation();
}

FText UUOUPuzzleDebugProviderComponent::GetDebugSummaryText_Implementation() const
{
	const AUOUPuzzleConditionGroupActor* GroupActor = GetConditionGroupActor();
	if (GroupActor == nullptr)
	{
		return FText::FromString(TEXT("Puzzle debug provider requires UOU Puzzle Condition Group Actor owner."));
	}

	const UUOUPuzzleConditionGroupComponent* GroupComponent = GroupActor->PuzzleConditionGroupComponent;
	const int32 ConditionCount = GroupComponent != nullptr
		? GroupComponent->GetConditionCount()
		: UOUPuzzleDebugProviderPrivate::CountValidActors(GroupActor->ConditionActors);
	const int32 SatisfiedCount = GroupComponent != nullptr ? GroupComponent->GetSatisfiedCount() : 0;
	const bool bSatisfied = GroupActor->IsSatisfied();

	TArray<FString> SummaryLines;
	if (!SummaryText.IsEmpty())
	{
		SummaryLines.Add(SummaryText.ToString());
	}

	SummaryLines.Add(FString::Printf(
		TEXT("%s  %d/%d | Results: %d"),
		bSatisfied ? TEXT("Satisfied") : TEXT("Unsatisfied"),
		SatisfiedCount,
		ConditionCount,
		GroupActor->ResultBindings.Num()));

	if (bShowConnectedActorDebugInfo && MaxConnectedActorDebugInfoLines > 0)
	{
		int32 UsedLineCount = 0;

		for (const AActor* InputActor : InputActors)
		{
			UOUPuzzleDebugProviderPrivate::AppendActorDebugSection(
				SummaryLines,
				TEXT("Input"),
				InputActor,
				MaxConnectedActorDebugInfoLines,
				UsedLineCount);
		}

		for (const AActor* ConditionActor : GroupActor->ConditionActors)
		{
			UOUPuzzleDebugProviderPrivate::AppendActorDebugSection(
				SummaryLines,
				TEXT("Condition"),
				ConditionActor,
				MaxConnectedActorDebugInfoLines,
				UsedLineCount);
		}

		for (const FOUUPuzzleResultBinding& Binding : GroupActor->ResultBindings)
		{
			UOUPuzzleDebugProviderPrivate::AppendActorDebugSection(
				SummaryLines,
				TEXT("Result"),
				Binding.TargetActor.Get(),
				MaxConnectedActorDebugInfoLines,
				UsedLineCount);
		}

		if (UsedLineCount >= MaxConnectedActorDebugInfoLines)
		{
			SummaryLines.Add(TEXT("..."));
		}
	}
	else if (bShowConditionDetails && MaxConditionDetailLines > 0)
	{
		int32 UsedLineCount = 0;

		for (const AActor* ConditionActor : GroupActor->ConditionActors)
		{
			UOUPuzzleDebugProviderPrivate::AppendActorDebugSection(
				SummaryLines,
				TEXT("Condition"),
				ConditionActor,
				MaxConditionDetailLines,
				UsedLineCount);
		}
	}

	return FText::FromString(FString::Join(SummaryLines, LINE_TERMINATOR));
}

void UUOUPuzzleDebugProviderComponent::GetDebugConnections_Implementation(TArray<FUOUDebugConnection>& OutConnections) const
{
	OutConnections.Reset();

	const AUOUPuzzleConditionGroupActor* GroupActor = GetConditionGroupActor();
	if (GroupActor == nullptr)
	{
		return;
	}

	UUOUPuzzleDebugProviderComponent* MutableThis = const_cast<UUOUPuzzleDebugProviderComponent*>(this);
	const FText InputLabel = bShowConnectionLabels ? FText::FromString(TEXT("Input")) : FText::GetEmpty();
	const FText ConditionLabel = bShowConnectionLabels ? FText::FromString(TEXT("Condition")) : FText::GetEmpty();

	if (bShowInputConnections)
	{
		for (AActor* InputActor : InputActors)
		{
			if (InputActor == nullptr)
			{
				continue;
			}

			for (AActor* ConditionActor : GroupActor->ConditionActors)
			{
				UOUPuzzleDebugProviderPrivate::AddConnection(
					OutConnections,
					InputActor,
					ConditionActor,
					EUOUDebugConnectionType::PuzzleInput,
					InputLabel,
					InputConnectionColor,
					ConnectionThickness);
			}
		}
	}

	if (bShowConditionConnections)
	{
		for (AActor* ConditionActor : GroupActor->ConditionActors)
		{
			UOUPuzzleDebugProviderPrivate::AddConnection(
				OutConnections,
				ConditionActor,
				MutableThis,
				EUOUDebugConnectionType::PuzzleCondition,
				ConditionLabel,
				ConditionConnectionColor,
				ConnectionThickness);
		}
	}

	if (bShowResultConnections)
	{
		for (const FOUUPuzzleResultBinding& Binding : GroupActor->ResultBindings)
		{
			const FString LabelText = FString::Printf(
				TEXT("Result: %s / %s"),
				*UOUPuzzleDebugProviderPrivate::GetResultActionName(Binding.SatisfiedAction),
				*UOUPuzzleDebugProviderPrivate::GetResultActionName(Binding.UnsatisfiedAction));
			const FText ResultLabel = bShowConnectionLabels ? FText::FromString(LabelText) : FText::GetEmpty();

			UOUPuzzleDebugProviderPrivate::AddConnection(
				OutConnections,
				MutableThis,
				Binding.TargetActor.Get(),
				EUOUDebugConnectionType::PuzzleResult,
				ResultLabel,
				ResultConnectionColor,
				ConnectionThickness);
		}
	}
}

const AUOUPuzzleConditionGroupActor* UUOUPuzzleDebugProviderComponent::GetConditionGroupActor() const
{
	return Cast<AUOUPuzzleConditionGroupActor>(GetOwner());
}
