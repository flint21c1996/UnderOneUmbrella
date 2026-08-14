// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUDevelopmentDebugDrawSubsystem.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/ActorComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Debug/UOUPuzzleDebugProviderComponent.h"
#include "Debug/UOUDebugProvider.h"
#include "Debug/UOUPuzzleDebugInfoProvider.h"
#include "Debug/UOUDevelopmentToolsBuild.h"
#include "DrawDebugHelpers.h"
#include "DynamicRHI.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMemory.h"
#include "InputCoreTypes.h"
#include "NiagaraComponent.h"
#include "NiagaraParameterStore.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationPath.h"
#include "Particles/ParticleSystemComponent.h"
#include "Player/UOUCameraControllerComponent.h"
#include "Player/UOUCharacter.h"
#include "Player/UOUInteractionComponent.h"
#include "Player/UOUPushPullInteractorComponent.h"
#include "Player/UOURainReceiverComponent.h"
#include "Player/UOUUmbrellaComponent.h"
#include "Player/UOUUmbrellaLightInteractionComponent.h"
#include "Player/UOUUmbrellaLightShadeVolumeComponent.h"
#include "Player/UOUWaterContainerComponent.h"
#include "Puzzle/PushPull/UOUPushPullObjectComponent.h"
#include "Puzzle/HeatWire/UOUHeatWireActor.h"
#include "Puzzle/Weight/UOUWeightedButtonComponent.h"
#include "RHICommandList.h"
#include "RHIStats.h"
#include "RenderCounters.h"
#include "RenderTimer.h"
#include "UOUDevelopmentDebugControlSubsystem.h"
#include "UOUDevelopmentDebugDrawContext.h"
#include "UnrealClient.h"
#include "World/Environment/UOUEnvironmentVisualComponent.h"
#include "World/Environment/UOUPlayerBlockingWallActor.h"
#include "World/Light/UOULightInteractionSurfaceComponent.h"
#include "World/Light/UOULightExposureReceiverComponent.h"
#include "World/Light/UOULightExposureSourceComponent.h"
#include "World/Light/UOULightSourceActor.h"
#include "World/Light/UOURotatableMirrorComponent.h"
#include "World/Light/UOULightReceivableInterface.h"
#include "World/NPC/UOUNPCCharacter.h"
#include "World/Pour/UOUPourDropActor.h"
#include "World/RainArea/UOUUmbrellaRainArea.h"
#include "World/WaterTarget/UOUWaterBasinReactionComponentBase.h"
#include "World/WaterTarget/UOUWaterBasinTargetComponent.h"

#if !UOU_WITH_DEVELOPMENT_TOOLS
#error UOUDevelopmentDebugDrawSubsystem must only be compiled when development tools are enabled.
#endif

namespace UOUDevelopmentDebugDrawPrivate
{
	constexpr float PuzzleProviderRefreshIntervalSeconds = 1.0f;
	constexpr float PerformanceUpdateIntervalSeconds = 0.5f;
	constexpr float VFXUpdateIntervalSeconds = 0.5f;

	FString FormatMemoryGB(uint64 Bytes)
	{
		constexpr double BytesPerGB = 1024.0 * 1024.0 * 1024.0;
		return FString::Printf(TEXT("%.2f GB"), static_cast<double>(Bytes) / BytesPerGB);
	}

	FString FormatCompactCount(int32 Count)
	{
		if (Count >= 1000000)
		{
			return FString::Printf(TEXT("%.1fM"), Count / 1000000.0f);
		}

		if (Count >= 10000)
		{
			return FString::Printf(TEXT("%.1fK"), Count / 1000.0f);
		}

		return FString::FromInt(Count);
	}

	FIntPoint GetFallbackViewportSize(const UWorld* World)
	{
		if (World == nullptr)
		{
			return FIntPoint::ZeroValue;
		}

		const UGameViewportClient* GameViewport = World->GetGameViewport();
		if (GameViewport == nullptr || GameViewport->Viewport == nullptr)
		{
			return FIntPoint::ZeroValue;
		}

		return GameViewport->Viewport->GetSizeXY();
	}

	const TCHAR* GetYesNo(bool bValue)
	{
		return bValue ? TEXT("Yes") : TEXT("No");
	}

	FString GetActorDebugName(const AActor* Actor)
	{
		return Actor != nullptr ? Actor->GetName() : TEXT("None");
	}

	FString GetComponentDebugName(const UActorComponent* Component)
	{
		if (Component == nullptr)
		{
			return TEXT("None");
		}

		const AActor* Owner = Component->GetOwner();
		return Owner != nullptr
			? FString::Printf(TEXT("%s.%s"), *Owner->GetName(), *Component->GetName())
			: Component->GetName();
	}

	FString GetMovementModeName(const UCharacterMovementComponent* MovementComponent)
	{
		if (MovementComponent == nullptr)
		{
			return TEXT("None");
		}

		switch (MovementComponent->MovementMode)
		{
		case MOVE_None:
			return TEXT("None");
		case MOVE_Walking:
			return TEXT("Walking");
		case MOVE_NavWalking:
			return TEXT("NavWalking");
		case MOVE_Falling:
			return TEXT("Falling");
		case MOVE_Swimming:
			return TEXT("Swimming");
		case MOVE_Flying:
			return TEXT("Flying");
		case MOVE_Custom:
			return FString::Printf(TEXT("Custom(%d)"), MovementComponent->CustomMovementMode);
		default:
			return TEXT("Unknown");
		}
	}

	FString GetPathFollowingStatusName(const UPathFollowingComponent* PathFollowingComponent)
	{
		if (PathFollowingComponent == nullptr)
		{
			return TEXT("No Controller");
		}

		switch (PathFollowingComponent->GetStatus())
		{
		case EPathFollowingStatus::Idle:
			return TEXT("Idle");
		case EPathFollowingStatus::Waiting:
			return TEXT("Waiting");
		case EPathFollowingStatus::Paused:
			return TEXT("Paused");
		case EPathFollowingStatus::Moving:
			return TEXT("Moving");
		default:
			return TEXT("Unknown");
		}
	}

	FString GetEnumValueName(const UEnum* Enum, int64 Value)
	{
		return Enum != nullptr ? Enum->GetNameStringByValue(Value) : TEXT("Unknown");
	}

	FString GetUmbrellaStateName(EUOUUmbrellaState UmbrellaState)
	{
		return GetEnumValueName(StaticEnum<EUOUUmbrellaState>(), static_cast<int64>(UmbrellaState));
	}

	FString GetUmbrellaDirectionStateName(EUOUUmbrellaDirectionState UmbrellaState)
	{
		return GetEnumValueName(
			StaticEnum<EUOUUmbrellaDirectionState>(),
			static_cast<int64>(UmbrellaState));
	}

	FString GetPourReceiverTypeName(EUOUUmbrellaPourReceiverType ReceiverType)
	{
		return GetEnumValueName(
			StaticEnum<EUOUUmbrellaPourReceiverType>(),
			static_cast<int64>(ReceiverType));
	}

	FString GetLightInteractionModeName(EUOULightInteractionMode LightInteractionMode)
	{
		return GetEnumValueName(
			StaticEnum<EUOULightInteractionMode>(),
			static_cast<int64>(LightInteractionMode));
	}

	FString FormatVectorCompact(const FVector& Vector)
	{
		return FString::Printf(TEXT("%.0f, %.0f, %.0f"), Vector.X, Vector.Y, Vector.Z);
	}

	FName NormalizeNiagaraUserParameterName(FName ParameterName)
	{
		if (ParameterName.IsNone())
		{
			return NAME_None;
		}

		FString ParameterNameString = ParameterName.ToString();
		if (ParameterNameString.StartsWith(TEXT("User.")))
		{
			ParameterNameString.RightChopInline(5);
		}

		return FName(*ParameterNameString);
	}

	const TCHAR* GetNiagaraTypeDebugName(const FNiagaraTypeDefinition& Type)
	{
		if (Type == FNiagaraTypeDefinition::GetPositionDef())
		{
			return TEXT("Position");
		}
		if (Type == FNiagaraTypeDefinition::GetVec3Def())
		{
			return TEXT("Vec3");
		}
		if (Type == FNiagaraTypeDefinition::GetVec2Def())
		{
			return TEXT("Vec2");
		}
		if (Type == FNiagaraTypeDefinition::GetFloatDef())
		{
			return TEXT("Float");
		}
		if (Type == FNiagaraTypeDefinition::GetBoolDef())
		{
			return TEXT("Bool");
		}
		if (Type == FNiagaraTypeDefinition::GetIntDef())
		{
			return TEXT("Int");
		}
		return TEXT("Other");
	}

	const FNiagaraVariableWithOffset* FindNiagaraParameterInStore(
		const FNiagaraParameterStore& Store,
		FName ParameterName)
	{
		const FNiagaraTypeDefinition CandidateTypes[] =
		{
			FNiagaraTypeDefinition::GetPositionDef(),
			FNiagaraTypeDefinition::GetVec3Def(),
			FNiagaraTypeDefinition::GetVec2Def(),
			FNiagaraTypeDefinition::GetFloatDef(),
			FNiagaraTypeDefinition::GetBoolDef(),
			FNiagaraTypeDefinition::GetIntDef()
		};

		for (const FNiagaraTypeDefinition& CandidateType : CandidateTypes)
		{
			const FNiagaraVariable QueryParameter(CandidateType, ParameterName);
			if (const FNiagaraVariableWithOffset* Parameter =
				Store.FindParameterVariable(QueryParameter, false))
			{
				return Parameter;
			}
		}

		return nullptr;
	}

	FString DescribeNiagaraParameterBinding(const UNiagaraComponent* Effect, FName ParameterName)
	{
		if (Effect == nullptr || ParameterName.IsNone())
		{
			return TEXT("ParamDebug: invalid effect/name");
		}

		TArray<FName, TInlineAllocator<3>> NamesToCheck;
		NamesToCheck.AddUnique(ParameterName);
		const FName NormalizedName = NormalizeNiagaraUserParameterName(ParameterName);
		NamesToCheck.AddUnique(NormalizedName);
		if (!NormalizedName.ToString().StartsWith(TEXT("User.")))
		{
			NamesToCheck.AddUnique(FName(*FString::Printf(TEXT("User.%s"), *NormalizedName.ToString())));
		}

		auto AppendMatchingParameters = [](FString& OutText, const TCHAR* Label, const FNiagaraParameterStore& Store)
		{
			TArray<FNiagaraVariable> Parameters;
			Store.GetParameters(Parameters);
			OutText += FString::Printf(TEXT("\n%s RainBlocker:"), Label);
			int32 MatchCount = 0;
			for (const FNiagaraVariable& Parameter : Parameters)
			{
				const FString CandidateName = Parameter.GetName().ToString();
				if (!CandidateName.Contains(TEXT("RainBlocker")))
				{
					continue;
				}

				++MatchCount;
				OutText += FString::Printf(
					TEXT(" %s:%s"),
					*CandidateName,
					GetNiagaraTypeDebugName(Parameter.GetType()));
			}

			if (MatchCount == 0)
			{
				OutText += TEXT(" none");
			}
		};

		FString Result = FString::Printf(TEXT("Param %s |"), *ParameterName.ToString());
		const UNiagaraSystem* NiagaraSystem = Effect->GetAsset();
		for (const FName NameToCheck : NamesToCheck)
		{
			const FNiagaraVariableWithOffset* SystemParameter = NiagaraSystem != nullptr
				? FindNiagaraParameterInStore(NiagaraSystem->GetExposedParameters(), NameToCheck)
				: nullptr;
			const FNiagaraVariableWithOffset* OverrideParameter =
				FindNiagaraParameterInStore(Effect->GetOverrideParameters(), NameToCheck);
			Result += FString::Printf(
				TEXT(" %s Sys:%s Ovr:%s |"),
				*NameToCheck.ToString(),
				SystemParameter != nullptr ? GetNiagaraTypeDebugName(SystemParameter->GetType()) : TEXT("-"),
				OverrideParameter != nullptr ? GetNiagaraTypeDebugName(OverrideParameter->GetType()) : TEXT("-"));
		}
		if (NiagaraSystem != nullptr)
		{
			AppendMatchingParameters(Result, TEXT("SysAll"), NiagaraSystem->GetExposedParameters());
		}
		AppendMatchingParameters(Result, TEXT("OvrAll"), Effect->GetOverrideParameters());

		return Result;
	}

	bool TryGetDebugObjectLocation(const UObject* Object, FVector& OutLocation)
	{
		if (const AActor* Actor = Cast<AActor>(Object))
		{
			OutLocation = Actor->GetActorLocation();
			return true;
		}

		if (const USceneComponent* SceneComponent = Cast<USceneComponent>(Object))
		{
			OutLocation = SceneComponent->GetComponentLocation();
			return true;
		}

		if (const UUOUPuzzleDebugProviderComponent* PuzzleDebugProvider =
			Cast<UUOUPuzzleDebugProviderComponent>(Object))
		{
			OutLocation = PuzzleDebugProvider->GetConditionGroupNodeWorldLocation();
			return true;
		}

		if (const UActorComponent* ActorComponent = Cast<UActorComponent>(Object))
		{
			if (const AActor* Owner = ActorComponent->GetOwner())
			{
				OutLocation = Owner->GetActorLocation();
				return true;
			}
		}

		return false;
	}

	AActor* GetDebugObjectOwnerActor(UObject* Object)
	{
		if (AActor* Actor = Cast<AActor>(Object))
		{
			return Actor;
		}

		const UActorComponent* ActorComponent = Cast<UActorComponent>(Object);
		return ActorComponent != nullptr ? ActorComponent->GetOwner() : nullptr;
	}

	FString BuildPuzzleProviderLabelText(UObject* ProviderObject)
	{
		if (!IsValid(ProviderObject))
		{
			return FString();
		}

		const FText DisplayName = IUOUDebugProvider::Execute_GetDebugDisplayName(ProviderObject);
		const FText SummaryText = IUOUDebugProvider::Execute_GetDebugSummaryText(ProviderObject);
		FString LabelText = DisplayName.IsEmpty()
			? ProviderObject->GetName()
			: DisplayName.ToString();

		if (!SummaryText.IsEmpty())
		{
			LabelText += LINE_TERMINATOR;
			LabelText += SummaryText.ToString();
		}

		return LabelText;
	}

	void DrawLightPathSegment(
		UWorld* World,
		const FUOULightPathSegmentData& Segment,
		const FColor& Color)
	{
		if (World == nullptr)
		{
			return;
		}

		const FVector Start = Segment.Start;
		FVector End = Segment.End;
		FVector Direction = (End - Start).GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			Direction = Segment.Direction.GetSafeNormal();
			End = Start + Direction * FMath::Max(0.0f, Segment.Length);
		}
		if (Direction.IsNearlyZero())
		{
			return;
		}

		DrawDebugLine(World, Start, End, Color, false, 0.0f, 0, 2.0f);
		DrawDebugPoint(World, End, 8.0f, Color, false, 0.0f);

		const float StartRadius = FMath::Max(0.0f, Segment.StartRadius);
		const float EndRadius = FMath::Max(0.0f, Segment.EndRadius);
		if (StartRadius <= KINDA_SMALL_NUMBER && EndRadius <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		FVector RadiusAxisX = FVector::ZeroVector;
		FVector RadiusAxisY = FVector::ZeroVector;
		Direction.FindBestAxisVectors(RadiusAxisX, RadiusAxisY);
		constexpr int32 SegmentCount = 12;
		for (int32 Index = 0; Index < SegmentCount; ++Index)
		{
			const float Angle = UE_TWO_PI * static_cast<float>(Index)
				/ static_cast<float>(SegmentCount);
			const float NextAngle = UE_TWO_PI * static_cast<float>(Index + 1)
				/ static_cast<float>(SegmentCount);
			const FVector RadiusDirection =
				RadiusAxisX * FMath::Cos(Angle) + RadiusAxisY * FMath::Sin(Angle);
			const FVector NextRadiusDirection =
				RadiusAxisX * FMath::Cos(NextAngle) + RadiusAxisY * FMath::Sin(NextAngle);
			const FVector StartPoint = Start + RadiusDirection * StartRadius;
			const FVector NextStartPoint = Start + NextRadiusDirection * StartRadius;
			const FVector EndPoint = End + RadiusDirection * EndRadius;
			const FVector NextEndPoint = End + NextRadiusDirection * EndRadius;

			DrawDebugLine(World, StartPoint, EndPoint, Color, false, 0.0f, 0, 1.0f);
			if (StartRadius > KINDA_SMALL_NUMBER)
			{
				DrawDebugLine(
					World,
					StartPoint,
					NextStartPoint,
					Color,
					false,
					0.0f,
					0,
					1.0f);
			}
			if (EndRadius > KINDA_SMALL_NUMBER)
			{
				DrawDebugLine(
					World,
					EndPoint,
					NextEndPoint,
					Color,
					false,
					0.0f,
					0,
					1.0f);
			}
		}
	}
}

bool UUOUDevelopmentDebugDrawSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World != nullptr
		&& World->IsGameWorld()
		&& World->GetNetMode() != NM_DedicatedServer;
}

void UUOUDevelopmentDebugDrawSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UUOUDevelopmentDebugControlSubsystem>();
	if (UWorld* World = GetWorld())
	{
		DebugControlSubsystem = World->GetSubsystem<UUOUDevelopmentDebugControlSubsystem>();
	}
	PuzzleProviderRefreshTimeRemaining = 0.0f;
}

void UUOUDevelopmentDebugDrawSubsystem::Deinitialize()
{
	PlayerDebugText.Reset();
	ResetPerformanceDebugState();
	ResetVFXDebugState();
	PuzzleDebugProviders.Reset();
	PuzzleProviderRefreshTimeRemaining = 0.0f;
	DebugControlSubsystem.Reset();
	Super::Deinitialize();
}

void UUOUDevelopmentDebugDrawSubsystem::Tick(float DeltaTime)
{
	const UUOUDevelopmentDebugControlSubsystem* ControlSubsystem = DebugControlSubsystem.Get();
	if (ControlSubsystem == nullptr || !ControlSubsystem->IsDebugToolsEnabled())
	{
		PlayerDebugText.Reset();
		ResetPerformanceDebugState();
		ResetVFXDebugState();
		return;
	}

	if (ControlSubsystem->IsDebugCategoryEnabled(EUOUDebugCategory::Player))
	{
		RefreshPlayerDebugText();
		DrawPlayerUmbrellaRainBlockerDebug();
		DrawPlayerUmbrellaPourTraceDebug();
		DrawPlayerUmbrellaPourPlacementDebug();
	}
	else
	{
		PlayerDebugText.Reset();
	}

	if (ControlSubsystem->IsDebugCategoryEnabled(EUOUDebugCategory::Interaction))
	{
		DrawInteractionDebug();
	}

	if (ControlSubsystem->IsDebugCategoryEnabled(EUOUDebugCategory::NPC))
	{
		DrawNPCDebug();
	}

	if (ControlSubsystem->IsDebugCategoryEnabled(EUOUDebugCategory::Performance))
	{
		RefreshPerformanceDebugText(DeltaTime);
	}
	else
	{
		ResetPerformanceDebugState();
	}

	if (ControlSubsystem->IsDebugCategoryEnabled(EUOUDebugCategory::VFX))
	{
		RefreshVFXDebugData(DeltaTime);
		DrawVFXOwnerLabels();
		DrawRainAreaVFXDebug();
		DrawEnvironmentVisualDebug();
	}
	else
	{
		ResetVFXDebugState();
	}

	if (!ControlSubsystem->IsDebugCategoryEnabled(EUOUDebugCategory::Puzzle))
	{
		return;
	}

	DrawUmbrellaLightReflectorDebug();
	DrawLightExposureSourceDebug();
	DrawSelectedPuzzleInfo();

	PuzzleProviderRefreshTimeRemaining -= DeltaTime;
	if (PuzzleProviderRefreshTimeRemaining <= 0.0f)
	{
		RefreshPuzzleDebugProviders();
		PuzzleProviderRefreshTimeRemaining =
			UOUDevelopmentDebugDrawPrivate::PuzzleProviderRefreshIntervalSeconds;
	}

	DrawPuzzleProviderCustomDebug();
	DrawPuzzleProviderConnections();
	DrawPuzzleProviderLabels();
}

void UUOUDevelopmentDebugDrawSubsystem::RefreshPlayerDebugText()
{
	UWorld* World = GetWorld();
	const APlayerController* LocalPlayerController = World != nullptr
		? World->GetFirstPlayerController()
		: nullptr;
	APawn* PlayerPawn = LocalPlayerController != nullptr ? LocalPlayerController->GetPawn() : nullptr;
	if (PlayerPawn == nullptr)
	{
		PlayerDebugText = TEXT("Player\nPawn: None");
		return;
	}
	if (!ShouldDrawActor(PlayerPawn))
	{
		PlayerDebugText = TEXT("Player\nSelect the player actor to inspect it.");
		return;
	}

	AUOUCharacter* UOUCharacter = Cast<AUOUCharacter>(PlayerPawn);
	ACharacter* Character = Cast<ACharacter>(PlayerPawn);
	const UCharacterMovementComponent* MovementComponent = Character != nullptr
		? Character->GetCharacterMovement()
		: nullptr;
	const UUOUUmbrellaComponent* UmbrellaComponent =
		PlayerPawn->FindComponentByClass<UUOUUmbrellaComponent>();
	const UUOUPushPullInteractorComponent* PushPullComponent = UOUCharacter != nullptr
		? UOUCharacter->GetPushPullInteractorComponent()
		: PlayerPawn->FindComponentByClass<UUOUPushPullInteractorComponent>();
	const UUOUInteractionComponent* InteractionComponent =
		PlayerPawn->FindComponentByClass<UUOUInteractionComponent>();
	const UUOUCameraControllerComponent* CameraControllerComponent = UOUCharacter != nullptr
		? UOUCharacter->GetCameraControllerComponent()
		: PlayerPawn->FindComponentByClass<UUOUCameraControllerComponent>();
	const UUOUUmbrellaLightInteractionComponent* UmbrellaLightInteractionComponent =
		PlayerPawn->FindComponentByClass<UUOUUmbrellaLightInteractionComponent>();

	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("Player: %s"), *PlayerPawn->GetName()));

	const FVector Velocity = PlayerPawn->GetVelocity();
	TArray<FString> JumpBlockedReasons;
	if (PushPullComponent != nullptr && PushPullComponent->BlocksJumping())
	{
		JumpBlockedReasons.Add(TEXT("PushPull"));
	}
	if (UmbrellaComponent != nullptr && UmbrellaComponent->BlocksJumping())
	{
		JumpBlockedReasons.Add(TEXT("Umbrella"));
	}

	Lines.Add(FString::Printf(
		TEXT("State: %s | Grounded: %s | Speed: %.1f"),
		*UOUDevelopmentDebugDrawPrivate::GetMovementModeName(MovementComponent),
		UOUDevelopmentDebugDrawPrivate::GetYesNo(
			MovementComponent != nullptr && MovementComponent->IsMovingOnGround()),
		Velocity.Size2D()));
	Lines.Add(FString::Printf(
		TEXT("Location: %s | ZVel: %.1f"),
		*UOUDevelopmentDebugDrawPrivate::FormatVectorCompact(PlayerPawn->GetActorLocation()),
		Velocity.Z));
	Lines.Add(FString::Printf(
		TEXT("Jump: %s | Blocked: %s"),
		UOUDevelopmentDebugDrawPrivate::GetYesNo(Character != nullptr && Character->CanJump()),
		JumpBlockedReasons.Num() > 0 ? *FString::Join(JumpBlockedReasons, TEXT(", ")) : TEXT("No")));

	const bool bPhysicalContextKeyDown = LocalPlayerController != nullptr
		&& LocalPlayerController->IsInputKeyDown(EKeys::RightMouseButton);
	const bool bRoutesToPour = UmbrellaComponent != nullptr
		&& UmbrellaComponent->HasUmbrella()
		&& UmbrellaComponent->IsUpsideDown()
		&& UmbrellaComponent->GetCurrentStoredWater() > 0.0f;
	Lines.Add(FString::Printf(
		TEXT("Input: RMB %s | Context Route: %s"),
		UOUDevelopmentDebugDrawPrivate::GetYesNo(bPhysicalContextKeyDown),
		bRoutesToPour ? TEXT("Pour") : TEXT("PushPull")));
	if (UOUCharacter != nullptr)
	{
		Lines.Add(FString::Printf(
			TEXT("Counts: Context %d/%d | PushPull %d/%d"),
			UOUCharacter->GetContextInteractPressedCount(),
			UOUCharacter->GetContextInteractReleasedCount(),
			UOUCharacter->GetPushPullPressedCount(),
			UOUCharacter->GetPushPullReleasedCount()));
	}

	if (UmbrellaComponent == nullptr)
	{
		Lines.Add(TEXT("Umbrella: None"));
	}
	else
	{
		const float StoredWater = UmbrellaComponent->GetCurrentStoredWater();
		const float MaxWater = UmbrellaComponent->StoredWaterContainer != nullptr
			? UmbrellaComponent->StoredWaterContainer->MaxAmount
			: 0.0f;
		const float RainExposure = UmbrellaComponent->GetCurrentPlayerRainAmount();
		const float MaxRainExposure = UmbrellaComponent->RainReceiver != nullptr
			? UmbrellaComponent->RainReceiver->MaxExposure
			: 0.0f;

		Lines.Add(FString::Printf(
			TEXT("Umbrella: Owned %s | State: %s | Direction: %s"),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(UmbrellaComponent->HasUmbrella()),
			*UOUDevelopmentDebugDrawPrivate::GetUmbrellaStateName(UmbrellaComponent->CurrentState),
			*UOUDevelopmentDebugDrawPrivate::GetUmbrellaDirectionStateName(
				UmbrellaComponent->CurrentDirectionState)));
		Lines.Add(FString::Printf(
			TEXT("Water: %.2f / %.2f | Rain: %.2f / %.2f"),
			StoredWater,
			MaxWater,
			RainExposure,
			MaxRainExposure));
		Lines.Add(FString::Printf(
			TEXT("Collect: %s | BlockRain: %s | BlocksJump: %s"),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(UmbrellaComponent->CanCollectWater()),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(UmbrellaComponent->IsBlockingRain()),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(UmbrellaComponent->BlocksJumping())));

		if (UmbrellaLightInteractionComponent != nullptr)
		{
			const UUOULightInteractionSurfaceComponent* LightSurfaceComponent =
				UmbrellaLightInteractionComponent->LightSurfaceComponent;
			Lines.Add(FString::Printf(
				TEXT("Light Surface: %s | Mode: %s"),
				LightSurfaceComponent != nullptr ? *LightSurfaceComponent->GetName() : TEXT("None"),
				LightSurfaceComponent != nullptr
					? *UOUDevelopmentDebugDrawPrivate::GetLightInteractionModeName(
						LightSurfaceComponent->LightInteractionMode)
					: TEXT("None")));
		}

		Lines.Add(FString::Printf(
			TEXT("Pour: %s | Hit: %s | Delivered: %s"),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(UmbrellaComponent->IsPouring()),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(UmbrellaComponent->bLastPourTraceHit),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(UmbrellaComponent->bLastPourDeliveredWater)));
		Lines.Add(FString::Printf(
			TEXT("Pour Target: %s | Receiver: %s"),
			*UmbrellaComponent->LastPourTargetName,
			*UOUDevelopmentDebugDrawPrivate::GetPourReceiverTypeName(
				UmbrellaComponent->LastPourReceiverType)));
		Lines.Add(FString::Printf(
			TEXT("Pour Amount: %.2f | Stored: %.2f -> %.2f"),
			UmbrellaComponent->LastPourAmount,
			UmbrellaComponent->LastPourStoredWaterBefore,
			UmbrellaComponent->LastPourStoredWaterAfter));
	}

	if (InteractionComponent == nullptr)
	{
		Lines.Add(TEXT("Interaction: None"));
	}
	else
	{
		Lines.Add(FString::Printf(
			TEXT("Interaction: Enabled %s | Candidate: %s"),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(InteractionComponent->bInteractionEnabled),
			*UOUDevelopmentDebugDrawPrivate::GetComponentDebugName(
				InteractionComponent->CurrentCandidateComponent)));
		Lines.Add(FString::Printf(
			TEXT("Interaction Range: %.1f | Radius: %.1f"),
			InteractionComponent->InteractionRange,
			InteractionComponent->InteractionProbeRadius));
	}

	if (PushPullComponent == nullptr)
	{
		Lines.Add(TEXT("PushPull: None"));
	}
	else
	{
		const UUOUPushPullObjectComponent* CandidateObject =
			PushPullComponent->GetCurrentCandidateObject();
		const UUOUPushPullObjectComponent* GrabbedObject = PushPullComponent->GetGrabbedObject();
		Lines.Add(FString::Printf(
			TEXT("PushPull: Candidate %s | Grabbed %s"),
			CandidateObject != nullptr
				? *UOUDevelopmentDebugDrawPrivate::GetActorDebugName(CandidateObject->GetOwner())
				: TEXT("None"),
			GrabbedObject != nullptr
				? *UOUDevelopmentDebugDrawPrivate::GetActorDebugName(GrabbedObject->GetOwner())
				: TEXT("None")));
		Lines.Add(FString::Printf(
			TEXT("Hands: %s | Held: %s | TooFar: %s"),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(PushPullComponent->CanUseHandsForDebug()),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(PushPullComponent->IsGrabInputHeld()),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(
				PushPullComponent->IsGrabbedObjectTooFarForDebug())));
		Lines.Add(FString::Printf(
			TEXT("Axis: %s | Input: %.2f | Last: %s"),
			*UOUDevelopmentDebugDrawPrivate::FormatVectorCompact(
				PushPullComponent->GetGrabbedMoveAxis()),
			PushPullComponent->GetCurrentAxisInput(),
			*PushPullComponent->GetLastFailureReason()));
	}

	if (CameraControllerComponent != nullptr)
	{
		Lines.Add(FString::Printf(
			TEXT("Camera: Yaw %.1f -> %.1f | Dist %.1f -> %.1f | Occluded %d"),
			CameraControllerComponent->GetMovementYaw(),
			CameraControllerComponent->GetTargetCameraYaw(),
			CameraControllerComponent->GetCurrentCameraDistance(),
			CameraControllerComponent->GetTargetCameraDistance(),
			CameraControllerComponent->GetOccludedMeshCount()));
	}

	PlayerDebugText = FString::Join(Lines, LINE_TERMINATOR);
}

void UUOUDevelopmentDebugDrawSubsystem::DrawPlayerUmbrellaRainBlockerDebug() const
{
	UWorld* World = GetWorld();
	const APlayerController* PlayerController = World != nullptr
		? World->GetFirstPlayerController()
		: nullptr;
	const APawn* PlayerPawn = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
	const UUOUUmbrellaComponent* UmbrellaComponent = PlayerPawn != nullptr
		? PlayerPawn->FindComponentByClass<UUOUUmbrellaComponent>()
		: nullptr;
	if (World == nullptr || !ShouldDrawActor(PlayerPawn) || UmbrellaComponent == nullptr)
	{
		return;
	}

	FVector BlockerWorldCenter = FVector::ZeroVector;
	FRotator BlockerWorldRotation = FRotator::ZeroRotator;
	FVector BlockerHalfExtent = FVector::ZeroVector;
	if (!UmbrellaComponent->TryGetGameplayRainBlockerVolumeData(
			BlockerWorldCenter,
			BlockerWorldRotation,
			BlockerHalfExtent))
	{
		return;
	}

	constexpr float Thickness = 2.0f;
	const bool bIsActiveBlocker = UmbrellaComponent->IsBlockingRain();
	const FColor PlayerDebugColor = bIsActiveBlocker
		? FColor::Cyan
		: FColor(90, 90, 90);
	const FVector RotationAxisZ = BlockerWorldRotation.Quaternion().GetAxisZ();

	DrawDebugSphere(
		World,
		BlockerWorldCenter,
		8.0f,
		12,
		PlayerDebugColor,
		false,
		0.0f,
		0,
		Thickness);
	DrawDebugBox(
		World,
		BlockerWorldCenter,
		BlockerHalfExtent,
		BlockerWorldRotation.Quaternion(),
		PlayerDebugColor,
		false,
		0.0f,
		0,
		Thickness);
	DrawDebugLine(
		World,
		BlockerWorldCenter + RotationAxisZ * BlockerHalfExtent.Z,
		BlockerWorldCenter - RotationAxisZ * BlockerHalfExtent.Z,
		PlayerDebugColor,
		false,
		0.0f,
		0,
		Thickness);

	const FVector& LocalOffset = UmbrellaComponent->GetRainBlockerLocalOffset();
	DrawDebugString(
		World,
		BlockerWorldCenter + RotationAxisZ * (BlockerHalfExtent.Z + 18.0f),
		FString::Printf(
			TEXT("Gameplay RainBlocker %s Half %.1f %.1f %.1f Offset %.1f %.1f %.1f"),
			bIsActiveBlocker ? TEXT("Active") : TEXT("Inactive"),
			BlockerHalfExtent.X,
			BlockerHalfExtent.Y,
			BlockerHalfExtent.Z,
			LocalOffset.X,
			LocalOffset.Y,
			LocalOffset.Z),
		nullptr,
		PlayerDebugColor,
		0.0f,
		false,
		1.0f);
}

void UUOUDevelopmentDebugDrawSubsystem::DrawPlayerUmbrellaPourTraceDebug() const
{
	UWorld* World = GetWorld();
	const APlayerController* PlayerController = World != nullptr
		? World->GetFirstPlayerController()
		: nullptr;
	const APawn* PlayerPawn = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
	const UUOUUmbrellaComponent* UmbrellaComponent = PlayerPawn != nullptr
		? PlayerPawn->FindComponentByClass<UUOUUmbrellaComponent>()
		: nullptr;
	if (World == nullptr
		|| !ShouldDrawActor(PlayerPawn)
		|| UmbrellaComponent == nullptr
		|| !UmbrellaComponent->bHasLastPourTrace)
	{
		return;
	}

	constexpr float Thickness = 3.0f;
	constexpr float ImpactCrossSize = 18.0f;
	const FVector DrawEnd = UmbrellaComponent->bLastPourTraceHit
		? UmbrellaComponent->LastPourTraceImpactPoint
		: UmbrellaComponent->LastPourTraceEnd;
	const FColor TraceColor = UmbrellaComponent->bLastPourDeliveredWater
		? FColor::Green
		: (UmbrellaComponent->bLastPourTraceHit ? FColor::Red : FColor::Cyan);
	const FColor ImpactPointColor = UmbrellaComponent->bLastPourCheckedWaterBasinImpactPoint
		? (UmbrellaComponent->bLastPourImpactPointInsideWaterBasin ? FColor::Green : FColor::Red)
		: FColor::Orange;

	DrawDebugLine(
		World,
		UmbrellaComponent->LastPourTraceStart,
		DrawEnd,
		TraceColor,
		false,
		0.0f,
		0,
		Thickness);
	DrawDebugSphere(
		World,
		UmbrellaComponent->LastPourTraceStart,
		6.0f,
		12,
		TraceColor,
		false,
		0.0f,
		0,
		Thickness);

	if (UmbrellaComponent->bLastPourTraceHit)
	{
		const FVector& ImpactPoint = UmbrellaComponent->LastPourTraceImpactPoint;
		DrawDebugSphere(
			World,
			ImpactPoint,
			8.0f,
			12,
			ImpactPointColor,
			false,
			0.0f,
			0,
			Thickness);
		DrawDebugLine(
			World,
			ImpactPoint - FVector(ImpactCrossSize, 0.0f, 0.0f),
			ImpactPoint + FVector(ImpactCrossSize, 0.0f, 0.0f),
			ImpactPointColor,
			false,
			0.0f,
			0,
			Thickness);
		DrawDebugLine(
			World,
			ImpactPoint - FVector(0.0f, ImpactCrossSize, 0.0f),
			ImpactPoint + FVector(0.0f, ImpactCrossSize, 0.0f),
			ImpactPointColor,
			false,
			0.0f,
			0,
			Thickness);
		DrawDebugLine(
			World,
			ImpactPoint - FVector(0.0f, 0.0f, ImpactCrossSize),
			ImpactPoint + FVector(0.0f, 0.0f, ImpactCrossSize),
			ImpactPointColor,
			false,
			0.0f,
			0,
			Thickness);
	}

	const FString ImpactPointText = UmbrellaComponent->bLastPourTraceHit
		? FString::Printf(
			TEXT("\nImpactPoint: X %.1f / Y %.1f / Z %.1f"),
			UmbrellaComponent->LastPourTraceImpactPoint.X,
			UmbrellaComponent->LastPourTraceImpactPoint.Y,
			UmbrellaComponent->LastPourTraceImpactPoint.Z)
		: FString();
	const FString WaterBasinImpactText = UmbrellaComponent->bLastPourCheckedWaterBasinImpactPoint
		? FString::Printf(
			TEXT("\nBasin Check: %s"),
			UmbrellaComponent->bLastPourImpactPointInsideWaterBasin
				? TEXT("Inside")
				: TEXT("Outside"))
		: FString();
	const FString LabelText = FString::Printf(
		TEXT("Pour Trace\nHit: %s\nTarget: %s\nReceiver: %s\nAmount: %.2f\nStored: %.2f -> %.2f%s%s"),
		*UmbrellaComponent->LastPourHitName,
		*UmbrellaComponent->LastPourTargetName,
		*UOUDevelopmentDebugDrawPrivate::GetPourReceiverTypeName(
			UmbrellaComponent->LastPourReceiverType),
		UmbrellaComponent->LastPourAmount,
		UmbrellaComponent->LastPourStoredWaterBefore,
		UmbrellaComponent->LastPourStoredWaterAfter,
		*ImpactPointText,
		*WaterBasinImpactText);
	DrawDebugString(
		World,
		DrawEnd + FVector(0.0f, 0.0f, 24.0f),
		LabelText,
		nullptr,
		TraceColor,
		0.0f,
		true,
		1.0f);
}

void UUOUDevelopmentDebugDrawSubsystem::DrawPlayerUmbrellaPourPlacementDebug() const
{
	UWorld* World = GetWorld();
	const APlayerController* PlayerController = World != nullptr
		? World->GetFirstPlayerController()
		: nullptr;
	const APawn* PlayerPawn = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
	const UUOUUmbrellaComponent* UmbrellaComponent = PlayerPawn != nullptr
		? PlayerPawn->FindComponentByClass<UUOUUmbrellaComponent>()
		: nullptr;
	if (World == nullptr)
	{
		return;
	}

	constexpr float Radius = 10.0f;
	constexpr float Thickness = 2.0f;
	const bool bDrawPlayerPlacement = ShouldDrawActor(PlayerPawn)
		&& UmbrellaComponent != nullptr
		&& UmbrellaComponent->HasUmbrella();
	if (bDrawPlayerPlacement)
	{
		FTransform SocketTransform = FTransform::Identity;
		if (UmbrellaComponent->TryGetPouringPointTransform(SocketTransform))
		{
			const FVector SocketLocation = SocketTransform.GetLocation();
			const USkeletalMeshComponent* SocketSource =
				UmbrellaComponent->GetPouringSocketSourceComponent();
			const FString SocketDebugText = FString::Printf(
				TEXT("PourSocket\nComponent: %s\nMesh: %s\nSocket: %s\nOffset: %.1f %.1f %.1f"),
				*GetNameSafe(SocketSource),
				SocketSource != nullptr
					? *GetNameSafe(SocketSource->GetSkeletalMeshAsset())
					: TEXT("None"),
				*UmbrellaComponent->GetPouringSocketName().ToString(),
				UmbrellaComponent->GetPouringSocketWorldUnitOffset().X,
				UmbrellaComponent->GetPouringSocketWorldUnitOffset().Y,
				UmbrellaComponent->GetPouringSocketWorldUnitOffset().Z);
			DrawDebugSphere(
				World, SocketLocation, Radius, 16, FColor::Magenta, false, 0.0f, 0, Thickness);
			DrawDebugCoordinateSystem(
				World,
				SocketLocation,
				SocketTransform.Rotator(),
				Radius * 2.5f,
				false,
				0.0f,
				0,
				Thickness);
			DrawDebugString(
				World,
				SocketLocation + FVector(0.0f, 0.0f, Radius + 18.0f),
				SocketDebugText,
				nullptr,
				FColor::Magenta,
				0.0f,
				true);
		}

		FVector DropLocation = FVector::ZeroVector;
		FVector DropDirection = FVector::ForwardVector;
		if (UmbrellaComponent->TryGetPourDropSpawnPlacement(DropLocation, DropDirection))
		{
			const FVector SafeDirection = DropDirection.IsNearlyZero()
				? FVector::DownVector
				: DropDirection.GetSafeNormal();
			DrawDebugSphere(
				World, DropLocation, Radius * 0.7f, 16, FColor::Yellow, false, 0.0f, 0, Thickness);
			DrawDebugLine(
				World,
				DropLocation,
				DropLocation + SafeDirection * 120.0f,
				FColor::Yellow,
				false,
				0.0f,
				0,
				Thickness);
			DrawDebugLine(
				World,
				DropLocation,
				DropLocation + FVector::DownVector * 120.0f,
				FColor::Cyan,
				false,
				0.0f,
				0,
				Thickness);
			DrawDebugString(
				World,
				DropLocation + FVector(0.0f, 0.0f, Radius + 36.0f),
				TEXT("DropSpawn"),
				nullptr,
				FColor::Yellow,
				0.0f,
				true);
		}
	}

	for (TActorIterator<AUOUPourDropActor> It(World); It; ++It)
	{
		const AUOUPourDropActor* DropActor = *It;
		const USphereComponent* CollisionComponent = IsValid(DropActor)
			? DropActor->CollisionComponent
			: nullptr;
		if (!ShouldDrawActor(DropActor) || CollisionComponent == nullptr)
		{
			continue;
		}

		DrawDebugSphere(
			World,
			CollisionComponent->GetComponentLocation(),
			FMath::Max(1.0f, CollisionComponent->GetScaledSphereRadius()),
			24,
			DropActor->bHasDeliveredWater ? FColor::Green : FColor::Cyan,
			false,
			0.0f,
			0,
			1.5f);
	}
}

void UUOUDevelopmentDebugDrawSubsystem::DrawNPCDebug() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	for (TActorIterator<AUOUNPCCharacter> It(World); It; ++It)
	{
		const AUOUNPCCharacter* NPC = *It;
		if (!IsValid(NPC) || !ShouldDrawActor(NPC))
		{
			continue;
		}

		const FUOUNPCActionRequest ActionRequest = NPC->GetCurrentActionRequest();
		const AAIController* AIController = Cast<AAIController>(NPC->GetController());
		const UPathFollowingComponent* PathFollowingComponent = AIController != nullptr
			? AIController->GetPathFollowingComponent()
			: nullptr;
		FVector TargetLocation = FVector::ZeroVector;
		const bool bHasTargetLocation = NPC->GetCurrentActionTargetLocation(TargetLocation);

		TArray<FString> Lines;
		Lines.Add(UOUDevelopmentDebugDrawPrivate::GetActorDebugName(NPC));
		Lines.Add(FString::Printf(
			TEXT("Activated: %s"),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(NPC->bActivated)));
		Lines.Add(FString::Printf(
			TEXT("Action Request: %s"),
			NPC->bHasActiveActionRequest ? TEXT("Active") : TEXT("Legacy")));
		Lines.Add(FString::Printf(
			TEXT("Action: %s"),
			*UOUDevelopmentDebugDrawPrivate::GetEnumValueName(
				StaticEnum<EUOUNPCActionType>(),
				static_cast<int64>(ActionRequest.ActionType))));
		Lines.Add(FString::Printf(TEXT("Source: %s"), *GetNameSafe(NPC->ActiveActionSource.Get())));
		Lines.Add(FString::Printf(
			TEXT("Movement: %s"),
			*UOUDevelopmentDebugDrawPrivate::GetMovementModeName(NPC->GetCharacterMovement())));
		Lines.Add(FString::Printf(
			TEXT("Path: %s"),
			*UOUDevelopmentDebugDrawPrivate::GetPathFollowingStatusName(PathFollowingComponent)));
		Lines.Add(FString::Printf(
			TEXT("Pending Jump Move: %s"),
			UOUDevelopmentDebugDrawPrivate::GetYesNo(NPC->bPendingMoveAfterJumpLanding)));

		if (ActionRequest.bUseTargetActor)
		{
			Lines.Add(FString::Printf(
				TEXT("Target Actor: %s"),
				*UOUDevelopmentDebugDrawPrivate::GetActorDebugName(ActionRequest.TargetActor.Get())));
		}
		else
		{
			Lines.Add(FString::Printf(
				TEXT("Target Location: %s"),
				*ActionRequest.TargetLocation.ToCompactString()));
		}

		if (bHasTargetLocation)
		{
			Lines.Add(FString::Printf(
				TEXT("Distance: %.1f / %.1f"),
				FVector::Dist2D(NPC->GetActorLocation(), TargetLocation),
				NPC->GetCurrentActionAcceptanceRadius()));
		}

		const UAnimMontage* RequestedMontage = ActionRequest.AnimationMontage != nullptr
			? ActionRequest.AnimationMontage.Get()
			: NPC->ActivationMontage.Get();
		const USkeletalMeshComponent* MeshComponent = NPC->GetMesh();
		UAnimInstance* AnimInstance = MeshComponent != nullptr
			? MeshComponent->GetAnimInstance()
			: nullptr;
		const UAnimMontage* ActiveMontage = AnimInstance != nullptr
			? AnimInstance->GetCurrentActiveMontage()
			: nullptr;
		Lines.Add(FString::Printf(TEXT("Requested Montage: %s"), *GetNameSafe(RequestedMontage)));
		Lines.Add(FString::Printf(TEXT("Active Montage: %s"), *GetNameSafe(ActiveMontage)));
		Lines.Add(FString::Printf(TEXT("Anim Rate: %.2f"), ActionRequest.AnimationPlayRate));
		Lines.Add(FString::Printf(
			TEXT("Anim Section: %s"),
			*ActionRequest.AnimationStartSection.ToString()));

		DrawDebugString(
			World,
			NPC->GetActorLocation()
				+ FVector(0.0f, 0.0f, NPC->GetSimpleCollisionHalfHeight() + 60.0f),
			FString::Join(Lines, LINE_TERMINATOR),
			nullptr,
			FColor::White,
			0.0f,
			true,
			0.9f);

		if (bHasTargetLocation)
		{
			const FVector StartLocation = NPC->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
			const FVector EndLocation = TargetLocation + FVector(0.0f, 0.0f, 40.0f);
			const float AcceptanceRadius = FMath::Max(
				0.0f,
				NPC->GetCurrentActionAcceptanceRadius());
			DrawDebugDirectionalArrow(
				World, StartLocation, EndLocation, 80.0f, FColor::Green, false, 0.0f, 0, 2.0f);
			DrawDebugSphere(World, TargetLocation, 24.0f, 12, FColor::Green, false, 0.0f, 0, 2.0f);

			if (AcceptanceRadius > 0.0f)
			{
				DrawDebugCylinder(
					World,
					TargetLocation,
					TargetLocation + FVector(0.0f, 0.0f, 80.0f),
					AcceptanceRadius,
					24,
					FColor::Green,
					false,
					0.0f,
					0,
					1.0f);
			}
		}

		const FNavPathSharedPtr Path = PathFollowingComponent != nullptr
			? PathFollowingComponent->GetPath()
			: nullptr;
		if (!Path.IsValid())
		{
			continue;
		}

		const TArray<FNavPathPoint>& PathPoints = Path->GetPathPoints();
		for (int32 Index = 0; Index < PathPoints.Num(); ++Index)
		{
			const FVector PointLocation =
				PathPoints[Index].Location + FVector(0.0f, 0.0f, 20.0f);
			DrawDebugSphere(World, PointLocation, 12.0f, 8, FColor::Cyan, false, 0.0f, 0, 1.0f);

			if (Index > 0)
			{
				const FVector PreviousLocation =
					PathPoints[Index - 1].Location + FVector(0.0f, 0.0f, 20.0f);
				DrawDebugLine(
					World,
					PreviousLocation,
					PointLocation,
					FColor::Cyan,
					false,
					0.0f,
					0,
					2.0f);
			}
		}
	}
}

void UUOUDevelopmentDebugDrawSubsystem::DrawInteractionDebug() const
{
	UWorld* World = GetWorld();
	const APlayerController* PlayerController = World != nullptr
		? World->GetFirstPlayerController()
		: nullptr;
	const APawn* PlayerPawn = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
	const UUOUPushPullInteractorComponent* PushPullComponent = PlayerPawn != nullptr
		? PlayerPawn->FindComponentByClass<UUOUPushPullInteractorComponent>()
		: nullptr;
	if (World == nullptr || !ShouldDrawActor(PlayerPawn) || PushPullComponent == nullptr)
	{
		return;
	}

	const FVector DetectionOrigin = PushPullComponent->GetCandidateDetectionOriginLocation();
	FVector CandidateLocation = FVector::ZeroVector;
	const bool bHasCandidate =
		PushPullComponent->TryGetCurrentCandidateReferenceLocation(CandidateLocation);
	DrawDebugSphere(
		World,
		DetectionOrigin,
		PushPullComponent->GetCandidateSearchRadius(),
		24,
		bHasCandidate ? FColor::Green : FColor::Cyan,
		false,
		0.0f,
		0,
		1.5f);

	const FColor InteractionColor = FColor::Orange;
	if (bHasCandidate)
	{
		DrawDebugLine(
			World,
			DetectionOrigin,
			CandidateLocation,
			InteractionColor,
			false,
			0.0f,
			0,
			2.0f);
		DrawDebugSphere(
			World,
			CandidateLocation,
			14.0f,
			12,
			InteractionColor,
			false,
			0.0f,
			0,
			1.5f);
	}

	FVector GrabbedLocation = FVector::ZeroVector;
	if (PushPullComponent->TryGetCurrentGrabbedReferenceLocation(GrabbedLocation))
	{
		DrawDebugDirectionalArrow(
			World,
			GrabbedLocation,
			GrabbedLocation + PushPullComponent->GetGrabbedMoveAxis() * 100.0f,
			25.0f,
			InteractionColor,
			false,
			0.0f,
			0,
			3.0f);
	}
}

void UUOUDevelopmentDebugDrawSubsystem::RefreshPerformanceDebugText(float DeltaTime)
{
	const float SafeDeltaTime = FMath::Max(0.0f, DeltaTime);
	PerformanceAccumulatedDeltaTime += SafeDeltaTime;
	++PerformanceSampleCount;
	PerformanceUpdateTimeRemaining -= SafeDeltaTime;
	if (PerformanceUpdateTimeRemaining > 0.0f && !PerformanceDebugText.IsEmpty())
	{
		return;
	}

	const float AverageDeltaTime = PerformanceSampleCount > 0
		? PerformanceAccumulatedDeltaTime / PerformanceSampleCount
		: SafeDeltaTime;
	const float FPS = AverageDeltaTime > KINDA_SMALL_NUMBER ? 1.0f / AverageDeltaTime : 0.0f;

	TArray<FString> Lines;
	Lines.Add(TEXT("Performance"));
	Lines.Add(FString::Printf(TEXT("FPS: %.1f | Frame: %.2f ms"), FPS, AverageDeltaTime * 1000.0f));
	Lines.Add(FString::Printf(
		TEXT("Game: %.2f ms | Draw: %.2f ms | RHI: %.2f ms"),
		FPlatformTime::ToMilliseconds(GGameThreadTime),
		FPlatformTime::ToMilliseconds(GRenderThreadTime),
		FPlatformTime::ToMilliseconds(GRHIThreadTime)));
	Lines.Add(FString::Printf(
		TEXT("GPU: %.2f ms | Input: %.2f ms"),
		FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()),
		FPlatformTime::ToMilliseconds64(GInputLatencyTime)));

	const FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();
	Lines.Add(FString::Printf(
		TEXT("Memory: %s"),
		*UOUDevelopmentDebugDrawPrivate::FormatMemoryGB(MemoryStats.UsedPhysical)));

	float ScreenPercentage = GPixelRenderCounters.GetResolutionFraction() * 100.0f;
	FIntPoint RenderResolution = GPixelRenderCounters.GetRenderResolution();
	if (ScreenPercentage <= 0.0f || RenderResolution.X <= 0 || RenderResolution.Y <= 0)
	{
		ScreenPercentage = 100.0f;
		RenderResolution = UOUDevelopmentDebugDrawPrivate::GetFallbackViewportSize(GetWorld());
	}

	Lines.Add(FString::Printf(
		TEXT("RenderRes: %.1f%% (%dx%d) | Draws: %d | Prims: %s"),
		ScreenPercentage,
		RenderResolution.X,
		RenderResolution.Y,
		GNumDrawCallsRHI[0],
		*UOUDevelopmentDebugDrawPrivate::FormatCompactCount(GNumPrimitivesDrawnRHI[0])));

	int32 ActorCount = 0;
	int32 ComponentCount = 0;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!IsValid(Actor))
			{
				continue;
			}

			++ActorCount;
			TInlineComponentArray<UActorComponent*> Components(Actor);
			ComponentCount += Components.Num();
		}
	}

	int32 WorldCount = 0;
	if (GEngine != nullptr)
	{
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			if (WorldContext.World() != nullptr)
			{
				++WorldCount;
			}
		}
	}

	Lines.Add(FString::Printf(
		TEXT("Actors: %d | Components: %d | Worlds: %d"),
		ActorCount,
		ComponentCount,
		WorldCount));

	PerformanceDebugText = FString::Join(Lines, LINE_TERMINATOR);
	PerformanceAccumulatedDeltaTime = 0.0f;
	PerformanceSampleCount = 0;
	PerformanceUpdateTimeRemaining =
		UOUDevelopmentDebugDrawPrivate::PerformanceUpdateIntervalSeconds;
}

void UUOUDevelopmentDebugDrawSubsystem::ResetPerformanceDebugState()
{
	PerformanceDebugText.Reset();
	PerformanceUpdateTimeRemaining = 0.0f;
	PerformanceAccumulatedDeltaTime = 0.0f;
	PerformanceSampleCount = 0;
}

void UUOUDevelopmentDebugDrawSubsystem::RefreshVFXDebugData(float DeltaTime)
{
	const UUOUDevelopmentDebugControlSubsystem* ControlSubsystem = DebugControlSubsystem.Get();
	const TArray<AActor*> SelectedActors = ControlSubsystem != nullptr
		? ControlSubsystem->GetSelectedDebugActors()
		: TArray<AActor*>();
	TArray<TWeakObjectPtr<AActor>> WeakSelectedActors;
	WeakSelectedActors.Reserve(SelectedActors.Num());
	for (AActor* SelectedActor : SelectedActors)
	{
		if (IsValid(SelectedActor))
		{
			WeakSelectedActors.Add(SelectedActor);
		}
	}

	if (VFXCachedSelectedActors != WeakSelectedActors)
	{
		VFXUpdateTimeRemaining = 0.0f;
	}

	VFXUpdateTimeRemaining -= FMath::Max(0.0f, DeltaTime);
	if (VFXUpdateTimeRemaining > 0.0f && !VFXDebugText.IsEmpty())
	{
		return;
	}

	VFXOwnerLabelTexts.Reset();
	VFXOwnerLabelLocations.Reset();

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		VFXDebugText = TEXT("VFX\nWorld: None");
		VFXUpdateTimeRemaining = UOUDevelopmentDebugDrawPrivate::VFXUpdateIntervalSeconds;
		return;
	}
	if (SelectedActors.IsEmpty())
	{
		VFXDebugText = TEXT("VFX\nSelect a VFX actor to inspect it.");
		VFXUpdateTimeRemaining = UOUDevelopmentDebugDrawPrivate::VFXUpdateIntervalSeconds;
		VFXCachedSelectedActors.Reset();
		return;
	}

	struct FVFXOwnerStats
	{
		int32 ActiveNiagaraCount = 0;
		int32 ActiveCascadeCount = 0;
		int32 LocationSampleCount = 0;
		FVector AccumulatedLocation = FVector::ZeroVector;
	};

	int32 TotalNiagaraCount = 0;
	int32 ActiveNiagaraCount = 0;
	int32 TotalCascadeCount = 0;
	int32 ActiveCascadeCount = 0;
	TMap<AActor*, FVFXOwnerStats> OwnerStats;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || !ShouldDrawActor(Actor))
		{
			continue;
		}

		TArray<UNiagaraComponent*> NiagaraComponents;
		Actor->GetComponents<UNiagaraComponent>(NiagaraComponents);
		TotalNiagaraCount += NiagaraComponents.Num();
		for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
		{
			if (!IsValid(NiagaraComponent)
				|| !NiagaraComponent->IsRegistered()
				|| !NiagaraComponent->IsActive())
			{
				continue;
			}

			++ActiveNiagaraCount;
			FVFXOwnerStats& Stats = OwnerStats.FindOrAdd(Actor);
			++Stats.ActiveNiagaraCount;
			++Stats.LocationSampleCount;
			Stats.AccumulatedLocation += NiagaraComponent->GetComponentLocation();
		}

		TArray<UParticleSystemComponent*> CascadeComponents;
		Actor->GetComponents<UParticleSystemComponent>(CascadeComponents);
		TotalCascadeCount += CascadeComponents.Num();
		for (UParticleSystemComponent* CascadeComponent : CascadeComponents)
		{
			if (!IsValid(CascadeComponent)
				|| !CascadeComponent->IsRegistered()
				|| !CascadeComponent->IsActive())
			{
				continue;
			}

			++ActiveCascadeCount;
			FVFXOwnerStats& Stats = OwnerStats.FindOrAdd(Actor);
			++Stats.ActiveCascadeCount;
			++Stats.LocationSampleCount;
			Stats.AccumulatedLocation += CascadeComponent->GetComponentLocation();
		}
	}

	VFXDebugText = FString::Printf(
		TEXT("VFX\nNiagara Components: %d / %d active\nCascade Components: %d / %d active\nOwners: %d"),
		ActiveNiagaraCount,
		TotalNiagaraCount,
		ActiveCascadeCount,
		TotalCascadeCount,
		OwnerStats.Num());

	VFXOwnerLabelTexts.Reserve(OwnerStats.Num());
	VFXOwnerLabelLocations.Reserve(OwnerStats.Num());
	for (const TPair<AActor*, FVFXOwnerStats>& Pair : OwnerStats)
	{
		const AActor* Owner = Pair.Key;
		const FVFXOwnerStats& Stats = Pair.Value;
		if (!IsValid(Owner) || !ShouldDrawActor(Owner) || Stats.LocationSampleCount <= 0)
		{
			continue;
		}

		VFXOwnerLabelTexts.Add(FString::Printf(
			TEXT("VFX: %s\nNiagara: %d\nCascade: %d"),
			*Owner->GetName(),
			Stats.ActiveNiagaraCount,
			Stats.ActiveCascadeCount));
		VFXOwnerLabelLocations.Add(
			Stats.AccumulatedLocation / static_cast<float>(Stats.LocationSampleCount)
			+ FVector(0.0f, 0.0f, 120.0f));
	}

	VFXUpdateTimeRemaining = UOUDevelopmentDebugDrawPrivate::VFXUpdateIntervalSeconds;
	VFXCachedSelectedActors = MoveTemp(WeakSelectedActors);
}

void UUOUDevelopmentDebugDrawSubsystem::DrawVFXOwnerLabels() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const int32 LabelCount = FMath::Min(VFXOwnerLabelTexts.Num(), VFXOwnerLabelLocations.Num());
	for (int32 Index = 0; Index < LabelCount; ++Index)
	{
		DrawDebugString(
			World,
			VFXOwnerLabelLocations[Index],
			VFXOwnerLabelTexts[Index],
			nullptr,
			FColor::Cyan,
			0.0f,
			true,
			0.9f);
	}
}

void UUOUDevelopmentDebugDrawSubsystem::DrawRainAreaVFXDebug() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	constexpr float Thickness = 2.0f;
	for (TActorIterator<AUOUUmbrellaRainArea> It(World); It; ++It)
	{
		const AUOUUmbrellaRainArea* RainArea = *It;
		const UBoxComponent* RainVolume = IsValid(RainArea)
			? RainArea->GetRainVolumeComponent()
			: nullptr;
		if (!ShouldDrawActor(RainArea) || RainVolume == nullptr)
		{
			continue;
		}

		const FVector BoxExtent = RainVolume->GetScaledBoxExtent();
		const FVector VolumeCenter = RainVolume->GetComponentLocation();
		const FQuat VolumeRotation = RainVolume->GetComponentQuat();
		const FVector VolumeUp = RainVolume->GetUpVector();
		const bool bFlowUpward =
			RainArea->GetFlowDirection() == EUOURainAreaFlowDirection::Upward;
		const FVector RainSpawnPlaneWorldPosition = bFlowUpward
			? VolumeCenter - VolumeUp * BoxExtent.Z
			: VolumeCenter + VolumeUp * BoxExtent.Z;
		const FVector GroundSplashWorldPosition = bFlowUpward
			? VolumeCenter + VolumeUp * (BoxExtent.Z - RainArea->GetGroundSplashHeightOffset())
			: VolumeCenter - VolumeUp * (BoxExtent.Z - RainArea->GetGroundSplashHeightOffset());
		const FVector VisualAreaHalfExtent(BoxExtent.X, BoxExtent.Y, 2.0f);

		DrawDebugBox(
			World, VolumeCenter, BoxExtent, VolumeRotation, FColor::Cyan, false, 0.0f, 0, Thickness);
		DrawDebugBox(
			World,
			RainSpawnPlaneWorldPosition,
			VisualAreaHalfExtent,
			VolumeRotation,
			FColor::Cyan,
			false,
			0.0f,
			0,
			Thickness);
		DrawDebugBox(
			World,
			GroundSplashWorldPosition,
			VisualAreaHalfExtent,
			VolumeRotation,
			FColor::Cyan,
			false,
			0.0f,
			0,
			Thickness);
		DrawDebugLine(
			World,
			GroundSplashWorldPosition,
			RainSpawnPlaneWorldPosition,
			FColor::Cyan,
			false,
			0.0f,
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
			FColor::Cyan,
			0.0f,
			false,
			1.0f);
	}
}

void UUOUDevelopmentDebugDrawSubsystem::DrawEnvironmentVisualDebug() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	constexpr float Thickness = 2.0f;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || !ShouldDrawActor(Actor))
		{
			continue;
		}

		TArray<UUOUEnvironmentVisualComponent*> VisualComponents;
		Actor->GetComponents<UUOUEnvironmentVisualComponent>(VisualComponents);
		for (const UUOUEnvironmentVisualComponent* VisualComponent : VisualComponents)
		{
			FVector BlockerWorldCenter = FVector::ZeroVector;
			FVector BlockerHalfExtent = FVector::ZeroVector;
			float BlockerIntensity = 0.0f;
			if (!IsValid(VisualComponent)
				|| !VisualComponent->GetRainBlockerState(
					BlockerWorldCenter,
					BlockerHalfExtent,
					BlockerIntensity))
			{
				continue;
			}

			TArray<UNiagaraComponent*> Effects;
			VisualComponent->GetRegisteredEffectComponents(Effects);
			for (const UNiagaraComponent* Effect : Effects)
			{
				if (!IsValid(Effect))
				{
					continue;
				}

				const FVector EffectLocalBlockerPosition =
					Effect->GetComponentTransform().InverseTransformPosition(BlockerWorldCenter);
				DrawDebugBox(
					World,
					BlockerWorldCenter,
					BlockerHalfExtent,
					Effect->GetComponentQuat(),
					FColor::Magenta,
					false,
					0.0f,
					0,
					Thickness);
				DrawDebugSphere(
					World,
					BlockerWorldCenter,
					6.0f,
					8,
					FColor::Magenta,
					false,
					0.0f,
					0,
					Thickness);
				DrawDebugString(
					World,
					BlockerWorldCenter + FVector(0.0f, 0.0f, BlockerHalfExtent.Z + 36.0f),
					FString::Printf(
						TEXT("RB Local %.1f %.1f %.1f\nIntensity %.2f"),
						EffectLocalBlockerPosition.X,
						EffectLocalBlockerPosition.Y,
						EffectLocalBlockerPosition.Z,
						BlockerIntensity),
					nullptr,
					FColor::Magenta,
					0.0f,
					false,
					1.0f);

				if (GEngine != nullptr)
				{
					const uint64 DebugKey =
						0x554F55000000ull + static_cast<uint64>(PointerHash(Effect));
					GEngine->AddOnScreenDebugMessage(
						DebugKey,
						0.0f,
						FColor::Magenta,
						FString::Printf(
							TEXT("%s\nRainBlockerLocal %.1f %.1f %.1f\n%s"),
							*Effect->GetName(),
							EffectLocalBlockerPosition.X,
							EffectLocalBlockerPosition.Y,
							EffectLocalBlockerPosition.Z,
							*UOUDevelopmentDebugDrawPrivate::DescribeNiagaraParameterBinding(
								Effect,
								VisualComponent->GetRainBlockerLocalPositionParameterName())));
				}
			}
		}
	}
}

void UUOUDevelopmentDebugDrawSubsystem::ResetVFXDebugState()
{
	VFXDebugText.Reset();
	VFXOwnerLabelTexts.Reset();
	VFXOwnerLabelLocations.Reset();
	VFXUpdateTimeRemaining = 0.0f;
	VFXCachedSelectedActors.Reset();
}

TStatId UUOUDevelopmentDebugDrawSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UUOUDevelopmentDebugDrawSubsystem, STATGROUP_Tickables);
}

void UUOUDevelopmentDebugDrawSubsystem::DrawUmbrellaLightReflectorDebug() const
{
	UWorld* World = GetWorld();
	const UUOUDevelopmentDebugControlSubsystem* ControlSubsystem = DebugControlSubsystem.Get();
	const TArray<AActor*> SelectedActors = ControlSubsystem != nullptr
		? ControlSubsystem->GetSelectedDebugActors()
		: TArray<AActor*>();
	if (World == nullptr || SelectedActors.IsEmpty())
	{
		return;
	}

	constexpr float ReflectorArrowLength = 180.0f;
	constexpr float ReflectorThickness = 3.0f;
	for (AActor* SelectedActor : SelectedActors)
	{
		if (!IsValid(SelectedActor))
		{
			continue;
		}

		TArray<UUOUUmbrellaLightInteractionComponent*> LightInteractionComponents;
		SelectedActor->GetComponents<UUOUUmbrellaLightInteractionComponent>(
			LightInteractionComponents);
		for (const UUOUUmbrellaLightInteractionComponent* LightInteractionComponent
			: LightInteractionComponents)
		{
			if (!IsValid(LightInteractionComponent)
				|| !IsValid(LightInteractionComponent->LightSurfaceComponent))
			{
				continue;
			}

			const UUOULightInteractionSurfaceComponent* LightSurfaceComponent =
				LightInteractionComponent->LightSurfaceComponent;
			const EUOULightInteractionMode InteractionMode =
				LightSurfaceComponent->LightInteractionMode;
			const bool bShadeActive =
				IsValid(LightInteractionComponent->LightShadeVolumeComponent)
				&& LightInteractionComponent->LightShadeVolumeComponent->CanShadeLight();
			const bool bDebugBlocking = InteractionMode == EUOULightInteractionMode::Blocking
				|| (InteractionMode == EUOULightInteractionMode::Disabled && bShadeActive);
			const FColor SurfaceColor = InteractionMode == EUOULightInteractionMode::Reflecting
				? FColor::Magenta
				: (bDebugBlocking ? FColor::Yellow : FColor::Silver);
			const FVector SurfaceLocation = LightSurfaceComponent->GetComponentLocation();
			const FVector SurfaceExtent = LightSurfaceComponent->GetScaledBoxExtent();

			DrawDebugBox(
				World,
				SurfaceLocation,
				SurfaceExtent,
				LightSurfaceComponent->GetComponentQuat(),
				SurfaceColor,
				false,
				0.0f,
				0,
				ReflectorThickness);

			const AActor* Owner = LightInteractionComponent->GetOwner();
			const FVector ReflectionDirection = Owner != nullptr
				? Owner->GetActorForwardVector().GetSafeNormal()
				: LightSurfaceComponent->GetForwardVector().GetSafeNormal();
			if (InteractionMode == EUOULightInteractionMode::Reflecting
				&& !ReflectionDirection.IsNearlyZero())
			{
				DrawDebugDirectionalArrow(
					World,
					SurfaceLocation,
					SurfaceLocation + ReflectionDirection * ReflectorArrowLength,
					24.0f,
					FColor::Green,
					false,
					0.0f,
					0,
					ReflectorThickness);
			}

			const TCHAR* ModeText = InteractionMode == EUOULightInteractionMode::Reflecting
				? TEXT("Reflecting")
				: (bDebugBlocking ? TEXT("Blocking") : TEXT("Disabled"));
			DrawDebugString(
				World,
				SurfaceLocation + FVector(0.0f, 0.0f, SurfaceExtent.Z + 20.0f),
				FString::Printf(
					TEXT("Umbrella Reflector: %s\nExtent: %.1f %.1f %.1f"),
					ModeText,
					SurfaceExtent.X,
					SurfaceExtent.Y,
					SurfaceExtent.Z),
				nullptr,
				SurfaceColor,
				0.0f,
				false,
				1.0f);
		}
	}
}

void UUOUDevelopmentDebugDrawSubsystem::DrawLightExposureSourceDebug() const
{
	UWorld* World = GetWorld();
	const UUOUDevelopmentDebugControlSubsystem* ControlSubsystem = DebugControlSubsystem.Get();
	const TArray<AActor*> SelectedActors = ControlSubsystem != nullptr
		? ControlSubsystem->GetSelectedDebugActors()
		: TArray<AActor*>();
	if (World == nullptr || SelectedActors.IsEmpty())
	{
		return;
	}

	for (AActor* SelectedActor : SelectedActors)
	{
		if (!IsValid(SelectedActor))
		{
			continue;
		}

		TArray<UUOULightExposureSourceComponent*> SourceComponents;
		SelectedActor->GetComponents<UUOULightExposureSourceComponent>(SourceComponents);
		for (int32 SourceIndex = 0; SourceIndex < SourceComponents.Num(); ++SourceIndex)
		{
			const UUOULightExposureSourceComponent* SourceComponent =
				SourceComponents[SourceIndex];
			if (!IsValid(SourceComponent))
			{
				continue;
			}

			FVector LabelLocation = SelectedActor->GetActorLocation()
				+ FVector(0.0f, 0.0f, 180.0f + static_cast<float>(SourceIndex) * 140.0f);
			bool bHasPathStart = false;
			for (const FUOULightPathData& Path : SourceComponent->LightPaths)
			{
				for (const FUOULightPathSegmentData& Segment : Path.Segments)
				{
					if (!bHasPathStart)
					{
						LabelLocation = Segment.Start
							+ FVector(
								0.0f,
								0.0f,
								80.0f + static_cast<float>(SourceIndex) * 140.0f);
						bHasPathStart = true;
					}

					FColor SegmentColor = Segment.bReflected ? FColor::Cyan : FColor::Yellow;
					switch (Segment.HitType)
					{
					case EUOULightPathHitType::Receiver:
						SegmentColor = FColor::Green;
						break;
					case EUOULightPathHitType::BlockingSurface:
						SegmentColor = FColor::Red;
						break;
					case EUOULightPathHitType::ReflectingSurface:
						SegmentColor = FColor::Magenta;
						break;
					case EUOULightPathHitType::None:
					default:
						break;
					}

					UOUDevelopmentDebugDrawPrivate::DrawLightPathSegment(
						World,
						Segment,
						SegmentColor);

					for (UObject* ReceiverObject : Segment.ReachedReceivers)
					{
						if (!IsValid(ReceiverObject)
							|| !ReceiverObject->GetClass()->ImplementsInterface(
								UUOULightReceivableInterface::StaticClass()))
						{
							continue;
						}

						const FVector ReceiverLocation =
							IUOULightReceivableInterface::Execute_GetLightReceiverPosition(
								ReceiverObject);
						DrawDebugPoint(
							World,
							ReceiverLocation,
							12.0f,
							FColor::Green,
							false,
							0.0f);
					}
				}
			}

			const FString SummaryText = FString::Printf(
				TEXT("Light Source: %s\nPaths %d / Reflections %d\nReceivers %d | Lit %d | Reflected %d | Blocked %d\nLast Lit: %s\nLast Blocked: %s\nReflection: %s"),
				SourceComponent->bEmitLight ? TEXT("On") : TEXT("Off"),
				SourceComponent->LightPaths.Num(),
				SourceComponent->ReflectionPaths.Num(),
				SourceComponent->LastReceiverCount,
				SourceComponent->LastLitCount,
				SourceComponent->LastReflectedCount,
				SourceComponent->LastBlockedCount,
				*SourceComponent->LastLitTargetName,
				*SourceComponent->LastBlockedName,
				*SourceComponent->LastReflectionPath);
			DrawDebugString(
				World,
				LabelLocation,
				SummaryText,
				nullptr,
				SourceComponent->bEmitLight ? FColor::Yellow : FColor::Silver,
				0.0f,
				true,
				0.85f);
		}
	}
}

void UUOUDevelopmentDebugDrawSubsystem::DrawSelectedPuzzleInfo() const
{
	UWorld* World = GetWorld();
	const UUOUDevelopmentDebugControlSubsystem* ControlSubsystem = DebugControlSubsystem.Get();
	const TArray<AActor*> SelectedActors = ControlSubsystem != nullptr
		? ControlSubsystem->GetSelectedDebugActors()
		: TArray<AActor*>();
	if (World == nullptr || SelectedActors.IsEmpty())
	{
		return;
	}

	for (AActor* SelectedActor : SelectedActors)
	{
		if (!IsValid(SelectedActor))
		{
			continue;
		}

		TArray<UObject*> InfoProviders;
		if (SelectedActor->GetClass()->ImplementsInterface(UUOUPuzzleDebugInfoProvider::StaticClass()))
		{
			InfoProviders.Add(SelectedActor);
		}

		TInlineComponentArray<UActorComponent*> Components(SelectedActor);
		for (UActorComponent* Component : Components)
		{
			if (!IsValid(Component)
				|| Component->IsA<UUOULightExposureReceiverComponent>()
				|| Component->IsA<UUOULightExposureSourceComponent>()
				|| Component->IsA<UUOURotatableMirrorComponent>()
				|| Component->IsA<UUOUWaterBasinReactionComponentBase>()
				|| !Component->GetClass()->ImplementsInterface(
					UUOUPuzzleDebugInfoProvider::StaticClass()))
			{
				continue;
			}

			InfoProviders.Add(Component);
		}

		const FBox ActorBounds = SelectedActor->GetComponentsBoundingBox(true);
		const float BaseWorldZ = ActorBounds.IsValid
			? ActorBounds.Max.Z + 180.0f
			: SelectedActor->GetActorLocation().Z + 180.0f;
		for (int32 Index = 0; Index < InfoProviders.Num(); ++Index)
		{
			UObject* Provider = InfoProviders[Index];
			if (!IsValid(Provider))
			{
				continue;
			}

			const TArray<FString> DebugLines =
				IUOUPuzzleDebugInfoProvider::Execute_GetPuzzleDebugInfo(Provider);
			if (DebugLines.IsEmpty())
			{
				continue;
			}

			const FString DebugText = FString::Printf(
				TEXT("%s\n%s"),
				*Provider->GetName(),
				*FString::Join(DebugLines, LINE_TERMINATOR));
			FVector DrawLocation = SelectedActor->GetActorLocation();
			DrawLocation.Z = BaseWorldZ + static_cast<float>(Index) * 150.0f;
			DrawDebugString(
				World,
				DrawLocation,
				DebugText,
				nullptr,
				FColor::White,
				0.0f,
				true,
				0.85f);
		}
	}
}

void UUOUDevelopmentDebugDrawSubsystem::RefreshPuzzleDebugProviders()
{
	PuzzleDebugProviders.Reset();

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor))
		{
			continue;
		}

		TryAddPuzzleDebugProvider(Actor);

		TInlineComponentArray<UActorComponent*> Components(Actor);
		for (UActorComponent* Component : Components)
		{
			TryAddPuzzleDebugProvider(Component);
		}
	}
}

int32 UUOUDevelopmentDebugDrawSubsystem::GetPuzzleDebugProviderCount() const
{
	int32 ValidProviderCount = 0;
	for (const TWeakObjectPtr<UObject>& ProviderObject : PuzzleDebugProviders)
	{
		if (ProviderObject.IsValid())
		{
			++ValidProviderCount;
		}
	}

	return ValidProviderCount;
}

void UUOUDevelopmentDebugDrawSubsystem::GetSelectableDebugActors(
	TArray<FUOUDevelopmentDebugActorEntry>& OutActors) const
{
	OutActors.Reset();
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	TSet<TWeakObjectPtr<AActor>> AddedActors;
	const auto AddActor = [&OutActors, &AddedActors](AActor* Actor, EUOUDebugCategory Category)
	{
		if (!IsValid(Actor))
		{
			return;
		}
		const TWeakObjectPtr<AActor> WeakActor(Actor);
		if (AddedActors.Contains(WeakActor))
		{
			return;
		}

		AddedActors.Add(WeakActor);
		FUOUDevelopmentDebugActorEntry& Entry = OutActors.AddDefaulted_GetRef();
		Entry.Actor = Actor;
		Entry.Category = Category;
	};

	if (const APlayerController* PlayerController = World->GetFirstPlayerController())
	{
		AddActor(PlayerController->GetPawn(), EUOUDebugCategory::Player);
	}

	for (TActorIterator<AUOUPourDropActor> It(World); It; ++It)
	{
		AddActor(*It, EUOUDebugCategory::Player);
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor))
		{
			continue;
		}

		if (Actor->IsA<AUOUNPCCharacter>())
		{
			AddActor(Actor, EUOUDebugCategory::NPC);
			continue;
		}

		bool bPuzzleActor = Actor->IsA<AUOULightSourceActor>()
			|| Actor->IsA<AUOUHeatWireActor>()
			|| Actor->IsA<AUOUPlayerBlockingWallActor>()
			|| Actor->FindComponentByClass<UUOUWeightedButtonComponent>() != nullptr
			|| Actor->FindComponentByClass<UUOUWaterBasinTargetComponent>() != nullptr
			|| Actor->FindComponentByClass<UUOUWaterBasinReactionComponentBase>() != nullptr
			|| Actor->FindComponentByClass<UUOULightExposureSourceComponent>() != nullptr
			|| Actor->FindComponentByClass<UUOULightExposureReceiverComponent>() != nullptr
			|| Actor->FindComponentByClass<UUOURotatableMirrorComponent>() != nullptr;
		if (!bPuzzleActor
			&& Actor->GetClass()->ImplementsInterface(UUOUDebugProvider::StaticClass()))
		{
			bPuzzleActor = IUOUDebugProvider::Execute_GetDebugCategory(Actor)
				== EUOUDebugCategory::Puzzle;
		}
		if (!bPuzzleActor)
		{
			TInlineComponentArray<UActorComponent*> Components(Actor);
			for (UActorComponent* Component : Components)
			{
				if (IsValid(Component)
					&& Component->GetClass()->ImplementsInterface(UUOUDebugProvider::StaticClass())
					&& IUOUDebugProvider::Execute_GetDebugCategory(Component)
						== EUOUDebugCategory::Puzzle)
				{
					bPuzzleActor = true;
					break;
				}
			}
		}
		if (!bPuzzleActor
			&& Actor->GetClass()->ImplementsInterface(
				UUOUPuzzleDebugInfoProvider::StaticClass()))
		{
			bPuzzleActor = true;
		}
		if (!bPuzzleActor)
		{
			TInlineComponentArray<UActorComponent*> Components(Actor);
			for (UActorComponent* Component : Components)
			{
				if (!IsValid(Component)
					|| Component->IsA<UUOULightExposureReceiverComponent>()
					|| Component->IsA<UUOULightExposureSourceComponent>()
					|| Component->IsA<UUOURotatableMirrorComponent>())
				{
					continue;
				}

				if (Component->GetClass()->ImplementsInterface(
					UUOUPuzzleDebugInfoProvider::StaticClass()))
				{
					bPuzzleActor = true;
					break;
				}
			}
		}

		if (bPuzzleActor)
		{
			AddActor(Actor, EUOUDebugCategory::Puzzle);
			continue;
		}

		const bool bVFXActor = Actor->IsA<AUOUUmbrellaRainArea>()
			|| Actor->FindComponentByClass<UUOUEnvironmentVisualComponent>() != nullptr
			|| Actor->FindComponentByClass<UNiagaraComponent>() != nullptr
			|| Actor->FindComponentByClass<UParticleSystemComponent>() != nullptr;
		if (bVFXActor)
		{
			AddActor(Actor, EUOUDebugCategory::VFX);
		}
	}

	OutActors.Sort([](
		const FUOUDevelopmentDebugActorEntry& Left,
		const FUOUDevelopmentDebugActorEntry& Right)
	{
		if (Left.Category != Right.Category)
		{
			return static_cast<uint8>(Left.Category) < static_cast<uint8>(Right.Category);
		}

		return GetNameSafe(Left.Actor.Get()) < GetNameSafe(Right.Actor.Get());
	});
}

bool UUOUDevelopmentDebugDrawSubsystem::ShouldDrawActor(const AActor* Actor) const
{
	const UUOUDevelopmentDebugControlSubsystem* ControlSubsystem =
		DebugControlSubsystem.Get();
	return ControlSubsystem != nullptr && ControlSubsystem->ShouldDrawDebugActor(Actor);
}

void UUOUDevelopmentDebugDrawSubsystem::TryAddPuzzleDebugProvider(UObject* ProviderObject)
{
	if (!IsValid(ProviderObject)
		|| !ProviderObject->GetClass()->ImplementsInterface(UUOUDebugProvider::StaticClass())
		|| IUOUDebugProvider::Execute_GetDebugCategory(ProviderObject) != EUOUDebugCategory::Puzzle)
	{
		return;
	}

	PuzzleDebugProviders.AddUnique(TWeakObjectPtr<UObject>(ProviderObject));
}

void UUOUDevelopmentDebugDrawSubsystem::DrawPuzzleProviderCustomDebug() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	FUOUDevelopmentDebugDrawContext DrawContext(*World);
	for (const TWeakObjectPtr<UObject>& WeakProviderObject : PuzzleDebugProviders)
	{
		UObject* ProviderObject = WeakProviderObject.Get();
		if (!IsValid(ProviderObject)
			|| !ShouldDrawActor(
				UOUDevelopmentDebugDrawPrivate::GetDebugObjectOwnerActor(ProviderObject))
			|| !ProviderObject->GetClass()->ImplementsInterface(UUOUDebugProvider::StaticClass())
			|| !IUOUDebugProvider::Execute_IsDebugProviderEnabled(ProviderObject))
		{
			continue;
		}

		if (const IUOUDebugProvider* NativeProvider = Cast<IUOUDebugProvider>(ProviderObject))
		{
			NativeProvider->GatherDevelopmentDebugDraw(DrawContext);
		}
	}
}

void UUOUDevelopmentDebugDrawSubsystem::DrawPuzzleProviderConnections() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	for (const TWeakObjectPtr<UObject>& WeakProviderObject : PuzzleDebugProviders)
	{
		UObject* ProviderObject = WeakProviderObject.Get();
		if (!IsValid(ProviderObject)
			|| !ShouldDrawActor(
				UOUDevelopmentDebugDrawPrivate::GetDebugObjectOwnerActor(ProviderObject))
			|| !ProviderObject->GetClass()->ImplementsInterface(UUOUDebugProvider::StaticClass())
			|| !IUOUDebugProvider::Execute_IsDebugProviderEnabled(ProviderObject))
		{
			continue;
		}

		TArray<FUOUDebugConnection> Connections;
		IUOUDebugProvider::Execute_GetDebugConnections(ProviderObject, Connections);
		for (const FUOUDebugConnection& Connection : Connections)
		{
			FVector SourceLocation = FVector::ZeroVector;
			FVector TargetLocation = FVector::ZeroVector;
			if (!UOUDevelopmentDebugDrawPrivate::TryGetDebugObjectLocation(
					Connection.SourceObject.Get(),
					SourceLocation)
				|| !UOUDevelopmentDebugDrawPrivate::TryGetDebugObjectLocation(
					Connection.TargetObject.Get(),
					TargetLocation)
				|| SourceLocation.Equals(TargetLocation))
			{
				continue;
			}

			DrawDebugDirectionalArrow(
				World,
				SourceLocation,
				TargetLocation,
				80.0f,
				Connection.Color,
				false,
				0.0f,
				0,
				FMath::Max(0.0f, Connection.Thickness));
		}
	}
}

void UUOUDevelopmentDebugDrawSubsystem::DrawPuzzleProviderLabels() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	for (const TWeakObjectPtr<UObject>& WeakProviderObject : PuzzleDebugProviders)
	{
		UObject* ProviderObject = WeakProviderObject.Get();
		if (!IsValid(ProviderObject)
			|| !ShouldDrawActor(
				UOUDevelopmentDebugDrawPrivate::GetDebugObjectOwnerActor(ProviderObject))
			|| !ProviderObject->GetClass()->ImplementsInterface(UUOUDebugProvider::StaticClass())
			|| !IUOUDebugProvider::Execute_IsDebugProviderEnabled(ProviderObject))
		{
			continue;
		}

		const FString LabelText =
			UOUDevelopmentDebugDrawPrivate::BuildPuzzleProviderLabelText(ProviderObject);
		if (const IUOUDebugProvider* NativeProvider = Cast<IUOUDebugProvider>(ProviderObject);
			NativeProvider != nullptr && !NativeProvider->ShouldDrawDevelopmentDebugLabel())
		{
			continue;
		}

		if (LabelText.IsEmpty())
		{
			continue;
		}

		DrawDebugString(
			World,
			IUOUDebugProvider::Execute_GetDebugWorldLocation(ProviderObject),
			LabelText,
			nullptr,
			FColor::White,
			0.0f,
			true,
			1.0f);
	}
}
