// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "UObject/UObjectIterator.h"

bool UUOUWaterBasinTargetComponent::bRuntimeDebugOverlayEnabled = false;
bool UUOUWaterBasinTargetComponent::bRuntimeDebugConnectionLinesEnabled = true;
EUOUWaterBasinDebugOverlayScope UUOUWaterBasinTargetComponent::RuntimeDebugOverlayScope = EUOUWaterBasinDebugOverlayScope::SpecificTarget;
TWeakObjectPtr<UUOUWaterBasinTargetComponent> UUOUWaterBasinTargetComponent::RuntimeDebugTarget;

namespace
{
	constexpr float MinWorldUnitsPerTile = 1.0f;
	constexpr float DebugPercentScale = 100.0f;
	constexpr float DebugTextLifeTime = 0.0f;
	constexpr float DebugTextScale = 1.0f;
	constexpr float DebugConnectionLineLifeTime = 0.0f;
	constexpr float DebugConnectionLineThickness = 4.0f;
	constexpr float DebugGroupLabelOffsetZ = 120.0f;
	constexpr float DebugTargetLabelOffsetZ = 80.0f;
	constexpr float DebugMaxWaterBoxLifeTime = 0.0f;
	constexpr float DebugMaxWaterBoxThickness = 3.0f;
	constexpr float MinWaterVisualDepthWorld = 0.1f;

	// 연결 그룹의 공통 수면 높이는 이분 탐색으로 찾습니다.
	// 40회 반복하면 탐색 높이 범위가 2^40번 쪼개지므로,
	// 1,000,000 Unreal Unit 높이의 큰 맵에서도 마지막 오차 폭이 약 0.000001uu 이하가 됩니다.
	// 수면 시각 표현이나 퍼즐 판정에서 체감할 수 없는 수준이라 고정 반복으로 충분합니다.
	constexpr int32 SurfaceSolveBinarySearchIterationCount = 40;
}

UUOUWaterBasinTargetComponent::UUOUWaterBasinTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UUOUWaterBasinTargetComponent::BeginPlay()
{
	Super::BeginPlay();

	NormalizeConnections();
	CurrentWaterVolume = FMath::Clamp(InitialWaterVolume, 0.0f, GetCapacity());
	UpdateCachedWaterState();

	// 모든 Actor의 BeginPlay가 끝난 뒤 첫 Tick에서 그룹 초기 물량을 한 번 맞춥니다.
	bPendingInitialRedistribution = true;
}

void UUOUWaterBasinTargetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bPendingInitialRedistribution)
	{
		bPendingInitialRedistribution = false;
		RedistributeConnectedWater();
	}

	DrawRuntimeDebug();
}


//에디터에서 속성 변경시 호출되는 함수
#if WITH_EDITOR
void UUOUWaterBasinTargetComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{

	const FName ChangedPropertyName = PropertyChangedEvent.Property
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UUOUWaterBasinTargetComponent, ConnectedTargets))
	{
		if (PropertyChangedEvent.ChangeType != EPropertyChangeType::ArrayAdd)
		{
			NormalizeConnections();
		}
	}
	else
	{
		NormalizeConnections();
	}


	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UUOUWaterBasinTargetComponent, WaterVisualComponent)
		|| ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UUOUWaterBasinTargetComponent, WaterVisualComponentName)
		|| ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UUOUWaterBasinTargetComponent, bAutoFindWaterVisualComponent))
	{
		bCapturedWaterVisualTransform = false;
	}

	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UUOUWaterBasinTargetComponent, InitialWaterVolume))
	{
		CurrentWaterVolume = FMath::Clamp(InitialWaterVolume, 0.0f, GetCapacity());
	}
	else if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UUOUWaterBasinTargetComponent, CurrentWaterDepth))
	{
		CurrentWaterVolume = FMath::Clamp(CurrentWaterDepth, 0.0f, GetMaxWaterHeight()) * GetSurfaceArea();
	}
	else if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UUOUWaterBasinTargetComponent, CurrentFillRatio))
	{
		CurrentWaterVolume = FMath::Clamp(CurrentFillRatio, 0.0f, 1.0f) * GetCapacity();
	}
	else if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UUOUWaterBasinTargetComponent, WaterSurfaceWorldZ))
	{
		CurrentWaterVolume = GetVolumeAtSurfaceWorldZ(WaterSurfaceWorldZ);
	}
	else if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UUOUWaterBasinTargetComponent, InitialWaterVolume)
		|| ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UUOUWaterBasinTargetComponent, CurrentWaterDepth)
		|| ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UUOUWaterBasinTargetComponent, CurrentFillRatio)
		|| ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UUOUWaterBasinTargetComponent, WaterSurfaceWorldZ))
	{
		UpdateCachedWaterState();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UUOUWaterBasinTargetComponent::AddWater(float Volume, bool bApplyToConnectedGroup)
{
	if (Volume <= 0.0f)
	{
		return;
	}

	if (bApplyToConnectedGroup)
	{
		// 그룹에 물을 추가할 때는 이 Target 하나가 아니라 그룹 전체의 총 부피를 먼저 구합니다.
		// 이후 ApplyWaterVolumeToConnectedGroup에서 공통 수면 높이를 찾아 각 Target에 다시 분배합니다.
		TArray<UUOUWaterBasinTargetComponent*> Group;
		GetConnectedGroup(Group);
		const FUOUWaterBasinGroupDebugData GroupData = BuildGroupDebugData(Group);
		ApplyWaterVolumeToConnectedGroup(GroupData.TotalVolume + Volume);
		return;
	}

	ApplyWaterVolumeToSingleTarget(CurrentWaterVolume + Volume);
}

void UUOUWaterBasinTargetComponent::RemoveWater(float Volume, bool bApplyToConnectedGroup)
{
	if (Volume <= 0.0f)
	{
		return;
	}

	if (bApplyToConnectedGroup)
	{
		// AddWater와 같은 흐름입니다. 그룹 총 부피에서 제거량을 뺀 뒤 다시 같은 SurfaceWorldZ가 되도록 나눕니다.
		TArray<UUOUWaterBasinTargetComponent*> Group;
		GetConnectedGroup(Group);
		const FUOUWaterBasinGroupDebugData GroupData = BuildGroupDebugData(Group);
		ApplyWaterVolumeToConnectedGroup(GroupData.TotalVolume - Volume);
		return;
	}

	ApplyWaterVolumeToSingleTarget(CurrentWaterVolume - Volume);
}

void UUOUWaterBasinTargetComponent::SetWaterDepth(float Depth, bool bApplyToConnectedGroup)
{
	// Depth는 퍼즐 타일 단위입니다. 실제 월드 높이는 WorldUnitsPerTile을 곱해 SurfaceWorldZ로 변환합니다.
	const float ClampedDepth = FMath::Clamp(Depth, 0.0f, GetMaxWaterHeight());
	const float SurfaceWorldZ = GetBottomWorldZ() + (ClampedDepth * FMath::Max(WorldUnitsPerTile, MinWorldUnitsPerTile));
	SetWaterSurfaceWorldZ(SurfaceWorldZ, bApplyToConnectedGroup);
}

void UUOUWaterBasinTargetComponent::SetWaterSurfaceWorldZ(float SurfaceWorldZ, bool bApplyToConnectedGroup)
{
	if (bApplyToConnectedGroup)
	{
		// 그룹 수위 직접 지정은 목표 부피를 찾는 대신 SurfaceWorldZ를 먼저 고정합니다.
		// 각 Target은 자신의 바닥 높이와 면적에 따라 해당 수면에서의 부피를 계산합니다.
		TArray<UUOUWaterBasinTargetComponent*> Group;
		GetConnectedGroup(Group);
		ApplyGroupSurfaceToTargets(Group, SurfaceWorldZ);

		const FUOUWaterBasinGroupDebugData GroupData = BuildGroupDebugData(Group);
		UpdateGroupRuntimeCache(GroupData);
		BroadcastGroupChanged(Group);
		return;
	}

	ApplyWaterVolumeToSingleTarget(GetVolumeAtSurfaceWorldZ(SurfaceWorldZ));
}

void UUOUWaterBasinTargetComponent::FillWater(bool bApplyToConnectedGroup)
{
	if (bApplyToConnectedGroup)
	{
		TArray<UUOUWaterBasinTargetComponent*> Group;
		GetConnectedGroup(Group);
		const FUOUWaterBasinGroupDebugData GroupData = BuildGroupDebugData(Group);
		ApplyWaterVolumeToConnectedGroup(GroupData.TotalCapacity);
		return;
	}

	ApplyWaterVolumeToSingleTarget(GetCapacity());
}

void UUOUWaterBasinTargetComponent::DrainWater(bool bApplyToConnectedGroup)
{
	if (bApplyToConnectedGroup)
	{
		ApplyWaterVolumeToConnectedGroup(0.0f);
		return;
	}

	ApplyWaterVolumeToSingleTarget(0.0f);
}

void UUOUWaterBasinTargetComponent::RedistributeConnectedWater()
{
	// 연결 설정이 바뀌었거나 초기화 직후일 때 사용합니다.
	// 현재 그룹이 가진 총 부피는 유지하고, 물리적으로 같은 수면 높이가 되도록 다시 배분합니다.
	TArray<UUOUWaterBasinTargetComponent*> Group;
	GetConnectedGroup(Group);
	const FUOUWaterBasinGroupDebugData GroupData = BuildGroupDebugData(Group);
	ApplyWaterVolumeToConnectedGroup(GroupData.TotalVolume);
}

void UUOUWaterBasinTargetComponent::GetConnectedGroup(TArray<UUOUWaterBasinTargetComponent*>& OutGroup) const
{
	OutGroup.Reset();

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TSet<UUOUWaterBasinTargetComponent*> Visited;
	TArray<UUOUWaterBasinTargetComponent*> Queue;
	Queue.Add(const_cast<UUOUWaterBasinTargetComponent*>(this));

	while (Queue.Num() > 0)
	{
		UUOUWaterBasinTargetComponent* Current = Queue[0];
		Queue.RemoveAt(0, 1, EAllowShrinking::No);

		if (!IsValid(Current) || Visited.Contains(Current) || Current->GetWorld() != World)
		{
			continue;
		}

		Visited.Add(Current);
		OutGroup.Add(Current);

		TArray<UUOUWaterBasinTargetComponent*> Neighbors;
		Current->GetConnectedTargetComponents(Neighbors);

		for (UUOUWaterBasinTargetComponent* Neighbor : Neighbors)
		{
			if (IsValid(Neighbor) && !Visited.Contains(Neighbor))
			{
				Queue.Add(Neighbor);
			}
		}

		// 수동 목록을 한쪽에만 넣어도 물 그룹으로는 연결되도록 역방향 참조를 함께 탐색합니다.
		for (TObjectIterator<UUOUWaterBasinTargetComponent> It; It; ++It)
		{
			UUOUWaterBasinTargetComponent* Candidate = *It;
			if (!IsValid(Candidate) || Candidate == Current || Candidate->GetWorld() != World || Visited.Contains(Candidate))
			{
				continue;
			}

			if (Candidate->IsDirectlyConnectedTo(Current))
			{
				Queue.Add(Candidate);
			}
		}
	}
}

float UUOUWaterBasinTargetComponent::GetBottomWorldZ() const
{
	if (BottomHeightMode == EUOUWaterBasinBottomHeightMode::ManualWorldZ)
	{
		return ManualBottomWorldZ;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return ManualBottomWorldZ;
	}

	if (BottomHeightMode == EUOUWaterBasinBottomHeightMode::ActorLocationZ)
	{
		return Owner->GetActorLocation().Z;
	}

	FBox BasinBounds;
	if (TryGetBasinBounds(BasinBounds))
	{
		return BasinBounds.Min.Z;
	}

	if (BottomHeightMode == EUOUWaterBasinBottomHeightMode::ActorBoundsMinZ)
	{
		// WaterVisual이 유일한 Primitive인 Actor는 Basin bounds 계산에서 Visual이 제외되어 실패할 수 있습니다.
		// 이때 실패한 Bounds.Min을 쓰거나 ActorLocation을 그대로 바닥으로 쓰면 중앙 피벗 기준으로 수면이 커집니다.
		const float MaxDepthWorld = GetMaxWaterHeight() * FMath::Max(WorldUnitsPerTile, MinWorldUnitsPerTile);
		return Owner->GetActorLocation().Z - (MaxDepthWorld * 0.5f);
	}


	return Owner->GetActorLocation().Z;
}

float UUOUWaterBasinTargetComponent::GetTopWorldZ() const
{
	return GetBottomWorldZ() + (GetMaxWaterHeight() * FMath::Max(WorldUnitsPerTile, MinWorldUnitsPerTile));
}

float UUOUWaterBasinTargetComponent::GetSurfaceArea() const
{
	const AActor* Owner = GetOwner();
	const FVector ActorScale = Owner ? Owner->GetActorScale3D().GetAbs() : FVector::OneVector;

	if (VolumeSizeMode == EUOUWaterBasinVolumeSizeMode::Manual)
	{
		// Manual 값은 Scale 1 기준값입니다. 배치된 Actor Scale을 곱해 실제 퍼즐 면적으로 바꿉니다.
		return FMath::Max(ManualSurfaceArea * ActorScale.X * ActorScale.Y, KINDA_SMALL_NUMBER);
	}

	if (!Owner)
	{
		return FMath::Max(ManualSurfaceArea, KINDA_SMALL_NUMBER);
	}

	FBox BasinBounds;
	if (TryGetBasinBounds(BasinBounds))
	{
		// ActorBounds는 이미 월드 Scale이 반영된 bounds입니다. 월드 cm를 퍼즐 타일 단위로 변환합니다.
		const float Unit = FMath::Max(WorldUnitsPerTile, MinWorldUnitsPerTile);
		const FVector SizeInTiles = BasinBounds.GetSize() / Unit;
		return FMath::Max(SizeInTiles.X * SizeInTiles.Y, KINDA_SMALL_NUMBER);
	}

	return FMath::Max(ManualSurfaceArea * ActorScale.X * ActorScale.Y, KINDA_SMALL_NUMBER);
}

float UUOUWaterBasinTargetComponent::GetMaxWaterHeight() const
{
	const AActor* Owner = GetOwner();
	const FVector ActorScale = Owner ? Owner->GetActorScale3D().GetAbs() : FVector::OneVector;

	if (VolumeSizeMode == EUOUWaterBasinVolumeSizeMode::Manual)
	{
		// Manual 최대 높이도 Scale 1 기준값입니다. Actor Scale Z를 곱해 최종 타일 높이로 사용합니다.
		return FMath::Max(ManualMaxWaterHeight * ActorScale.Z, KINDA_SMALL_NUMBER);
	}

	if (!Owner)
	{
		return FMath::Max(ManualMaxWaterHeight, KINDA_SMALL_NUMBER);
	}

	FBox BasinBounds;
	if (TryGetBasinBounds(BasinBounds))
	{
		const float Unit = FMath::Max(WorldUnitsPerTile, MinWorldUnitsPerTile);
		return FMath::Max(BasinBounds.GetSize().Z / Unit, KINDA_SMALL_NUMBER);
	}

	return FMath::Max(ManualMaxWaterHeight * ActorScale.Z, KINDA_SMALL_NUMBER);
}

float UUOUWaterBasinTargetComponent::GetCapacity() const
{
	return GetSurfaceArea() * GetMaxWaterHeight();
}

float UUOUWaterBasinTargetComponent::GetWaterDepthWorld() const
{
	return CurrentWaterDepth * FMath::Max(WorldUnitsPerTile, MinWorldUnitsPerTile);
}

FUOUWaterBasinGroupDebugData UUOUWaterBasinTargetComponent::GetConnectedGroupDebugData() const
{
	TArray<UUOUWaterBasinTargetComponent*> Group;
	GetConnectedGroup(Group);
	return BuildGroupDebugData(Group);
}

void UUOUWaterBasinTargetComponent::SetRuntimeDebugOverlay(bool bEnabled, bool bShowConnectionLines, EUOUWaterBasinDebugOverlayScope Scope, UUOUWaterBasinTargetComponent* Target)
{
	bRuntimeDebugOverlayEnabled = bEnabled;
	bRuntimeDebugConnectionLinesEnabled = bShowConnectionLines;
	RuntimeDebugOverlayScope = Scope;
	RuntimeDebugTarget = Target;
}

bool UUOUWaterBasinTargetComponent::IsRuntimeDebugOverlayEnabled()
{
	return bRuntimeDebugOverlayEnabled;
}

void UUOUWaterBasinTargetComponent::NormalizeConnections()
{
	TSet<AActor*> SeenActors;
	const AActor* Owner = GetOwner();

	for (int32 Index = ConnectedTargets.Num() - 1; Index >= 0; --Index)
	{
		AActor* ConnectedActor = ConnectedTargets[Index];
		if (!IsValid(ConnectedActor) || ConnectedActor == Owner || SeenActors.Contains(ConnectedActor))
		{
			ConnectedTargets.RemoveAtSwap(Index, 1, EAllowShrinking::No);
			continue;
		}

		UUOUWaterBasinTargetComponent* ConnectedTarget = ConnectedActor->FindComponentByClass<UUOUWaterBasinTargetComponent>();
		if (!IsValid(ConnectedTarget) || ConnectedTarget == this)
		{
			ConnectedTargets.RemoveAtSwap(Index, 1, EAllowShrinking::No);
			continue;
		}

		SeenActors.Add(ConnectedActor);
	}
}

void UUOUWaterBasinTargetComponent::GetConnectedTargetComponents(TArray<UUOUWaterBasinTargetComponent*>& OutTargets) const
{
	OutTargets.Reset();

	for (AActor* ConnectedActor : ConnectedTargets)
	{
		if (!IsValid(ConnectedActor))
		{
			continue;
		}

		UUOUWaterBasinTargetComponent* ConnectedTarget = ConnectedActor->FindComponentByClass<UUOUWaterBasinTargetComponent>();
		if (IsValid(ConnectedTarget) && ConnectedTarget != this)
		{
			OutTargets.Add(ConnectedTarget);
		}
	}
}

void UUOUWaterBasinTargetComponent::ApplyWaterVolumeToSingleTarget(float NewVolume)
{
	CurrentWaterVolume = FMath::Clamp(NewVolume, 0.0f, GetCapacity());
	UpdateCachedWaterState();

	FUOUWaterBasinGroupDebugData SingleData;
	SingleData.TargetCount = 1;
	SingleData.TotalVolume = CurrentWaterVolume;
	SingleData.TotalCapacity = GetCapacity();
	SingleData.FillRatio = SingleData.TotalCapacity > KINDA_SMALL_NUMBER ? SingleData.TotalVolume / SingleData.TotalCapacity : 0.0f;
	SingleData.SurfaceWorldZ = WaterSurfaceWorldZ;
	SingleData.LowestBottomWorldZ = GetBottomWorldZ();
	SingleData.HighestTopWorldZ = GetTopWorldZ();
	LastGroupTotalVolume = SingleData.TotalVolume;
	LastGroupTotalCapacity = SingleData.TotalCapacity;
	LastGroupSurfaceWorldZ = SingleData.SurfaceWorldZ;

	OnWaterStateChanged.Broadcast(this);
}

void UUOUWaterBasinTargetComponent::ApplyWaterVolumeToConnectedGroup(float NewTotalVolume)
{
	TArray<UUOUWaterBasinTargetComponent*> Group;
	GetConnectedGroup(Group);

	if (Group.Num() == 0)
	{
		return;
	}

	const FUOUWaterBasinGroupDebugData CurrentGroupData = BuildGroupDebugData(Group);
	const float ClampedVolume = FMath::Clamp(NewTotalVolume, 0.0f, CurrentGroupData.TotalCapacity);
	// 그룹 전체가 같은 수면 높이를 공유하도록, 총 부피에 대응하는 SurfaceWorldZ를 먼저 구합니다.
	const float SurfaceWorldZ = SolveSurfaceWorldZForVolume(Group, ClampedVolume);

	ApplyGroupSurfaceToTargets(Group, SurfaceWorldZ);

	const FUOUWaterBasinGroupDebugData NewGroupData = BuildGroupDebugData(Group);
	UpdateGroupRuntimeCache(NewGroupData);
	BroadcastGroupChanged(Group);
}

void UUOUWaterBasinTargetComponent::ApplyGroupSurfaceToTargets(const TArray<UUOUWaterBasinTargetComponent*>& Group, float SurfaceWorldZ)
{
	for (UUOUWaterBasinTargetComponent* Target : Group)
	{
		if (!IsValid(Target))
		{
			continue;
		}

		// 같은 SurfaceWorldZ라도 Target의 바닥 높이/면적/최대 높이에 따라 실제 부피는 달라집니다.
		Target->CurrentWaterVolume = Target->GetVolumeAtSurfaceWorldZ(SurfaceWorldZ);
		Target->UpdateCachedWaterState();
	}
}

void UUOUWaterBasinTargetComponent::UpdateCachedWaterState()
{
	const float Capacity = GetCapacity();
	CurrentWaterVolume = FMath::Clamp(CurrentWaterVolume, 0.0f, Capacity);

	// 핵심 변환식:
	//   Volume = SurfaceArea * Depth
	//   Depth = Volume / SurfaceArea
	//   SurfaceWorldZ = BottomWorldZ + Depth * WorldUnitsPerTile
	const float SurfaceArea = GetSurfaceArea();
	CurrentWaterDepth = SurfaceArea > KINDA_SMALL_NUMBER ? CurrentWaterVolume / SurfaceArea : 0.0f;
	CurrentWaterDepth = FMath::Clamp(CurrentWaterDepth, 0.0f, GetMaxWaterHeight());

	const float MaxHeight = GetMaxWaterHeight();
	CurrentFillRatio = MaxHeight > KINDA_SMALL_NUMBER ? CurrentWaterDepth / MaxHeight : 0.0f;
	WaterSurfaceWorldZ = GetBottomWorldZ() + GetWaterDepthWorld();

	UpdateWaterVisual();
}

void UUOUWaterBasinTargetComponent::UpdateGroupRuntimeCache(const FUOUWaterBasinGroupDebugData& GroupData)
{
	TArray<UUOUWaterBasinTargetComponent*> Group;
	GetConnectedGroup(Group);

	for (UUOUWaterBasinTargetComponent* Target : Group)
	{
		if (!IsValid(Target))
		{
			continue;
		}

		Target->LastGroupTotalVolume = GroupData.TotalVolume;
		Target->LastGroupTotalCapacity = GroupData.TotalCapacity;
		Target->LastGroupSurfaceWorldZ = GroupData.SurfaceWorldZ;
	}
}

void UUOUWaterBasinTargetComponent::BroadcastGroupChanged(const TArray<UUOUWaterBasinTargetComponent*>& Group)
{
	for (UUOUWaterBasinTargetComponent* Target : Group)
	{
		if (IsValid(Target))
		{
			Target->OnWaterStateChanged.Broadcast(Target);
		}
	}
}

void UUOUWaterBasinTargetComponent::UpdateWaterVisual()
{
	if (!bUpdateWaterVisual)
	{
		return;
	}

	ResolveWaterVisualComponent();
	if (!WaterVisualComponent)
	{
		return;
	}

	const float MaxDepthWorld = GetMaxWaterHeight() * FMath::Max(WorldUnitsPerTile, MinWorldUnitsPerTile);
	if (MaxDepthWorld <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float DepthWorld = GetWaterDepthWorld();
	const bool bShouldHide = bHideWaterVisualWhenEmpty && DepthWorld <= KINDA_SMALL_NUMBER;
	WaterVisualComponent->SetHiddenInGame(bShouldHide, true);
	WaterVisualComponent->SetVisibility(!bShouldHide, true);

	// 수위 표현은 WaterVisual의 Z Scale을 계속 사용합니다.
	// 다만 StaticMeshComponent의 Scale 한 축이 정확히 0이면 Physics/Query Body가 비정상 상태가 될 수 있으므로,
	// 물이 비어 있을 때는 Hidden으로 숨기고 Mesh 자체는 아주 작은 최소 두께를 유지합니다.
	const float SafeMinVisualDepthWorld = FMath::Min(MaxDepthWorld, MinWaterVisualDepthWorld);
	const float VisibleDepthWorld = FMath::Clamp(DepthWorld, SafeMinVisualDepthWorld, MaxDepthWorld);
	if (ApplyWaterVisualBounds(VisibleDepthWorld))
	{
		return;
	}

	CaptureWaterVisualTransformIfNeeded();

	FVector NewScale = WaterVisualComponent->GetComponentScale();

	FVector LocalMin = FVector::ZeroVector;
	FVector LocalMax = FVector::ZeroVector;
	FVector LocalCenter = FVector::ZeroVector;
	if (UStaticMeshComponent* WaterVisualMeshComponent = Cast<UStaticMeshComponent>(WaterVisualComponent.Get()))
	{
		WaterVisualMeshComponent->GetLocalBounds(LocalMin, LocalMax);
		LocalCenter = (LocalMin + LocalMax) * 0.5f;

		const FVector LocalSize = LocalMax - LocalMin;
		if (LocalSize.Z > KINDA_SMALL_NUMBER)
		{
			NewScale.Z = VisibleDepthWorld / LocalSize.Z;
		}
		else
		{
			const float HeightRatio = FMath::Clamp(VisibleDepthWorld / MaxDepthWorld, 0.0f, 1.0f);
			NewScale.Z = InitialWaterVisualScale.Z * HeightRatio;
		}
	}
	else
	{
		const float HeightRatio = FMath::Clamp(VisibleDepthWorld / MaxDepthWorld, 0.0f, 1.0f);
		NewScale.Z = InitialWaterVisualScale.Z * HeightRatio;
	}

	const FTransform CurrentWaterVisualTransform = WaterVisualComponent->GetComponentTransform();
	const FVector CurrentVisualCenter = CurrentWaterVisualTransform.TransformPosition(LocalCenter);

	WaterVisualComponent->SetWorldScale3D(NewScale);

	if (bAutoPlaceWaterVisual)
	{
		FVector DesiredCenter = CurrentVisualCenter;

		FBox BasinBounds;
		if (TryGetBasinBounds(BasinBounds))
		{
			const FVector BasinCenter = BasinBounds.GetCenter();
			DesiredCenter.X = BasinCenter.X;
			DesiredCenter.Y = BasinCenter.Y;
		}

		const FQuat WaterVisualRotation = WaterVisualComponent->GetComponentQuat();
		const FVector PivotOffset = WaterVisualRotation.RotateVector(LocalCenter * NewScale);
		DesiredCenter.Z = GetBottomWorldZ() + (VisibleDepthWorld * 0.5f);
		WaterVisualComponent->SetWorldLocation(DesiredCenter - PivotOffset);
	}
}

void UUOUWaterBasinTargetComponent::ResolveWaterVisualComponent()
{
	if (IsValid(WaterVisualComponent) || !bAutoFindWaterVisualComponent)
	{
		return;
	}

	WaterVisualComponent = FindWaterVisualComponent();
	if (WaterVisualComponent)
	{
		bCapturedWaterVisualTransform = false;
	}
}

USceneComponent* UUOUWaterBasinTargetComponent::FindWaterVisualComponent() const
{
	const AActor* Owner = GetOwner();
	if (!Owner || WaterVisualComponentName.IsNone())
	{
		return nullptr;
	}

	const FString TargetName = WaterVisualComponentName.ToString();

	TArray<USceneComponent*> SceneComponents;
	Owner->GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (!IsValid(SceneComponent))
		{
			continue;
		}

		if (SceneComponent->GetFName() == WaterVisualComponentName || SceneComponent->ComponentTags.Contains(WaterVisualComponentName))
		{
			return SceneComponent;
		}

		if (SceneComponent->GetName().Contains(TargetName, ESearchCase::IgnoreCase))
		{
			return SceneComponent;
		}
	}

	return nullptr;
}

bool UUOUWaterBasinTargetComponent::ApplyWaterVisualBounds(float VisibleDepthWorld)
{
	if (!bFitWaterVisualToBasinBounds || !WaterVisualComponent)
	{
		return false;
	}

	FBox BasinBounds;
	if (!TryGetBasinBounds(BasinBounds))
	{
		return false;
	}

	UStaticMeshComponent* WaterVisualMeshComponent = Cast<UStaticMeshComponent>(WaterVisualComponent.Get());
	if (!WaterVisualMeshComponent)
	{
		return false;
	}

	FVector LocalMin = FVector::ZeroVector;
	FVector LocalMax = FVector::ZeroVector;
	WaterVisualMeshComponent->GetLocalBounds(LocalMin, LocalMax);

	const FVector LocalSize = LocalMax - LocalMin;
	if (LocalSize.X <= KINDA_SMALL_NUMBER || LocalSize.Y <= KINDA_SMALL_NUMBER || LocalSize.Z <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector BasinSize = BasinBounds.GetSize();
	const FVector LocalCenter = (LocalMin + LocalMax) * 0.5f;

	// X/Y는 Basin 영역 전체를 덮고, Z는 현재 물 깊이만큼만 차오르게 만듭니다.
	// 따라서 WaterVisual의 최상단 월드 Z는 GetBottomWorldZ() + VisibleDepthWorld가 됩니다.
	const FVector NewScale(
		BasinSize.X / LocalSize.X,
		BasinSize.Y / LocalSize.Y,
		VisibleDepthWorld / LocalSize.Z);

	WaterVisualMeshComponent->SetWorldScale3D(NewScale);

	if (bAutoPlaceWaterVisual)
	{
		FVector DesiredCenter = BasinBounds.GetCenter();
		DesiredCenter.Z = GetBottomWorldZ() + (VisibleDepthWorld * 0.5f);

		const FVector PivotOffset = LocalCenter * NewScale;
		WaterVisualMeshComponent->SetWorldLocation(DesiredCenter - PivotOffset);
	}

	return true;
}

void UUOUWaterBasinTargetComponent::CaptureWaterVisualTransformIfNeeded()
{
	if (!WaterVisualComponent || bCapturedWaterVisualTransform)
	{
		return;
	}

	InitialWaterVisualScale = WaterVisualComponent->GetComponentScale();
	InitialWaterVisualLocation = WaterVisualComponent->GetComponentLocation();
	bCapturedWaterVisualTransform = true;
}

void UUOUWaterBasinTargetComponent::DrawRuntimeDebug()
{
	if (!bRuntimeDebugOverlayEnabled || !ShouldDrawTargetDebug())
	{
		return;
	}

	DrawTargetDebugString();

	if (RuntimeDebugOverlayScope == EUOUWaterBasinDebugOverlayScope::SpecificConnectedGroup)
	{
		TArray<UUOUWaterBasinTargetComponent*> Group;
		GetConnectedGroup(Group);

		for (const UUOUWaterBasinTargetComponent* Target : Group)
		{
			if (IsValid(Target))
			{
				Target->DrawMaxWaterCapacityDebugBox();
			}
		}
	}
	else
	{
		DrawMaxWaterCapacityDebugBox();
	}

	if (!bRuntimeDebugConnectionLinesEnabled)
	{
		return;
	}

	if (RuntimeDebugOverlayScope == EUOUWaterBasinDebugOverlayScope::SpecificConnectedGroup)
	{
		DrawConnectedGroupConnections();
	}
	else
	{
		DrawSpecificTargetConnections();
	}
}

void UUOUWaterBasinTargetComponent::DrawMaxWaterCapacityDebugBox() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector BoxCenter = FVector::ZeroVector;
	FVector BoxExtent = FVector::ZeroVector;
	FQuat BoxRotation = FQuat::Identity;
	if (!BuildMaxWaterCapacityDebugBox(BoxCenter, BoxExtent, BoxRotation))
	{
		return;
	}

	DrawDebugBox(
		World,
		BoxCenter,
		BoxExtent,
		BoxRotation,
		FColor::Blue,
		false,
		DebugMaxWaterBoxLifeTime,
		0,
		DebugMaxWaterBoxThickness);
}

bool UUOUWaterBasinTargetComponent::BuildMaxWaterCapacityDebugBox(FVector& OutCenter, FVector& OutExtent, FQuat& OutRotation) const
{
	const USceneComponent* ResolvedWaterVisualComponent = WaterVisualComponent.Get();
	if (!ResolvedWaterVisualComponent && bAutoFindWaterVisualComponent)
	{
		ResolvedWaterVisualComponent = FindWaterVisualComponent();
	}

	const UStaticMeshComponent* WaterVisualMeshComponent = Cast<UStaticMeshComponent>(ResolvedWaterVisualComponent);
	if (!WaterVisualMeshComponent)
	{
		return false;
	}

	FVector LocalMin = FVector::ZeroVector;
	FVector LocalMax = FVector::ZeroVector;
	WaterVisualMeshComponent->GetLocalBounds(LocalMin, LocalMax);

	const FVector LocalSize = LocalMax - LocalMin;
	if (LocalSize.X <= KINDA_SMALL_NUMBER || LocalSize.Y <= KINDA_SMALL_NUMBER || LocalSize.Z <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FTransform WaterVisualTransform = WaterVisualMeshComponent->GetComponentTransform();
	FVector DebugScale = WaterVisualTransform.GetScale3D();
	DebugScale.Z = 1.0f;

	const FVector LocalCenter = (LocalMin + LocalMax) * 0.5f;
	const FVector LocalExtent = LocalSize * 0.5f;

	// WaterVisual 큐브 메시의 X/Y는 현재 크기를 따르고, Z는 Scale 1 기준의 최대 높이로 표시합니다.
	OutCenter = WaterVisualTransform.GetLocation() + WaterVisualTransform.GetRotation().RotateVector(LocalCenter * DebugScale);
	OutExtent = LocalExtent * DebugScale.GetAbs();
	OutRotation = WaterVisualTransform.GetRotation();
	return true;
}

void UUOUWaterBasinTargetComponent::DrawTargetDebugString() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (RuntimeDebugOverlayScope == EUOUWaterBasinDebugOverlayScope::SpecificConnectedGroup)
	{
		const FUOUWaterBasinGroupDebugData GroupData = GetConnectedGroupDebugData();
		const FString Text = FString::Printf(
			TEXT("Water Group\nTargets: %d\nVolume: %.2f / %.2f\nFill: %.1f%%\nSurface Z: %.1f"),
			GroupData.TargetCount,
			GroupData.TotalVolume,
			GroupData.TotalCapacity,
			GroupData.FillRatio * DebugPercentScale,
			GroupData.SurfaceWorldZ);

		DrawDebugString(World, GetDebugLabelWorld(), Text, nullptr, FColor::Yellow, DebugTextLifeTime, true, DebugTextScale);
		return;
	}

	const FString Text = FString::Printf(
		TEXT("Water Target\nVolume: %.2f / %.2f\nDepth: %.2f\nFill: %.1f%%\nSurface Z: %.1f"),
		CurrentWaterVolume,
		GetCapacity(),
		CurrentWaterDepth,
		CurrentFillRatio * DebugPercentScale,
		WaterSurfaceWorldZ);

	DrawDebugString(World, GetDebugLabelWorld(), Text, nullptr, FColor::Cyan, DebugTextLifeTime, true, DebugTextScale);
}

void UUOUWaterBasinTargetComponent::DrawSpecificTargetConnections() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Start = GetDebugCenterWorld();
	for (TObjectIterator<UUOUWaterBasinTargetComponent> It; It; ++It)
	{
		UUOUWaterBasinTargetComponent* Candidate = *It;
		if (!IsValid(Candidate) || Candidate == this || Candidate->GetWorld() != World)
		{
			continue;
		}

		if (IsDirectlyConnectedTo(Candidate) || Candidate->IsDirectlyConnectedTo(this))
		{
			DrawDebugLine(World, Start, Candidate->GetDebugCenterWorld(), FColor::Cyan, false, DebugConnectionLineLifeTime, 0, DebugConnectionLineThickness);
		}
	}
}

void UUOUWaterBasinTargetComponent::DrawConnectedGroupConnections() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<UUOUWaterBasinTargetComponent*> Group;
	GetConnectedGroup(Group);

	for (int32 A = 0; A < Group.Num(); ++A)
	{
		UUOUWaterBasinTargetComponent* From = Group[A];
		if (!IsValid(From))
		{
			continue;
		}

		for (int32 B = A + 1; B < Group.Num(); ++B)
		{
			UUOUWaterBasinTargetComponent* To = Group[B];
			if (!IsValid(To))
			{
				continue;
			}

			if (From->IsDirectlyConnectedTo(To) || To->IsDirectlyConnectedTo(From))
			{
				DrawDebugLine(World, From->GetDebugCenterWorld(), To->GetDebugCenterWorld(), FColor::Yellow, false, DebugConnectionLineLifeTime, 0, DebugConnectionLineThickness);
			}
		}
	}
}

FVector UUOUWaterBasinTargetComponent::GetDebugCenterWorld() const
{
	FBox BasinBounds;
	if (TryGetBasinBounds(BasinBounds))
	{
		return BasinBounds.GetCenter();
	}

	AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
}

FVector UUOUWaterBasinTargetComponent::GetDebugLabelWorld() const
{
	if (RuntimeDebugOverlayScope == EUOUWaterBasinDebugOverlayScope::SpecificConnectedGroup)
	{
		TArray<UUOUWaterBasinTargetComponent*> Group;
		GetConnectedGroup(Group);

		FBox GroupBounds(ForceInit);
		for (const UUOUWaterBasinTargetComponent* Target : Group)
		{
			if (!IsValid(Target))
			{
				continue;
			}

			FBox BasinBounds;
			if (Target->TryGetBasinBounds(BasinBounds))
			{
				GroupBounds += BasinBounds;
			}
			else if (const AActor* Owner = Target->GetOwner())
			{
				GroupBounds += Owner->GetActorLocation();
			}
		}

		if (GroupBounds.IsValid)
		{
			FVector LabelLocation = GroupBounds.GetCenter();
			LabelLocation.Z = GroupBounds.Max.Z + DebugGroupLabelOffsetZ;
			return LabelLocation;
		}
	}

	FVector LabelLocation = GetDebugCenterWorld();
	LabelLocation.Z = GetTopWorldZ() + DebugTargetLabelOffsetZ;
	return LabelLocation;
}

bool UUOUWaterBasinTargetComponent::ShouldDrawTargetDebug() const
{
	const UUOUWaterBasinTargetComponent* Target = RuntimeDebugTarget.Get();
	return IsValid(Target) && Target == this && Target->GetWorld() == GetWorld();
}

bool UUOUWaterBasinTargetComponent::IsDirectlyConnectedTo(const UUOUWaterBasinTargetComponent* Other) const
{
	if (!Other)
	{
		return false;
	}

	for (const AActor* ConnectedActor : ConnectedTargets)
	{
		if (IsValid(ConnectedActor) && ConnectedActor == Other->GetOwner())
		{
			return true;
		}
	}

	return false;
}

bool UUOUWaterBasinTargetComponent::TryGetBasinBounds(FBox& OutBounds) const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	FBox Bounds(ForceInit);
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Owner->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!IsValid(PrimitiveComponent) || !PrimitiveComponent->IsRegistered())
		{
			continue;
		}

		if (PrimitiveComponent == WaterVisualComponent.Get())
		{
			continue;
		}

		if (!WaterVisualComponentName.IsNone())
		{
			const FString TargetName = WaterVisualComponentName.ToString();
			if (PrimitiveComponent->GetFName() == WaterVisualComponentName
				|| PrimitiveComponent->ComponentTags.Contains(WaterVisualComponentName)
				|| PrimitiveComponent->GetName().Contains(TargetName, ESearchCase::IgnoreCase))
			{
				continue;
			}
		}

		if (WaterVisualComponent && PrimitiveComponent->IsAttachedTo(WaterVisualComponent.Get()))
		{
			continue;
		}

		const FBox ComponentBounds = PrimitiveComponent->Bounds.GetBox();
		if (ComponentBounds.IsValid)
		{
			Bounds += ComponentBounds;
		}
	}

	if (!Bounds.IsValid)
	{
		return false;
	}

	OutBounds = Bounds;
	return true;
}

float UUOUWaterBasinTargetComponent::GetVolumeAtSurfaceWorldZ(float SurfaceWorldZ) const
{
	// SurfaceWorldZ가 바닥보다 낮으면 깊이 0, Top보다 높으면 최대 깊이로 clamp합니다.
	const float Unit = FMath::Max(WorldUnitsPerTile, MinWorldUnitsPerTile);
	const float Depth = FMath::Clamp((SurfaceWorldZ - GetBottomWorldZ()) / Unit, 0.0f, GetMaxWaterHeight());
	return Depth * GetSurfaceArea();
}

float UUOUWaterBasinTargetComponent::SolveSurfaceWorldZForVolume(const TArray<UUOUWaterBasinTargetComponent*>& Group, float TargetVolume) const
{
	if (Group.Num() == 0)
	{
		return GetBottomWorldZ();
	}

	float LowestBottom = TNumericLimits<float>::Max();
	float HighestTop = TNumericLimits<float>::Lowest();
	float TotalCapacity = 0.0f;

	for (const UUOUWaterBasinTargetComponent* Target : Group)
	{
		if (!IsValid(Target))
		{
			continue;
		}

		LowestBottom = FMath::Min(LowestBottom, Target->GetBottomWorldZ());
		HighestTop = FMath::Max(HighestTop, Target->GetTopWorldZ());
		TotalCapacity += Target->GetCapacity();
	}

	if (TotalCapacity <= KINDA_SMALL_NUMBER || LowestBottom == TNumericLimits<float>::Max())
	{
		return GetBottomWorldZ();
	}

	const float ClampedTargetVolume = FMath::Clamp(TargetVolume, 0.0f, TotalCapacity);
	if (ClampedTargetVolume <= KINDA_SMALL_NUMBER)
	{
		return LowestBottom;
	}

	if (ClampedTargetVolume >= TotalCapacity - KINDA_SMALL_NUMBER)
	{
		return HighestTop;
	}

	float Low = LowestBottom;
	float High = HighestTop;
	for (int32 Iteration = 0; Iteration < SurfaceSolveBinarySearchIterationCount; ++Iteration)
	{
		// SurfaceWorldZ가 높아질수록 그룹 총 부피가 단조 증가하므로 이분 탐색으로 목표 부피의 수면을 찾습니다.
		// Mid에서의 부피가 목표보다 작으면 더 높은 수면이 필요하므로 Low를 올리고,
		// 목표보다 크거나 같으면 더 낮은 수면도 가능한지 확인하기 위해 High를 내립니다.
		const float Mid = (Low + High) * 0.5f;
		float VolumeAtMid = 0.0f;

		for (const UUOUWaterBasinTargetComponent* Target : Group)
		{
			if (IsValid(Target))
			{
				VolumeAtMid += Target->GetVolumeAtSurfaceWorldZ(Mid);
			}
		}

		if (VolumeAtMid < ClampedTargetVolume)
		{
			Low = Mid;
		}
		else
		{
			High = Mid;
		}
	}

	return (Low + High) * 0.5f;
}

FUOUWaterBasinGroupDebugData UUOUWaterBasinTargetComponent::BuildGroupDebugData(const TArray<UUOUWaterBasinTargetComponent*>& Group) const
{
	FUOUWaterBasinGroupDebugData GroupData;
	GroupData.LowestBottomWorldZ = TNumericLimits<float>::Max();
	GroupData.HighestTopWorldZ = TNumericLimits<float>::Lowest();

	for (const UUOUWaterBasinTargetComponent* Target : Group)
	{
		if (!IsValid(Target))
		{
			continue;
		}

		++GroupData.TargetCount;
		GroupData.TotalVolume += Target->CurrentWaterVolume;
		GroupData.TotalCapacity += Target->GetCapacity();
		GroupData.LowestBottomWorldZ = FMath::Min(GroupData.LowestBottomWorldZ, Target->GetBottomWorldZ());
		GroupData.HighestTopWorldZ = FMath::Max(GroupData.HighestTopWorldZ, Target->GetTopWorldZ());
	}

	if (GroupData.TargetCount == 0)
	{
		GroupData.LowestBottomWorldZ = GetBottomWorldZ();
		GroupData.HighestTopWorldZ = GetTopWorldZ();
		GroupData.SurfaceWorldZ = GetBottomWorldZ();
		return GroupData;
	}

	GroupData.FillRatio = GroupData.TotalCapacity > KINDA_SMALL_NUMBER ? GroupData.TotalVolume / GroupData.TotalCapacity : 0.0f;
	GroupData.SurfaceWorldZ = SolveSurfaceWorldZForVolume(Group, GroupData.TotalVolume);
	return GroupData;
}
