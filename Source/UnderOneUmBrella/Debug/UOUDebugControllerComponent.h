// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Debug/UOUDebugTypes.h"
#include "UOUDebugControllerComponent.generated.h"

// Base component for one debug category controlled by AUOUDebugController.
UCLASS(Abstract, Blueprintable, ClassGroup=(Debug), meta=(BlueprintSpawnableComponent))
class UNDERONEUMBRELLA_API UUOUDebugControllerComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUDebugControllerComponentBase();

	UPROPERTY(BlueprintReadWrite, Transient, Category = "Debug|Common", meta = (ToolTip = "Enables this debug category at runtime."))
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Debug|Common", meta = (ToolTip = "Debug category represented by this component."))
	EUOUDebugCategory DebugCategory = EUOUDebugCategory::System;

	UPROPERTY(BlueprintReadWrite, Category = "Debug|Common", meta = (ToolTip = "Default color for debug lines, labels, and helpers."))
	FColor DebugColor = FColor::White;

	UPROPERTY(BlueprintReadWrite, Category = "Debug|Common", meta = (ToolTip = "Display priority. Higher values should be shown first when budget limits are added."))
	int32 Priority = 0;

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void SetDebugEnabled(bool bNewEnabled);

	UFUNCTION(BlueprintPure, Category = "Debug")
	bool IsDebugEnabled() const;

	UFUNCTION(BlueprintPure, Category = "Debug")
	FName GetDebugCategoryName() const;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Player Debug Controller"))
class UNDERONEUMBRELLA_API UUOUPlayerDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUPlayerDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Player", meta = (ToolTip = "Shows player debug information on the main viewport."))
	bool bShowViewportHUD = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Player", meta = (ToolTip = "Includes the current player interaction target."))
	bool bShowInteractionTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Player", meta = (ToolTip = "Includes player movement, input, and state values."))
	bool bShowMovementState = true;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU NPC Debug Controller"))
class UNDERONEUMBRELLA_API UUOUNPCDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUNPCDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|NPC", meta = (ToolTip = "Shows world labels near NPC actors."))
	bool bShowWorldLabels = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|NPC", meta = (ToolTip = "Shows NPC movement targets."))
	bool bShowMoveTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|NPC", meta = (ToolTip = "Shows NPC path and direction helper lines."))
	bool bShowPath = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|NPC", meta = (ToolTip = "Shows current NPC AI or action state."))
	bool bShowState = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|NPC", meta = (ToolTip = "Shows current requested and active NPC animation information."))
	bool bShowAnimation = true;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Puzzle Debug Controller"))
class UNDERONEUMBRELLA_API UUOUPuzzleDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUPuzzleDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "Shows summary labels above puzzle actors."))
	bool bShowWorldLabels = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "Shows connection lines between puzzle input, condition, and result actors."))
	bool bShowConnections = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "Shows compact puzzle summary text."))
	bool bShowSummaryText = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Puzzle", meta = (ToolTip = "Shows current active or inactive state for puzzle nodes."))
	bool bShowNodeState = true;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Interaction Debug Controller"))
class UNDERONEUMBRELLA_API UUOUInteractionDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUInteractionDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Interaction", meta = (ToolTip = "Shows interaction trace range and direction."))
	bool bShowTrace = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Interaction", meta = (ToolTip = "Shows the current interaction candidate."))
	bool bShowCandidate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Interaction", meta = (ToolTip = "Shows why interaction is currently blocked."))
	bool bShowFailReason = false;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU VFX Debug Controller"))
class UNDERONEUMBRELLA_API UUOUVFXDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUVFXDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|VFX", meta = (ToolTip = "Shows active VFX or particle counts."))
	bool bShowParticleCount = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|VFX", meta = (ToolTip = "Shows Niagara component owner locations."))
	bool bShowNiagaraOwners = false;
};

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent, DisplayName = "UOU Performance Debug Controller"))
class UNDERONEUMBRELLA_API UUOUPerformanceDebugControllerComponent : public UUOUDebugControllerComponentBase
{
	GENERATED_BODY()

public:
	UUOUPerformanceDebugControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Performance", meta = (ToolTip = "Shows performance information on the main viewport."))
	bool bShowViewportStats = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Performance", meta = (ToolTip = "Shows the current FPS value."))
	bool bShowFPS = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Performance", meta = (ToolTip = "Shows frame time in milliseconds."))
	bool bShowFrameTime = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Performance", meta = (ToolTip = "Shows actor, component, and world count summaries."))
	bool bShowWorldCounts = false;
};
