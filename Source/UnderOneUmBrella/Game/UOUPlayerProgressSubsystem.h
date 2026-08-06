// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UOUPlayerProgressSubsystem.generated.h"

class UUOUPlayerProgressSaveGame;
struct FUOUStageProgressRecord;

DECLARE_LOG_CATEGORY_EXTERN(LogUOUPlayerProgress, Log, All);

/** GameInstance가 유지되는 동안 로컬 플레이어의 영구 스테이지 진행도를 소유하고 관리합니다. */
UCLASS(Config=Game)
class UNDERONEUMBRELLA_API UUOUPlayerProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * 저장된 획득 ID 중 현재 스테이지의 RewardIds에도 존재하는 ID만 사용하여 UI 표시 수량을 계산합니다.
	 */
	UFUNCTION(BlueprintPure, Category = "Player Progress|Stage")
	void GetStageRewardCounts(
		FName StageId,
		const TArray<FName>& ValidRewardIds,
		int32& OutCollectedRewardCount,
		int32& OutTotalRewardCount,
		int32& OutMissingRewardCount) const;

	UFUNCTION(BlueprintPure, Category = "Player Progress|Stage")
	bool IsStageCompleted(FName StageId) const;

	/** 스테이지 입장 직전에 이번 도전에서 사용할 StageId, 레벨, 유효 Reward ID를 등록합니다. */
	UFUNCTION(BlueprintCallable, Category = "Player Progress|Stage Attempt")
	bool BeginStageAttempt(
		FName StageId,
		TSoftObjectPtr<UWorld> StageLevel,
		const TArray<FName>& ValidRewardIds);

	/** 수집 연출이 완료된 Reward ID를 이번 도전의 임시 목록에 중복 없이 추가합니다. */
	UFUNCTION(BlueprintCallable, Category = "Player Progress|Stage Attempt")
	bool RecordRewardCollected(FName RewardId);

	UFUNCTION(BlueprintPure, Category = "Player Progress|Stage Attempt")
	FName GetActiveStageId() const { return ActiveStageId; }

	UFUNCTION(BlueprintPure, Category = "Player Progress|Stage Attempt")
	int32 GetPendingRewardCount() const { return PendingRewardIds.Num(); }

	/** 이번 도전의 임시 Reward를 현재 스테이지의 영구 진행도에 합치고 SaveGame에 기록합니다. */
	UFUNCTION(BlueprintCallable, Category = "Player Progress|Stage Attempt")
	bool CommitCurrentStage();

	/** 현재 메모리에 로드된 진행도 객체를 설정된 SaveGame 슬롯에 기록합니다. */
	UFUNCTION(BlueprintCallable, Category = "Player Progress|Save")
	bool SaveProgress();

	/** 현재 메모리 객체를 버리고 설정된 진행도 슬롯을 다시 불러옵니다. */
	UFUNCTION(BlueprintCallable, Category = "Player Progress|Save")
	void ReloadProgress();

private:
	UUOUPlayerProgressSaveGame* CreateDefaultProgress() const;
	void LoadProgress();
	const FUOUStageProgressRecord* FindStageProgress(FName StageId) const;
	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	void ClearStageAttempt();

	/** 로드된 진행도 객체입니다. 에디터 설정값이 아닌 런타임 상태이며 이 Subsystem과 수명을 함께합니다. */
	UPROPERTY(Transient)
	TObjectPtr<UUOUPlayerProgressSaveGame> ProgressSave = nullptr;

	/** 오디오 설정과 분리하여 사용하는 플레이어 개인 진행도 슬롯 이름입니다. */
	UPROPERTY(EditDefaultsOnly, Config, Category = "Player Progress|Save")
	FString SaveSlotName = TEXT("UOUPlayerProgress");

	/** Unreal SaveGame API에 전달하는 로컬 플랫폼 사용자 인덱스입니다. */
	UPROPERTY(EditDefaultsOnly, Config, Category = "Player Progress|Save", meta = (ClampMin = "0"))
	int32 SaveUserIndex = 0;

	/** 현재 플레이 중인 스테이지의 DataTable 행 이름입니다. */
	FName ActiveStageId = NAME_None;

	/** PIE 접두사를 제거한 현재 스테이지 맵의 짧은 패키지 이름입니다. */
	FName ActiveStageLevelName = NAME_None;

	/** 현재 스테이지에서 수집 대상으로 인정하는 Reward ID 집합입니다. */
	TSet<FName> ExpectedRewardIds;

	/** 이번 도전에서 수집했지만 아직 SaveGame에 확정하지 않은 Reward ID 집합입니다. */
	TSet<FName> PendingRewardIds;
};
