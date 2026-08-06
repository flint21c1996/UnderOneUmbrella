// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "UOUStageSelectAreaComponent.generated.h"

class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnUOUStageSelectAreaPlayerEvent,
	APawn*, PlayerPawn);

/** Detects when the local player enters or leaves a stage selection node. */
UCLASS(ClassGroup=(StageSelect), meta=(BlueprintSpawnableComponent, DisplayName="UOU Stage Select Area"))
class UNDERONEUMBRELLA_API UUOUStageSelectAreaComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UUOUStageSelectAreaComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Stage Select|Debug")
	void SetDrawDebugAreaInGame(bool bEnabled);

	UPROPERTY(BlueprintAssignable, Category = "Stage Select|Area")
	FOnUOUStageSelectAreaPlayerEvent OnPlayerEntered;

	UPROPERTY(BlueprintAssignable, Category = "Stage Select|Area")
	FOnUOUStageSelectAreaPlayerEvent OnPlayerExited;

	/** Ignore non-player pawns such as NPCs and AI-controlled characters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage Select|Area")
	bool bOnlyLocalPlayerPawn = true;

	/** Draw this area's collision radius while playing. Compiled out of Shipping builds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage Select|Debug")
	bool bDrawDebugAreaInGame = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage Select|Debug", meta = (EditCondition = "bDrawDebugAreaInGame"))
	FColor DebugAreaColor = FColor::Cyan;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage Select|Debug", meta = (ClampMin = "8", ClampMax = "128", EditCondition = "bDrawDebugAreaInGame"))
	int32 DebugAreaSegments = 32;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage Select|Debug", meta = (ClampMin = "0.1", EditCondition = "bDrawDebugAreaInGame"))
	float DebugAreaThickness = 1.5f;

private:
	UFUNCTION()
	void HandleBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

	APawn* ResolveEligiblePlayerPawn(AActor* OtherActor) const;

	TMap<TWeakObjectPtr<APawn>, int32> PlayerOverlapCounts;
};
