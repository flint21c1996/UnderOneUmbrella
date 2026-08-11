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

	// 현재 월드의 액터와 컴포넌트에서 Puzzle 카테고리 Provider를 다시 수집합니다.
	UFUNCTION(BlueprintCallable, Category = "Development Debug|Puzzle")
	void RefreshPuzzleDebugProviders();

	// 현재 캐시에 남아 있는 유효한 Puzzle Provider 개수를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "Development Debug|Puzzle")
	int32 GetPuzzleDebugProviderCount() const;

private:
	void TryAddPuzzleDebugProvider(UObject* ProviderObject);

	// 같은 월드의 전체 및 카테고리 디버그 설정을 읽기 위한 약한 참조입니다.
	TWeakObjectPtr<UUOUDevelopmentDebugControlSubsystem> DebugControlSubsystem;

	// 현재 월드에서 발견한 Puzzle 카테고리 Provider의 약한 참조 캐시입니다.
	TArray<TWeakObjectPtr<UObject>> PuzzleDebugProviders;

	// 다음 Puzzle Provider 월드 스캔까지 남은 시간입니다.
	float PuzzleProviderRefreshTimeRemaining = 0.0f;
};
