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
	bool HasPuzzleResultReceiver(UObject* TargetObject)
	{
		if (TargetObject == nullptr)
		{
			return false;
		}

		if (TargetObject->GetClass()->ImplementsInterface(UUOUPuzzleResultReceiver::StaticClass()))
		{
			return true;
		}

		AActor* TargetActor = Cast<AActor>(TargetObject);
		if (TargetActor == nullptr)
		{
			return false;
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

	bool ExecutePuzzleResultReceiver(UObject* TargetObject, EOUUPuzzleResultAction Action)
	{
		if (TargetObject == nullptr || Action == EOUUPuzzleResultAction::None)
		{
			return false;
		}

		AActor* TargetActor = Cast<AActor>(TargetObject);
		if (TargetActor == nullptr)
		{
			if (!TargetObject->GetClass()->ImplementsInterface(UUOUPuzzleResultReceiver::StaticClass()))
			{
				return false;
			}

			IUOUPuzzleResultReceiver::Execute_ApplyPuzzleResult(TargetObject, Action);
			return true;
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
				ResolveResultTarget(Binding),
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
			ResolveResultTarget(Binding),
			Binding.UnsatisfiedAction,
			Binding.UnsatisfiedDelaySeconds);
	}
}

UObject* AUOUPuzzleConditionGroupActor::ResolveResultTarget(const FOUUPuzzleResultBinding& Binding) const
{
	if (Binding.bTargetSpecificComponent)
	{
		return Binding.TargetComponentReference.GetComponent(const_cast<AUOUPuzzleConditionGroupActor*>(this));
	}

	return Binding.TargetActor.Get();
}

bool AUOUPuzzleConditionGroupActor::ShouldSkipSatisfiedAction(const FOUUPuzzleResultBinding& Binding) const
{
	return Binding.bIgnoreSatisfiedActionAfterResultCompleted
		&& IsResultActionCompleted(ResolveResultTarget(Binding), Binding.SatisfiedAction);
}

bool AUOUPuzzleConditionGroupActor::ShouldSkipUnsatisfiedAction(const FOUUPuzzleResultBinding& Binding) const
{
	return Binding.bIgnoreUnsatisfiedActionAfterResultCompleted
		&& IsResultActionCompleted(ResolveResultTarget(Binding), Binding.SatisfiedAction);
}

bool AUOUPuzzleConditionGroupActor::IsResultActionCompleted(
	UObject* TargetObject,
	EOUUPuzzleResultAction Action) const
{
	if (TargetObject == nullptr || Action == EOUUPuzzleResultAction::None)
	{
		return false;
	}

	if (!TargetObject->GetClass()->ImplementsInterface(UUOUPuzzleResultCompletionState::StaticClass()))
	{
		return false;
	}

	return IUOUPuzzleResultCompletionState::Execute_IsPuzzleResultCompleted(TargetObject, Action);
}

bool AUOUPuzzleConditionGroupActor::ExecuteResultAction(UObject* TargetObject, EOUUPuzzleResultAction Action) const
{
	if (TargetObject == nullptr || Action == EOUUPuzzleResultAction::None)
	{
		return false;
	}

	if (!HasPuzzleResultReceiver(TargetObject))
	{
		return false;
	}

	// Actor 대상은 기존처럼 Actor와 수신 컴포넌트 전체에, 컴포넌트 대상은 해당 컴포넌트에만 전달합니다.
	ExecutePuzzleResultReceiver(TargetObject, Action);
	return true;
}

bool AUOUPuzzleConditionGroupActor::DispatchOrScheduleResultAction(
	UObject* TargetObject,
	EOUUPuzzleResultAction Action,
	float DelaySeconds)
{
	if (TargetObject == nullptr || Action == EOUUPuzzleResultAction::None)
	{
		return false;
	}

	if (!HasPuzzleResultReceiver(TargetObject))
	{
		return false;
	}

	const float SafeDelaySeconds = FMath::Max(0.0f, DelaySeconds);
	if (SafeDelaySeconds <= 0.0f)
	{
		return ExecuteResultAction(TargetObject, Action);
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return ExecuteResultAction(TargetObject, Action);
	}

	// 조건 결과를 바로 실행하지 않고, 지정된 시간 뒤 같은 대상 객체에 같은 액션을 전달합니다.
	// 대상 Actor 또는 컴포넌트가 그 사이 사라지면 약한 참조가 무효가 되어 아무 것도 실행하지 않습니다.
	const TWeakObjectPtr<UObject> WeakTargetObject(TargetObject);
	FTimerHandle DelayTimerHandle;
	World->GetTimerManager().SetTimer(
		DelayTimerHandle,
		FTimerDelegate::CreateWeakLambda(
			this,
			[WeakTargetObject, Action]()
			{
				UObject* ResolvedTargetObject = WeakTargetObject.Get();
				if (ResolvedTargetObject == nullptr)
				{
					return;
				}

				if (!HasPuzzleResultReceiver(ResolvedTargetObject))
				{
					return;
				}

				ExecutePuzzleResultReceiver(ResolvedTargetObject, Action);
			}),
		SafeDelaySeconds,
		false);

	return true;
}
