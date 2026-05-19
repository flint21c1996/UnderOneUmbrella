// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Debug/UOUDebugProviderComponent.h"
#include "UOUPuzzleDebugProviderComponent.generated.h"

class AUOUPuzzleConditionGroupActor;

// Puzzle debug provider that visualizes input, condition, and result actor relationships.
UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Puzzle Debug Provider"))
class UNDERONEUMBRELLA_API UUOUPuzzleDebugProviderComponent : public UUOUDebugProviderComponent
{
	GENERATED_BODY()

public:
	UUOUPuzzleDebugProviderComponent();

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "Actors that directly influence the puzzle conditions, such as buttons, light sources, or levers."))
	TArray<TObjectPtr<AActor>> InputActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "Draws links from input actors to condition actors."))
	bool bShowInputConnections = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "Draws links from condition actors to the condition group node."))
	bool bShowConditionConnections = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "Draws links from the condition group node to result actors."))
	bool bShowResultConnections = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "Draws a small point at the invisible condition group node."))
	bool bShowConditionGroupNode = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ClampMin = "0.0"))
	float ConditionGroupNodeSize = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle")
	FVector ConditionGroupNodeOffset = FVector(0.0f, 0.0f, 80.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle")
	FColor InputConnectionColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle")
	FColor ConditionConnectionColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle")
	FColor ResultConnectionColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ClampMin = "0.0"))
	float ConnectionThickness = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "Shows text labels on puzzle connection lines. Off by default to keep the view readable."))
	bool bShowConnectionLabels = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "Shows compact runtime condition values on the label board."))
	bool bShowConditionDetails = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (EditCondition = "bShowConditionDetails", ClampMin = "0"))
	int32 MaxConditionDetailLines = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "Collects compact debug values from connected actors and components into this condition group label."))
	bool bShowConnectedActorDebugInfo = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (EditCondition = "bShowConnectedActorDebugInfo", ClampMin = "0"))
	int32 MaxConnectedActorDebugInfoLines = 12;

	virtual FText GetDebugDisplayName_Implementation() const override;
	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual void GetDebugConnections_Implementation(TArray<FUOUDebugConnection>& OutConnections) const override;

	FVector GetConditionGroupNodeWorldLocation() const;

private:
	const AUOUPuzzleConditionGroupActor* GetConditionGroupActor() const;
};
