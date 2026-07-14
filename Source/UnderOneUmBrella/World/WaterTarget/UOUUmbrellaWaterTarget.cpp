// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUUmbrellaWaterTarget.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/UOUWaterContainerComponent.h"
#include "UObject/ConstructorHelpers.h"

AUOUUmbrellaWaterTarget::AUOUUmbrellaWaterTarget()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootScene);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	VisualMesh->SetGenerateOverlapEvents(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(
		TEXT("/Game/LevelPrototyping/Meshes/SM_Cube.SM_Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMeshFinder.Object);
		VisualMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.5f));
	}

	TargetWaterContainer = CreateDefaultSubobject<UUOUWaterContainerComponent>(TEXT("TargetWaterContainer"));
	TargetWaterContainer->MaxAmount = RequiredWater;
	TargetWaterContainer->InitialAmount = 0.0f;
	TargetWaterContainer->WeightMultiplier = 1.0f;
}

void AUOUUmbrellaWaterTarget::BeginPlay()
{
	Super::BeginPlay();

	RequiredWater = FMath::Max(0.0f, RequiredWater);
	CacheWeightedPrimitive();

	if (TargetWaterContainer != nullptr)
	{
		TargetWaterContainer->MaxAmount = FMath::Max(TargetWaterContainer->MaxAmount, RequiredWater);
	}

	BaseMassKg = WeightedPrimitive != nullptr ? WeightedPrimitive->GetMass() : 0.0f;
	EnsureRuntimeMaterials();
	RefreshActivatedState();
	RefreshMass();
	RefreshVisual();
	BroadcastChanged();
}

void AUOUUmbrellaWaterTarget::ReceiveWater(float WaterAmount)
{
	if (WaterAmount <= 0.0f || bIsActivated || TargetWaterContainer == nullptr)
	{
		return;
	}

	TargetWaterContainer->AddAmount(WaterAmount);
	RefreshActivatedState();
	RefreshMass();
	RefreshVisual();
	BroadcastChanged();
}

bool AUOUUmbrellaWaterTarget::CanAcceptPour_Implementation(const FUOUPourInputContext& Context) const
{
	return Context.Volume > 0.0f;
}

FUOUPourReceiveResult AUOUUmbrellaWaterTarget::TryReceivePour_Implementation(const FUOUPourInputContext& Context)
{
	FUOUPourReceiveResult Result;
	if (!CanAcceptPour_Implementation(Context))
	{
		return Result;
	}

	const float AmountBefore = GetReceivedWater();
	ReceiveWater(Context.Volume);
	Result.bAccepted = true;
	Result.AcceptedVolume = FMath::Max(GetReceivedWater() - AmountBefore, 0.0f);
	Result.ReceiverId = TEXT("UmbrellaWaterTarget");
	Result.ReceiverType = EUOUPourDropReceiverType::UmbrellaWaterTarget;
	Result.ReceiverObject = this;
	Result.ReceiverActor = this;
	return Result;
}

int32 AUOUUmbrellaWaterTarget::GetPourReceivePriority_Implementation() const
{
	// 기존 TryDeliverWater의 수신 타입 검사 순서를 유지합니다.
	return 300;
}

bool AUOUUmbrellaWaterTarget::CanAcceptPourAtLocation_Implementation(const FUOUPourInputContext& Context) const
{
	(void)Context;
	return false;
}

float AUOUUmbrellaWaterTarget::GetReceivedWater() const
{
	return TargetWaterContainer != nullptr ? TargetWaterContainer->CurrentAmount : 0.0f;
}

float AUOUUmbrellaWaterTarget::GetAddedWeight() const
{
	return TargetWaterContainer != nullptr ? TargetWaterContainer->GetWeightContribution() : 0.0f;
}

void AUOUUmbrellaWaterTarget::CacheWeightedPrimitive()
{
	if (WeightedPrimitive == nullptr)
	{
		WeightedPrimitive = VisualMesh;
	}
}

void AUOUUmbrellaWaterTarget::RefreshActivatedState()
{
	bIsActivated = GetReceivedWater() >= RequiredWater && RequiredWater > 0.0f;
}

void AUOUUmbrellaWaterTarget::RefreshMass()
{
	if (!bAddWaterToPhysicsMass || WeightedPrimitive == nullptr)
	{
		return;
	}

	WeightedPrimitive->SetMassOverrideInKg(NAME_None, BaseMassKg + GetAddedWeight(), true);
}

void AUOUUmbrellaWaterTarget::RefreshVisual()
{
	EnsureRuntimeMaterials();
	const FLinearColor TargetColor = bIsActivated ? ActivatedColor : IdleColor;

	for (UMaterialInstanceDynamic* MaterialInstance : RuntimeMaterialInstances)
	{
		if (MaterialInstance == nullptr)
		{
			continue;
		}

		MaterialInstance->SetVectorParameterValue(PrimaryColorParameterName, TargetColor);
		MaterialInstance->SetVectorParameterValue(SecondaryColorParameterName, TargetColor);
	}
}

void AUOUUmbrellaWaterTarget::EnsureRuntimeMaterials()
{
	if (VisualMesh == nullptr || RuntimeMaterialInstances.Num() == VisualMesh->GetNumMaterials())
	{
		return;
	}

	RuntimeMaterialInstances.Reset();
	const int32 MaterialCount = VisualMesh->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		UMaterialInstanceDynamic* DynamicMaterial = VisualMesh->CreateAndSetMaterialInstanceDynamic(MaterialIndex);
		RuntimeMaterialInstances.Add(DynamicMaterial);
	}
}

void AUOUUmbrellaWaterTarget::BroadcastChanged()
{
	OnWaterChanged.Broadcast(GetReceivedWater(), RequiredWater, bIsActivated);
}
