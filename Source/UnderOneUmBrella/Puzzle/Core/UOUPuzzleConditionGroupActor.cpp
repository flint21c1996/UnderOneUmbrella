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

DEFINE_LOG_CATEGORY_STATIC(LogUOUPuzzleConditionGroup, Log, All);

namespace
{
	UActorComponent* ResolveComponentReference(
		const FComponentReference& ComponentReference,
		AActor* OwningActor)
	{
		// 네이티브 UPROPERTY, 직접 Override, 컴포넌트 경로 등 엔진 기본 참조 방식을 우선합니다.
		if (UActorComponent* ResolvedComponent = ComponentReference.GetComponent(OwningActor))
		{
			return ResolvedComponent;
		}

		if (ComponentReference.ComponentProperty == NAME_None)
		{
			return nullptr;
		}

		// 레벨 인스턴스에 추가된 컴포넌트는 같은 이름의 UPROPERTY가 액터 클래스에 없을 수 있습니다.
		// 이 경우 Referenced Actor의 실제 컴포넌트 객체 이름으로 한 번 더 해석합니다.
		AActor* SearchActor = ComponentReference.OtherActor.IsValid()
			? ComponentReference.OtherActor.Get()
			: OwningActor;
		if (SearchActor == nullptr)
		{
			return nullptr;
		}

		TInlineComponentArray<UActorComponent*> Components(SearchActor);
		for (UActorComponent* Component : Components)
		{
			if (Component != nullptr && Component->GetFName() == ComponentReference.ComponentProperty)
			{
				UE_LOG(LogUOUPuzzleConditionGroup, Log,
					TEXT("Component reference resolved by instance-name fallback | Actor:%s Component:%s"),
					*GetNameSafe(SearchActor),
					*GetNameSafe(Component));
				return Component;
			}
		}

		UE_LOG(LogUOUPuzzleConditionGroup, Warning,
			TEXT("Component reference unresolved | Actor:%s ComponentName:%s Components:%d"),
			*GetNameSafe(SearchActor),
			*ComponentReference.ComponentProperty.ToString(),
			Components.Num());
		return nullptr;
	}

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
	UE_LOG(LogUOUPuzzleConditionGroup, Log,
		TEXT("Group setup refreshed | Group:%s ConditionActors:%d DirectReferences:%d ResolvedSources:%d Satisfied:%s"),
		*GetNameSafe(this),
		ConditionActors.Num(),
		ConditionSourceReferences.Num(),
		ResolvedConditionSources.Num(),
		PuzzleConditionGroupComponent->IsSatisfied() ? TEXT("Yes") : TEXT("No"));
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
	UE_LOG(LogUOUPuzzleConditionGroup, Log,
		TEXT("Group became satisfied | Group:%s ResultBindings:%d"),
		*GetNameSafe(this),
		ResultBindings.Num());
	OnSatisfied.Broadcast();
	DispatchResultBindings(true);
	ReceiveGroupSatisfied();
}

void AUOUPuzzleConditionGroupActor::HandleGroupUnsatisfied()
{
	UE_LOG(LogUOUPuzzleConditionGroup, Log,
		TEXT("Group became unsatisfied | Group:%s ResultBindings:%d"),
		*GetNameSafe(this),
		ResultBindings.Num());
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
		if (UActorComponent* Component = ResolveComponentReference(ConditionSourceReference, this))
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
	for (int32 BindingIndex = 0; BindingIndex < ResultBindings.Num(); ++BindingIndex)
	{
		FOUUPuzzleResultBinding& Binding = ResultBindings[BindingIndex];
		UObject* ResolvedTarget = ResolveResultTarget(Binding);
		UE_LOG(LogUOUPuzzleConditionGroup, Log,
			TEXT("Dispatch binding | Group:%s Index:%d State:%s SpecificComponent:%s Target:%s SatisfiedAction:%s UnsatisfiedAction:%s"),
			*GetNameSafe(this),
			BindingIndex,
			bSatisfied ? TEXT("Satisfied") : TEXT("Unsatisfied"),
			Binding.bTargetSpecificComponent ? TEXT("Yes") : TEXT("No"),
			*GetNameSafe(ResolvedTarget),
			*StaticEnum<EOUUPuzzleResultAction>()->GetNameStringByValue(static_cast<int64>(Binding.SatisfiedAction)),
			*StaticEnum<EOUUPuzzleResultAction>()->GetNameStringByValue(static_cast<int64>(Binding.UnsatisfiedAction)));

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
				ResolvedTarget,
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
			ResolvedTarget,
			Binding.UnsatisfiedAction,
			Binding.UnsatisfiedDelaySeconds);
	}
}

UObject* AUOUPuzzleConditionGroupActor::ResolveResultTarget(const FOUUPuzzleResultBinding& Binding) const
{
	if (Binding.bTargetSpecificComponent)
	{
		return ResolveComponentReference(
			Binding.TargetComponentReference,
			const_cast<AUOUPuzzleConditionGroupActor*>(this));
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
