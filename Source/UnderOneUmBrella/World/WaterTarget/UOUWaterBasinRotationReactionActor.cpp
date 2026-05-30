// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/WaterTarget/UOUWaterBasinRotationReactionActor.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "World/WaterTarget/UOUWaterBasinRotationReactionComponent.h"

AUOUWaterBasinRotationReactionActor::AUOUWaterBasinRotationReactionActor()
{
	// 실제 회전 갱신은 RotationReactionComponent가 담당하므로 Actor Tick은 필요하지 않습니다.
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
	RootScene->SetMobility(EComponentMobility::Movable);

	RotationCenter = CreateDefaultSubobject<USceneComponent>(TEXT("RotationCenter"));
	RotationCenter->SetupAttachment(RootScene);
	RotationCenter->SetMobility(EComponentMobility::Movable);

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Platform"));
	PlatformMesh->SetupAttachment(RotationCenter);
	PlatformMesh->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		PlatformMesh->SetStaticMesh(CubeMeshAsset.Object);
	}

	ForwardReference = CreateDefaultSubobject<UArrowComponent>(TEXT("ForwardReference"));
	ForwardReference->SetupAttachment(RotationCenter);
	ForwardReference->SetMobility(EComponentMobility::Movable);
	ForwardReference->SetArrowSize(2.0f);
	ForwardReference->SetHiddenInGame(true);

	RotationReaction = CreateDefaultSubobject<UUOUWaterBasinRotationReactionComponent>(TEXT("WaterBasinRotationReaction"));
	RotationReaction->RotationTargetComponent = PlatformMesh;
	RotationReaction->InputSideCenterComponent = RotationCenter;
	RotationReaction->InputSideForwardReferenceComponent = ForwardReference;
	RotationReaction->bUseOwnerRootWhenTargetMissing = false;

	ActivateAction.Command = EUOUWaterBasinRotationReactionPuzzleCommand::EnableReaction;
	DeactivateAction.Command = EUOUWaterBasinRotationReactionPuzzleCommand::DisableReaction;
	PauseAction.Command = EUOUWaterBasinRotationReactionPuzzleCommand::DisableReaction;
	ResumeAction.Command = EUOUWaterBasinRotationReactionPuzzleCommand::EnableReaction;
	ToggleAction.Command = EUOUWaterBasinRotationReactionPuzzleCommand::ToggleReaction;
}

void AUOUWaterBasinRotationReactionActor::PreInitializeComponents()
{
	if (RotationReaction != nullptr)
	{
		RotationReaction->bRotationReactionEnabled = bStartReactionEnabled;
	}

	Super::PreInitializeComponents();
}

void AUOUWaterBasinRotationReactionActor::BeginPlay()
{
	Super::BeginPlay();

	SetReactionEnabled(bStartReactionEnabled);
}

void AUOUWaterBasinRotationReactionActor::SetReactionEnabled(bool bEnabled)
{
	if (RotationReaction == nullptr)
	{
		return;
	}

	RotationReaction->SetRotationReactionEnabled(bEnabled);
}

bool AUOUWaterBasinRotationReactionActor::IsReactionEnabled() const
{
	return RotationReaction != nullptr && RotationReaction->IsRotationReactionEnabled();
}

void AUOUWaterBasinRotationReactionActor::ResetReaction(bool bResetObservedValue, bool bApplyBaseRotation)
{
	if (RotationReaction == nullptr)
	{
		return;
	}

	RotationReaction->ResetRotationReaction(bResetObservedValue, bApplyBaseRotation);
}

void AUOUWaterBasinRotationReactionActor::ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action)
{
	if (const FUOUWaterBasinRotationReactionPuzzleActionSetting* ActionSetting = GetActionSetting(Action))
	{
		ExecuteActionSetting(*ActionSetting);
	}
}

const FUOUWaterBasinRotationReactionPuzzleActionSetting* AUOUWaterBasinRotationReactionActor::GetActionSetting(EOUUPuzzleResultAction Action) const
{
	switch (Action)
	{
	case EOUUPuzzleResultAction::Activate:
		return &ActivateAction;

	case EOUUPuzzleResultAction::Deactivate:
		return &DeactivateAction;

	case EOUUPuzzleResultAction::Pause:
		return &PauseAction;

	case EOUUPuzzleResultAction::Resume:
		return &ResumeAction;

	case EOUUPuzzleResultAction::Toggle:
		return &ToggleAction;

	case EOUUPuzzleResultAction::None:
	default:
		return nullptr;
	}
}

void AUOUWaterBasinRotationReactionActor::ExecuteActionSetting(const FUOUWaterBasinRotationReactionPuzzleActionSetting& Setting)
{
	if (RotationReaction == nullptr)
	{
		return;
	}

	switch (Setting.Command)
	{
	case EUOUWaterBasinRotationReactionPuzzleCommand::EnableReaction:
		SetReactionEnabled(true);
		break;

	case EUOUWaterBasinRotationReactionPuzzleCommand::DisableReaction:
		SetReactionEnabled(false);
		break;

	case EUOUWaterBasinRotationReactionPuzzleCommand::ToggleReaction:
		SetReactionEnabled(!IsReactionEnabled());
		break;

	case EUOUWaterBasinRotationReactionPuzzleCommand::ResetReaction:
		ResetReaction(Setting.bResetObservedValue, Setting.bApplyBaseRotation);
		break;

	case EUOUWaterBasinRotationReactionPuzzleCommand::EnableAndReset:
		SetReactionEnabled(true);
		ResetReaction(Setting.bResetObservedValue, Setting.bApplyBaseRotation);
		break;

	case EUOUWaterBasinRotationReactionPuzzleCommand::DisableAndReset:
		SetReactionEnabled(false);
		ResetReaction(Setting.bResetObservedValue, Setting.bApplyBaseRotation);
		break;

	case EUOUWaterBasinRotationReactionPuzzleCommand::Ignore:
	default:
		break;
	}
}
