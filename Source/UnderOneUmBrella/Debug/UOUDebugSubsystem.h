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

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void RegisterDebugController(AUOUDebugController* DebugController);

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void UnregisterDebugController(AUOUDebugController* DebugController);

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void RegisterDebugProvider(UObject* ProviderObject);

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void UnregisterDebugProvider(UObject* ProviderObject);

	UFUNCTION(BlueprintPure, Category = "Debug")
	AUOUDebugController* GetActiveDebugController() const;

	UFUNCTION(BlueprintPure, Category = "Debug")
	bool IsDebugEnabled(EUOUDebugCategory Category) const;

	UFUNCTION(BlueprintPure, Category = "Debug", meta = (WorldContext = "WorldContextObject"))
	static bool IsDebugCategoryEnabled(const UObject* WorldContextObject, EUOUDebugCategory Category);

	UFUNCTION(BlueprintPure, Category = "Debug", meta = (WorldContext = "WorldContextObject"))
	static FColor GetDebugCategoryColor(const UObject* WorldContextObject, EUOUDebugCategory Category, FColor FallbackColor);

	UFUNCTION(BlueprintPure, Category = "Debug")
	UUOUDebugControllerComponentBase* FindDebugControllerComponent(EUOUDebugCategory Category) const;

	UFUNCTION(BlueprintPure, Category = "Debug")
	int32 GetRegisteredProviderCount() const;

private:
	void ResolveDebugController();
	void CompactRegisteredProviders();
	void DrawControllerStatus() const;
	void DrawRegisteredProviderConnections() const;
	void DrawRegisteredProviderLabelBoards() const;
	FString BuildControllerStatusText() const;

	TWeakObjectPtr<AUOUDebugController> ActiveDebugController;
	TArray<TWeakObjectPtr<UObject>> RegisteredProviders;
	float ControllerSearchTimeRemaining = 0.0f;
	float ProviderCompactTimeRemaining = 0.0f;
};
