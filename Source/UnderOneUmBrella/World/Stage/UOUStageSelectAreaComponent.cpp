// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Stage/UOUStageSelectAreaComponent.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UUOUStageSelectAreaComponent::UUOUStageSelectAreaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	InitSphereRadius(200.0f);
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetGenerateOverlapEvents(true);

	ShapeColor = FColor::Cyan;
	bDrawOnlyIfSelected = true;
}

void UUOUStageSelectAreaComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddDynamic(this, &UUOUStageSelectAreaComponent::HandleBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &UUOUStageSelectAreaComponent::HandleEndOverlap);

#if ENABLE_DRAW_DEBUG
	SetComponentTickEnabled(bDrawDebugAreaInGame);
#else
	SetComponentTickEnabled(false);
#endif
}

void UUOUStageSelectAreaComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if ENABLE_DRAW_DEBUG
	UWorld* World = GetWorld();
	if (!bDrawDebugAreaInGame || World == nullptr || !World->IsGameWorld())
	{
		return;
	}

	DrawDebugSphere(
		World,
		GetComponentLocation(),
		GetScaledSphereRadius(),
		FMath::Clamp(DebugAreaSegments, 8, 128),
		DebugAreaColor,
		false,
		0.0f,
		0,
		FMath::Max(0.1f, DebugAreaThickness));
#endif
}

void UUOUStageSelectAreaComponent::SetDrawDebugAreaInGame(const bool bEnabled)
{
	bDrawDebugAreaInGame = bEnabled;

#if ENABLE_DRAW_DEBUG
	SetComponentTickEnabled(bDrawDebugAreaInGame);
#else
	SetComponentTickEnabled(false);
#endif
}

void UUOUStageSelectAreaComponent::HandleBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	APawn* PlayerPawn = ResolveEligiblePlayerPawn(OtherActor);
	if (PlayerPawn == nullptr)
	{
		return;
	}

	int32& OverlapCount = PlayerOverlapCounts.FindOrAdd(PlayerPawn);
	++OverlapCount;
	if (OverlapCount == 1)
	{
		OnPlayerEntered.Broadcast(PlayerPawn);
	}
}

void UUOUStageSelectAreaComponent::HandleEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex)
{
	APawn* PlayerPawn = ResolveEligiblePlayerPawn(OtherActor);
	if (PlayerPawn == nullptr)
	{
		return;
	}

	int32* OverlapCount = PlayerOverlapCounts.Find(PlayerPawn);
	if (OverlapCount == nullptr)
	{
		return;
	}

	--(*OverlapCount);
	if (*OverlapCount <= 0)
	{
		PlayerOverlapCounts.Remove(PlayerPawn);
		OnPlayerExited.Broadcast(PlayerPawn);
	}
}

APawn* UUOUStageSelectAreaComponent::ResolveEligiblePlayerPawn(AActor* OtherActor) const
{
	APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (OtherPawn == nullptr)
	{
		return nullptr;
	}

	if (!bOnlyLocalPlayerPawn)
	{
		return OtherPawn;
	}

	const UWorld* World = GetWorld();
	const APawn* LocalPlayerPawn = World != nullptr ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
	return OtherPawn == LocalPlayerPawn ? OtherPawn : nullptr;
}
