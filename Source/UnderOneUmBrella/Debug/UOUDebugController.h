// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Debug/UOUDebugTypes.h"
#include "UOUDebugController.generated.h"

class USceneComponent;
class UUOUDebugControllerComponentBase;
class UUOUInteractionDebugControllerComponent;
class UUOUNPCDebugControllerComponent;
class UUOUPlayerDebugControllerComponent;
class UUOUPerformanceDebugControllerComponent;
class UUOUPuzzleDebugControllerComponent;
class UUOUVFXDebugControllerComponent;

// 레벨에 배치해서 통합 디버그 표시 정책을 조정하는 최상위 컨트롤러입니다.
UCLASS(Blueprintable, meta = (DisplayName = "UOU Debug Controller", ToolTip = "레벨 단위 통합 디버그 표시 옵션을 관리합니다."))
class UNDERONEUMBRELLA_API AUOUDebugController : public AActor
{
	GENERATED_BODY()

public:
	AUOUDebugController();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "디버그")
	TObjectPtr<USceneComponent> RootSceneComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|공통", meta = (ToolTip = "통합 디버그 시스템 전체를 활성화합니다."))
	bool bEnableDebugTools = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|공통", meta = (ToolTip = "Subsystem 상태와 등록된 Provider 수를 화면에 간단히 표시합니다."))
	bool bShowControllerStatus = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|표시", meta = (ClampMin = "0.0", ToolTip = "월드 디버그 UI가 표시될 기본 거리입니다."))
	float WorldDebugVisibleDistance = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|표시", meta = (ClampMin = "1", ToolTip = "한 번에 표시할 월드 디버그 UI의 최대 개수입니다."))
	int32 MaxVisibleWorldDebugItems = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "디버그|표시", meta = (ToolTip = "가까운 대상 전체 대신 화면 중앙 Trace 대상만 상세 표시할 때 사용합니다."))
	bool bOnlyShowFocusedActor = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "디버그|컨트롤러")
	TObjectPtr<UUOUPlayerDebugControllerComponent> PlayerDebugController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "디버그|컨트롤러")
	TObjectPtr<UUOUNPCDebugControllerComponent> NPCDebugController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "디버그|컨트롤러")
	TObjectPtr<UUOUPuzzleDebugControllerComponent> PuzzleDebugController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "디버그|컨트롤러")
	TObjectPtr<UUOUInteractionDebugControllerComponent> InteractionDebugController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "디버그|컨트롤러")
	TObjectPtr<UUOUVFXDebugControllerComponent> VFXDebugController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "디버그|컨트롤러")
	TObjectPtr<UUOUPerformanceDebugControllerComponent> PerformanceDebugController;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "디버그|컨트롤러", meta = (ToolTip = "현재 컨트롤러에 붙어 있는 도메인별 디버그 컨트롤러 목록입니다."))
	TArray<TObjectPtr<UUOUDebugControllerComponentBase>> DebugControllerComponents;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "디버그")
	void RefreshDebugControllerComponents();

	UFUNCTION(BlueprintPure, Category = "디버그")
	bool IsCategoryEnabled(EUOUDebugCategory Category) const;

	UFUNCTION(BlueprintPure, Category = "디버그")
	UUOUDebugControllerComponentBase* FindDebugControllerComponent(EUOUDebugCategory Category) const;

	const TArray<TObjectPtr<UUOUDebugControllerComponentBase>>& GetDebugControllerComponents() const;
};
