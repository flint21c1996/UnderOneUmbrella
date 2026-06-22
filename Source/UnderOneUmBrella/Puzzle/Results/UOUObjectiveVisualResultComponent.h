// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUObjectiveVisualResultComponent.generated.h"

class UNiagaraComponent;

// Puzzle Result 액션을 받아 목표 안내용 Niagara 표시 상태를 제어하는 컴포넌트입니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Objective Visual Result"))
class UNDERONEUMBRELLA_API UUOUObjectiveVisualResultComponent
	: public UActorComponent
	, public IUOUPuzzleResultReceiver
{
	GENERATED_BODY()

public:
	UUOUObjectiveVisualResultComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Objective Visual", meta = (ToolTip = "직접 지정할 목표 안내 Niagara 컴포넌트입니다. 비워두면 Reference, 이름/태그, 첫 Niagara 순서로 찾습니다."))
	TObjectPtr<UNiagaraComponent> TargetNiagaraComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Objective Visual", meta = (ToolTip = "같은 Actor 안에서 찾을 목표 안내 Niagara 컴포넌트 참조입니다."))
	FComponentReference TargetNiagaraComponentReference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Objective Visual", meta = (ToolTip = "자동 탐색 시 사용할 Niagara 컴포넌트 이름 또는 Component Tag입니다."))
	FName TargetNiagaraComponentName = TEXT("ObjectiveVisual");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Objective Visual", meta = (ToolTip = "직접 지정된 컴포넌트가 없을 때 이름 또는 태그로 Niagara 컴포넌트를 자동 탐색합니다."))
	bool bAutoFindNiagaraComponentByNameOrTag = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Objective Visual", meta = (ToolTip = "이름/태그로 찾지 못했을 때 소유 Actor의 첫 Niagara 컴포넌트를 사용할지 정합니다."))
	bool bAutoFindFirstNiagaraComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Objective Visual", meta = (ToolTip = "게임 시작 시 목표 안내 Niagara를 켜둘지 정합니다."))
	bool bStartActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Objective Visual", meta = (ToolTip = "Activate 액션을 받을 때 Niagara를 처음부터 다시 재생합니다."))
	bool bResetOnActivate = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Objective Visual", meta = (ToolTip = "비활성화할 때 컴포넌트 Visibility도 함께 끕니다."))
	bool bControlVisibility = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Objective Visual|Runtime")
	bool bObjectiveVisualActive = false;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Puzzle|Objective Visual")
	void ShowObjectiveVisual();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Puzzle|Objective Visual")
	void HideObjectiveVisual();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Puzzle|Objective Visual")
	void ToggleObjectiveVisual();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Objective Visual")
	void SetObjectiveVisualActive(bool bNewActive);

	UFUNCTION(BlueprintPure, Category = "Puzzle|Objective Visual")
	bool IsObjectiveVisualActive() const;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Objective Visual")
	UNiagaraComponent* ResolveTargetNiagaraComponent();

	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;

protected:
	void ApplyObjectiveVisualState();
	UNiagaraComponent* FindTargetNiagaraComponent() const;
};
