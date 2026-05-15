// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUTitleGameMode.h"

#include "UOUTitlePlayerController.h"

AUOUTitleGameMode::AUOUTitleGameMode()
{
	// 타이틀은 월드 안에서 플레이어를 조종하지 않으므로 Pawn 계열을 비워 둡니다.
	DefaultPawnClass = nullptr;
	SpectatorClass = nullptr;
	PlayerControllerClass = AUOUTitlePlayerController::StaticClass();
	HUDClass = nullptr;
	bStartPlayersAsSpectators = true;
}

UClass* AUOUTitleGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	// 다른 경로로 Pawn 클래스가 설정되어도 타이틀에서는 생성되지 않도록 한 번 더 막습니다.
	return nullptr;
}
