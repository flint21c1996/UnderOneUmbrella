// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Audio/UOUAudioTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "UOUAudioSettingsSaveGame.generated.h"

UCLASS()
class UNDERONEUMBRELLA_API UUOUAudioSettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Audio|Settings")
	float GetCategoryVolume(EUOUAudioCategory Category) const;

	UFUNCTION(BlueprintCallable, Category = "Audio|Settings")
	void SetCategoryVolume(EUOUAudioCategory Category, float Volume);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Audio|Settings")
	float MasterVolume = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Audio|Settings")
	float BGMVolume = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Audio|Settings")
	float SFXVolume = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Audio|Settings")
	float UIVolume = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Audio|Settings")
	float AmbienceVolume = 1.0f;
};
