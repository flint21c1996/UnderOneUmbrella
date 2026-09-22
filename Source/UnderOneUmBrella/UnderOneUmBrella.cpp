// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnderOneUmBrella.h"

#include "Debug/UOUDevelopmentCheatBuild.h"
#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "CoreGlobals.h"
#include "World/Sky/UOUVirtualPerspectiveSkyMaterialFactory.h"
#endif

class FUnderOneUmBrellaModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();

#if WITH_EDITOR
		if (GIsEditor)
		{
			EnsureUOUPerspectiveSkyCompositeMaterial();
		}
#endif

#if UOU_WITH_PUZZLE_CHEATS
		FModuleManager::Get().LoadModuleChecked(TEXT("UnderOneUmBrellaDevTools"));
#endif
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FUnderOneUmBrellaModule, UnderOneUmBrella, "UnderOneUmBrella");
