// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "UI/UOUTransitionMessageTypes.h"
#include "UOULevelTransitionOverlayWidget.generated.h"

class UBorder;
class UTextBlock;

UCLASS(Blueprintable)
class UNDERONEUMBRELLA_API UUOULevelTransitionOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	void ApplyTransitionMessage(const FUOUTransitionMessageSettings& MessageSettings);

	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	void SetTransitionBackgroundColor(FLinearColor NewColor);

	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	void SetTransitionOpacity(float NewOpacity);

	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	void SetTransitionBackgroundOpacity(float NewOpacity);

	UFUNCTION(BlueprintCallable, Category = "Level Transition")
	void SetTransitionMessageOpacity(float NewOpacity);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Level Transition|Bind")
	TObjectPtr<UBorder> FadeBackground = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Level Transition|Bind")
	TObjectPtr<UTextBlock> MessageText = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Transition|Display")
	ESlateVisibility VisibleOverlayVisibility = ESlateVisibility::HitTestInvisible;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Transition|Display")
	ESlateVisibility HiddenMessageVisibility = ESlateVisibility::Collapsed;

private:
	void EnsureFallbackWidgetTree();
	void CacheTransitionWidgetsFromTree();
	void ApplyTransitionBackgroundBrushColor();

	FLinearColor TransitionBackgroundColor = FLinearColor::Black;
	float TransitionBackgroundOpacity = 0.0f;
};
