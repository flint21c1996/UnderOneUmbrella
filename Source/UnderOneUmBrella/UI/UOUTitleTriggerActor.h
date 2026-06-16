// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UI/UOUUITypes.h"
#include "UOUTitleTriggerActor.generated.h"

class UBoxComponent;
class UUOUUISubsystem;

// Trigger actor that asks the HUD to show a chapter, place, or stage title.
// The HUD Blueprint owns the actual title-card layout and animation.
UCLASS()
class UNDERONEUMBRELLA_API AUOUTitleTriggerActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUTitleTriggerActor();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "UI|Title")
	void TriggerTitle(AActor* InstigatorActor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Title")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Title")
	FUOUTitleDisplayData TitleData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Title")
	bool bTriggerOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Title")
	bool bOnlyPlayerPawn = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Title")
	bool bTriggered = false;

private:
	UFUNCTION()
	void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UUOUUISubsystem* GetUISubsystem(AActor* InstigatorActor) const;
};