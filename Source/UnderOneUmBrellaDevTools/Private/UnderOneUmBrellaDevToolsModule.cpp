// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/UOUDevelopmentCheatBuild.h"
#include "Modules/ModuleManager.h"

#if !UOU_WITH_PUZZLE_CHEATS
#error UnderOneUmBrellaDevTools must only be compiled when puzzle cheats are enabled.
#endif

class FUnderOneUmBrellaDevToolsModule final : public IModuleInterface
{
};

IMPLEMENT_MODULE(FUnderOneUmBrellaDevToolsModule, UnderOneUmBrellaDevTools)
