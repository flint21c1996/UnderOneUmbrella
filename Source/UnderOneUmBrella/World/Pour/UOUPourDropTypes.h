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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop", meta = (ToolTip = "켜면 생성되는 PourDropActor가 액터 기본값 대신 이 프로필의 Drop 설정을 사용합니다."))
	bool bOverrideDropActorSettings = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Visual", meta = (ToolTip = "디버그용 DropActor mesh에 적용할 메쉬입니다. 실제 붓기 시각 표현은 Stream Visual이 담당하므로 보통 비워둡니다."))
	TObjectPtr<UStaticMesh> VisualMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Visual", meta = (ToolTip = "디버그용 DropActor mesh에 적용할 머티리얼 목록입니다. 비어 있는 슬롯은 무시됩니다."))
	TArray<TObjectPtr<UMaterialInterface>> VisualMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Visual", meta = (EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	FVector VisualMeshRelativeScale = FVector(0.08f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Visual", meta = (EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	FVector VisualMeshRelativeOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Visual", meta = (EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	FRotator VisualMeshRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Debug", meta = (EditCondition = "bOverrideDropActorSettings", EditConditionHides, ToolTip = "켜면 DropActor의 mesh를 디버그용으로 표시합니다. 꺼져 있어도 충돌/물 전달 판정은 그대로 동작합니다."))
	bool bShowDebugVisualMesh = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Trail", meta = (ToolTip = "DropActor에 붙일 보조 trail Niagara입니다. 기본 붓기 표현은 Stream Visual이 담당하므로 필요할 때만 사용합니다."))
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Impact", meta = (ToolTip = "DropActor가 유효한 대상에 닿았을 때 재생할 splash Niagara입니다. 비워두면 액터 기본값을 유지합니다."))
	TObjectPtr<UNiagaraSystem> ImpactSplashEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Drop|Impact", meta = (ClampMin = "0.0", EditCondition = "bOverrideDropActorSettings", EditConditionHides))
	float ImpactSplashScale = 1.0f;
};
