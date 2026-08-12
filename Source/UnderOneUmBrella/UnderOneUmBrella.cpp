// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnderOneUmBrella.h"

#include "Debug/UOUDevelopmentCheatBuild.h"
#include "Modules/ModuleManager.h"

class FUnderOneUmBrellaModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();

#if UOU_WITH_PUZZLE_CHEATS
		FModuleManager::Get().LoadModuleChecked(TEXT("UnderOneUmBrellaDevTools"));
#endif
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FUnderOneUmBrellaModule, UnderOneUmBrella, "UnderOneUmBrella");
