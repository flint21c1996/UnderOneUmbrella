// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Debug/UOUDebugTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "UOUDevelopmentDebugControlSubsystem.generated.h"

class AActor;

// 개발 도구 HUD와 런타임 디버그 설정 사이의 제어 진입점입니다.
UCLASS()
class UNDERONEUMBRELLADEVTOOLS_API UUOUDevelopmentDebugControlSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

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

	// 기존 단일 선택 호출자를 위해 현재 선택을 비우고 전달한 액터 하나만 선택합니다.
	UFUNCTION(BlueprintCallable, Category = "Development Debug|Selection")
	void SetSelectedDebugActor(AActor* NewSelectedActor);

	// 기존 단일 선택 렌더러를 위해 유효한 선택 액터 하나를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Development Debug|Selection")
	AActor* GetSelectedDebugActor() const;

	// HUD에서 전달한 액터를 현재 복수 선택 집합에 추가하거나 제거합니다.
	UFUNCTION(BlueprintCallable, Category = "Development Debug|Selection")
	void ToggleSelectedDebugActor(AActor* DebugActor);

	// 현재 선택한 모든 디버그 액터를 해제합니다.
	UFUNCTION(BlueprintCallable, Category = "Development Debug|Selection")
	void ClearSelectedDebugActors();

	// 전달한 액터가 현재 복수 선택 집합에 포함되어 있는지 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Development Debug|Selection")
	bool IsDebugActorSelected(const AActor* Actor) const;

	// 현재 복수 선택 집합에서 유효한 액터를 이름순으로 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Development Debug|Selection")
	TArray<AActor*> GetSelectedDebugActors() const;

	// 전달한 액터가 현재 선택된 디버그 대상인지 확인합니다.
	bool ShouldDrawDebugActor(const AActor* Actor) const;

private:
	// 현재 월드의 전체 디버그 표시 여부를 저장하는 개발 도구 런타임 상태입니다.
	UPROPERTY(Transient)
	bool bDebugToolsEnabled = true;

	// 전체 디버그 상태와 독립적으로 꺼져 있는 카테고리만 저장합니다.
	UPROPERTY(Transient)
	TSet<EUOUDebugCategory> DisabledDebugCategories;

	// HUD가 선택한 복수 액터의 약한 참조 집합이며 월드 액터의 수명을 소유하지 않습니다.
	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<AActor>> SelectedDebugActors;
};
