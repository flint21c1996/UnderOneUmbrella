// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Debug/UOUDebugProvider.h"
#include "UOUDebugProviderComponent.generated.h"

// Component-based debug provider for actors that should participate in the UOU debug system.
UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Debug Provider"))
class UNDERONEUMBRELLA_API UUOUDebugProviderComponent : public UActorComponent, public IUOUDebugProvider
{
	GENERATED_BODY()

public:
	UUOUDebugProviderComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (ToolTip = "Controls whether this provider contributes debug information."))
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (ToolTip = "Debug category controlled by the level debug controller."))
	EUOUDebugCategory DebugCategory = EUOUDebugCategory::System;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (ToolTip = "Optional display name shown on debug labels. Uses the owner name when empty."))
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (MultiLine = "true", ToolTip = "Optional summary text shown on debug labels."))
	FText SummaryText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (ToolTip = "World offset used for this provider's debug label."))
	FVector WorldLocationOffset = FVector(0.0f, 0.0f, 120.0f);

	virtual EUOUDebugCategory GetDebugCategory_Implementation() const override;
	virtual bool IsDebugProviderEnabled_Implementation() const override;
	virtual FText GetDebugDisplayName_Implementation() const override;
	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual FVector GetDebugWorldLocation_Implementation() const override;
	virtual void GetDebugConnections_Implementation(TArray<FUOUDebugConnection>& OutConnections) const override;
};
