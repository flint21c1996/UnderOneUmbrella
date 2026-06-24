// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TimerManager.h"
#include "UOULevelTransitionSubsystem.generated.h"

class APlayerController;
class UWorld;

USTRUCT(BlueprintType)
struct FUOULevelTransitionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Transition", meta = (ClampMin = "0.0", ToolTip = "Time spent fading the screen to black before opening the level."))
	float FadeOutDuration = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Transition", meta = (ClampMin = "0.0", ToolTip = "Time to keep the screen black before opening the level."))
	float BlackHoldDuration = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Transition", meta = (ClampMin = "0.0", ToolTip = "Time spent fading back in after the new level is loaded."))
	float FadeInDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Transition", meta = (ToolTip = "Screen fade color."))
	FLinearColor FadeColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Transition", meta = (ToolTip = "If enabled, camera fade also fades audio."))
	bool bFadeAudio = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Transition", meta = (ToolTip = "If enabled, player move and look input are blocked while the transition is running."))
	bool bLockPlayerInputDuringTransition = true;
};

UCLASS()
class UNDERONEUMBRELLA_API UUOULevelTransitionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	bool RequestLevelTransition(TSoftObjectPtr<UWorld> TargetLevel, FUOULevelTransitionSettings Settings);

	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	bool RequestLevelTransitionByName(FName LevelName, FUOULevelTransitionSettings Settings);

	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	bool RestartCurrentLevel(FUOULevelTransitionSettings Settings);

	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	void CancelTransition();

	UFUNCTION(BlueprintPure, Category = "Level Transition")
	bool IsTransitioning() const { return bIsTransitioning; }

private:
	enum class ETransitionTargetType : uint8
	{
		None,
		SoftLevel,
		LevelName
	};

	bool BeginTransition(FUOULevelTransitionSettings Settings);
	void FinishFadeOut();
	void OpenPendingLevel();
	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	void StartPostLoadFadeIn();
	void FinishTransition();
	void ClearTransitionTimers(UWorld* World);
	void ResetPendingTransition();
	void SetPlayerInputLocked(UWorld* World, bool bLocked) const;
	APlayerController* ResolvePlayerController(UWorld* World) const;
	UWorld* GetSubsystemWorld() const;

	ETransitionTargetType PendingTargetType = ETransitionTargetType::None;
	TSoftObjectPtr<UWorld> PendingTargetLevel;
	FName PendingLevelName = NAME_None;
	FUOULevelTransitionSettings ActiveSettings;
	TWeakObjectPtr<UWorld> FadeInWorld;

	FTimerHandle FadeOutTimerHandle;
	FTimerHandle BlackHoldTimerHandle;
	FTimerHandle FadeInTimerHandle;

	bool bIsTransitioning = false;
	bool bWaitingForPostLoadFadeIn = false;
};
