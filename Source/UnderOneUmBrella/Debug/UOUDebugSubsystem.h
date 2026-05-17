// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Debug/UOUDebugTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "UOUDebugSubsystem.generated.h"

class AUOUDebugController;
class UUOUDebugControllerComponentBase;

// 월드마다 자동 생성되어 DebugController와 DebugProvider를 연결하는 실행부입니다.
UCLASS()
class UNDERONEUMBRELLA_API UUOUDebugSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	UFUNCTION(BlueprintCallable, Category = "디버그")
	void RegisterDebugController(AUOUDebugController* DebugController);

	UFUNCTION(BlueprintCallable, Category = "디버그")
	void UnregisterDebugController(AUOUDebugController* DebugController);

	UFUNCTION(BlueprintCallable, Category = "디버그")
	void RegisterDebugProvider(UObject* ProviderObject);

	UFUNCTION(BlueprintCallable, Category = "디버그")
	void UnregisterDebugProvider(UObject* ProviderObject);

	UFUNCTION(BlueprintPure, Category = "디버그")
	AUOUDebugController* GetActiveDebugController() const;

	UFUNCTION(BlueprintPure, Category = "디버그")
	bool IsDebugEnabled(EUOUDebugCategory Category) const;

	UFUNCTION(BlueprintPure, Category = "디버그")
	UUOUDebugControllerComponentBase* FindDebugControllerComponent(EUOUDebugCategory Category) const;

	UFUNCTION(BlueprintPure, Category = "디버그")
	int32 GetRegisteredProviderCount() const;

private:
	void ResolveDebugController();
	void CompactRegisteredProviders();
	void DrawControllerStatus() const;
	FString BuildControllerStatusText() const;

	TWeakObjectPtr<AUOUDebugController> ActiveDebugController;
	TArray<TWeakObjectPtr<UObject>> RegisteredProviders;
	float ControllerSearchTimeRemaining = 0.0f;
};

