// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOUStageLightBeamVisualComponent.h"

#include "Components/DecalComponent.h"
#include "Components/LightComponent.h"
#include "Components/MeshComponent.h"
#include "NiagaraComponent.h"
#include "Player/UOUUmbrellaLightShadeVolumeComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "World/Light/UOULightExposureSourceComponent.h"
#include "World/Light/UOULightSourceActor.h"
#include "World/Light/UOULightBeamVisualComponent.h"
#include "World/Light/UOULightBeamVisualInterface.h"
#include "World/Light/UOULumenStaticRayVisualActor.h"
#include "World/Light/UOULumenZClippedStaticRayVisualActor.h"

namespace
{
	// 기존 빛 경로의 활성 우산 충돌만 구분합니다.
	bool IsStageUmbrellaHit(const FUOULightPathSegmentData& Segment)
	{
		const UPrimitiveComponent* Hit = Segment.HitComponent.Get();
		if (!IsValid(Hit))
		{
			return false;
		}
		const UUOUUmbrellaLightShadeVolumeComponent* Shade = Cast<UUOUUmbrellaLightShadeVolumeComponent>(Hit);
		if (Shade == nullptr && Hit->GetOwner() != nullptr)
		{
			Shade = Hit->GetOwner()->FindComponentByClass<UUOUUmbrellaLightShadeVolumeComponent>();
		}
		return Shade != nullptr && Shade->CanShadeLight();
	}
	// 기존 표현 데이터의 밝기 정규화 기준을 유지합니다.
	constexpr float StageVisualBrightnessNormalizationScale = 0.07f;

	float CalculateStageReferenceVisualLength(
		const TArray<FUOULightPathData>& LightPaths,
		const USpotLightComponent* SourceSpotLight)
	{
		float ReferenceLength = SourceSpotLight != nullptr
			? FMath::Max(0.0f, SourceSpotLight->AttenuationRadius)
			: 0.0f;
		for (const FUOULightPathData& PathData : LightPaths)
		{
			for (const FUOULightPathSegmentData& SegmentData : PathData.Segments)
			{
				ReferenceLength = FMath::Max(ReferenceLength, SegmentData.Length);
			}
		}
		return ReferenceLength;
	}
}

UUOUStageLightBeamVisualComponent::UUOUStageLightBeamVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	static ConstructorHelpers::FClassFinder<AUOULumenZClippedStaticRayVisualActor> SpotRayClass(
		TEXT("/Game/UOU/Effects/StylizedLightFX/Blueprints/BP_SpotRay"));
	if (SpotRayClass.Succeeded())
	{
		VFXActorClass = SpotRayClass.Class;
	}

	// 기존 BP의 표현 기본값만 복사하며 저장된 Stage BP 설정은 덮어쓰지 않습니다.
	static ConstructorHelpers::FClassFinder<AUOULightSourceActor> SourcePreset(
		TEXT("/Game/UOU/BluePrint/World/Lights/BP_LS01_DynamicRay_Spot"));
	if (SourcePreset.Succeeded())
	{
		const AUOULightSourceActor* Defaults = SourcePreset.Class->GetDefaultObject<AUOULightSourceActor>();
		if (const UUOULightBeamVisualComponent* Visual = Defaults->BeamVisual)
		{
			VisualBrightnessMultiplier = Visual->VisualBrightnessMultiplier;
			VisualOpacityMultiplier = Visual->VisualOpacityMultiplier;
			LumenDynamicRayPreset = Visual->LumenDynamicRayPreset;
			LumenStaticRayPreset = Visual->LumenStaticRayPreset;
			EndPadding = Visual->EndPadding;
		}
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> EndRangeDecalMaterialFinder(
		TEXT("/Game/UOU/Effects/StylizedLightFX/Materials/M_UOU_LightRangeDecal.M_UOU_LightRangeDecal"));
	if (EndRangeDecalMaterialFinder.Succeeded())
	{
		EndRangeDecalMaterial = EndRangeDecalMaterialFinder.Object;
	}
}

void UUOUStageLightBeamVisualComponent::BeginPlay()
{
	Super::BeginPlay();

	BoundSourceComponent = ResolveSourceComponent();
	BoundSourceSpotLight = ResolveSourceSpotLight();

	if (BoundSourceComponent != nullptr)
	{
		BoundSourceComponent->OnLightPathsUpdated.AddDynamic(
			this,
			&UUOUStageLightBeamVisualComponent::HandleLightPathsUpdated);
	}

	RefreshVisuals();
}

void UUOUStageLightBeamVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundSourceComponent != nullptr)
	{
		BoundSourceComponent->OnLightPathsUpdated.RemoveDynamic(
			this,
			&UUOUStageLightBeamVisualComponent::HandleLightPathsUpdated);
	}

	SetComponentTickEnabled(false);
	DestroyVFXActors();
	DestroyEndRangeDecals();
	BoundSourceSpotLight = nullptr;
	BoundSourceComponent = nullptr;

	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void UUOUStageLightBeamVisualComponent::PostEditChangeProperty(
	FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (IsValid(DirectVFXActor))
	{
		ApplyVFXAssetOverrides(DirectVFXActor);
		RefreshVisuals();
	}
}
#endif

void UUOUStageLightBeamVisualComponent::RefreshVisuals()
{
	if (BoundSourceComponent == nullptr)
	{
		BoundSourceComponent = ResolveSourceComponent();
	}
	if (BoundSourceSpotLight == nullptr)
	{
		BoundSourceSpotLight = ResolveSourceSpotLight();
	}

	const TArray<FUOULightPathData> LightPaths = BoundSourceComponent != nullptr
		? BoundSourceComponent->GetLightPaths()
		: TArray<FUOULightPathData>();
	UpdateUmbrellaFadeTarget(LightPaths);
	const float ReferenceVisualLength = CalculateStageReferenceVisualLength(
		LightPaths,
		BoundSourceSpotLight);
	UpdateDirectVFX(LightPaths, ReferenceVisualLength);
}

void UUOUStageLightBeamVisualComponent::HandleLightPathsUpdated(
	const TArray<FUOULightPathData>& LightPaths)
{
	UpdateUmbrellaFadeTarget(LightPaths);
	const float ReferenceVisualLength = CalculateStageReferenceVisualLength(
		LightPaths,
		BoundSourceSpotLight);
	UpdateDirectVFX(LightPaths, ReferenceVisualLength);
}

UUOULightExposureSourceComponent* UUOUStageLightBeamVisualComponent::ResolveSourceComponent() const
{
	const AActor* Owner = GetOwner();
	return Owner != nullptr
		? Owner->FindComponentByClass<UUOULightExposureSourceComponent>()
		: nullptr;
}

USpotLightComponent* UUOUStageLightBeamVisualComponent::ResolveSourceSpotLight() const
{
	const AActor* Owner = GetOwner();
	return Owner != nullptr
		? Owner->FindComponentByClass<USpotLightComponent>()
		: nullptr;
}

AActor* UUOUStageLightBeamVisualComponent::AcquireDirectVFXActor()
{
	if (!IsValid(DirectVFXActor))
	{
		DirectVFXActor = SpawnVFXActor();
	}
	return DirectVFXActor;
}

AActor* UUOUStageLightBeamVisualComponent::SpawnVFXActor()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (Owner == nullptr || World == nullptr || VFXActorClass == nullptr)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Owner;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* VFXActor = World->SpawnActor<AActor>(VFXActorClass, FTransform::Identity, SpawnParameters);
	if (VFXActor != nullptr)
	{
		VFXActor->AttachToActor(Owner, FAttachmentTransformRules::KeepWorldTransform);
		ConfigureSpawnedVFXActor(VFXActor);
	}
	return VFXActor;
}

void UUOUStageLightBeamVisualComponent::ConfigureSpawnedVFXActor(AActor* VFXActor) const
{
	if (VFXActor == nullptr)
	{
		return;
	}

	// 표현 액터가 게임플레이 광선과 캐릭터 충돌에 참여하지 않도록 합니다.
	VFXActor->SetActorEnableCollision(false);
	ConfigureVFXMesh(VFXActor);
	ApplyVFXAssetOverrides(VFXActor);

	if (!bDisableEmbeddedVFXLights)
	{
		return;
	}

	TInlineComponentArray<ULightComponent*> EmbeddedLights(VFXActor);
	for (ULightComponent* EmbeddedLight : EmbeddedLights)
	{
		if (EmbeddedLight != nullptr)
		{
			EmbeddedLight->SetVisibility(false);
		}
	}
}

void UUOUStageLightBeamVisualComponent::ApplyVFXAssetOverrides(AActor* VFXActor) const
{
	if (AUOULumenStaticRayVisualActor* StaticRayVisual = Cast<AUOULumenStaticRayVisualActor>(VFXActor))
	{
		StaticRayVisual->SetVisualAssetOverrides(
			bOverrideBeamMesh,
			BeamMeshOverride,
			bOverrideBeamMaterial,
			BeamMaterialOverride);
	}
	if (AUOULumenZClippedStaticRayVisualActor* ZClippedVisual =
		Cast<AUOULumenZClippedStaticRayVisualActor>(VFXActor))
	{
		ZClippedVisual->SetSparkleSystemOverride(
			bOverrideNiagaraSystem,
			NiagaraSystemOverride);
	}
}

void UUOUStageLightBeamVisualComponent::UpdateDirectVFX(
	const TArray<FUOULightPathData>& LightPaths,
	const float ReferenceVisualLength)
{
	if (!bEnableDirectVFX || VFXActorClass == nullptr || BoundSourceComponent == nullptr)
	{
		SetVFXActive(DirectVFXActor, false);
		if (DirectEndRangeDecal != nullptr)
		{
			DirectEndRangeDecal->SetVisibility(false);
		}
		return;
	}

	const FUOULightPathSegmentData* DirectSegment = nullptr;
	for (const FUOULightPathData& PathData : LightPaths)
	{
		DirectSegment = PathData.Segments.FindByPredicate(
			[](const FUOULightPathSegmentData& SegmentData)
			{
				return SegmentData.Length > KINDA_SMALL_NUMBER &&
					!SegmentData.Direction.IsNearlyZero();
			});
		if (DirectSegment != nullptr)
		{
			break;
		}
	}

	if (DirectSegment == nullptr)
	{
		SetVFXActive(DirectVFXActor, false);
		if (DirectEndRangeDecal != nullptr)
		{
			DirectEndRangeDecal->SetVisibility(false);
		}
		return;
	}

	AActor* VFXActor = AcquireDirectVFXActor();
	if (VFXActor == nullptr)
	{
		return;
	}

	ApplyVFXAssetOverrides(VFXActor);
	ApplySegmentToVFX(
		VFXActor,
		BuildVisualSegment(
			*DirectSegment,
			0,
			ReferenceVisualLength));
	UpdateEndRangeDecal(
		DirectEndRangeDecal,
		DirectEndRangeDecalMaterial,
		*DirectSegment,
		ResolveLightColor(),
		0);
}

void UUOUStageLightBeamVisualComponent::UpdateEndRangeDecal(
	TObjectPtr<UDecalComponent>& DecalComponent,
	TObjectPtr<UMaterialInstanceDynamic>& DynamicMaterial,
	const FUOULightPathSegmentData& SegmentData,
	const FLinearColor& LightColor,
	const int32 SortOrder)
{
	if (!bEnableEndRangeDecal)
	{
		if (DecalComponent != nullptr)
		{
			DecalComponent->SetVisibility(false);
		}
		return;
	}

	FVector DecalLocation = SegmentData.End;
	FVector SurfaceNormal = SegmentData.EndSurfaceNormal.GetSafeNormal();
	bool bHasProjectionSurface = !SurfaceNormal.IsNearlyZero() &&
		SegmentData.HitType != EUOULightPathHitType::None;

	if (!bHasProjectionSurface &&
		bProjectRangeEndDecalToGround &&
		SegmentData.EndReason == EUOULightReflectionPathEndReason::RangeEnded)
	{
		if (UWorld* World = GetWorld())
		{
			const float TraceHeight = FMath::Max(0.0f, RangeEndGroundTraceHeight);
			const float TraceDistance = FMath::Max(1.0f, RangeEndGroundTraceDistance);
			const FVector TraceStart = SegmentData.End + FVector::UpVector * TraceHeight;
			const FVector TraceEnd = SegmentData.End - FVector::UpVector * TraceDistance;
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UOULightRangeEndGround), false, GetOwner());
			if (GetOwner() != nullptr)
			{
				QueryParams.AddIgnoredActor(GetOwner());
			}

			FHitResult GroundHit;
			if (World->LineTraceSingleByChannel(
				GroundHit,
				TraceStart,
				TraceEnd,
				ECC_Visibility,
				QueryParams))
			{
				DecalLocation = GroundHit.ImpactPoint;
				SurfaceNormal = GroundHit.ImpactNormal.GetSafeNormal();
				bHasProjectionSurface = !SurfaceNormal.IsNearlyZero();
			}
		}
	}

	const bool bCanShowDecal = EndRangeDecalMaterial != nullptr &&
		bHasProjectionSurface &&
		SegmentData.EndRadius > KINDA_SMALL_NUMBER;
	if (!bCanShowDecal)
	{
		if (DecalComponent != nullptr)
		{
			DecalComponent->SetVisibility(false);
		}
		return;
	}

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	if (DecalComponent == nullptr)
	{
		DecalComponent = NewObject<UDecalComponent>(Owner);
		Owner->AddInstanceComponent(DecalComponent);
		if (USceneComponent* OwnerRoot = Owner->GetRootComponent())
		{
			DecalComponent->SetupAttachment(OwnerRoot);
		}
		DecalComponent->RegisterComponent();
		DecalComponent->SetFadeScreenSize(0.001f);
	}

	if (DynamicMaterial == nullptr || DecalComponent->GetDecalMaterial() != DynamicMaterial)
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(EndRangeDecalMaterial, this);
		DecalComponent->SetDecalMaterial(DynamicMaterial);
	}

	if (DynamicMaterial != nullptr)
	{
		DynamicMaterial->SetVectorParameterValue(TEXT("BeamColor"), LightColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), LightColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("CircleColor"), LightColor);
		DynamicMaterial->SetScalarParameterValue(
			TEXT("Opacity"),
			FMath::Clamp(EndRangeDecalOpacity, 0.0f, 1.0f));
		DynamicMaterial->SetScalarParameterValue(
			TEXT("CircleOpacity"),
			FMath::Clamp(EndRangeDecalOpacity, 0.0f, 1.0f));
	}

	const float Radius = FMath::Max(
		1.0f,
		SegmentData.EndRadius * FMath::Max(0.01f, EndRangeDecalRadiusScale));
	DecalComponent->DecalSize = FVector(
		FMath::Max(1.0f, EndRangeDecalDepth),
		Radius,
		Radius);
	DecalComponent->SetWorldLocationAndRotation(
		DecalLocation + SurfaceNormal * FMath::Max(0.0f, EndRangeDecalSurfaceOffset),
		SurfaceNormal.Rotation());
	DecalComponent->SetSortOrder(SortOrder);
	DecalComponent->SetVisibility(true);
}

void UUOUStageLightBeamVisualComponent::DestroyEndRangeDecals()
{
	if (DirectEndRangeDecal != nullptr)
	{
		DirectEndRangeDecal->DestroyComponent();
	}
	DirectEndRangeDecal = nullptr;
	DirectEndRangeDecalMaterial = nullptr;
}

void UUOUStageLightBeamVisualComponent::ApplySegmentToVFX(
	AActor* VFXActor,
	const FUOULightBeamVisualSegmentData& SegmentData)
{
	if (!IsValid(VFXActor))
	{
		return;
	}
	AppliedVisualSegments.Add(TWeakObjectPtr<AActor>(VFXActor), SegmentData);
	ApplyBeamOpacity(VFXActor);
}

bool UUOUStageLightBeamVisualComponent::ApplySegmentToLazyGodray(
	AActor* VFXActor,
	const FUOULightBeamVisualSegmentData& SegmentData)
{
	// TODO: 필요한 경우 무대 조명 전용 에셋 연동을 구현합니다.
	return false;
}

void UUOUStageLightBeamVisualComponent::UpdateCrossedLazyGodrayCard(
	AActor* VFXActor,
	const FVector& WorldBeamDirection) const
{
	// TODO: 필요한 경우 무대 조명 메시 카드 구성을 구현합니다.
}

void UUOUStageLightBeamVisualComponent::SetVFXActive(AActor* VFXActor, bool bActive)
{
	if (!IsValid(VFXActor))
	{
		return;
	}
	VFXActor->SetActorHiddenInGame(!bActive);
	if (!bActive)
	{
		// 비활성 구간이 페이드 Tick에서 다시 표시되는 것을 막습니다.
		AppliedVisualSegments.Remove(TWeakObjectPtr<AActor>(VFXActor));
	}
	if (VFXActor->GetClass()->ImplementsInterface(UUOULightBeamVisualInterface::StaticClass()))
	{
		IUOULightBeamVisualInterface::Execute_SetLightBeamVisualActive(VFXActor, bActive);
	}
}

FUOULightBeamVisualSegmentData UUOUStageLightBeamVisualComponent::BuildVisualSegment(
	const FUOULightPathSegmentData& SegmentData,
	int32 VisualSegmentIndex,
	const float ReferenceVisualLength)
{
	FUOULightBeamVisualSegmentData VisualData;
	VisualData.SegmentIndex = VisualSegmentIndex;
	VisualData.Color = ResolveLightColor();
	VisualData.Intensity = SegmentData.Intensity;
	VisualData.VisualBrightnessMultiplier =
		FMath::Max(0.0f, VisualBrightnessMultiplier) * StageVisualBrightnessNormalizationScale;
	VisualData.VisualOpacityMultiplier = FMath::Max(0.0f, VisualOpacityMultiplier);
	VisualData.LumenDynamicRayPresetOverride = FMath::Clamp(LumenDynamicRayPreset, 0, 8);
	VisualData.LumenStaticRayPresetOverride = FMath::Clamp(LumenStaticRayPreset, 0, 19);
	VisualData.Direction = SegmentData.Direction.GetSafeNormal();
	VisualData.ReferenceLength = FMath::Max(0.0f, ReferenceVisualLength);

	const FVector VisualStart = SegmentData.Start;
	const float AppliedEndPadding = FMath::Max(0.0f, EndPadding);
	const float VisibleEndDistance = FMath::Max(
		0.0f,
		SegmentData.Length - AppliedEndPadding);
	VisualData.Start = VisualStart;
	VisualData.Length = VisibleEndDistance;
	VisualData.End = VisualData.Start + VisualData.Direction * VisualData.Length;
	VisualData.StartRadius = SegmentData.StartRadius;
	const float VisibleEndRatio = SegmentData.Length > KINDA_SMALL_NUMBER
		? FMath::Clamp(VisibleEndDistance / SegmentData.Length, 0.0f, 1.0f)
		: 0.0f;
	VisualData.EndRadius = FMath::Lerp(
		SegmentData.StartRadius,
		SegmentData.EndRadius,
		VisibleEndRatio);
	VisualData.ConeAngle = SegmentData.ConeAngle;
	VisualData.EndReason = SegmentData.EndReason;
	if (IsStageUmbrellaHit(SegmentData))
	{
		// 차단 직전 길이와 폭을 유지합니다. 시작부터 차단된 경우 광원 범위를 사용합니다.
		const FUOULightBeamVisualSegmentData* PreviousVisual = UnblockedVisualSegments.Find(VisualSegmentIndex);
		VisualData.Length = PreviousVisual != nullptr ? PreviousVisual->Length : FMath::Max(0.0f, ReferenceVisualLength - EndPadding);
		VisualData.StartRadius = PreviousVisual != nullptr ? PreviousVisual->StartRadius : SegmentData.StartRadius;
		VisualData.EndRadius = PreviousVisual != nullptr ? PreviousVisual->EndRadius
			: VisualData.StartRadius + VisualData.Length * FMath::Tan(FMath::DegreesToRadians(SegmentData.ConeAngle));
		VisualData.End = VisualData.Start + VisualData.Direction * VisualData.Length;
	}
	else
	{
		UnblockedVisualSegments.Add(VisualSegmentIndex, VisualData);
	}
	return VisualData;
}

FLinearColor UUOUStageLightBeamVisualComponent::ResolveLightColor() const
{
	return BoundSourceSpotLight != nullptr
		? BoundSourceSpotLight->GetLightColor()
		: FLinearColor::White;
}

void UUOUStageLightBeamVisualComponent::DestroyVFXActors()
{
	if (IsValid(DirectVFXActor))
	{
		DirectVFXActor->Destroy();
	}
	DirectVFXActor = nullptr;
	AppliedVisualSegments.Reset();
	UnblockedVisualSegments.Reset();
}

void UUOUStageLightBeamVisualComponent::ConfigureVFXMesh(AActor* VFXActor) const
{
	// TODO: 무대 조명 메시의 그림자 등 초기 렌더링 설정을 구현합니다.
}

void UUOUStageLightBeamVisualComponent::UpdateUmbrellaFadeTarget(
	const TArray<FUOULightPathData>& LightPaths)
{
	bool bBlockedByUmbrella = false;
	for (const FUOULightPathData& Path : LightPaths)
	{
		for (const FUOULightPathSegmentData& Segment : Path.Segments)
		{
			const UPrimitiveComponent* HitComponent = Segment.HitComponent.Get();
			if (!IsValid(HitComponent))
			{
				continue;
			}
			const UUOUUmbrellaLightShadeVolumeComponent* Shade =
				Cast<UUOUUmbrellaLightShadeVolumeComponent>(HitComponent);
			if (Shade == nullptr && HitComponent->GetOwner() != nullptr)
			{
				Shade = HitComponent->GetOwner()->FindComponentByClass<UUOUUmbrellaLightShadeVolumeComponent>();
			}
			if (Shade != nullptr && Shade->CanShadeLight())
			{
				bBlockedByUmbrella = true;
				break;
			}
		}
		if (bBlockedByUmbrella)
		{
			break;
		}
	}

	const float NewTarget = bBlockedByUmbrella ? FMath::Clamp(UmbrellaBlockedAlpha, 0.0f, 1.0f) : 1.0f;
	if (FMath::IsNearlyEqual(NewTarget, FadeTargetAlpha))
	{
		return;
	}
	FadeStartAlpha = CurrentBeamAlpha;
	FadeTargetAlpha = NewTarget;
	FadeElapsed = 0.0f;
	SetComponentTickEnabled(true);
}

void UUOUStageLightBeamVisualComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	FadeElapsed += FMath::Max(0.0f, DeltaTime);
	const float Progress = BeamFadeDuration > KINDA_SMALL_NUMBER
		? FMath::Clamp(FadeElapsed / BeamFadeDuration, 0.0f, 1.0f)
		: 1.0f;
	const float EasedProgress = FMath::InterpEaseInOut(
		0.0f,
		1.0f,
		Progress,
		FMath::Max(1.0f, BeamFadeEaseExponent));
	CurrentBeamAlpha = Progress >= 1.0f
		? FadeTargetAlpha
		: FMath::Lerp(FadeStartAlpha, FadeTargetAlpha, EasedProgress);
	ApplyBeamOpacity(DirectVFXActor);
	// 전환 중에만 Tick을 사용하고 목표 알파에 도달하면 중지합니다.
	if (Progress >= 1.0f)
	{
		SetComponentTickEnabled(false);
	}
}

void UUOUStageLightBeamVisualComponent::ApplyBeamOpacity(AActor* VFXActor) const
{
	if (!IsValid(VFXActor))
	{
		return;
	}
	if (VFXActor->GetClass()->ImplementsInterface(UUOULightBeamVisualInterface::StaticClass()))
	{
		const FUOULightBeamVisualSegmentData* Original = AppliedVisualSegments.Find(TWeakObjectPtr<AActor>(VFXActor));
		if (Original != nullptr)
		{
			FUOULightBeamVisualSegmentData Faded = *Original;
			Faded.VisualOpacityMultiplier *= CurrentBeamAlpha;
			// 기존 레이어별 알파와 렌더러의 카메라 페이드를 유지합니다.
			IUOULightBeamVisualInterface::Execute_ApplyLightBeamSegment(VFXActor, Faded);
		}
		return;
	}
	const float Opacity = FMath::Clamp(CurrentBeamAlpha * VisualOpacityMultiplier, 0.0f, 1.0f);
	if (!MeshOpacityParameter.IsNone())
	{
		TInlineComponentArray<UMeshComponent*> MeshComponents(VFXActor);
		for (UMeshComponent* Mesh : MeshComponents)
		{
			if (IsValid(Mesh))
			{
				Mesh->SetScalarParameterValueOnMaterials(MeshOpacityParameter, Opacity);
			}
		}
	}
	if (!NiagaraOpacityParameter.IsNone())
	{
		TInlineComponentArray<UNiagaraComponent*> NiagaraComponents(VFXActor);
		for (UNiagaraComponent* Niagara : NiagaraComponents)
		{
			if (IsValid(Niagara))
			{
				Niagara->SetVariableFloat(NiagaraOpacityParameter, Opacity);
			}
		}
	}
}
