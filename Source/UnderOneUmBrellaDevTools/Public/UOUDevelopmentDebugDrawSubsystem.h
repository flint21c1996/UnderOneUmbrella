// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UOUDevelopmentDebugDrawSubsystem.generated.h"

class UUOUDevelopmentDebugControlSubsystem;

// 개발 도구 설정을 읽고 디버그 정보와 월드 표시를 렌더링할 실행부입니다.
UCLASS()
class UNDERONEUMBRELLADEVTOOLS_API UUOUDevelopmentDebugDrawSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	// 같은 월드의 전체 및 카테고리 디버그 설정을 읽기 위한 약한 참조입니다.
	TWeakObjectPtr<UUOUDevelopmentDebugControlSubsystem> DebugControlSubsystem;
};
