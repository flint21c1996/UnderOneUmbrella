// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "World/Pour/UOUPourDropActor.h"
#include "UOUPourContentProfile.generated.h"

class UMaterialInterface;
class UNiagaraSystem;
class UStaticMesh;

USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUPourStoredVisualSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual", meta = (ToolTip = "When enabled, WaterContainer fill visuals use this profile instead of the component fallback settings."))
	bool bOverrideContainerFillVisual = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual|Mesh", meta = (ToolTip = "Optional mesh applied when the fill visual component is a StaticMeshComponent. Leave empty to keep the component mesh."))
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual|Mesh", meta = (ToolTip = "Optional materials applied to the fill visual mesh. Empty entries are ignored."))
	TArray<TObjectPtr<UMaterialInterface>> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual|Niagara", meta = (ToolTip = "Optional Niagara system applied when the fill visual component is a NiagaraComponent. Leave empty to keep the component asset."))
	TObjectPtr<UNiagaraSystem> NiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual", meta = (ClampMin = "0.0", ToolTip = "Fill ratio interpolation speed for this content. Set to 0 to snap."))
	float FillVisualInterpSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual", meta = (ToolTip = "Relative location offset applied at full fill."))
	FVector FullLocationOffset = FVector(0.0f, 0.0f, 12.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual", meta = (ToolTip = "Scale multiplier at empty fill."))
	FVector EmptyScaleMultiplier = FVector(1.0f, 1.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual", meta = (ToolTip = "Scale multiplier at full fill."))
	FVector FullScaleMultiplier = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual", meta = (ToolTip = "Hide the fill visual only when target and displayed fill are both empty."))
	bool bHideWhenEmpty = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual|Mesh", meta = (ToolTip = "Optional scalar parameter updated on mesh materials. Leave None to disable material parameter updates."))
	FName MeshFillRatioParameterName = TEXT("FillRatio");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual|Niagara", meta = (ToolTip = "Optional Niagara float variable updated from fill ratio. Use a User parameter such as User.FillRatio. Leave None to disable Niagara parameter updates."))
	FName NiagaraFillRatioParameterName = TEXT("User.FillRatio");
};

// Defines how one pourable content type is represented while it is being poured.
UCLASS(BlueprintType, Const, meta = (DisplayName = "UOU Pour Content Profile"))
class UNDERONEUMBRELLA_API UUOUPourContentProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content")
	TSubclassOf<AUOUPourDropActor> DropActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Stored Visual")
	FUOUPourStoredVisualSettings StoredVisual;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Stream Visual", meta = (ToolTip = "Looping Niagara used to show this content while pouring."))
	TObjectPtr<UNiagaraSystem> StreamEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Stream Visual")
	FVector StreamRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Stream Visual", meta = (ToolTip = "Only Yaw is applied after the Niagara component local +Z is aligned with world up and local +Y faces the horizontal pour direction."))
	FRotator StreamRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Stream Visual", meta = (ClampMin = "0.0"))
	FVector StreamRelativeScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Stream Visual|Animation", meta = (ToolTip = "When enabled, stream location animation settings come from this content profile instead of UmbrellaComponent fallback values."))
	bool bOverrideUmbrellaStreamLocationAnimation = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Stream Visual|Animation", meta = (ClampMin = "0.0", EditCondition = "bOverrideUmbrellaStreamLocationAnimation", EditConditionHides, ToolTip = "Seconds used for the stream visual to move from its initial offset to the target pour location. Set to 0 to snap."))
	float StreamLocationInterpDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Stream Visual|Animation", meta = (ClampMin = "1.0", EditCondition = "bOverrideUmbrellaStreamLocationAnimation", EditConditionHides, ToolTip = "Ease-out power used for stream location animation. Higher values slow down more near the final position."))
	float StreamLocationEasePower = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Stream Visual|Animation", meta = (EditCondition = "bOverrideUmbrellaStreamLocationAnimation", EditConditionHides, ToolTip = "World-space offset added to the stream target location when the pour animation alpha is 0. This lets each content start from a different visual position before settling."))
	FVector StreamInitialWorldLocationOffset = FVector::ZeroVector;
};
