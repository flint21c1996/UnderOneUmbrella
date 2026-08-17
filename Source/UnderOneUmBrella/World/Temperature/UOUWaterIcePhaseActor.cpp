// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Temperature/UOUWaterIcePhaseActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "World/Light/UOULightExposureReceiverComponent.h"

AUOUWaterIcePhaseActor::AUOUWaterIcePhaseActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	WaterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterMesh"));
	WaterMesh->SetupAttachment(RootScene);
	WaterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	IceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IceMesh"));
	IceMesh->SetupAttachment(RootScene);
	IceMesh->SetVisibility(false, true);
	IceMesh->SetHiddenInGame(true, true);
	IceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WaterBlockingVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("WaterBlockingVolume"));
	WaterBlockingVolume->SetupAttachment(RootScene);
	WaterBlockingVolume->InitBoxExtent(FVector(100.0f, 100.0f, 200.0f));
	WaterBlockingVolume->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	WaterBlockingVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WaterBlockingVolume->SetCollisionObjectType(ECC_WorldDynamic);
	WaterBlockingVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	WaterBlockingVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	WaterBlockingVolume->SetGenerateOverlapEvents(false);

	LightReceiverVolume = CreateDefaultSubobject<USphereComponent>(TEXT("LightReceiverVolume"));
	LightReceiverVolume->SetupAttachment(RootScene);
	LightReceiverVolume->InitSphereRadius(100.0f);
	LightReceiverVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LightReceiverVolume->SetCollisionObjectType(ECC_WorldDynamic);
	LightReceiverVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
	LightReceiverVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	LightReceiverVolume->SetGenerateOverlapEvents(false);

	TemperatureReceiver = CreateDefaultSubobject<UUOULightExposureReceiverComponent>(TEXT("TemperatureReceiver"));
	TemperatureReceiver->bAutoFindReceiverTransform = false;
	TemperatureReceiver->ReceiverTransformReference.ComponentProperty =
		GET_MEMBER_NAME_CHECKED(AUOUWaterIcePhaseActor, LightReceiverVolume);
}

void AUOUWaterIcePhaseActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ValidateSettings();
	CurrentPhase = InitialPhase;
	ApplyPhaseState(false, CurrentPhase);
}

void AUOUWaterIcePhaseActor::BeginPlay()
{
	Super::BeginPlay();

	ValidateSettings();
	if (TemperatureReceiver == nullptr)
	{
		SetPhase(InitialPhase);
		return;
	}

	TemperatureReceiver->OnTemperatureChanged.RemoveDynamic(
		this,
		&AUOUWaterIcePhaseActor::HandleTemperatureChanged);
	TemperatureReceiver->OnTemperatureChanged.AddDynamic(
		this,
		&AUOUWaterIcePhaseActor::HandleTemperatureChanged);

	LastEvaluatedTemperature = TemperatureReceiver->CurrentTemperature;
	CurrentPhase = ResolveInitialPhase(LastEvaluatedTemperature);
	ApplyPhaseState(false, CurrentPhase);
}

void AUOUWaterIcePhaseActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TemperatureReceiver != nullptr)
	{
		TemperatureReceiver->OnTemperatureChanged.RemoveDynamic(
			this,
			&AUOUWaterIcePhaseActor::HandleTemperatureChanged);
	}

	Super::EndPlay(EndPlayReason);
}

void AUOUWaterIcePhaseActor::RefreshPhaseFromTemperature()
{
	if (TemperatureReceiver == nullptr)
	{
		return;
	}

	HandleTemperatureChanged(TemperatureReceiver->CurrentTemperature, LastEvaluatedTemperature);
}

void AUOUWaterIcePhaseActor::SetPhase(EUOUWaterIcePhase NewPhase)
{
	const EUOUWaterIcePhase PreviousPhase = CurrentPhase;
	CurrentPhase = NewPhase;
	ApplyPhaseState(CurrentPhase != PreviousPhase, PreviousPhase);
}

void AUOUWaterIcePhaseActor::HandleTemperatureChanged(float NewTemperature, float PreviousTemperature)
{
	LastEvaluatedTemperature = NewTemperature;

	if (CurrentPhase == EUOUWaterIcePhase::Water && NewTemperature <= FreezeTemperature)
	{
		SetPhase(EUOUWaterIcePhase::Ice);
		return;
	}

	if (CurrentPhase == EUOUWaterIcePhase::Ice && NewTemperature >= MeltTemperature)
	{
		SetPhase(EUOUWaterIcePhase::Water);
	}
}

void AUOUWaterIcePhaseActor::ValidateSettings()
{
	if (FreezeTemperature > MeltTemperature)
	{
		Swap(FreezeTemperature, MeltTemperature);
	}
}

void AUOUWaterIcePhaseActor::ApplyPhaseState(
	bool bBroadcastChange,
	EUOUWaterIcePhase PreviousPhase)
{
	const bool bWater = CurrentPhase == EUOUWaterIcePhase::Water;
	if (WaterMesh != nullptr)
	{
		WaterMesh->SetVisibility(bWater, true);
		WaterMesh->SetHiddenInGame(!bWater, true);
		WaterMesh->SetCollisionEnabled(
			bWater ? WaterCollisionEnabled.GetValue() : ECollisionEnabled::NoCollision);
	}

	if (IceMesh != nullptr)
	{
		IceMesh->SetVisibility(!bWater, true);
		IceMesh->SetHiddenInGame(bWater, true);
		IceMesh->SetCollisionEnabled(
			bWater ? ECollisionEnabled::NoCollision : IceCollisionEnabled.GetValue());
	}

	if (WaterBlockingVolume != nullptr)
	{
		WaterBlockingVolume->SetCollisionEnabled(
			bWater ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}

	if (bBroadcastChange)
	{
		OnPhaseChanged.Broadcast(CurrentPhase, PreviousPhase);
	}
}

EUOUWaterIcePhase AUOUWaterIcePhaseActor::ResolveInitialPhase(float Temperature) const
{
	if (Temperature <= FreezeTemperature)
	{
		return EUOUWaterIcePhase::Ice;
	}

	if (Temperature >= MeltTemperature)
	{
		return EUOUWaterIcePhase::Water;
	}

	return InitialPhase;
}
