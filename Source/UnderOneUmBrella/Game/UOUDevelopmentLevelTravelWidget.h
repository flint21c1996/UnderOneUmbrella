// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/UOULevelTransitionSubsystem.h"
#include "UOUDevelopmentLevelTravelWidget.generated.h"

class UBorder;
class UButton;
class UEditableTextBox;
class UHorizontalBox;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UWorld;
class UUOULevelTransitionSubsystem;
class UUOUDevelopmentLevelTravelWidget;

UCLASS()
class UNDERONEUMBRELLA_API UUOUDevelopmentLevelTravelCommand : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UUOUDevelopmentLevelTravelWidget* InOwner, TSoftObjectPtr<UWorld> InTargetLevel);

	UFUNCTION()
	void HandleClicked();

private:
	UPROPERTY(Transient)
	TObjectPtr<UUOUDevelopmentLevelTravelWidget> Owner = nullptr;

	UPROPERTY(Transient)
	TSoftObjectPtr<UWorld> TargetLevel;
};

UCLASS(Blueprintable)
class UNDERONEUMBRELLA_API UUOUDevelopmentLevelTravelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void SetQuickLevels(const TArray<TSoftObjectPtr<UWorld>>& InQuickLevels);
	void SetTitleLevel(TSoftObjectPtr<UWorld> InTitleLevel);
	void TravelToLevel(TSoftObjectPtr<UWorld> TargetLevel);

private:
	void BuildWidgetTree();
	void RebuildQuickLevelButtons(UScrollBox* QuickLevelList);
	UTextBlock* CreateTextBlock(const FText& Text, int32 FontSize) const;
	UButton* CreateButton(const FText& Label) const;
	UUOULevelTransitionSubsystem* GetTransitionSubsystem() const;
	FUOULevelTransitionSettings MakeDevelopmentTransitionSettings() const;

	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleNextClicked();

	UFUNCTION()
	void HandleTitleClicked();

	UFUNCTION()
	void HandleOpenByNameClicked();

	UPROPERTY(Transient)
	TArray<TSoftObjectPtr<UWorld>> QuickLevels;

	UPROPERTY(Transient)
	TSoftObjectPtr<UWorld> TitleLevel;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> LevelNameTextBox = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UUOUDevelopmentLevelTravelCommand>> QuickLevelCommands;
};
