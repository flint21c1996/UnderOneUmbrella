// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UOUTitleGameMode.generated.h"

// 타이틀 맵 전용 GameMode입니다.
// 메뉴만 보여주는 화면이라 Pawn과 HUD를 생성하지 않습니다.
UCLASS()
class UNDERONEUMBRELLA_API AUOUTitleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AUOUTitleGameMode();

	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
};
