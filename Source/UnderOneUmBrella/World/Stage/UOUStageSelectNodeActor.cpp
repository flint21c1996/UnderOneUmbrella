// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Stage/UOUStageSelectNodeActor.h"

#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Game/UOUMapSelectPlayerController.h"
#include "Game/UOUPlayerProgressSubsystem.h"
#include "Game/UOUStageDefinitionRegistry.h"
#include "Game/UOUStageSelectTypes.h"
#include "GameFramework/Pawn.h"
#include "World/Stage/UOUStageSelectAreaComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

AUOUStageSelectNodeActor::AUOUStageSelectNodeActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	StageSelectArea = CreateDefaultSubobject<UUOUStageSelectAreaComponent>(TEXT("StageSelectArea"));
	StageSelectArea->SetupAttachment(RootScene);
}

void AUOUStageSelectNodeActor::BeginPlay()
{
	if (!bStageSelectionEnabled)
	{
		if (StageSelectArea != nullptr)
		{
			StageSelectArea->SetGenerateOverlapEvents(false);
			StageSelectArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		SetActorTickEnabled(false);
	}

	Super::BeginPlay();

	if (bStageSelectionEnabled && StageSelectArea != nullptr)
	{
		StageSelectArea->OnPlayerEntered.AddUniqueDynamic(
			this,
			&AUOUStageSelectNodeActor::HandlePlayerEnteredStageArea);
		StageSelectArea->OnPlayerExited.AddUniqueDynamic(
			this,
			&AUOUStageSelectNodeActor::HandlePlayerExitedStageArea);
	}
}

bool AUOUStageSelectNodeActor::GetStageDefinition(FUOUStageDefinition& OutStageDefinition) const
{
	OutStageDefinition = FUOUStageDefinition();
	if (!bStageSelectionEnabled)
	{
		return false;
	}

	const FName ResolvedStageId = ResolveStageId();
	if (ResolvedStageId.IsNone())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Stage select node '%s' has no StageId configured."),
			*GetName());
		return false;
	}

	const FString Context = FString::Printf(TEXT("StageSelectNode:%s"), *GetName());
	const FUOUStageDefinitionRow* Row =
		FUOUStageDefinitionRegistry::FindStageById(ResolvedStageId, Context);
	if (Row == nullptr)
	{
		return false;
	}

	OutStageDefinition.StageId = ResolvedStageId;
	OutStageDefinition.DisplayName = Row->DisplayName;
	OutStageDefinition.Description = Row->Description;
	OutStageDefinition.Thumbnail = Row->Thumbnail;
	OutStageDefinition.Level = Row->Level;
	OutStageDefinition.RewardIds = Row->RewardIds;
	OutStageDefinition.TotalRewardCount = Row->RewardIds.Num();
	OutStageDefinition.CollectedRewardCount = 0;
	OutStageDefinition.MissingRewardCount = OutStageDefinition.TotalRewardCount;
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (const UUOUPlayerProgressSubsystem* ProgressSubsystem =
				GameInstance->GetSubsystem<UUOUPlayerProgressSubsystem>())
			{
				ProgressSubsystem->GetStageRewardCounts(
					OutStageDefinition.StageId,
					OutStageDefinition.RewardIds,
					OutStageDefinition.CollectedRewardCount,
					OutStageDefinition.TotalRewardCount,
					OutStageDefinition.MissingRewardCount);
			}
		}
	}
	OutStageDefinition.bUnlocked = Row->bUnlocked;
	return true;
}

TArray<FName> AUOUStageSelectNodeActor::GetAvailableStageIds() const
{
	return FUOUStageDefinitionRegistry::GetStageIds();
}

#if WITH_EDITOR
EDataValidationResult AUOUStageSelectNodeActor::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	bool bIsValid = ParentResult != EDataValidationResult::Invalid;
	auto AddValidationError = [&Context, &bIsValid](const FText& ErrorMessage)
	{
		Context.AddError(ErrorMessage);
		bIsValid = false;
	};

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return ParentResult;
	}

	if (!bStageSelectionEnabled)
	{
		Context.AddWarning(FText::FromString(
			TEXT("Stage Selection이 비활성화되어 이 노드는 런타임 동작과 StageId 검증에서 제외됩니다.")));
		return bIsValid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
	}

	UDataTable* StageDefinitionTable =
		FUOUStageDefinitionRegistry::LoadStageDefinitionTable();
	if (StageDefinitionTable == nullptr)
	{
		AddValidationError(FText::FromString(
			TEXT("Project Settings에 Stage Definition DataTable이 설정되지 않았습니다.")));
		return EDataValidationResult::Invalid;
	}

	if (!FUOUStageDefinitionRegistry::HasValidRowStruct(StageDefinitionTable))
	{
		AddValidationError(FText::FromString(
			TEXT("Stage Definition DataTable의 RowStruct가 FUOUStageDefinitionRow가 아닙니다.")));
		return EDataValidationResult::Invalid;
	}

	const FName ResolvedStageId = ResolveStageId();
	if (ResolvedStageId.IsNone())
	{
		AddValidationError(FText::FromString(TEXT("StageId가 선택되지 않았습니다.")));
	}
	else
	{
		const FString ValidationContext =
			FString::Printf(TEXT("StageSelectNodeValidation:%s"), *GetName());
		if (FUOUStageDefinitionRegistry::FindStageById(ResolvedStageId, ValidationContext) == nullptr)
		{
			AddValidationError(FText::Format(
				FText::FromString(TEXT("StageId '{0}'에 해당하는 DataTable 행이 없습니다.")),
				FText::FromName(ResolvedStageId)));
		}
	}

	if (StageId.IsNone() && !StageRow.RowName.IsNone())
	{
		Context.AddWarning(FText::Format(
			FText::FromString(
				TEXT("기존 StageRow의 '{0}'를 사용 중입니다. StageId로 이전해야 합니다.")),
			FText::FromName(StageRow.RowName)));
	}

	return bIsValid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif

bool AUOUStageSelectNodeActor::ActivateStage()
{
	if (!bStageSelectionEnabled)
	{
		return false;
	}

	FUOUStageDefinition StageDefinition;
	if (!GetStageDefinition(StageDefinition))
	{
		return false;
	}

	const UWorld* World = GetWorld();
	AUOUMapSelectPlayerController* MapSelectController = World != nullptr
		? Cast<AUOUMapSelectPlayerController>(World->GetFirstPlayerController())
		: nullptr;
	return MapSelectController != nullptr && MapSelectController->EnterStage(StageDefinition);
}

FName AUOUStageSelectNodeActor::ResolveStageId() const
{
	return !StageId.IsNone() ? StageId : StageRow.RowName;
}

void AUOUStageSelectNodeActor::HandlePlayerEnteredStageArea(APawn* PlayerPawn)
{
	if (!bStageSelectionEnabled)
	{
		return;
	}

	AUOUMapSelectPlayerController* MapSelectController = PlayerPawn != nullptr
		? Cast<AUOUMapSelectPlayerController>(PlayerPawn->GetController())
		: nullptr;
	if (MapSelectController != nullptr)
	{
		MapSelectController->RegisterStageSelectNode(this);
	}
}

void AUOUStageSelectNodeActor::HandlePlayerExitedStageArea(APawn* PlayerPawn)
{
	if (!bStageSelectionEnabled)
	{
		return;
	}

	AUOUMapSelectPlayerController* MapSelectController = PlayerPawn != nullptr
		? Cast<AUOUMapSelectPlayerController>(PlayerPawn->GetController())
		: nullptr;
	if (MapSelectController != nullptr)
	{
		MapSelectController->UnregisterStageSelectNode(this);
	}
}
