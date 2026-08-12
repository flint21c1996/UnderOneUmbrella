// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Debug/UOUDebugTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "UOUDevelopmentDebugControlSubsystem.generated.h"

class AUOUDebugController;
class AActor;

// 개발 도구 HUD와 런타임 디버그 설정 사이의 제어 진입점입니다.
UCLASS()
class UNDERONEUMBRELLADEVTOOLS_API UUOUDevelopmentDebugControlSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	// 개발 도구가 소유한 전체 디버그 활성 상태를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Development Debug")
	bool IsDebugToolsEnabled() const;

	// 개발 도구가 소유한 전체 디버그 활성 상태를 변경합니다.
	UFUNCTION(BlueprintCallable, Category = "Development Debug")
	void SetDebugToolsEnabled(bool bNewEnabled);

	// 전체 활성 상태와 별개로 지정한 디버그 카테고리의 활성 상태를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Development Debug")
	bool IsDebugCategoryEnabled(EUOUDebugCategory Category) const;

	// 전체 활성 상태를 유지하면서 지정한 디버그 카테고리의 활성 상태만 변경합니다.
	UFUNCTION(BlueprintCallable, Category = "Development Debug")
	void SetDebugCategoryEnabled(EUOUDebugCategory Category, bool bNewEnabled);

	// HUD에서 선택한 액터를 디버그 표시 대상으로 설정합니다.
	UFUNCTION(BlueprintCallable, Category = "Development Debug|Selection")
	void SetSelectedDebugActor(AActor* NewSelectedActor);

	// 현재 HUD에서 선택한 디버그 대상 액터를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Development Debug|Selection")
	AActor* GetSelectedDebugActor() const;

	// 전달한 액터가 현재 선택된 디버그 대상인지 확인합니다.
	bool ShouldDrawDebugActor(const AActor* Actor) const;

private:
	void ApplyMasterStateToLegacyController() const;
	void ImportCategoryStatesFromLegacyController(const AUOUDebugController& DebugController);
	void ApplyCategoryStateToLegacyController(EUOUDebugCategory Category) const;
	AUOUDebugController* ResolveDebugController() const;

	// 현재 월드의 전체 디버그 표시 여부를 저장하는 개발 도구 런타임 상태입니다.
	UPROPERTY(Transient)
	bool bDebugToolsEnabled = true;

	// 전체 디버그 상태와 독립적으로 꺼져 있는 카테고리만 저장합니다.
	UPROPERTY(Transient)
	TSet<EUOUDebugCategory> DisabledDebugCategories;

	// HUD가 선택한 단일 액터입니다. 월드 액터의 수명을 소유하지 않습니다.
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> SelectedDebugActor;
};
