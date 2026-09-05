// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Game/UOUStageSelectTypes.h"
#include "GameFramework/Actor.h"
#include "UOUStageSelectNodeActor.generated.h"

class USceneComponent;
class UUOUStageSelectAreaComponent;
class APawn;

UCLASS(meta = (DisplayName = "UOU Stage Select Node"))
class UNDERONEUMBRELLA_API AUOUStageSelectNodeActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUStageSelectNodeActor();

	/** 중앙 DataTable에서 이 노드의 StageId에 해당하는 스테이지 정보를 구성합니다. */
	UFUNCTION(BlueprintCallable, Category = "Stage Select")
	bool GetStageDefinition(FUOUStageDefinition& OutStageDefinition) const;

	UFUNCTION(BlueprintCallable, Category = "Stage Select")
	bool ActivateStage();

	/** 중앙 DataTable의 RowName을 StageId 선택 목록으로 반환합니다. */
	UFUNCTION()
	TArray<FName> GetAvailableStageIds() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Select")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Select")
	TObjectPtr<UUOUStageSelectAreaComponent> StageSelectArea = nullptr;

	/** false이면 이 노드는 Stage Select 등록, 팝업 표시, 스테이지 입장에 참여하지 않습니다. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Stage Select")
	bool bStageSelectionEnabled = true;

	/** 이 노드가 나타내는 스테이지입니다. 중앙 DataTable의 RowName과 같은 값입니다. */
	UPROPERTY(
		EditInstanceOnly,
		BlueprintReadOnly,
		Category = "Stage Select",
		meta = (GetOptions = "GetAvailableStageIds"))
	FName StageId = NAME_None;

	/** 기존 맵에 저장된 RowName을 StageId로 이전하기 전까지 사용하는 호환 데이터입니다. */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use StageId instead."))
	FDataTableRowHandle StageRow;

	/** Kept temporarily so existing Blueprint graphs can be migrated without becoming invalid. */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use StageId instead."))
	int32 StageIndex = 0;

protected:
	virtual void BeginPlay() override;

private:
	/** 새 StageId를 우선 사용하고, 비어 있으면 기존 StageRow.RowName을 반환합니다. */
	FName ResolveStageId() const;

	UFUNCTION()
	void HandlePlayerEnteredStageArea(APawn* PlayerPawn);

	UFUNCTION()
	void HandlePlayerExitedStageArea(APawn* PlayerPawn);
};
