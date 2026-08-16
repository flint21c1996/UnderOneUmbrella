// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOULightBeamVisualComponent.h"

#include "Components/BoxComponent.h"
#include "Components/LightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "UObject/FieldIterator.h"
#include "UObject/UnrealType.h"
#include "World/Light/UOULightBeamVisualInterface.h"
#include "World/Light/UOULightExposureSourceComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogUOULightBeamVisual, Log, All);

namespace
{
	struct FLazyGodrayBeamProfile
	{
		float LengthMultiplier = 1.0f;
		float DiameterMultiplier = 1.0f;
		float ConvergentRate = 0.0f;
	};

	FLazyGodrayBeamProfile CalculateLazyGodrayBeamProfile(
		const FUOULightBeamVisualSegmentData& SegmentData,
		const float BaseLength,
		const float BaseDiameter)
	{
		FLazyGodrayBeamProfile Profile;
		const float SafeBaseLength = FMath::Max(0.01f, BaseLength);
		const float SafeBaseDiameter = FMath::Max(0.01f, BaseDiameter);
		const float SafeStartRadius = FMath::Max(0.0f, SegmentData.StartRadius);
		const float SafeEndRadius = FMath::Max(0.0f, SegmentData.EndRadius);

		Profile.LengthMultiplier = FMath::Max(0.0f, SegmentData.Length) / SafeBaseLength;

		// LazyGodray V2는 끝 폭을 Godray Width로 사용하고, 시작 폭을
		// EndWidth * (1 - GodrayConvergent)로 계산한다.
		if (SafeEndRadius >= SafeStartRadius && SafeEndRadius > KINDA_SMALL_NUMBER)
		{
			Profile.DiameterMultiplier = (SafeEndRadius * 2.0f) / SafeBaseDiameter;
			Profile.ConvergentRate = FMath::Clamp(
				1.0f - (SafeStartRadius / SafeEndRadius),
				0.0f,
				1.0f);
		}
		else
		{
			// 에셋이 지원하지 않는 역원뿔은 시작 단면이 잘리지 않도록 원기둥으로 표현한다.
			Profile.DiameterMultiplier = (FMath::Max(SafeStartRadius, SafeEndRadius) * 2.0f)
				/ SafeBaseDiameter;
			Profile.ConvergentRate = 0.0f;
		}

		return Profile;
	}

	FVector CalculateJunctionPlaneNormal(
		const FVector& IncomingDirection,
		const FVector& ReflectedDirection)
	{
		const FVector SafeIncomingDirection = IncomingDirection.GetSafeNormal();
		const FVector SafeReflectedDirection = ReflectedDirection.GetSafeNormal();
		if (SafeIncomingDirection.IsNearlyZero() || SafeReflectedDirection.IsNearlyZero())
		{
			return FVector::ZeroVector;
		}

		FVector PlaneNormal = (SafeReflectedDirection - SafeIncomingDirection).GetSafeNormal();
		if (PlaneNormal.IsNearlyZero())
		{
			return FVector::ZeroVector;
		}

		// 입사광이 존재하는 쪽이 양수가 되도록 법선 방향을 고정합니다.
		if (FVector::DotProduct(-SafeIncomingDirection, PlaneNormal) < 0.0f)
		{
			PlaneNormal *= -1.0f;
		}
		return PlaneNormal;
	}

	FString NormalizeBlueprintMemberText(FString MemberText)
	{
		FString Normalized = MoveTemp(MemberText).ToLower();
		Normalized.ReplaceInline(TEXT(" "), TEXT(""));
		Normalized.ReplaceInline(TEXT("_"), TEXT(""));
		Normalized.ReplaceInline(TEXT("-"), TEXT(""));
		return Normalized;
	}

	void UpdateLazyGodraySpatialComponents(
		AActor* VFXActor,
		const FUOULightBeamVisualSegmentData& SegmentData,
		const FVector& LocalBeamAxis)
	{
		if (VFXActor == nullptr || LocalBeamAxis.IsNearlyZero())
		{
			return;
		}

		const FVector SafeLocalAxis = LocalBeamAxis.GetSafeNormal();
		const FVector AbsoluteAxis = SafeLocalAxis.GetAbs();
		const float SafeLength = FMath::Max(0.0f, SegmentData.Length);
		const float HalfLength = SafeLength * 0.5f;
		const float MaxRadius = FMath::Max(
			FMath::Max(0.0f, SegmentData.StartRadius),
			FMath::Max(0.0f, SegmentData.EndRadius));

		TInlineComponentArray<UBoxComponent*> BoxComponents(VFXActor);
		for (UBoxComponent* BoxComponent : BoxComponents)
		{
			if (BoxComponent == nullptr ||
				!NormalizeBlueprintMemberText(BoxComponent->GetName()).Contains(TEXT("godraycardbounds")))
			{
				continue;
			}

			const FVector BoxExtent = FVector(MaxRadius) * (FVector::OneVector - AbsoluteAxis)
				+ AbsoluteAxis * HalfLength;
			BoxComponent->SetRelativeLocation(SafeLocalAxis * HalfLength);
			BoxComponent->SetBoxExtent(BoxExtent.ComponentMax(FVector(0.01f)), false);
		}

		TInlineComponentArray<UNiagaraComponent*> NiagaraComponents(VFXActor);
		for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
		{
			if (NiagaraComponent == nullptr)
			{
				continue;
			}

			bool bHasCylinderHeight = false;
			const float CurrentCylinderHeight = NiagaraComponent->GetVariableFloat(
				TEXT("User.CylinderHeight"),
				bHasCylinderHeight);
			if (bHasCylinderHeight && !FMath::IsNearlyEqual(CurrentCylinderHeight, SafeLength))
			{
				NiagaraComponent->SetVariableFloat(TEXT("User.CylinderHeight"), SafeLength);
			}

			bool bHasCylinderRadius = false;
			const float CurrentCylinderRadius = NiagaraComponent->GetVariableFloat(
				TEXT("User.CylinderRadius"),
				bHasCylinderRadius);
			if (bHasCylinderRadius && !FMath::IsNearlyEqual(CurrentCylinderRadius, MaxRadius))
			{
				NiagaraComponent->SetVariableFloat(TEXT("User.CylinderRadius"), MaxRadius);
			}

			// LazyGodray Dust 원기둥은 Niagara 원점을 중심으로 양쪽에 생성됩니다.
			// VFX 액터는 구간 시작점에 있으므로 원점을 중앙으로 옮겨 차단물 뒤로
			// 먼지가 절반만큼 넘어가거나 시작점 뒤에 남는 현상을 방지합니다.
			const FVector WorldMidpoint = VFXActor->GetActorTransform().TransformPosition(
				SafeLocalAxis * HalfLength);
			NiagaraComponent->SetWorldLocation(WorldMidpoint);
		}
	}

	bool IsMatchingBlueprintMember(
		const FString& InternalName,
		const FString& DisplayName,
		const FString& NormalizedName)
	{
		const FString NormalizedInternalName = NormalizeBlueprintMemberText(InternalName);
		const FString NormalizedDisplayName = NormalizeBlueprintMemberText(DisplayName);

		// Blueprint 컴파일 과정에서 내부 이름 뒤에 GUID나 숫자 접미사가 붙을 수 있습니다.
		return NormalizedInternalName == NormalizedName
			|| NormalizedDisplayName == NormalizedName
			|| NormalizedInternalName.StartsWith(NormalizedName);
	}

	FProperty* FindNormalizedProperty(const UClass* Class, const FString& NormalizedName)
	{
		for (TFieldIterator<FProperty> PropertyIt(Class, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			if (IsMatchingBlueprintMember(
				PropertyIt->GetName(),
				PropertyIt->GetDisplayNameText().ToString(),
				NormalizedName))
			{
				return *PropertyIt;
			}
		}
		return nullptr;
	}

	bool SetNumericBlueprintProperty(UObject* Object, const FString& NormalizedName, const double Value)
	{
		if (Object == nullptr)
		{
			return false;
		}

		FNumericProperty* NumericProperty = CastField<FNumericProperty>(
			FindNormalizedProperty(Object->GetClass(), NormalizedName));
		if (NumericProperty == nullptr)
		{
			return false;
		}

		void* ValueAddress = NumericProperty->ContainerPtrToValuePtr<void>(Object);
		if (NumericProperty->IsFloatingPoint())
		{
			NumericProperty->SetFloatingPointPropertyValue(ValueAddress, Value);
			return true;
		}
		return false;
	}

	bool SetLinearColorBlueprintProperty(
		UObject* Object,
		const FString& NormalizedName,
		const FLinearColor& Value)
	{
		if (Object == nullptr)
		{
			return false;
		}

		FStructProperty* StructProperty = CastField<FStructProperty>(
			FindNormalizedProperty(Object->GetClass(), NormalizedName));
		if (StructProperty == nullptr || StructProperty->Struct != TBaseStructure<FLinearColor>::Get())
		{
			return false;
		}

		*StructProperty->ContainerPtrToValuePtr<FLinearColor>(Object) = Value;
		return true;
	}

	bool SetBoolBlueprintProperty(UObject* Object, const FString& NormalizedName, const bool bValue)
	{
		if (Object == nullptr)
		{
			return false;
		}

		FBoolProperty* BoolProperty = CastField<FBoolProperty>(
			FindNormalizedProperty(Object->GetClass(), NormalizedName));
		if (BoolProperty == nullptr)
		{
			return false;
		}

		void* ValueAddress = BoolProperty->ContainerPtrToValuePtr<void>(Object);
		BoolProperty->SetPropertyValue(ValueAddress, bValue);
		return true;
	}

	UFunction* FindNormalizedFunction(const UClass* Class, const FString& NormalizedName)
	{
		for (TFieldIterator<UFunction> FunctionIt(Class, EFieldIteratorFlags::IncludeSuper); FunctionIt; ++FunctionIt)
		{
			if (IsMatchingBlueprintMember(
				FunctionIt->GetName(),
				FunctionIt->GetDisplayNameText().ToString(),
				NormalizedName))
			{
				return *FunctionIt;
			}
		}
		return nullptr;
	}
}

UUOULightBeamVisualComponent::UUOULightBeamVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOULightBeamVisualComponent::BeginPlay()
{
	Super::BeginPlay();

	BoundSourceComponent = ResolveSourceComponent();
	BoundSourceSpotLight = ResolveSourceSpotLight();

	if (BoundSourceComponent != nullptr)
	{
		BoundSourceComponent->OnLightPathsUpdated.AddDynamic(
			this,
			&UUOULightBeamVisualComponent::HandleLightPathsUpdated);
	}

	RefreshVisuals();
}

void UUOULightBeamVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundSourceComponent != nullptr)
	{
		BoundSourceComponent->OnLightPathsUpdated.RemoveDynamic(
			this,
			&UUOULightBeamVisualComponent::HandleLightPathsUpdated);
	}

	DestroyVFXActors();
	WarnedIncompatibleVFXClasses.Reset();
	bHasWarnedReflectionVFXLimit = false;
	BoundSourceSpotLight = nullptr;
	BoundSourceComponent = nullptr;

	Super::EndPlay(EndPlayReason);
}

void UUOULightBeamVisualComponent::RefreshVisuals()
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
	UpdateDirectVFX(LightPaths);
	UpdateReflectionVFX(LightPaths);
}

void UUOULightBeamVisualComponent::HandleLightPathsUpdated(
	const TArray<FUOULightPathData>& LightPaths)
{
	UpdateDirectVFX(LightPaths);
	UpdateReflectionVFX(LightPaths);
}

UUOULightExposureSourceComponent* UUOULightBeamVisualComponent::ResolveSourceComponent() const
{
	const AActor* Owner = GetOwner();
	return Owner != nullptr
		? Owner->FindComponentByClass<UUOULightExposureSourceComponent>()
		: nullptr;
}

USpotLightComponent* UUOULightBeamVisualComponent::ResolveSourceSpotLight() const
{
	const AActor* Owner = GetOwner();
	return Owner != nullptr
		? Owner->FindComponentByClass<USpotLightComponent>()
		: nullptr;
}

AActor* UUOULightBeamVisualComponent::AcquireDirectVFXActor()
{
	if (!IsValid(DirectVFXActor))
	{
		DirectVFXActor = SpawnVFXActor();
	}
	return DirectVFXActor;
}

AActor* UUOULightBeamVisualComponent::AcquireReflectionVFXActor(int32 PoolIndex)
{
	if (ReflectionVFXPool.IsValidIndex(PoolIndex) && IsValid(ReflectionVFXPool[PoolIndex]))
	{
		return ReflectionVFXPool[PoolIndex];
	}

	AActor* VFXActor = SpawnVFXActor();
	if (VFXActor == nullptr)
	{
		return nullptr;
	}

	if (ReflectionVFXPool.IsValidIndex(PoolIndex))
	{
		ReflectionVFXPool[PoolIndex] = VFXActor;
	}
	else
	{
		ReflectionVFXPool.Add(VFXActor);
	}
	return VFXActor;
}

AActor* UUOULightBeamVisualComponent::SpawnVFXActor()
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

void UUOULightBeamVisualComponent::ConfigureSpawnedVFXActor(AActor* VFXActor) const
{
	if (VFXActor == nullptr)
	{
		return;
	}

	// 표현 액터가 게임플레이 광선과 캐릭터 충돌에 참여하지 않도록 합니다.
	VFXActor->SetActorEnableCollision(false);
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

void UUOULightBeamVisualComponent::UpdateDirectVFX(
	const TArray<FUOULightPathData>& LightPaths)
{
	if (!bEnableDirectVFX || VFXActorClass == nullptr || BoundSourceComponent == nullptr)
	{
		SetVFXActive(DirectVFXActor, false);
		return;
	}

	const FUOULightPathSegmentData* DirectSegment = nullptr;
	const FUOULightPathSegmentData* JunctionReflectedSegment = nullptr;
	for (const FUOULightPathData& PathData : LightPaths)
	{
		DirectSegment = PathData.Segments.FindByPredicate(
			[](const FUOULightPathSegmentData& SegmentData)
			{
				return !SegmentData.bReflected &&
					SegmentData.Length > KINDA_SMALL_NUMBER &&
					!SegmentData.Direction.IsNearlyZero();
			});
		if (DirectSegment != nullptr)
		{
			JunctionReflectedSegment = PathData.Segments.FindByPredicate(
				[](const FUOULightPathSegmentData& SegmentData)
				{
					return SegmentData.bReflected &&
						SegmentData.Length > KINDA_SMALL_NUMBER &&
						!SegmentData.Direction.IsNearlyZero();
				});
			break;
		}
	}

	if (DirectSegment == nullptr)
	{
		SetVFXActive(DirectVFXActor, false);
		return;
	}

	AActor* VFXActor = AcquireDirectVFXActor();
	if (VFXActor == nullptr)
	{
		return;
	}

	ApplySegmentToVFX(
		VFXActor,
		BuildVisualSegment(*DirectSegment, 0, nullptr, JunctionReflectedSegment));
}

void UUOULightBeamVisualComponent::UpdateReflectionVFX(
	const TArray<FUOULightPathData>& LightPaths)
{
	if (!bEnableReflectionVFX || VFXActorClass == nullptr || MaxReflectionVFXCount <= 0)
	{
		HideUnusedReflectionVFX(0);
		return;
	}

	int32 EligibleSegmentCount = 0;
	for (const FUOULightPathData& PathData : LightPaths)
	{
		for (const FUOULightPathSegmentData& SegmentData : PathData.Segments)
		{
			if (SegmentData.bReflected &&
				SegmentData.Length > KINDA_SMALL_NUMBER &&
				!SegmentData.Direction.IsNearlyZero())
			{
				++EligibleSegmentCount;
			}
		}
	}

	if (EligibleSegmentCount > MaxReflectionVFXCount)
	{
		if (!bHasWarnedReflectionVFXLimit)
		{
			UE_LOG(
				LogUOULightBeamVisual,
				Warning,
				TEXT("%s: 반사 VFX 구간 %d개 중 최대 %d개만 표시합니다."),
				*GetNameSafe(GetOwner()),
				EligibleSegmentCount,
				MaxReflectionVFXCount);
			bHasWarnedReflectionVFXLimit = true;
		}
	}
	else
	{
		bHasWarnedReflectionVFXLimit = false;
	}

	const FLinearColor LightColor = ResolveLightColor();
	int32 VFXIndex = 0;
	for (const FUOULightPathData& PathData : LightPaths)
	{
		for (int32 SegmentIndex = 0; SegmentIndex < PathData.Segments.Num(); ++SegmentIndex)
		{
			const FUOULightPathSegmentData& SegmentData = PathData.Segments[SegmentIndex];
			if (VFXIndex >= MaxReflectionVFXCount)
			{
				break;
			}
			if (!SegmentData.bReflected ||
				SegmentData.Length <= KINDA_SMALL_NUMBER ||
				SegmentData.Direction.IsNearlyZero())
			{
				continue;
			}

			AActor* VFXActor = AcquireReflectionVFXActor(VFXIndex);
			if (VFXActor == nullptr)
			{
				continue;
			}

			const FUOULightPathSegmentData* NextReflectedSegment =
				PathData.Segments.IsValidIndex(SegmentIndex + 1) &&
				PathData.Segments[SegmentIndex + 1].bReflected
					? &PathData.Segments[SegmentIndex + 1]
					: nullptr;
			const FUOULightPathSegmentData* PreviousSegment =
				PathData.Segments.IsValidIndex(SegmentIndex - 1)
					? &PathData.Segments[SegmentIndex - 1]
					: nullptr;
			FUOULightBeamVisualSegmentData VisualData = BuildVisualSegment(
				SegmentData,
				VFXIndex + 1,
				PreviousSegment,
				NextReflectedSegment);
			VisualData.Color = LightColor;
			ApplySegmentToVFX(VFXActor, VisualData);
			++VFXIndex;
		}

		if (VFXIndex >= MaxReflectionVFXCount)
		{
			break;
		}
	}

	HideUnusedReflectionVFX(VFXIndex);
	ActiveReflectionVFXCount = VFXIndex;
}

void UUOULightBeamVisualComponent::HideUnusedReflectionVFX(int32 FirstUnusedIndex)
{
	for (int32 PoolIndex = FMath::Max(0, FirstUnusedIndex); PoolIndex < ReflectionVFXPool.Num(); ++PoolIndex)
	{
		SetVFXActive(ReflectionVFXPool[PoolIndex], false);
	}
	ActiveReflectionVFXCount = FMath::Clamp(FirstUnusedIndex, 0, ReflectionVFXPool.Num());
}

void UUOULightBeamVisualComponent::ApplySegmentToVFX(
	AActor* VFXActor,
	const FUOULightBeamVisualSegmentData& SegmentData)
{
	if (VFXActor == nullptr)
	{
		return;
	}

	if (VFXActor->GetClass()->ImplementsInterface(UUOULightBeamVisualInterface::StaticClass()))
	{
		IUOULightBeamVisualInterface::Execute_ApplyLightBeamSegment(VFXActor, SegmentData);
	}
	else if (!ApplySegmentToLazyGodray(VFXActor, SegmentData))
	{
		SetVFXActive(VFXActor, false);
		return;
	}

	SetVFXActive(VFXActor, true);
}

bool UUOULightBeamVisualComponent::ApplySegmentToLazyGodray(
	AActor* VFXActor,
	const FUOULightBeamVisualSegmentData& SegmentData)
{
	if (!bEnableAutomaticLazyGodrayAdapter || VFXActor == nullptr || SegmentData.Direction.IsNearlyZero())
	{
		return false;
	}

	const FVector SegmentDirection = SegmentData.Direction.GetSafeNormal();
	FVector LocalBeamAxis = FVector::ForwardVector;
	bool bDetectedBeamAxis = false;
	if (bAutoDetectLazyGodrayBeamAxis)
	{
		TInlineComponentArray<USpotLightComponent*> EmbeddedSpotLights(VFXActor);
		if (!EmbeddedSpotLights.IsEmpty() && EmbeddedSpotLights[0] != nullptr)
		{
			// LazyGodray V2의 메시와 먼지는 액터 로컬 -Z로 진행하며, 내부 SpotLight도 같은 축을 사용합니다.
			// 현재 액터 회전을 역변환하면 에셋마다 다른 로컬 진행축을 별도 BP 작업 없이 얻을 수 있습니다.
			LocalBeamAxis = VFXActor->GetActorTransform()
				.InverseTransformVectorNoScale(EmbeddedSpotLights[0]->GetForwardVector())
				.GetSafeNormal();
			bDetectedBeamAxis = !LocalBeamAxis.IsNearlyZero();
		}
	}

	// 자동 감지와 기존 수동 Offset을 함께 적용하면 LazyGodray V2의 축이 두 번 보정됩니다.
	// 내부 SpotLight에서 축을 얻지 못한 에셋에만 기존 +X 기준 Offset을 대체 경로로 사용합니다.
	const FQuat BeamAlignmentRotation = bDetectedBeamAxis
		? FQuat::FindBetweenNormals(LocalBeamAxis, SegmentDirection)
		: SegmentDirection.Rotation().Quaternion() * LazyGodrayRotationOffset.Quaternion();
	VFXActor->SetActorLocationAndRotation(
		SegmentData.Start,
		BeamAlignmentRotation);

	const FLazyGodrayBeamProfile BeamProfile = CalculateLazyGodrayBeamProfile(
		SegmentData,
		LazyGodrayBaseLength,
		LazyGodrayBaseDiameter);

	const bool bSetLength = SetNumericBlueprintProperty(
		VFXActor,
		TEXT("overalllengthmultiplier"),
		BeamProfile.LengthMultiplier);
	const bool bSetDiameter = SetNumericBlueprintProperty(
		VFXActor,
		TEXT("overalldiametermultiplier"),
		BeamProfile.DiameterMultiplier);
	const bool bSetV2ConvergentRate = SetNumericBlueprintProperty(
		VFXActor,
		TEXT("overalllightrayconvergentrate"),
		BeamProfile.ConvergentRate);
	// LazyGodray V1.2는 V2와 다른 이름의 토글/수렴률을 사용합니다.
	// 둘 다 설정해야 반사 지점의 시작 반경을 유지하고 새 광원처럼 뾰족해지지 않습니다.
	const bool bSetV1ConvergentToggle = SetBoolBlueprintProperty(
		VFXActor,
		TEXT("uselightrayconvergent"),
		BeamProfile.ConvergentRate > KINDA_SMALL_NUMBER);
	const bool bSetV1ConvergentRate = SetNumericBlueprintProperty(
		VFXActor,
		TEXT("volumetriclightrayconvergentrate"),
		BeamProfile.ConvergentRate);
	const bool bSetConvergentProfile = bSetV2ConvergentRate ||
		(bSetV1ConvergentToggle && bSetV1ConvergentRate);
	const bool bSetIntensity = SetNumericBlueprintProperty(
		VFXActor,
		TEXT("overallintensity"),
		FMath::Max(0.0f, SegmentData.Intensity * LazyGodrayIntensityScale));
	const bool bSetColor = SetLinearColorBlueprintProperty(
		VFXActor,
		TEXT("overalltintcolor"),
		SegmentData.Color);

	bool bCalledUpdateFunction = false;
	if (UFunction* UpdateFunction = FindNormalizedFunction(
		VFXActor->GetClass(),
		TEXT("onchanginggodrayparameters")))
	{
		uint8* Parameters = static_cast<uint8*>(FMemory_Alloca(UpdateFunction->ParmsSize));
		FMemory::Memzero(Parameters, UpdateFunction->ParmsSize);
		VFXActor->ProcessEvent(UpdateFunction, Parameters);
		bCalledUpdateFunction = true;
	}

	// 에셋 BP는 머티리얼 폭/높이는 갱신하지만 V2의 보조 Bounds와 Dust 실린더는
	// 전체 배율을 항상 따라가지 않으므로 최종 경로 단면으로 명시적으로 맞춘다.
	UpdateLazyGodraySpatialComponents(VFXActor, SegmentData, LocalBeamAxis);
	UpdateCrossedLazyGodrayCard(VFXActor, SegmentDirection);

	// 일부 Fab 에셋은 변수의 내부 이름이 버전마다 달라질 수 있습니다.
	// 갱신 함수나 주요 크기 값 중 하나라도 적용됐다면 기본 표현을 유지합니다.
	// BP 갱신 이벤트가 Godray Width/Height/Z Offset과 godray_card_bounds,
	// NS_Dust 실린더를 같은 길이·끝 지름 프로필로 갱신한다.
	const bool bHasRequiredAdapterMembers = bSetLength && bSetDiameter && bCalledUpdateFunction;
	if ((!bHasRequiredAdapterMembers || !bSetIntensity || !bSetColor || !bSetConvergentProfile) &&
		!WarnedIncompatibleVFXClasses.Contains(VFXActor->GetClass()))
	{
		WarnedIncompatibleVFXClasses.Add(VFXActor->GetClass());
		UE_LOG(
			LogUOULightBeamVisual,
			Warning,
			TEXT("%s: LazyGodray 자동 연동이 완전하지 않습니다 (Length=%d, Diameter=%d, Convergent=%d, Intensity=%d, Color=%d, Update=%d)."),
			*GetNameSafe(VFXActor->GetClass()),
			bSetLength,
			bSetDiameter,
			bSetConvergentProfile,
			bSetIntensity,
			bSetColor,
			bCalledUpdateFunction);
	}

	return bHasRequiredAdapterMembers;
}

void UUOULightBeamVisualComponent::UpdateCrossedLazyGodrayCard(
	AActor* VFXActor,
	const FVector& WorldBeamDirection) const
{
	if (VFXActor == nullptr || WorldBeamDirection.IsNearlyZero())
	{
		return;
	}

	UStaticMeshComponent* SourceCard = nullptr;
	TInlineComponentArray<UStaticMeshComponent*> MeshComponents(VFXActor);
	for (UStaticMeshComponent* MeshComponent : MeshComponents)
	{
		if (MeshComponent == nullptr)
		{
			continue;
		}

		const FString NormalizedName = NormalizeBlueprintMemberText(MeshComponent->GetName());
		if (NormalizedName.StartsWith(TEXT("uoucrossgodraycard")))
		{
			MeshComponent->SetVisibility(false, true);
			continue;
		}

		if (NormalizedName.Contains(TEXT("godraycard")))
		{
			SourceCard = MeshComponent;
			break;
		}
	}

	if (SourceCard == nullptr)
	{
		return;
	}

	if (!bAddCrossedLazyGodrayCard)
	{
		return;
	}

	const int32 CardPlaneCount = FMath::Clamp(LazyGodrayCardPlaneCount, 1, 8);
	const FVector SafeBeamDirection = WorldBeamDirection.GetSafeNormal();
	for (int32 CardIndex = 1; CardIndex < CardPlaneCount; ++CardIndex)
	{
		const FName CrossCardName(*FString::Printf(TEXT("UOUCrossGodrayCard_%d"), CardIndex));
		UStaticMeshComponent* CrossCard = FindObjectFast<UStaticMeshComponent>(VFXActor, CrossCardName);
		if (CrossCard == nullptr)
		{
			CrossCard = NewObject<UStaticMeshComponent>(VFXActor, CrossCardName);
		}
		if (CrossCard == nullptr)
		{
			continue;
		}

		if (!CrossCard->IsRegistered())
		{
			VFXActor->AddInstanceComponent(CrossCard);
			USceneComponent* AttachParent = SourceCard->GetAttachParent();
			if (AttachParent == nullptr)
			{
				AttachParent = VFXActor->GetRootComponent();
			}
			if (AttachParent != nullptr)
			{
				CrossCard->AttachToComponent(
					AttachParent,
					FAttachmentTransformRules::KeepWorldTransform,
					SourceCard->GetAttachSocketName());
			}
			CrossCard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			CrossCard->RegisterComponent();
		}

		CrossCard->SetStaticMesh(SourceCard->GetStaticMesh());
		for (int32 MaterialIndex = 0; MaterialIndex < SourceCard->GetNumMaterials(); ++MaterialIndex)
		{
			CrossCard->SetMaterial(MaterialIndex, SourceCard->GetMaterial(MaterialIndex));
		}
		CrossCard->SetCastShadow(SourceCard->CastShadow);
		CrossCard->SetTranslucentSortPriority(SourceCard->TranslucencySortPriority);

		const float AngleRadians = PI * static_cast<float>(CardIndex) /
			static_cast<float>(CardPlaneCount);
		const FQuat CrossRotation = FQuat(SafeBeamDirection, AngleRadians) *
			SourceCard->GetComponentQuat();
		CrossCard->SetWorldLocationAndRotation(SourceCard->GetComponentLocation(), CrossRotation);
		CrossCard->SetWorldScale3D(SourceCard->GetComponentScale());
		CrossCard->SetVisibility(SourceCard->IsVisible(), true);
	}
}

void UUOULightBeamVisualComponent::SetVFXActive(AActor* VFXActor, bool bActive) const
{
	if (VFXActor == nullptr)
	{
		return;
	}

	VFXActor->SetActorHiddenInGame(!bActive);
	TInlineComponentArray<UNiagaraComponent*> NiagaraComponents(VFXActor);
	for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
	{
		if (NiagaraComponent == nullptr)
		{
			continue;
		}

		NiagaraComponent->SetVisibility(bActive, true);
		if (bActive)
		{
			if (!NiagaraComponent->IsActive())
			{
				NiagaraComponent->Activate(true);
			}
		}
		else
		{
			NiagaraComponent->DeactivateImmediate();
		}
	}
	if (VFXActor->GetClass()->ImplementsInterface(UUOULightBeamVisualInterface::StaticClass()))
	{
		IUOULightBeamVisualInterface::Execute_SetLightBeamVisualActive(VFXActor, bActive);
	}
}

FUOULightBeamVisualSegmentData UUOULightBeamVisualComponent::BuildVisualSegment(
	const FUOULightPathSegmentData& SegmentData,
	int32 VisualSegmentIndex,
	const FUOULightPathSegmentData* PreviousSegment,
	const FUOULightPathSegmentData* NextReflectedSegment) const
{
	FUOULightBeamVisualSegmentData VisualData;
	VisualData.SegmentIndex = VisualSegmentIndex;
	VisualData.bReflected = SegmentData.bReflected;
	VisualData.bEndsAtReflection = SegmentData.HitType == EUOULightPathHitType::ReflectingSurface;
	VisualData.Color = ResolveLightColor();
	VisualData.Intensity = SegmentData.Intensity;
	VisualData.VisualBrightnessMultiplier = FMath::Max(0.0f, VisualBrightnessMultiplier);
	VisualData.VisualOpacityMultiplier = FMath::Max(0.0f, VisualOpacityMultiplier);
	VisualData.LumenDynamicRayPresetOverride = FMath::Clamp(LumenDynamicRayPreset, 0, 8);
	VisualData.LumenStaticRayPresetOverride = FMath::Clamp(LumenStaticRayPreset, 0, 19);
	VisualData.Direction = SegmentData.Direction.GetSafeNormal();
	VisualData.JunctionClipFeather = FMath::Max(0.0f, ReflectionJunctionClipFeather);

	if (SegmentData.bReflected && PreviousSegment != nullptr)
	{
		const FVector StartPlaneNormal = CalculateJunctionPlaneNormal(
			SegmentData.IncomingDirection,
			SegmentData.Direction);
		if (!StartPlaneNormal.IsNearlyZero())
		{
			VisualData.bUseStartJunctionClip = true;
			VisualData.StartJunctionPlanePosition = PreviousSegment->End;
			VisualData.StartJunctionPlaneNormal = StartPlaneNormal;
		}
	}

	if (VisualData.bEndsAtReflection && NextReflectedSegment != nullptr)
	{
		const FVector EndPlaneNormal = CalculateJunctionPlaneNormal(
			SegmentData.Direction,
			NextReflectedSegment->Direction);
		if (!EndPlaneNormal.IsNearlyZero())
		{
			VisualData.bUseEndJunctionClip = true;
			VisualData.EndJunctionPlanePosition = SegmentData.End;
			VisualData.EndJunctionPlaneNormal = EndPlaneNormal;
		}
	}

	// 반사 연결부는 실제 충돌점까지 메시를 이어 붙이고 머티리얼 평면으로 뒤쪽만 자릅니다.
	// 벽·수신체 종단에만 기존 EndPadding을 적용합니다.
	const FVector VisualStart = VisualData.bUseStartJunctionClip && PreviousSegment != nullptr
		? PreviousSegment->End
		: SegmentData.Start;
	const float StartExtension = FVector::Distance(VisualStart, SegmentData.Start);
	const float AppliedEndPadding = VisualData.bEndsAtReflection
		? 0.0f
		: FMath::Max(0.0f, EndPadding);
	const float VisibleEndDistance = FMath::Max(
		0.0f,
		SegmentData.Length + StartExtension - AppliedEndPadding);
	VisualData.Start = VisualStart;
	VisualData.Length = VisibleEndDistance;
	VisualData.End = VisualData.Start + VisualData.Direction * VisualData.Length;
	VisualData.StartRadius = SegmentData.StartRadius;
	const float VisibleEndRatio = SegmentData.Length > KINDA_SMALL_NUMBER
		? FMath::Clamp((VisibleEndDistance - StartExtension) / SegmentData.Length, 0.0f, 1.0f)
		: 0.0f;
	VisualData.EndRadius = FMath::Lerp(
		SegmentData.StartRadius,
		SegmentData.EndRadius,
		VisibleEndRatio);
	VisualData.ConeAngle = SegmentData.ConeAngle;
	VisualData.EndReason = SegmentData.EndReason;
	return VisualData;
}

FLinearColor UUOULightBeamVisualComponent::ResolveLightColor() const
{
	return BoundSourceSpotLight != nullptr
		? BoundSourceSpotLight->GetLightColor()
		: FLinearColor::White;
}

void UUOULightBeamVisualComponent::DestroyVFXActors()
{
	if (IsValid(DirectVFXActor))
	{
		DirectVFXActor->Destroy();
	}
	DirectVFXActor = nullptr;

	for (AActor* ReflectionVFXActor : ReflectionVFXPool)
	{
		if (IsValid(ReflectionVFXActor))
		{
			ReflectionVFXActor->Destroy();
		}
	}
	ReflectionVFXPool.Reset();
	ActiveReflectionVFXCount = 0;
}
