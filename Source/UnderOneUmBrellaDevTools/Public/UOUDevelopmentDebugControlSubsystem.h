// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
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

private:
	void ApplyMasterStateToLegacyController() const;
	AUOUDebugController* ResolveDebugController() const;

	// 현재 월드의 전체 디버그 표시 여부를 저장하는 개발 도구 런타임 상태입니다.
	UPROPERTY(Transient)
	bool bDebugToolsEnabled = true;
};
