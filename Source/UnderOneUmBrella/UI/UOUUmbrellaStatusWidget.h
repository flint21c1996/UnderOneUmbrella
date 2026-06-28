// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/UOUUITypes.h"
#include "UOUUmbrellaStatusWidget.generated.h"

class USizeBox;
class UWidget;

// C++ base for the player's umbrella HUD cluster.
// Blueprint owns layout and brushes; this class only maps umbrella visual state to widget visibility.
UCLASS(Blueprintable)
class UNDERONEUMBRELLA_API UUOUUmbrellaStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Umbrella Status")
	void ApplyUmbrellaHUDState(const FUOUUmbrellaHUDState& State);

	UFUNCTION(BlueprintPure, Category = "Umbrella Status")
	const FUOUUmbrellaHUDState& GetCurrentUmbrellaHUDState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Umbrella Status")
	bool IsUmbrellaStatusVisible() const { return bUmbrellaStatusVisible; }

	UFUNCTION(BlueprintPure, Category = "Umbrella Status|Stored Water")
	bool IsStoredWaterVisible() const { return bStoredWaterVisible; }

	UFUNCTION(BlueprintPure, Category = "Umbrella Status|Stored Water")
	float GetStoredWaterRatio() const { return CurrentState.StoredWaterRatio; }

	UFUNCTION(BlueprintPure, Category = "Umbrella Status|Stored Water")
	float GetStoredWaterAmount() const { return CurrentState.StoredWater; }

	UFUNCTION(BlueprintPure, Category = "Umbrella Status|Stored Water")
	float GetMaxStoredWaterAmount() const { return CurrentState.MaxStoredWater; }

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Umbrella Status|Bind")
	TObjectPtr<UWidget> UmbrellaStatusRoot = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Umbrella Status|Bind")
	TObjectPtr<UWidget> FrameImage = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Umbrella Status|Bind")
	TObjectPtr<UWidget> ClosedUmbrellaImage = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Umbrella Status|Bind")
	TObjectPtr<UWidget> OpenUmbrellaImage = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Umbrella Status|Bind")
	TObjectPtr<UWidget> ClosedReversedUmbrellaImage = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Umbrella Status|Bind")
	TObjectPtr<UWidget> OpenReversedUmbrellaImage = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Umbrella Status|Bind")
	TObjectPtr<UWidget> StoredWaterRoot = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Umbrella Status|Bind")
	TObjectPtr<UWidget> StoredWaterBackImage = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Umbrella Status|Bind")
	TObjectPtr<USizeBox> StoredWaterFillClip = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Umbrella Status|Bind")
	TObjectPtr<UWidget> StoredWaterFill = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Umbrella Status|Bind")
	TObjectPtr<UWidget> StoredWaterFrameImage = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Umbrella Status|Bind")
	TObjectPtr<UWidget> StoredWaterEffectImage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella Status|Display")
	bool bHideWhenNoUmbrella = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella Status|Stored Water")
	bool bShowStoredWaterOnlyWhenOpenReversed = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella Status|Stored Water")
	bool bResizeStoredWaterFillClip = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella Status|Stored Water", meta = (ClampMin = "0.0"))
	float StoredWaterFillClipFullWidth = 128.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella Status|Display")
	ESlateVisibility ShownVisibility = ESlateVisibility::SelfHitTestInvisible;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella Status|Display")
	ESlateVisibility HiddenVisibility = ESlateVisibility::Collapsed;

protected:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Umbrella Status")
	void BP_OnUmbrellaHUDStateApplied(const FUOUUmbrellaHUDState& State);

	// Resource-specific stored-water presentation lives in Blueprint so later UI assets can be swapped without C++ changes.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Umbrella Status|Stored Water")
	void BP_OnStoredWaterPresentationChanged(float StoredWaterRatio, float StoredWater, float MaxStoredWater, bool bVisible);

private:
	void ResolveStoredWaterFillClipFullWidth();
	void RefreshVisibility();
	void RefreshStateImages();
	void RefreshStoredWater();
	void SetWidgetVisible(UWidget* Widget, bool bNewVisible) const;
	UWidget* ResolveRootWidget() const;
	bool ShouldShowStoredWater() const;

	FUOUUmbrellaHUDState CurrentState;
	float ResolvedStoredWaterFillClipFullWidth = 128.0f;
	bool bUmbrellaStatusVisible = false;
	bool bStoredWaterVisible = false;
};
