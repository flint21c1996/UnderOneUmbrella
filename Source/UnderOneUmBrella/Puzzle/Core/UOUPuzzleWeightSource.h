// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UOUPuzzleWeightSource.generated.h"

UINTERFACE(BlueprintType)
class UUOUPuzzleWeightSource : public UInterface
{
	GENERATED_BODY()
};

// ???명꽣?섏씠?ㅻ뒗 踰꾪듉怨???몄씠 怨듯넻 諛⑹떇?쇰줈 ?쎌쓣 臾닿쾶 媛믪쓣 ?쒓났?쒕떎.
class IUOUPuzzleWeightSource
{
	GENERATED_BODY()

public:
	virtual float GetPuzzleWeight() const = 0;
};
