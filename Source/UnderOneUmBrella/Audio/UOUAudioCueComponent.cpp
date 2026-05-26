// Copyright Epic Games, Inc. All Rights Reserved.

#include "Audio/UOUAudioCueComponent.h"

#include "Audio/UOUAudioSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UUOUAudioCueComponent::UUOUAudioCueComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUOUAudioCueComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!AutoPlayCueId.IsNone())
	{
		PlayCue(AutoPlayCueId);
	}
}

void UUOUAudioCueComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bStopPlayedCuesOnEndPlay)
	{
		if (UUOUAudioSubsystem* AudioSubsystem = GetAudioSubsystem())
		{
			for (const TPair<FName, FName>& PlayedCueEventPair : PlayedCueEventIds)
			{
				const FName PlayedCueKey = PlayedCueEventPair.Key;
				const FName AudioEventId = PlayedCueEventPair.Value;

				FString CueKeyString = PlayedCueKey.ToString();
				FString CueIdString;
				FString InstanceIdString;
				if (CueKeyString.Split(TEXT("::"), &CueIdString, &InstanceIdString))
				{
					AudioSubsystem->StopAudioEvent(AudioEventId, FName(*InstanceIdString));
				}
			}
		}

		PlayedCueEventIds.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

bool UUOUAudioCueComponent::PlayCue(FName CueId)
{
	return PlayCueAtLocation(CueId, GetComponentLocation());
}

bool UUOUAudioCueComponent::PlayCueAtLocation(FName CueId, FVector Location)
{
	return PlayCueWithInstance(CueId, NAME_None, Location);
}

bool UUOUAudioCueComponent::PlayCueWithInstance(FName CueId, FName InstanceId, FVector Location)
{
	FUOUAudioCueDefinition CueDefinition;
	if (!TryGetCueDefinition(CueId, CueDefinition))
	{
		UE_LOG(LogUOUAudio, Warning, TEXT("Audio cue '%s' was not found on '%s'."), *CueId.ToString(), *GetName());
		return false;
	}

	return PlayCueDefinition(CueDefinition, InstanceId, Location);
}

bool UUOUAudioCueComponent::StopCue(FName CueId, float OverrideFadeOutTime)
{
	return StopCueWithInstance(CueId, NAME_None, OverrideFadeOutTime);
}

bool UUOUAudioCueComponent::StopCueWithInstance(FName CueId, FName InstanceId, float OverrideFadeOutTime)
{
	FUOUAudioCueDefinition CueDefinition;
	if (!TryGetCueDefinition(CueId, CueDefinition))
	{
		return false;
	}

	if (CueDefinition.AudioEventId.IsNone())
	{
		return false;
	}

	UUOUAudioSubsystem* AudioSubsystem = GetAudioSubsystem();
	if (AudioSubsystem == nullptr)
	{
		return false;
	}

	const FName ResolvedInstanceId = ResolveInstanceId(CueDefinition, InstanceId);
	const bool bStopped = AudioSubsystem->StopAudioEvent(CueDefinition.AudioEventId, ResolvedInstanceId, OverrideFadeOutTime);
	if (bStopped)
	{
		PlayedCueEventIds.Remove(BuildPlayedCueKey(CueDefinition.CueId, ResolvedInstanceId));
	}

	return bStopped;
}

bool UUOUAudioCueComponent::HasCue(FName CueId) const
{
	FUOUAudioCueDefinition CueDefinition;
	return TryGetCueDefinition(CueId, CueDefinition);
}

bool UUOUAudioCueComponent::ResolveAudioEventId(FName CueId, FName& OutAudioEventId) const
{
	FUOUAudioCueDefinition CueDefinition;
	if (!TryGetCueDefinition(CueId, CueDefinition))
	{
		OutAudioEventId = NAME_None;
		return false;
	}

	OutAudioEventId = CueDefinition.AudioEventId;
	return !OutAudioEventId.IsNone();
}

bool UUOUAudioCueComponent::TryGetCueDefinition(FName CueId, FUOUAudioCueDefinition& OutCueDefinition) const
{
	if (CueId.IsNone())
	{
		return false;
	}

	for (const FUOUAudioCueDefinition& CueOverride : CueOverrides)
	{
		if (CueOverride.CueId == CueId)
		{
			OutCueDefinition = CueOverride;
			return true;
		}
	}

	return AudioProfile != nullptr && AudioProfile->TryGetCue(CueId, OutCueDefinition);
}

bool UUOUAudioCueComponent::PlayCueDefinition(const FUOUAudioCueDefinition& CueDefinition, FName OverrideInstanceId, FVector Location)
{
	if (CueDefinition.AudioEventId.IsNone())
	{
		UE_LOG(LogUOUAudio, Warning, TEXT("Audio cue '%s' has no AudioEventId."), *CueDefinition.CueId.ToString());
		return false;
	}

	UUOUAudioSubsystem* AudioSubsystem = GetAudioSubsystem();
	if (AudioSubsystem == nullptr)
	{
		return false;
	}

	const FName ResolvedInstanceId = ResolveInstanceId(CueDefinition, OverrideInstanceId);
	const bool bPlayed = AudioSubsystem->PlayAudioEventInstance(CueDefinition.AudioEventId, ResolvedInstanceId, Location);
	if (bPlayed)
	{
		PlayedCueEventIds.Add(BuildPlayedCueKey(CueDefinition.CueId, ResolvedInstanceId), CueDefinition.AudioEventId);
	}

	return bPlayed;
}

FName UUOUAudioCueComponent::ResolveInstanceId(const FUOUAudioCueDefinition& CueDefinition, FName OverrideInstanceId) const
{
	if (!OverrideInstanceId.IsNone())
	{
		return OverrideInstanceId;
	}

	if (!CueDefinition.InstanceId.IsNone())
	{
		return CueDefinition.InstanceId;
	}

	if (!DefaultInstanceId.IsNone())
	{
		return DefaultInstanceId;
	}

	return BuildDefaultInstanceId();
}

FName UUOUAudioCueComponent::BuildDefaultInstanceId() const
{
	if (!bUseOwnerNameAsDefaultInstanceId)
	{
		return NAME_None;
	}

	const AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return NAME_None;
	}

	return FName(*FString::Printf(TEXT("%s.%s"), *Owner->GetName(), *GetName()));
}

FName UUOUAudioCueComponent::BuildPlayedCueKey(FName CueId, FName InstanceId) const
{
	return FName(FString::Printf(TEXT("%s::%s"), *CueId.ToString(), *InstanceId.ToString()));
}

UUOUAudioSubsystem* UUOUAudioCueComponent::GetAudioSubsystem() const
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World != nullptr ? World->GetGameInstance() : nullptr;
	return GameInstance != nullptr ? GameInstance->GetSubsystem<UUOUAudioSubsystem>() : nullptr;
}
