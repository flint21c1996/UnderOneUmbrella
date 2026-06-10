// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UI/UOUUITypes.h"
#include "UOUDialogueSourceComponent.generated.h"

class UUOUDialogueSequenceData;
class UUOUUISubsystem;
class USceneComponent;

// Component that gives an NPC or world object dialogue content and playback rules.
// Designers can set bubble anchors, speaker name, repeat rules, and inline or DataAsset lines.
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent, DisplayName="UOU Dialogue Source"))
class UNDERONEUMBRELLA_API UUOUDialogueSourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUDialogueSourceComponent();

	// Requests this source's dialogue through the local UI subsystem.
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	bool StartDialogue(AActor* InstigatorActor);

	// Checks repeat, cooldown, and line availability before playback.
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool CanStartDialogue() const;

	// Returns the scene component the speech bubble should follow.
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	USceneComponent* ResolveBubbleAnchor() const;

	// Returns the number of playable lines from the DataAsset or inline list.
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	int32 GetLineCount() const;

	const FUOUDialogueLine* GetLine(int32 LineIndex) const;
	AActor* GetSpeakerActor() const;
	FText GetSpeakerName() const;
	void MarkDialogueStarted();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UUOUDialogueSequenceData> DialogueSequence = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FUOUDialogueLine> InlineLines;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FText DefaultSpeakerName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Bubble")
	TObjectPtr<USceneComponent> BubbleAnchor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Bubble")
	FName BubbleAnchorComponentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules")
	bool bCanReplay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Rules", meta = (ClampMin = "0.0"))
	float StartCooldown = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue|Runtime")
	bool bHasPlayed = false;

private:
	UUOUUISubsystem* GetUISubsystem(AActor* InstigatorActor) const;
	float GetWorldTimeSeconds() const;

	float LastStartTime = -1000.0f;
};