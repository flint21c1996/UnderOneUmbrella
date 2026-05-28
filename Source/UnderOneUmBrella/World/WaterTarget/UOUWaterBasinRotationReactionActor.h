// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUWaterBasinRotationReactionActor.generated.h"

class UArrowComponent;
class USceneComponent;
class UStaticMeshComponent;
class UUOUWaterBasinRotationReactionComponent;

UENUM(BlueprintType)
enum class EUOUWaterBasinRotationReactionPuzzleCommand : uint8
{
	Ignore UMETA(DisplayName = "무시", ToolTip = "이 퍼즐 결과 액션을 처리하지 않습니다."),
	EnableReaction UMETA(DisplayName = "회전 반응 켜기", ToolTip = "물 입력과 물 상태 변화에 따른 회전 반응을 켭니다."),
	DisableReaction UMETA(DisplayName = "회전 반응 끄기", ToolTip = "물 입력과 물 상태 변화에 따른 회전 반응을 끕니다."),
	ToggleReaction UMETA(DisplayName = "회전 반응 토글", ToolTip = "현재 회전 반응 활성 상태를 반전합니다."),
	ResetReaction UMETA(DisplayName = "회전 리셋", ToolTip = "회전 반응의 누적 각도와 기준 상태를 리셋합니다."),
	EnableAndReset UMETA(DisplayName = "켜고 리셋", ToolTip = "회전 반응을 켠 뒤 누적 각도와 기준 상태를 리셋합니다."),
	DisableAndReset UMETA(DisplayName = "끄고 리셋", ToolTip = "회전 반응을 끈 뒤 누적 각도와 기준 상태를 리셋합니다.")
};

USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUWaterBasinRotationReactionPuzzleActionSetting
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle Result", meta = (ToolTip = "이 퍼즐 결과 액션이 들어왔을 때 회전 반응 Actor가 실행할 명령입니다."))
	EUOUWaterBasinRotationReactionPuzzleCommand Command = EUOUWaterBasinRotationReactionPuzzleCommand::EnableReaction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle Result", meta = (EditCondition = "Command == EUOUWaterBasinRotationReactionPuzzleCommand::ResetReaction || Command == EUOUWaterBasinRotationReactionPuzzleCommand::EnableAndReset || Command == EUOUWaterBasinRotationReactionPuzzleCommand::DisableAndReset", EditConditionHides, ToolTip = "리셋할 때 마지막으로 관찰한 물 값을 함께 초기화합니다. 켜져 있으면 다음 평가에서 현재 물 상태를 새 기준으로 잡습니다."))
	bool bResetObservedValue = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle Result", meta = (EditCondition = "Command == EUOUWaterBasinRotationReactionPuzzleCommand::ResetReaction || Command == EUOUWaterBasinRotationReactionPuzzleCommand::EnableAndReset || Command == EUOUWaterBasinRotationReactionPuzzleCommand::DisableAndReset", EditConditionHides, ToolTip = "리셋할 때 회전 대상을 기본 회전으로 되돌릴지 정합니다."))
	bool bApplyBaseRotation = true;
};

// WaterBasinRotationReactionComponent를 기본으로 가진 퍼즐 결과 수신 Actor입니다.
// 퍼즐 조건 그룹에서 받은 Activate/Deactivate 같은 결과 액션을 회전 반응 켜기, 끄기, 리셋으로 변환합니다.
UCLASS(meta=(DisplayName="UOU Water Basin Rotation Reaction Actor"))
class UNDERONEUMBRELLA_API AUOUWaterBasinRotationReactionActor : public AActor, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	AUOUWaterBasinRotationReactionActor();

	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Basin Rotation Reaction")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Basin Rotation Reaction")
	TObjectPtr<USceneComponent> RotationCenter = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Basin Rotation Reaction")
	TObjectPtr<UStaticMeshComponent> PlatformMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Basin Rotation Reaction|Input Side")
	TObjectPtr<UArrowComponent> ForwardReference = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water Basin Rotation Reaction")
	TObjectPtr<UUOUWaterBasinRotationReactionComponent> RotationReaction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Activation", meta = (ToolTip = "게임 시작 시 회전 반응을 켤지 정합니다. 꺼두면 퍼즐 결과 Activate 등으로 켜질 때까지 물 입력에 반응하지 않습니다."))
	bool bStartReactionEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Puzzle Result|Activate")
	FUOUWaterBasinRotationReactionPuzzleActionSetting ActivateAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Basin Rotation Reaction|Puzzle Result|Deactivate")
	FUOUWaterBasinRotationReactionPuzzleActionSetting DeactivateAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Water Basin Rotation Reaction|Puzzle Result|Pause")
	FUOUWaterBasinRotationReactionPuzzleActionSetting PauseAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Water Basin Rotation Reaction|Puzzle Result|Resume")
	FUOUWaterBasinRotationReactionPuzzleActionSetting ResumeAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Water Basin Rotation Reaction|Puzzle Result|Toggle")
	FUOUWaterBasinRotationReactionPuzzleActionSetting ToggleAction;

	UFUNCTION(BlueprintCallable, Category = "Water Basin Rotation Reaction|Actions")
	void SetReactionEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Water Basin Rotation Reaction|Runtime")
	bool IsReactionEnabled() const;

	UFUNCTION(BlueprintCallable, Category = "Water Basin Rotation Reaction|Actions")
	void ResetReaction(bool bResetObservedValue = true, bool bApplyBaseRotation = true);

	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

private:
	const FUOUWaterBasinRotationReactionPuzzleActionSetting* GetActionSetting(EOUUPuzzleResultAction Action) const;
	void ExecuteActionSetting(const FUOUWaterBasinRotationReactionPuzzleActionSetting& Setting);
};
