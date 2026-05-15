// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UOUGameMode.generated.h"

// 프로젝트 기본 플레이 규칙과 시작 캐릭터를 연결하는 게임 모드다.
// 레벨이 열리면 어떤 플레이어를 기본으로 쓸지 여기서 결정한다.
UCLASS(minimalapi)
class AUOUGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// 기본 플레이어 클래스를 우리 프로젝트 전용 캐릭터로 지정한다.
	AUOUGameMode();
};
