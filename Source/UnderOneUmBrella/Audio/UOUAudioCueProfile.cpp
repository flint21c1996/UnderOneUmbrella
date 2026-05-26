// Copyright Epic Games, Inc. All Rights Reserved.

#include "Audio/UOUAudioCueProfile.h"

bool UUOUAudioCueProfile::TryGetCue(FName CueId, FUOUAudioCueDefinition& OutCueDefinition) const
{
	if (CueId.IsNone())
	{
		return false;
	}

	for (const FUOUAudioCueDefinition& CueDefinition : Cues)
	{
		if (CueDefinition.CueId == CueId)
		{
			OutCueDefinition = CueDefinition;
			return true;
		}
	}

	return false;
}
