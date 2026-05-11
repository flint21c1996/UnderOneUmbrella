// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUPuzzleMoverActor.generated.h"

class USceneComponent;

// 조건 그룹의 결과를 받아 지정한 지점 사이를 오가는 퍼즐 결과 액터다.
UCLASS(meta=(DisplayName="UOU Puzzle Mover Actor"))
class AUOUPuzzleMoverActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUPuzzleMoverActor();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> MovingTarget = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> InactivePoint = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<USceneComponent> ActivePoint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Movement")
	float MoveSpeed = 200.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	bool bActivated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	bool bPaused = false;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Activate();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Deactivate();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Pause();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Resume();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Toggle();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void SetActivated(bool bNewActivated);

protected:
	void MoveTarget(float DeltaSeconds);
};
