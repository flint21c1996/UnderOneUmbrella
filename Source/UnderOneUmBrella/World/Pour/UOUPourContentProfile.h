// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "World/Pour/UOUPourDropActor.h"
#include "UOUPourContentProfile.generated.h"

class UNiagaraSystem;

// Defines how one pourable content type is represented while it is being poured.
UCLASS(BlueprintType, Const, meta = (DisplayName = "UOU Pour Content Profile"))
class UNDERONEUMBRELLA_API UUOUPourContentProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content")
	TSubclassOf<AUOUPourDropActor> DropActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Stream Visual", meta = (ToolTip = "Looping Niagara used to show this content while pouring."))
	TObjectPtr<UNiagaraSystem> StreamEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Stream Visual")
	FVector StreamRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Stream Visual", meta = (ToolTip = "Only Roll is applied so the Niagara component local +Y axis stays aligned with the pour direction."))
	FRotator StreamRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Stream Visual", meta = (ClampMin = "0.0"))
	FVector StreamRelativeScale = FVector::OneVector;
};
