// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Debug/UOUDebugTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "UOUDevelopmentDebugControlSubsystem.generated.h"

class AUOUDebugController;

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
};
