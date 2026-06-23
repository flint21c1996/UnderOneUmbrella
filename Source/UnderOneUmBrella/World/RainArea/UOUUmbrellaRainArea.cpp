// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/RainArea/UOUUmbrellaRainArea.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "Player/UOUUmbrellaComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectIterator.h"
#include "World/Environment/UOUEnvironmentVisualComponent.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"

namespace
{
	constexpr float MaxRainAreaFlowSpeed = 3000.0f;

	// 비 방향 설정에 맞춰 속도 부호를 통일합니다.
	// Niagara 쪽은 최종적으로 보정된 수직 속도만 받도록 해서 에디터 세팅 실수를 줄입니다.
	float NormalizeRainAreaFlowSpeed(float FlowSpeed, EUOURainAreaFlowDirection FlowDirection)
	{
		const float SpeedMagnitude = FMath::Clamp(FMath::Abs(FlowSpeed), 0.0f, MaxRainAreaFlowSpeed);
		return FlowDirection == EUOURainAreaFlowDirection::Upward ? SpeedMagnitude : -SpeedMagnitude;
	}
}

AUOUUmbrellaRainArea::AUOUUmbrellaRainArea()
{
	PrimaryActorTick.bCanEverTick = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	RainVisual = CreateDefaultSubobject<UUOUEnvironmentVisualComponent>(TEXT("RainVisual"));
	RainVisual->SetupAttachment(RootScene);

	PrimaryRainEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PrimaryRainEffect"));
	PrimaryRainEffect->SetupAttachment(RainVisual);
	PrimaryRainEffect->SetAutoActivate(false);

	SecondaryRainEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SecondaryRainEffect"));
	SecondaryRainEffect->SetupAttachment(RainVisual);
	SecondaryRainEffect->SetAutoActivate(false);

	RainVisual->SetEffectComponents(PrimaryRainEffect, SecondaryRainEffect);

	RainVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("RainVolume"));
	RainVolume->SetupAttachment(RootScene);
	RainVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RainVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	RainVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RainVolume->SetGenerateOverlapEvents(true);
	RainVolume->SetBoxExtent(FVector(250.0f, 250.0f, 200.0f));

	PreviewVolumeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewVolumeMesh"));
	PreviewVolumeMesh->SetupAttachment(RootScene);
	PreviewVolumeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewVolumeMesh->SetGenerateOverlapEvents(false);
	PreviewVolumeMesh->SetCastShadow(false);
	PreviewVolumeMesh->SetHiddenInGame(false);
	PreviewVolumeMesh->SetVisibility(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		PreviewVolumeMesh->SetStaticMesh(CubeMeshFinder.Object);
	}

}

void AUOUUmbrellaRainArea::SetWaterBasinRainFillEnabled(bool bEnabled)
{
	bEnableWaterBasinRainFill = bEnabled;
}

bool AUOUUmbrellaRainArea::IsWaterBasinRainFillEnabled() const
{
	return bEnableWaterBasinRainFill;
}

void AUOUUmbrellaRainArea::BeginPlay()
{
	Super::BeginPlay();
	RainFillRate = FMath::Max(0.0f, RainFillRate);
	RainVisualIntensity = FMath::Clamp(RainVisualIntensity, 0.0f, 1.0f);
	RainSpawnRate = FMath::Max(0.0f, RainSpawnRate);
	GroundSplashIntensityMultiplier = FMath::Max(0.0f, GroundSplashIntensityMultiplier);
	RainFallSpeed = NormalizeRainAreaFlowSpeed(RainFallSpeed, FlowDirection);

	// BeginPlay 시점에 Construction에서 정리된 값이 있어도 한 번 더 내부 컴포넌트와 동기화합니다.
	// 블루프린트 인스턴스에서 바뀐 Niagara 참조나 프리뷰 설정을 런타임 상태에 맞추기 위한 단계입니다.
	if (RainVisual != nullptr)
	{
		RainVisual->SetEffectComponents(PrimaryRainEffect, SecondaryRainEffect);
	}
	ApplyPreviewSettings();
	ApplyEnvironmentVisualSettings();
}

void AUOUUmbrellaRainArea::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RainFillRate = FMath::Max(0.0f, RainFillRate);
	RainVisualIntensity = FMath::Clamp(RainVisualIntensity, 0.0f, 1.0f);
	RainSpawnRate = FMath::Max(0.0f, RainSpawnRate);
	GroundSplashIntensityMultiplier = FMath::Max(0.0f, GroundSplashIntensityMultiplier);
	RainFallSpeed = NormalizeRainAreaFlowSpeed(RainFallSpeed, FlowDirection);
	if (RainVisual != nullptr)
	{
		RainVisual->SetEffectComponents(PrimaryRainEffect, SecondaryRainEffect);
	}
	ApplyPreviewSettings();
	ApplyEnvironmentVisualSettings();
}

#if WITH_EDITOR
void AUOUUmbrellaRainArea::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.Property != nullptr
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	bHasExplicitRainEffectSystemSelection |= PropertyName == GET_MEMBER_NAME_CHECKED(AUOUUmbrellaRainArea, RainEffectSystem);
	bHasExplicitGroundSplashEffectSystemSelection |= PropertyName == GET_MEMBER_NAME_CHECKED(AUOUUmbrellaRainArea, GroundSplashEffectSystem);

	Super::PostEditChangeProperty(PropertyChangedEvent);

	RainFillRate = FMath::Max(0.0f, RainFillRate);
	RainVisualIntensity = FMath::Clamp(RainVisualIntensity, 0.0f, 1.0f);
	RainSpawnRate = FMath::Max(0.0f, RainSpawnRate);
	GroundSplashIntensityMultiplier = FMath::Max(0.0f, GroundSplashIntensityMultiplier);
	RainFallSpeed = NormalizeRainAreaFlowSpeed(RainFallSpeed, FlowDirection);
	if (RainVisual != nullptr)
	{
		RainVisual->SetEffectComponents(PrimaryRainEffect, SecondaryRainEffect);
	}
	ApplyPreviewSettings();
	ApplyEnvironmentVisualSettings();
}

void AUOUUmbrellaRainArea::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (RainVisual != nullptr)
	{
		RainVisual->SetEffectComponents(PrimaryRainEffect, SecondaryRainEffect);
	}
	ApplyPreviewSettings();
	ApplyEnvironmentVisualSettings();
}
#endif

void AUOUUmbrellaRainArea::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (RainVolume == nullptr)
	{
		return;
	}

	// 비주얼 상태는 매 프레임 최신 에디터 세팅과 런타임 토글을 반영합니다.
	ApplyEnvironmentVisualState();
	DrawRainVisualDebug();

	TArray<AActor*> OverlappingActors;
	RainVolume->GetOverlappingActors(OverlappingActors);

	bool bHasRainBlocker = false;
	FVector RainBlockerWorldCenter = FVector::ZeroVector;
	FRotator RainBlockerWorldRotation = FRotator::ZeroRotator;
	FVector RainBlockerHalfExtent = FVector::ZeroVector;
	bool bHasVisualRainBlocker = false;
	FVector VisualRainBlockerWorldCenter = FVector::ZeroVector;
	FVector VisualRainBlockerHalfExtent = FVector::ZeroVector;

	// RainVolume 안의 플레이어 우산 상태를 모읍니다.
	// 게임플레이 물 차단은 실제 Blocking 상태만 쓰고, 비주얼 차단은 뒤집힌 우산도 보여주기 위해 따로 계산합니다.
	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (OverlappingActor == nullptr)
		{
			continue;
		}

		if (UUOUUmbrellaComponent* UmbrellaComponent = OverlappingActor->FindComponentByClass<UUOUUmbrellaComponent>())
		{
			if (RainFillRate > 0.0f)
			{
				UmbrellaComponent->ApplyRainExposure(RainFillRate * DeltaSeconds);
			}

			FVector CandidateBlockerWorldCenter = FVector::ZeroVector;
			FRotator CandidateBlockerWorldRotation = FRotator::ZeroRotator;
			FVector CandidateBlockerHalfExtent = FVector::ZeroVector;
			const bool bBlocksGameplayRain = UmbrellaComponent->IsBlockingRain();
			const bool bBlocksRainVisual = bBlocksGameplayRain || UmbrellaComponent->IsUpsideDown();
			if (bBlocksRainVisual
				&& UmbrellaComponent->TryGetRainBlockerVolumeData(CandidateBlockerWorldCenter, CandidateBlockerWorldRotation, CandidateBlockerHalfExtent)
				&& CandidateBlockerHalfExtent.SizeSquared() > VisualRainBlockerHalfExtent.SizeSquared())
			{
				bHasVisualRainBlocker = true;
				VisualRainBlockerWorldCenter = CandidateBlockerWorldCenter;
				VisualRainBlockerHalfExtent = CandidateBlockerHalfExtent;
			}

			if (bBlocksGameplayRain
				&& CandidateBlockerHalfExtent.SizeSquared() > RainBlockerHalfExtent.SizeSquared())
			{
				bHasRainBlocker = true;
				RainBlockerWorldCenter = CandidateBlockerWorldCenter;
				RainBlockerWorldRotation = CandidateBlockerWorldRotation;
				RainBlockerHalfExtent = CandidateBlockerHalfExtent;
			}
		}
	}

	// 물받이 판정은 우산이 막고 있는 영역을 제외한 대상에게만 비 입력을 전달합니다.
	ApplyRainToWaterBasinTargets(
		DeltaSeconds,
		bHasRainBlocker,
		RainBlockerWorldCenter,
		RainBlockerWorldRotation,
		RainBlockerHalfExtent);

	// Niagara 비주얼은 가장 큰 우산 차단 영역 하나를 받아 파티클을 뚫고 지나가지 않게 표현합니다.
	ApplyEnvironmentVisualRainBlocker(
		bHasVisualRainBlocker,
		VisualRainBlockerWorldCenter,
		VisualRainBlockerHalfExtent,
		bHasVisualRainBlocker ? RainVisualIntensity : 0.0f);
}

void AUOUUmbrellaRainArea::ApplyEnvironmentVisualEffectSystems()
{
	// 에디터에서 사용자가 직접 고른 Niagara System은 유지하고,
	// 명시 선택이 없을 때만 현재 NiagaraComponent의 기본 에셋을 초기값으로 가져옵니다.
	if (RainEffectSystem != nullptr)
	{
		bHasExplicitRainEffectSystemSelection = true;
	}
	else if (!bHasExplicitRainEffectSystemSelection && PrimaryRainEffect != nullptr)
	{
		RainEffectSystem = PrimaryRainEffect->GetAsset();
		bHasExplicitRainEffectSystemSelection = RainEffectSystem != nullptr;
	}

	if (GroundSplashEffectSystem != nullptr)
	{
		bHasExplicitGroundSplashEffectSystemSelection = true;
	}
	else if (!bHasExplicitGroundSplashEffectSystemSelection && SecondaryRainEffect != nullptr)
	{
		GroundSplashEffectSystem = SecondaryRainEffect->GetAsset();
		bHasExplicitGroundSplashEffectSystemSelection = GroundSplashEffectSystem != nullptr;
	}

	if (RainVisual != nullptr)
	{
		RainVisual->SetEffectSystems(RainEffectSystem, GroundSplashEffectSystem);
	}
}

void AUOUUmbrellaRainArea::ApplyEnvironmentVisualSettings()
{
	ApplyEnvironmentVisualEffectSystems();
	ApplyEnvironmentVisualGeometry();
	ApplyEnvironmentVisualState();
}

void AUOUUmbrellaRainArea::ApplyEnvironmentVisualGeometry()
{
	if (RainVisual == nullptr || RainVolume == nullptr)
	{
		return;
	}

	const FVector BoxExtent = RainVolume->GetScaledBoxExtent();
	const FVector VolumeCenter = RainVolume->GetComponentLocation();
	const FVector VolumeUp = RainVolume->GetUpVector();
	const bool bFlowUpward = FlowDirection == EUOURainAreaFlowDirection::Upward;
	const FVector RainSpawnPlaneWorldPosition = bFlowUpward
		? VolumeCenter - VolumeUp * BoxExtent.Z
		: VolumeCenter + VolumeUp * BoxExtent.Z;
	const FVector GroundSplashWorldPosition = bFlowUpward
		? VolumeCenter + VolumeUp * (BoxExtent.Z - GroundSplashHeightOffset)
		: VolumeCenter - VolumeUp * (BoxExtent.Z - GroundSplashHeightOffset);

	RainVisual->SetWorldLocationAndRotation(VolumeCenter, RainVolume->GetComponentRotation());

	const FTransform VisualTransform = RainVisual->GetComponentTransform();
	const FVector RainLocalPosition = VisualTransform.InverseTransformPosition(RainSpawnPlaneWorldPosition);
	const FVector GroundSplashLocalPosition = VisualTransform.InverseTransformPosition(GroundSplashWorldPosition);
	const FVector RainKillVolumeLocalCenter = FVector::ZeroVector;
	const FRotator EffectLocalRotation = FRotator::ZeroRotator;
	const FVector2D AreaSize(BoxExtent.X * 2.0f, BoxExtent.Y * 2.0f);
	const FVector KillVolumeSize(BoxExtent.X * 2.0f, BoxExtent.Y * 2.0f, BoxExtent.Z * 2.0f);

	// Niagara는 RainVisual 기준의 로컬 좌표를 받습니다.
	// 그래서 RainVolume 월드 위치를 RainVisual 로컬 공간으로 변환해서 스폰면과 소멸 박스를 맞춥니다.
	RainVisual->ConfigureRainVisual(
		RainLocalPosition,
		GroundSplashLocalPosition,
		EffectLocalRotation,
		AreaSize,
		RainKillVolumeLocalCenter,
		KillVolumeSize);
}

void AUOUUmbrellaRainArea::ApplyEnvironmentVisualState()
{
	if (RainVisual == nullptr)
	{
		return;
	}

	const float PrimaryIntensity = FMath::Clamp(RainVisualIntensity, 0.0f, 1.0f);
	const float SecondaryIntensity = FMath::Clamp(RainVisualIntensity * GroundSplashIntensityMultiplier, 0.0f, 1.0f);

	// 비주얼 파라미터는 EnvironmentVisualComponent를 통해 Niagara에 전달됩니다.
	// 게임플레이 비 노출량인 RainFillRate와 시각적 SpawnRate는 서로 별개 값입니다.
	RainFallSpeed = NormalizeRainAreaFlowSpeed(RainFallSpeed, FlowDirection);
	RainVisual->SetRainSpawnRate(RainSpawnRate);
	RainVisual->SetRainFallSpeed(RainFallSpeed);
	RainVisual->SetVisualIntensities(PrimaryIntensity, SecondaryIntensity);
	RainVisual->SetVisualsEnabled(bEnableRainVisuals);
}

void AUOUUmbrellaRainArea::ApplyEnvironmentVisualRainBlocker(bool bIsBlocking, const FVector& BlockerWorldCenter, const FVector& BlockerHalfExtent, float BlockerIntensity)
{
	if (RainVisual == nullptr)
	{
		return;
	}

	const FTransform VisualTransform = RainVisual->GetComponentTransform();
	const FVector BlockerLocalCenter = bIsBlocking
		? VisualTransform.InverseTransformPosition(BlockerWorldCenter)
		: FVector::ZeroVector;

	// 우산 차단 위치는 RainVisual 로컬 좌표로 넘겨야 Niagara 모듈의 Kill/Mask 계산과 좌표계가 맞습니다.
	RainVisual->SetRainBlockerData(
		bIsBlocking,
		BlockerLocalCenter,
		BlockerHalfExtent,
		BlockerIntensity);
}

void AUOUUmbrellaRainArea::ApplyRainToWaterBasinTargets(float DeltaSeconds, bool bHasRainBlocker, const FVector& RainBlockerWorldCenter, const FRotator& RainBlockerWorldRotation, const FVector& RainBlockerHalfExtent) const
{
	if (!bEnableWaterBasinRainFill)
	{
		return;
	}

	const float RainAmount = FMath::Max(0.0f, RainFillRate) * FMath::Max(0.0f, DeltaSeconds);
	if (RainAmount <= 0.0f || RainVolume == nullptr)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	TSet<UUOUWaterBasinTargetComponent*> ProcessedTargets;
	for (TObjectIterator<UUOUWaterBasinTargetComponent> It; It; ++It)
	{
		UUOUWaterBasinTargetComponent* Target = *It;
		if (!IsValid(Target) || Target->GetWorld() != World || ProcessedTargets.Contains(Target)
			|| !Target->CanReceiveRainFill())
		{
			continue;
		}

		AActor* TargetOwner = Target->GetOwner();
		if (!IsValid(TargetOwner)
			|| !DoesActorBoundsOverlapRainVolume(TargetOwner)
			|| (bHasRainBlocker && IsActorBlockedByRainBlocker(TargetOwner, RainBlockerWorldCenter, RainBlockerWorldRotation, RainBlockerHalfExtent)))
		{
			continue;
		}

		// 연결된 물받이 그룹은 한 번만 처리합니다.
		// 같은 물 저장 장치에 여러 타겟 컴포넌트가 있어도 중복으로 물이 차지 않게 하기 위함입니다.
		TArray<UUOUWaterBasinTargetComponent*> Group;
		Target->GetConnectedGroup(Group);
		for (UUOUWaterBasinTargetComponent* GroupTarget : Group)
		{
			if (IsValid(GroupTarget))
			{
				ProcessedTargets.Add(GroupTarget);
			}
		}

		FUOUWaterBasinInputContext InputContext;
		InputContext.Volume = RainAmount;
		InputContext.Duration = RainAmount;
		InputContext.Source = EUOUWaterBasinInputSource::Rain;
		InputContext.WorldDirection = FlowDirection == EUOURainAreaFlowDirection::Upward
			? RainVolume->GetUpVector()
			: -RainVolume->GetUpVector();
		InputContext.WorldLocation = TargetOwner->GetActorLocation();
		InputContext.InstigatorActor = const_cast<AUOUUmbrellaRainArea*>(this);
		InputContext.bApplyToConnectedGroup = true;
		Target->ReceiveWaterInput(InputContext);
	}
}

bool AUOUUmbrellaRainArea::DoesActorBoundsOverlapRainVolume(const AActor* Actor) const
{
	if (RainVolume == nullptr || !IsValid(Actor))
	{
		return false;
	}

	FVector ActorOrigin = FVector::ZeroVector;
	FVector ActorExtent = FVector::ZeroVector;
	Actor->GetActorBounds(false, ActorOrigin, ActorExtent);
	if (ActorExtent.IsNearlyZero())
	{
		ActorOrigin = Actor->GetActorLocation();
	}

	const FTransform RainVolumeTransform = RainVolume->GetComponentTransform();
	const FVector BoxExtent = RainVolume->GetUnscaledBoxExtent();
	FBox ActorBoundsInRainVolumeLocal(ForceInit);

	// 회전된 RainVolume도 처리할 수 있도록 Actor bounds의 8개 꼭짓점을 RainVolume 로컬 공간으로 변환합니다.
	for (int32 XIndex = 0; XIndex < 2; ++XIndex)
	{
		for (int32 YIndex = 0; YIndex < 2; ++YIndex)
		{
			for (int32 ZIndex = 0; ZIndex < 2; ++ZIndex)
			{
				const FVector CornerWorldLocation = ActorOrigin + FVector(
					XIndex == 0 ? -ActorExtent.X : ActorExtent.X,
					YIndex == 0 ? -ActorExtent.Y : ActorExtent.Y,
					ZIndex == 0 ? -ActorExtent.Z : ActorExtent.Z);

				ActorBoundsInRainVolumeLocal += RainVolumeTransform.InverseTransformPosition(CornerWorldLocation);
			}
		}
	}

	return ActorBoundsInRainVolumeLocal.Min.X <= BoxExtent.X
		&& ActorBoundsInRainVolumeLocal.Max.X >= -BoxExtent.X
		&& ActorBoundsInRainVolumeLocal.Min.Y <= BoxExtent.Y
		&& ActorBoundsInRainVolumeLocal.Max.Y >= -BoxExtent.Y
		&& ActorBoundsInRainVolumeLocal.Min.Z <= BoxExtent.Z
		&& ActorBoundsInRainVolumeLocal.Max.Z >= -BoxExtent.Z;
}

bool AUOUUmbrellaRainArea::IsActorBlockedByRainBlocker(const AActor* Actor, const FVector& BlockerWorldCenter, const FRotator& BlockerWorldRotation, const FVector& BlockerHalfExtent) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	FVector ActorOrigin = FVector::ZeroVector;
	FVector ActorExtent = FVector::ZeroVector;
	Actor->GetActorBounds(false, ActorOrigin, ActorExtent);
	if (ActorExtent.IsNearlyZero())
	{
		ActorOrigin = Actor->GetActorLocation();
	}

	const FVector SafeHalfExtent(
		FMath::Max(0.0f, BlockerHalfExtent.X),
		FMath::Max(0.0f, BlockerHalfExtent.Y),
		FMath::Max(0.0f, BlockerHalfExtent.Z));
	if (SafeHalfExtent.X <= KINDA_SMALL_NUMBER || SafeHalfExtent.Y <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FTransform BlockerTransform(BlockerWorldRotation, BlockerWorldCenter);
	FBox ActorBoundsInBlockerLocal(ForceInit);

	// 우산 차단 박스가 회전되어 있어도 비교할 수 있도록 대상 Actor bounds를 차단 박스 로컬 공간으로 옮깁니다.
	for (int32 XIndex = 0; XIndex < 2; ++XIndex)
	{
		for (int32 YIndex = 0; YIndex < 2; ++YIndex)
		{
			for (int32 ZIndex = 0; ZIndex < 2; ++ZIndex)
			{
				const FVector CornerWorldLocation = ActorOrigin + FVector(
					XIndex == 0 ? -ActorExtent.X : ActorExtent.X,
					YIndex == 0 ? -ActorExtent.Y : ActorExtent.Y,
					ZIndex == 0 ? -ActorExtent.Z : ActorExtent.Z);

				ActorBoundsInBlockerLocal += BlockerTransform.InverseTransformPosition(CornerWorldLocation);
			}
		}
	}

	const bool bOverlapsBlockerArea = ActorBoundsInBlockerLocal.Min.X <= SafeHalfExtent.X
		&& ActorBoundsInBlockerLocal.Max.X >= -SafeHalfExtent.X
		&& ActorBoundsInBlockerLocal.Min.Y <= SafeHalfExtent.Y
		&& ActorBoundsInBlockerLocal.Max.Y >= -SafeHalfExtent.Y;

	return bOverlapsBlockerArea && ActorBoundsInBlockerLocal.Min.Z <= SafeHalfExtent.Z;
}

void AUOUUmbrellaRainArea::DrawRainVisualDebug() const
{
	if (!bDrawRainVisualDebug
		|| !UUOUDebugSubsystem::IsDebugWorldDrawEnabled(this, EUOUDebugCategory::VFX)
		|| RainVolume == nullptr)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const FVector BoxExtent = RainVolume->GetScaledBoxExtent();
	const FVector VolumeCenter = RainVolume->GetComponentLocation();
	const FQuat VolumeRotation = RainVolume->GetComponentQuat();
	const FVector VolumeUp = RainVolume->GetUpVector();
	const bool bFlowUpward = FlowDirection == EUOURainAreaFlowDirection::Upward;
	const FVector RainSpawnPlaneWorldPosition = bFlowUpward
		? VolumeCenter - VolumeUp * BoxExtent.Z
		: VolumeCenter + VolumeUp * BoxExtent.Z;
	const FVector GroundSplashWorldPosition = bFlowUpward
		? VolumeCenter + VolumeUp * (BoxExtent.Z - GroundSplashHeightOffset)
		: VolumeCenter - VolumeUp * (BoxExtent.Z - GroundSplashHeightOffset);
	const FVector VisualAreaHalfExtent(BoxExtent.X, BoxExtent.Y, 2.0f);
	const float Thickness = FMath::Max(0.0f, RainVisualDebugThickness);
	const float LifeTime = 0.0f;
	const FColor VFXDebugColor = UUOUDebugSubsystem::GetDebugCategoryColor(this, EUOUDebugCategory::VFX, FColor::Cyan);

	// VFX 디버그는 RainVolume 전체, 비 스폰면, 바닥 물튐 위치를 한 번에 확인하기 위한 표시입니다.
	DrawDebugBox(
		World,
		VolumeCenter,
		BoxExtent,
		VolumeRotation,
		VFXDebugColor,
		false,
		LifeTime,
		0,
		Thickness);

	DrawDebugBox(
		World,
		RainSpawnPlaneWorldPosition,
		VisualAreaHalfExtent,
		VolumeRotation,
		VFXDebugColor,
		false,
		LifeTime,
		0,
		Thickness);

	DrawDebugBox(
		World,
		GroundSplashWorldPosition,
		VisualAreaHalfExtent,
		VolumeRotation,
		VFXDebugColor,
		false,
		LifeTime,
		0,
		Thickness);

	DrawDebugLine(
		World,
		GroundSplashWorldPosition,
		RainSpawnPlaneWorldPosition,
		VFXDebugColor,
		false,
		LifeTime,
		0,
		Thickness);

	DrawDebugString(
		World,
		RainSpawnPlaneWorldPosition + VolumeUp * 20.0f,
		FString::Printf(
			TEXT("RainAreaSize %.1f x %.1f"),
			BoxExtent.X * 2.0f,
			BoxExtent.Y * 2.0f),
		nullptr,
		VFXDebugColor,
		LifeTime,
		false,
		1.0f);
}

void AUOUUmbrellaRainArea::ApplyPreviewSettings()
{
	if (PreviewVolumeMesh == nullptr || RainVolume == nullptr)
	{
		return;
	}

	PreviewVolumeMesh->SetVisibility(bShowEditorPreview);
	PreviewVolumeMesh->SetHiddenInGame(!bShowPreviewInGame);

	if (PreviewMaterial != nullptr)
	{
		PreviewVolumeMesh->SetMaterial(0, PreviewMaterial);
	}

	const FVector BoxExtent = RainVolume->GetUnscaledBoxExtent();
	const FVector BaseScale(
		BoxExtent.X / 50.0f,
		BoxExtent.Y / 50.0f,
		BoxExtent.Z / 50.0f);

	// 프리뷰 메쉬는 RainVolume의 상대 위치와 회전을 그대로 따라갑니다.
	// 스케일만 자동 맞춤 또는 수동 입력 중 하나로 결정합니다.
	PreviewVolumeMesh->SetRelativeLocation(RainVolume->GetRelativeLocation());
	PreviewVolumeMesh->SetRelativeRotation(RainVolume->GetRelativeRotation());

	if (bAutoFitPreviewScaleToRainVolume)
	{
		PreviewVolumeMesh->SetRelativeScale3D(FVector(
			BaseScale.X * PreviewScaleMultiplier.X,
			BaseScale.Y * PreviewScaleMultiplier.Y,
			BaseScale.Z * PreviewScaleMultiplier.Z));
	}
	else
	{
		PreviewVolumeMesh->SetRelativeScale3D(ManualPreviewRelativeScale);
	}
}
