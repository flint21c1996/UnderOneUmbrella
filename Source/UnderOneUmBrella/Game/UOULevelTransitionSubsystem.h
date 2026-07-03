// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TimerManager.h"
#include "UI/UOUTransitionMessageTypes.h"
#include "UOULevelTransitionSubsystem.generated.h"

class APlayerController;
class AUOULevelTransitionSettingsActor;
class UUOULevelTransitionOverlayWidget;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Transition|Map Settings", meta = (DisplayName = "현재 맵 Exit 설정 사용", ToolTip = "켜져 있으면 현재 맵의 UOU Level Transition Settings Actor에 있는 Exit 설정으로 페이드 아웃을 덮어씁니다."))
	bool bUseCurrentMapExitSettings = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Transition|Map Settings", meta = (DisplayName = "도착 맵 Enter 설정 사용", ToolTip = "켜져 있으면 도착 맵의 UOU Level Transition Settings Actor에 있는 Enter 설정으로 페이드 인을 덮어씁니다."))
	bool bUseLoadedMapEnterSettings = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Transition|Message", meta = (DisplayName = "페이드 아웃 문구", ToolTip = "현재 레벨에서 화면이 검게 가려진 뒤 표시할 문구입니다. 비워두면 표시하지 않습니다."))
	FUOUTransitionMessageSettings FadeOutMessageSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Transition|Message", meta = (DisplayName = "페이드 인 문구", ToolTip = "새 레벨이 로드된 뒤 화면이 밝아지는 동안 표시할 문구입니다. 비워두면 표시하지 않습니다."))
	FUOUTransitionMessageSettings FadeInMessageSettings;
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

	bool RequestLevelTransitionFromWorld(UWorld* SourceWorld, TSoftObjectPtr<UWorld> TargetLevel, FUOULevelTransitionSettings Settings);

	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	bool RequestLevelTransitionByName(FName LevelName, FUOULevelTransitionSettings Settings);

	bool RequestLevelTransitionByNameFromWorld(UWorld* SourceWorld, FName LevelName, FUOULevelTransitionSettings Settings);

	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	bool RestartCurrentLevel(FUOULevelTransitionSettings Settings);

	bool RestartCurrentLevelFromWorld(UWorld* SourceWorld, FUOULevelTransitionSettings Settings);

	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	bool RequestNextLevel(FUOULevelTransitionSettings Settings);

	bool RequestNextLevelFromWorld(UWorld* SourceWorld, FUOULevelTransitionSettings Settings);

	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	bool RequestPreviousLevel(FUOULevelTransitionSettings Settings);

	bool RequestPreviousLevelFromWorld(UWorld* SourceWorld, FUOULevelTransitionSettings Settings);

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

	enum class ETransitionMessageStage : uint8
	{
		None,
		FadeOut,
		FadeIn
	};

	bool BeginTransition(UWorld* TransitionWorld, FUOULevelTransitionSettings Settings);
	void UpdateFadeOutOverlay();
	void UpdateFadeInOverlay();
	void FinishFadeOut();
	void StartFadeOutMessageSequence();
	void StartFadeInMessageSequence();
	void StartMessageSequence(ETransitionMessageStage MessageStage, const FUOUTransitionMessageSettings& MessageSettings);
	void UpdateMessageFadeIn();
	void FinishMessageFadeIn();
	void FinishMessageHold();
	void UpdateMessageFadeOut();
	void FinishMessageSequence();
	void ContinueAfterFadeOutMessageSequence();
	void ContinueAfterFadeInMessageSequence();
	void OpenPendingLevel();
	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	void StartPostLoadFadeIn();
	void FinishTransition();
	void ClearTransitionTimers(UWorld* World);
	void ResetPendingTransition();
	void ApplyCurrentMapExitSettings(UWorld* World, FUOULevelTransitionSettings& Settings) const;
	bool ApplyLoadedMapEnterSettings(UWorld* World, FUOULevelTransitionSettings& Settings) const;
	const AUOULevelTransitionSettingsActor* FindLevelTransitionSettingsActor(UWorld* World) const;
	UUOULevelTransitionOverlayWidget* GetOrCreateTransitionOverlay(UWorld* World, const FUOUTransitionMessageSettings& MessageSettings);
	void ShowTransitionOverlay(UWorld* World, const FUOUTransitionMessageSettings& MessageSettings, float InitialBackgroundOpacity, float InitialMessageOpacity);
	void SetTransitionBackgroundOpacity(float NewOpacity);
	void SetTransitionMessageOpacity(float NewOpacity);
	void HideTransitionOverlay();
	void SetPlayerInputLocked(UWorld* World, bool bLocked) const;
	APlayerController* ResolvePlayerController(UWorld* World) const;
	UWorld* GetActiveTransitionWorld() const;
	UWorld* GetSubsystemWorld() const;

	ETransitionTargetType PendingTargetType = ETransitionTargetType::None;
	TSoftObjectPtr<UWorld> PendingTargetLevel;
	FName PendingLevelName = NAME_None;
	FUOULevelTransitionSettings ActiveSettings;
	FUOUTransitionMessageSettings ActiveMessageSettings;
	TWeakObjectPtr<UWorld> ActiveTransitionWorld;
	TWeakObjectPtr<UWorld> FadeInWorld;

	FTimerHandle FadeOutTimerHandle;
	FTimerHandle BlackHoldTimerHandle;
	FTimerHandle FadeInTimerHandle;
	FTimerHandle MessageTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UUOULevelTransitionOverlayWidget> TransitionOverlayWidget = nullptr;

	float FadeOverlayElapsedTime = 0.0f;
	float MessageElapsedTime = 0.0f;
	ETransitionMessageStage ActiveMessageStage = ETransitionMessageStage::None;
	bool bIsTransitioning = false;
	bool bWaitingForPostLoadFadeIn = false;
	bool bInputLockedDuringTransition = false;
};
