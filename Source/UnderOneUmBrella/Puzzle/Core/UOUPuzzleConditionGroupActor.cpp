// Copyright Epic Games, Inc. All Rights Reserved.

#include "Puzzle/Core/UOUPuzzleConditionGroupActor.h"

#include "Components/SceneComponent.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"

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

void AUOUPuzzleConditionGroupActor::ResolveConditionSourcesFromActors()
{
	// ConditionActors는 퍼즐 원인을 담는 액터 목록입니다.
	// 각 액터 안의 조건 소스 컴포넌트를 꺼내 하나의 그룹으로 묶습니다.
	ResolvedConditionSources.Reset();

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

void AUOUPuzzleConditionGroupActor::DispatchResultBindings(bool bSatisfied) const
{
	// 현재 만족 상태에 맞는 액션을 골라 결과 액터들에게 순서대로 전달합니다.
	for (const FOUUPuzzleResultBinding& Binding : ResultBindings)
	{
		ExecuteResultAction(
			Binding.TargetActor.Get(),
			bSatisfied ? Binding.SatisfiedAction : Binding.UnsatisfiedAction);
	}
}

void AUOUPuzzleConditionGroupActor::ExecuteResultAction(AActor* TargetActor, EOUUPuzzleResultAction Action) const
{
	if (TargetActor == nullptr || Action == EOUUPuzzleResultAction::None)
	{
		return;
	}

	if (!TargetActor->GetClass()->ImplementsInterface(UUOUPuzzleResultReceiver::StaticClass()))
	{
		return;
	}

	// 결과 액터가 구현한 공통 인터페이스 진입점을 호출합니다.
	// 내부에서 어떤 동작을 할지는 각 액터가 ApplyPuzzleResult로 재정의합니다.
	IUOUPuzzleResultReceiver::Execute_ApplyPuzzleResult(TargetActor, Action);
}
