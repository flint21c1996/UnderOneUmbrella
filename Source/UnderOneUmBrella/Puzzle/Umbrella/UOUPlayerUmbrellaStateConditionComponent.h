// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Player/UOUUmbrellaComponent.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "UOUPlayerUmbrellaStateConditionComponent.generated.h"

class APawn;

// 현재 플레이어의 우산 상태를 퍼즐 조건 소스로 노출합니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Player Umbrella State Condition"))
class UNDERONEUMBRELLA_API UUOUPlayerUmbrellaStateConditionComponent : public UUOUPuzzleConditionSourceComponent
{
	GENERATED_BODY()

public:
	UUOUPlayerUmbrellaStateConditionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual TArray<FString> GetPuzzleDebugInfo_Implementation() const override;
	virtual void GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Umbrella", meta = (ClampMin = "0"))
	int32 PlayerIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Umbrella")
	EUOUUmbrellaState RequiredUmbrellaState = EUOUUmbrellaState::Open;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle|Runtime")
	TObjectPtr<APawn> CachedPlayerPawn = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle|Runtime")
	TObjectPtr<UUOUUmbrellaComponent> CachedUmbrellaComponent = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Umbrella")
	void RefreshPlayerUmbrellaReference();

protected:
	UFUNCTION()
	void HandleUmbrellaStateChanged(EUOUUmbrellaState NewState, bool bHasUmbrella);

	void HandleActorSpawned(AActor* SpawnedActor);
	void StartWaitingForPlayerSpawn();
	void StopWaitingForPlayerSpawn();
	void StartPlayerResolveRetry();
	void StopPlayerResolveRetry();
	void RetryPlayerUmbrellaReference();
	void ClearPlayerUmbrellaReference();
	void RefreshConditionState();
	bool DoesCurrentUmbrellaStateMatch() const;

	FDelegateHandle ActorSpawnedHandle;
	FTimerHandle PlayerResolveRetryTimerHandle;
};
