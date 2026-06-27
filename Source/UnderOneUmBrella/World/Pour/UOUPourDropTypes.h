// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraSystem.h"
#include "UOUPourDropTypes.generated.h"

UENUM(BlueprintType)
enum class EUOUPourDropReceiverType : uint8
{
	None,
	PurePourReceiver,
	UmbrellaWaterTarget,
	WaterBasinTarget,
	WaterContainer
};

USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUPourDropVisualSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop", meta = (ToolTip = "When enabled, spawned PourDropActor instances use these profile settings instead of the actor class defaults."))
	bool bOverrideDropActorSettings = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Visual", meta = (ToolTip = "Optional mesh applied to the spawned drop visual. Leave empty to keep the actor default."))
	TObjectPtr<UStaticMesh> VisualMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Visual", meta = (ToolTip = "Optional materials applied to the spawned drop visual. Empty entries are ignored."))
	TArray<TObjectPtr<UMaterialInterface>> VisualMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Visual", meta = (EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	FVector VisualMeshRelativeScale = FVector(0.08f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Visual", meta = (EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	FVector VisualMeshRelativeOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Visual", meta = (EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	FRotator VisualMeshRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Debug", meta = (EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	bool bShowDebugVisualMesh = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Trail", meta = (ToolTip = "Optional trail Niagara applied to the spawned drop. Leave empty to keep the actor default."))
	TObjectPtr<UNiagaraSystem> TrailEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Trail", meta = (EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	bool bActivateTrailEffect = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Collision", meta = (ClampMin = "0.0", EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	float CollisionRadius = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Collision", meta = (ClampMin = "0.0", EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	float WaterBasinDeliveryVerticalTolerance = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Movement", meta = (ClampMin = "0.0", EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	float InitialSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Movement", meta = (ClampMin = "0.0", EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	float MaxSpeed = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Movement", meta = (EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	float GravityScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Movement", meta = (EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	bool bUseVerticalDescent = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Lifetime", meta = (ClampMin = "0.0", EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	float DropLifeSpan = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Impact", meta = (EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	bool bDestroyOnFirstValidReceiver = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Impact", meta = (EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	bool bDestroyOnBlockingHitWithoutReceiver = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Impact", meta = (EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	bool bSpawnSplashOnlyWhenDelivered = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Impact", meta = (ToolTip = "Optional impact splash Niagara applied to the spawned drop. Leave empty to keep the actor default."))
	TObjectPtr<UNiagaraSystem> ImpactSplashEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Impact", meta = (ClampMin = "0.0", EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	float ImpactSplashScale = 1.0f;
};
