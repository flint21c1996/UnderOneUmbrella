// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Core/UOUPuzzleConditionGroupActor.h"

#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Debug/UOUPuzzleDebugProviderComponent.h"
#include "Engine/World.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "Puzzle/Core/UOUPuzzleResultCompletionState.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "TimerManager.h"

namespace
{
	bool HasPuzzleResultReceiver(AActor* TargetActor)
	{
		if (TargetActor == nullptr)
		{
			return false;
		}

		if (TargetActor->GetClass()->ImplementsInterface(UUOUPuzzleResultReceiver::StaticClass()))
		{
			return true;
		}

		TArray<UActorComponent*> Components;
		TargetActor->GetComponents(Components);
		for (UActorComponent* Component : Components)
		{
			if (Component != nullptr
				&& Component->GetClass()->ImplementsInterface(UUOUPuzzleResultReceiver::StaticClass()))
			{
				return true;
			}
		}

		return false;
	}

	bool ExecutePuzzleResultReceiver(AActor* TargetActor, EOUUPuzzleResultAction Action)
	{
		if (TargetActor == nullptr || Action == EOUUPuzzleResultAction::None)
		{
			return false;
		}

		bool bHandled = false;
		if (TargetActor->GetClass()->ImplementsInterface(UUOUPuzzleResultReceiver::StaticClass()))
		{
			IUOUPuzzleResultReceiver::Execute_ApplyPuzzleResult(TargetActor, Action);
			bHandled = true;
		}

		TArray<UActorComponent*> Components;
		TargetActor->GetComponents(Components);
		for (UActorComponent* Component : Components)
		{
			if (Component == nullptr
				|| !Component->GetClass()->ImplementsInterface(UUOUPuzzleResultReceiver::StaticClass()))
			{
				continue;
			}

			IUOUPuzzleResultReceiver::Execute_ApplyPuzzleResult(Component, Action);
			bHandled = true;
		}

		return bHandled;
	}
}

AUOUPuzzleConditionGroupActor::AUOUPuzzleConditionGroupActor()
{
	// 조건 그룹 액터는 직접 Tick하지 않고 내부 조건 변화 이벤트만 구독합니다.
	PrimaryActorTick.bCanEverTick = false;

	// 씬 배치용 루트와 실제 계산을 담당할 그룹 컴포넌트를 함께 준비합니다.
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	PuzzleConditionGroupComponent = CreateDefaultSubobject<UUOUPuzzleConditionGroupComponent>(
		TEXT("PuzzleConditionGroupComponent"));
	PuzzleConditionGroupComponent->bAutoCollectLocalConditionSources = false;

	PuzzleDebugProviderComponent = CreateDefaultSubobject<UUOUPuzzleDebugProviderComponent>(
		TEXT("PuzzleDebugProviderComponent"));
}

void AUOUPuzzleConditionGroupActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshGroupSetup();
}

void AUOUPuzzleConditionGroupActor::BeginPlay()
{
	Super::BeginPlay();

	if (PuzzleConditionGroupComponent != nullptr)
	{
		PuzzleConditionGroupComponent->OnSatisfied.RemoveDynamic(this, &AUOUPuzzleConditionGroupActor::HandleGroupSatisfied);
		PuzzleConditionGroupComponent->OnSatisfied.AddDynamic(this, &AUOUPuzzleConditionGroupActor::HandleGroupSatisfied);

		PuzzleConditionGroupComponent->OnUnsatisfied.RemoveDynamic(
			this,
			&AUOUPuzzleConditionGroupActor::HandleGroupUnsatisfied);
		PuzzleConditionGroupComponent->OnUnsatisfied.AddDynamic(
			this,
			&AUOUPuzzleConditionGroupActor::HandleGroupUnsatisfied);

		PuzzleConditionGroupComponent->OnStateChanged.RemoveDynamic(
			this,
			&AUOUPuzzleConditionGroupActor::HandleGroupStateChanged);
		PuzzleConditionGroupComponent->OnStateChanged.AddDynamic(
			this,
			&AUOUPuzzleConditionGroupActor::HandleGroupStateChanged);
	}

	RefreshGroupSetup();
}

void AUOUPuzzleConditionGroupActor::RefreshGroupSetup()
{
	// 액터 목록에서 조건 소스를 다시 모아 내부 그룹 컴포넌트에 주입합니다.
	ResolveConditionSourcesFromActors();

	if (PuzzleConditionGroupComponent == nullptr)
	{
		return;
	}

	TArray<UUOUPuzzleConditionSourceComponent*> SourcePointers;
	SourcePointers.Reserve(ResolvedConditionSources.Num());
	for (UUOUPuzzleConditionSourceComponent* ConditionSource : ResolvedConditionSources)
	{
		if (ConditionSource != nullptr)
		{
			SourcePointers.Add(ConditionSource);
		}
	}

	PuzzleConditionGroupComponent->SetExternalConditionSources(SourcePointers);
	PuzzleConditionGroupComponent->RefreshNow();
}

bool AUOUPuzzleConditionGroupActor::IsSatisfied() const
{
	return PuzzleConditionGroupComponent != nullptr && PuzzleConditionGroupComponent->IsSatisfied();
}

#if UOU_WITH_PUZZLE_CHEATS
bool AUOUPuzzleConditionGroupActor::ForceSatisfiedForCheat()
{
	return PuzzleConditionGroupComponent != nullptr
		&& PuzzleConditionGroupComponent->ForceSatisfiedForCheat();
}
#endif

void AUOUPuzzleConditionGroupActor::HandleGroupSatisfied()
{
	OnSatisfied.Broadcast();
	DispatchResultBindings(true);
	ReceiveGroupSatisfied();
}

void AUOUPuzzleConditionGroupActor::HandleGroupUnsatisfied()
{
	OnUnsatisfied.Broadcast();
	DispatchResultBindings(false);
	ReceiveGroupUnsatisfied();
}

void AUOUPuzzleConditionGroupActor::HandleGroupStateChanged(bool bNewSatisfied)
{
	OnStateChanged.Broadcast(bNewSatisfied);
	ReceiveGroupStateChanged(bNewSatisfied);
}

void AUOUPuzzleConditionGroupActor::ResetResultBindingExecutionState()
{
	for (FOUUPuzzleResultBinding& Binding : ResultBindings)
	{
		Binding.bHasRunSatisfiedAction = false;
	}
}

void AUOUPuzzleConditionGroupActor::ResolveConditionSourcesFromActors()
{
	// ConditionActors는 퍼즐 원인을 담는 액터 목록입니다.
	// 각 액터 안의 조건 소스 컴포넌트를 꺼내 하나의 그룹으로 묶습니다.
	ResolvedConditionSources.Reset();

	for (const FComponentReference& ConditionSourceReference : ConditionSourceReferences)
	{
		if (UActorComponent* Component = ConditionSourceReference.GetComponent(this))
		{
			if (UUOUPuzzleConditionSourceComponent* ConditionSource =
				Cast<UUOUPuzzleConditionSourceComponent>(Component))
			{
				ResolvedConditionSources.AddUnique(ConditionSource);
			}
		}
	}

	if (!bCollectConditionSourcesFromConditionActors)
	{
		return;
	}

	for (AActor* ConditionActor : ConditionActors)
	{
		if (ConditionActor == nullptr)
		{
			continue;
		}

		TInlineComponentArray<UUOUPuzzleConditionSourceComponent*> ConditionSourceComponents(ConditionActor);
		for (UUOUPuzzleConditionSourceComponent* ConditionSource : ConditionSourceComponents)
		{
			if (ConditionSource != nullptr)
			{
				ResolvedConditionSources.AddUnique(ConditionSource);
			}
		}
	}
}

void AUOUPuzzleConditionGroupActor::DispatchResultBindings(bool bSatisfied)
{
	// 현재 만족 상태에 맞는 액션을 골라 결과 액터들에게 순서대로 전달합니다.
	for (FOUUPuzzleResultBinding& Binding : ResultBindings)
	{
		if (bSatisfied)
		{
			if (ShouldSkipSatisfiedAction(Binding))
			{
				continue;
			}

			if (Binding.bRunSatisfiedActionOnlyOnce && Binding.bHasRunSatisfiedAction)
			{
				continue;
			}

			if (DispatchOrScheduleResultAction(
				Binding.TargetActor.Get(),
				Binding.SatisfiedAction,
				Binding.SatisfiedDelaySeconds))
			{
				Binding.bHasRunSatisfiedAction = true;
			}
			continue;
		}

		if (ShouldSkipUnsatisfiedAction(Binding))
		{
			continue;
		}

		DispatchOrScheduleResultAction(
			Binding.TargetActor.Get(),
			Binding.UnsatisfiedAction,
			Binding.UnsatisfiedDelaySeconds);
	}
}

bool AUOUPuzzleConditionGroupActor::ShouldSkipSatisfiedAction(const FOUUPuzzleResultBinding& Binding) const
{
	return Binding.bIgnoreSatisfiedActionAfterResultCompleted
		&& IsResultActionCompleted(Binding.TargetActor.Get(), Binding.SatisfiedAction);
}

bool AUOUPuzzleConditionGroupActor::ShouldSkipUnsatisfiedAction(const FOUUPuzzleResultBinding& Binding) const
{
	return Binding.bIgnoreUnsatisfiedActionAfterResultCompleted
		&& IsResultActionCompleted(Binding.TargetActor.Get(), Binding.SatisfiedAction);
}

bool AUOUPuzzleConditionGroupActor::IsResultActionCompleted(
	AActor* TargetActor,
	EOUUPuzzleResultAction Action) const
{
	if (TargetActor == nullptr || Action == EOUUPuzzleResultAction::None)
	{
		return false;
	}

	if (!TargetActor->GetClass()->ImplementsInterface(UUOUPuzzleResultCompletionState::StaticClass()))
	{
		return false;
	}

	return IUOUPuzzleResultCompletionState::Execute_IsPuzzleResultCompleted(TargetActor, Action);
}

bool AUOUPuzzleConditionGroupActor::ExecuteResultAction(AActor* TargetActor, EOUUPuzzleResultAction Action) const
{
	if (TargetActor == nullptr || Action == EOUUPuzzleResultAction::None)
	{
		return false;
	}

	if (!HasPuzzleResultReceiver(TargetActor))
	{
		return false;
	}

	// 결과 액터가 구현한 공통 인터페이스 진입점을 호출합니다.
	// 내부에서 어떤 동작을 할지는 각 액터가 ApplyPuzzleResult로 재정의합니다.
	ExecutePuzzleResultReceiver(TargetActor, Action);
	return true;
}

bool AUOUPuzzleConditionGroupActor::DispatchOrScheduleResultAction(
	AActor* TargetActor,
	EOUUPuzzleResultAction Action,
	float DelaySeconds)
{
	if (TargetActor == nullptr || Action == EOUUPuzzleResultAction::None)
	{
		return false;
	}

	if (!HasPuzzleResultReceiver(TargetActor))
	{
		return false;
	}

	const float SafeDelaySeconds = FMath::Max(0.0f, DelaySeconds);
	if (SafeDelaySeconds <= 0.0f)
	{
		return ExecuteResultAction(TargetActor, Action);
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return ExecuteResultAction(TargetActor, Action);
	}

	// 조건 결과를 바로 실행하지 않고, 지정된 시간 뒤 같은 TargetActor에 같은 액션을 전달합니다.
	// 대상이 그 사이 사라지면 약한 참조가 무효가 되어 아무 것도 실행하지 않습니다.
	const TWeakObjectPtr<AActor> WeakTargetActor(TargetActor);
	FTimerHandle DelayTimerHandle;
	World->GetTimerManager().SetTimer(
		DelayTimerHandle,
		FTimerDelegate::CreateWeakLambda(
			this,
			[WeakTargetActor, Action]()
			{
				AActor* ResolvedTargetActor = WeakTargetActor.Get();
				if (ResolvedTargetActor == nullptr)
				{
					return;
				}

				if (!HasPuzzleResultReceiver(ResolvedTargetActor))
				{
					return;
				}

				ExecutePuzzleResultReceiver(ResolvedTargetActor, Action);
			}),
		SafeDelaySeconds,
		false);

	return true;
}
