// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/UOUUITypes.h"
#include "UOUUmbrellaStatusWidget.generated.h"

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella Status|Display")
	bool bHideWhenNoUmbrella = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella Status|Display")
	ESlateVisibility ShownVisibility = ESlateVisibility::SelfHitTestInvisible;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella Status|Display")
	ESlateVisibility HiddenVisibility = ESlateVisibility::Collapsed;

protected:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Umbrella Status")
	void BP_OnUmbrellaHUDStateApplied(const FUOUUmbrellaHUDState& State);

private:
	void RefreshVisibility();
	void RefreshStateImages();
	void SetWidgetVisible(UWidget* Widget, bool bNewVisible) const;
	UWidget* ResolveRootWidget() const;

	FUOUUmbrellaHUDState CurrentState;
	bool bUmbrellaStatusVisible = false;
};
