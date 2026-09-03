// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOUBubbleConversationData.h"

bool UUOUBubbleConversationData::HasValidLines() const
{
	return Lines.ContainsByPredicate([](const FUOUBubbleConversationLine& Line)
	{
		return !Line.SpeakerId.IsNone()
			&& !Line.BubbleText.IsEmpty()
			&& Line.BubbleDuration > 0.0f;
	});
}
