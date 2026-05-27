// Copyright Epic Games, Inc. All Rights Reserved.

#include "Audio/UOUAudioDataAsset.h"

bool UUOUAudioDataAsset::TryGetAudioEvent(FName EventId, FUOUAudioEventDefinition& OutEventDefinition) const
{
	if (EventId.IsNone())
	{
		return false;
	}

	for (const FUOUAudioEventDefinition& AudioEvent : AudioEvents)
	{
		if (AudioEvent.EventId == EventId)
		{
			OutEventDefinition = AudioEvent;
			return true;
		}
	}

	return false;
}
