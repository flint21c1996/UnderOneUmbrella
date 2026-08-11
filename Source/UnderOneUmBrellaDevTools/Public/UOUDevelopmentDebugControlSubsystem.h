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

	// 기존 활성 디버그 컨트롤러의 전체 활성 상태를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Development Debug")
	bool IsDebugToolsEnabled() const;

	// 기존 활성 디버그 컨트롤러의 전체 활성 상태를 변경합니다.
	UFUNCTION(BlueprintCallable, Category = "Development Debug")
	bool SetDebugToolsEnabled(bool bNewEnabled);

private:
	AUOUDebugController* ResolveDebugController() const;
};
