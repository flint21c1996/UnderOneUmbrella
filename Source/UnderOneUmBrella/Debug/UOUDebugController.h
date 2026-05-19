// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Debug/UOUDebugTypes.h"
#include "UOUDebugController.generated.h"

class USceneComponent;
class UUOUDebugControllerComponentBase;
class UUOUInteractionDebugControllerComponent;
class UUOUNPCDebugControllerComponent;
class UUOUPlayerDebugControllerComponent;
class UUOUPerformanceDebugControllerComponent;
class UUOUPuzzleDebugControllerComponent;
class UUOUVFXDebugControllerComponent;

// Level-placed controller that owns the top-level debug display options.
UCLASS(Blueprintable, meta = (DisplayName = "UOU Debug Controller", ToolTip = "Controls level-wide UOU debug display options."))
class UNDERONEUMBRELLA_API AUOUDebugController : public AActor
{
	GENERATED_BODY()

public:
	AUOUDebugController();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
	TObjectPtr<USceneComponent> RootSceneComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Controller", meta = (ToolTip = "Enables or disables the entire integrated debug system."))
	bool bEnableDebugTools = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Controller", meta = (ToolTip = "Shows a compact controller status message on the viewport."))
	bool bShowControllerStatus = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Categories", meta = (ToolTip = "Enables player debug displays."))
	bool bEnablePlayerDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Categories", meta = (ToolTip = "Enables NPC debug displays."))
	bool bEnableNPCDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Categories", meta = (ToolTip = "Enables puzzle debug displays."))
	bool bEnablePuzzleDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Categories", meta = (ToolTip = "Enables interaction debug displays."))
	bool bEnableInteractionDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Categories", meta = (ToolTip = "Enables VFX debug displays."))
	bool bEnableVFXDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Categories", meta = (ToolTip = "Enables performance debug displays."))
	bool bEnablePerformanceDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Display", meta = (ClampMin = "0.0", ToolTip = "Default visibility distance for world debug UI."))
	float WorldDebugVisibleDistance = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Display", meta = (ClampMin = "1", ToolTip = "Maximum number of world debug UI items shown at once."))
	int32 MaxVisibleWorldDebugItems = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Display", meta = (ToolTip = "Reserved option for showing details only on the nearest or focused actor."))
	bool bOnlyShowFocusedActor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug|Display", meta = (ToolTip = "Uses the built-in DrawDebugString shadow for world debug text."))
	bool bUseWorldTextShadow = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug|Controllers")
	TObjectPtr<UUOUPlayerDebugControllerComponent> PlayerDebugController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug|Controllers")
	TObjectPtr<UUOUNPCDebugControllerComponent> NPCDebugController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug|Controllers")
	TObjectPtr<UUOUPuzzleDebugControllerComponent> PuzzleDebugController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug|Controllers")
	TObjectPtr<UUOUInteractionDebugControllerComponent> InteractionDebugController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug|Controllers")
	TObjectPtr<UUOUVFXDebugControllerComponent> VFXDebugController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug|Controllers")
	TObjectPtr<UUOUPerformanceDebugControllerComponent> PerformanceDebugController;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Debug|Controllers", meta = (ToolTip = "Runtime list of debug controller components attached to this controller."))
	TArray<TObjectPtr<UUOUDebugControllerComponentBase>> DebugControllerComponents;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Debug")
	void RefreshDebugControllerComponents();

	UFUNCTION(BlueprintPure, Category = "Debug")
	bool IsCategoryEnabled(EUOUDebugCategory Category) const;

	UFUNCTION(BlueprintPure, Category = "Debug")
	UUOUDebugControllerComponentBase* FindDebugControllerComponent(EUOUDebugCategory Category) const;

	const TArray<TObjectPtr<UUOUDebugControllerComponentBase>>& GetDebugControllerComponents() const;
};
