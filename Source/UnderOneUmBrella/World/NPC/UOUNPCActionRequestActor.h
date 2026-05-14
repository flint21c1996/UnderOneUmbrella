// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/NPC/UOUNPCActionTypes.h"
#include "UOUNPCActionRequestActor.generated.h"

class AUOUNPCCharacter;
class USceneComponent;

// 활성화될 때 NPC에 단일 액션 요청을 전달하는 액터입니다.
UCLASS(Blueprintable, meta = (DisplayName = "UOU NPC Action Request", ToolTip = "활성화될 때 설정된 NPC 액션 하나를 요청합니다."))
class AUOUNPCActionRequestActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUNPCActionRequestActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Action")
	TObjectPtr<USceneComponent> RootScene = nullptr;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Action", meta = (ToolTip = "이 액션 요청을 받을 NPC입니다."))
	TObjectPtr<AUOUNPCCharacter> TargetNPC = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Action", meta = (ToolTip = "Activate가 호출될 때 대상 NPC에 전달할 액션 데이터입니다."))
	FUOUNPCActionRequest ActionRequest;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Action", meta = (ToolTip = "플레이가 시작될 때 이 액션 요청을 자동으로 보냅니다."))
	bool bActivateOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Action", meta = (ToolTip = "Deactivate가 호출될 때 이 액터가 요청한 NPC 액션을 비웁니다."))
	bool bClearNPCActionOnDeactivate = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Runtime", meta = (ToolTip = "Activate 호출 후 이 요청이 비활성화되거나 정리되기 전까지 true입니다."))
	bool bActivated = false;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Activate();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Deactivate();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Actions")
	void Toggle();
};
