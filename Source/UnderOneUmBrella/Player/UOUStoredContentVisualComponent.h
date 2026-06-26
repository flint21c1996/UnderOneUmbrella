// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "World/Pour/UOUPourContentProfile.h"
#include "UOUStoredContentVisualComponent.generated.h"

class UNiagaraComponent;
class USceneComponent;
class UUOUWaterContainerComponent;

// Drives the visual representation of content stored in a WaterContainerComponent.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class UUOUStoredContentVisualComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UUOUStoredContentVisualComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Container", meta = (ToolTip = "Container that provides stored amount and content profile. Leave empty to auto-find on the owner."))
	TObjectPtr<UUOUWaterContainerComponent> WaterContainerComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Container", meta = (ToolTip = "Optional component name or tag used when auto-finding the WaterContainerComponent. Leave None to use the first container on the owner."))
	FName WaterContainerComponentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Container")
	bool bAutoFindWaterContainerComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual", meta = (ToolTip = "Mesh or Niagara component that is scaled/parameterized by stored fill ratio. Leave empty to auto-find by name or child attachment."))
	TObjectPtr<USceneComponent> StoredVisualComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual", meta = (ToolTip = "Component name or tag used for auto-find."))
	FName StoredVisualComponentName = TEXT("StoredWaterVisual");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual")
	bool bAutoFindStoredVisualComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual")
	bool bUpdateStoredVisual = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual", meta = (ClampMin = "0.0", ToolTip = "Fallback fill ratio interpolation speed when the content profile does not override visual motion. Set to 0 to snap."))
	float FillVisualInterpSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual", meta = (ToolTip = "Fallback relative location offset applied at full fill."))
	FVector FullLocationOffset = FVector(0.0f, 0.0f, 12.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual", meta = (ToolTip = "Fallback scale multiplier at empty fill."))
	FVector EmptyScaleMultiplier = FVector(1.0f, 1.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual", meta = (ToolTip = "Fallback scale multiplier at full fill."))
	FVector FullScaleMultiplier = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual", meta = (ToolTip = "Hide the stored visual only when target and displayed fill are both empty."))
	bool bHideWhenEmpty = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Mesh", meta = (ToolTip = "Fallback scalar parameter updated on mesh materials. Leave None to disable material parameter updates."))
	FName MeshFillRatioParameterName = TEXT("FillRatio");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Niagara", meta = (ToolTip = "Fallback Niagara float variable updated from fill ratio. Use a User parameter such as User.FillRatio. Leave None to disable Niagara parameter updates."))
	FName NiagaraFillRatioParameterName = TEXT("User.FillRatio");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Niagara", meta = (ToolTip = "When enabled, Niagara visuals activate while visible and deactivate when hidden."))
	bool bAutoActivateNiagara = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Niagara", meta = (ToolTip = "When enabled, Niagara plane visuals keep their initial relative scale and use location/parameters for fill expression."))
	bool bKeepNiagaraScaleForFill = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stored Content Visual|Runtime")
	bool bResolvedWaterContainerComponent = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stored Content Visual|Runtime")
	FString ResolvedWaterContainerComponentName = TEXT("None");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stored Content Visual|Runtime")
	bool bResolvedStoredVisualComponent = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stored Content Visual|Runtime")
	FString ResolvedStoredVisualComponentName = TEXT("None");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stored Content Visual|Runtime")
	float DisplayedFillVisualRatio = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Stored Content Visual")
	void RefreshStoredContentVisual(bool bSnapToTarget = false);

protected:
	void ResolveWaterContainerComponent();
	UUOUWaterContainerComponent* FindWaterContainerComponent() const;

	void ResolveStoredVisualComponent();
	USceneComponent* FindStoredVisualComponent() const;

	void BindWaterContainerEvents();
	void UnbindWaterContainerEvents();

	void CaptureStoredVisualTransformIfNeeded();
	void ApplyStoredVisualContentProfile();
	void UpdateStoredVisual(float DeltaTime, bool bSnapToTarget = false);
	void ApplyStoredVisualTransform(float FillRatio);
	void ApplyStoredVisualParameters(float FillRatio);
	bool ShouldShowStoredVisual() const;
	float GetTargetFillVisualRatio() const;
	const FUOUPourStoredVisualSettings* GetProfileStoredVisualSettings() const;
	const FUOUPourStoredVisualSettings* GetActiveMotionSettings() const;

	UFUNCTION()
	void HandleWaterAmountChanged(float NewAmount, float MaxAmount);

	UFUNCTION()
	void HandlePourContentProfileChanged(UUOUPourContentProfile* NewProfile);

private:
	UPROPERTY(Transient)
	TObjectPtr<UUOUWaterContainerComponent> BoundWaterContainerComponent = nullptr;

	bool bCapturedStoredVisualTransform = false;
	FVector InitialStoredVisualRelativeLocation = FVector::ZeroVector;
	FVector InitialStoredVisualRelativeScale = FVector::OneVector;
};
