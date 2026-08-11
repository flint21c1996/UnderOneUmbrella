// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnderOneUmBrella.h"
#include "Debug/UOUDevelopmentCheatBuild.h"
#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "Debug/UOUDebugController.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#endif

class FUnderOneUmBrellaModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();

#if UOU_WITH_PUZZLE_CHEATS
		FModuleManager::Get().LoadModuleChecked(TEXT("UnderOneUmBrellaDevTools"));
#endif

#if WITH_EDITOR
		MapOpenedHandle = FEditorDelegates::OnMapOpened.AddRaw(this, &FUnderOneUmBrellaModule::HandleMapOpened);
		NewCurrentLevelHandle = FEditorDelegates::NewCurrentLevel.AddRaw(this, &FUnderOneUmBrellaModule::HandleNewCurrentLevel);
		PostWorldInitializationHandle = FWorldDelegates::OnPostWorldInitialization.AddRaw(this, &FUnderOneUmBrellaModule::HandlePostWorldInitialization);

		if (GEditor != nullptr)
		{
			EnsureDebugControllerForEditorWorld(GEditor->GetEditorWorldContext().World());
		}
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		if (MapOpenedHandle.IsValid())
		{
			FEditorDelegates::OnMapOpened.Remove(MapOpenedHandle);
		}

		if (NewCurrentLevelHandle.IsValid())
		{
			FEditorDelegates::NewCurrentLevel.Remove(NewCurrentLevelHandle);
		}

		if (PostWorldInitializationHandle.IsValid())
		{
			FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitializationHandle);
		}
#endif

		FDefaultGameModuleImpl::ShutdownModule();
	}

#if WITH_EDITOR
private:
	void HandleMapOpened(const FString& Filename, bool bAsTemplate)
	{
		if (GEditor != nullptr)
		{
			EnsureDebugControllerForEditorWorld(GEditor->GetEditorWorldContext().World());
		}
	}

	void HandleNewCurrentLevel()
	{
		if (GEditor != nullptr)
		{
			EnsureDebugControllerForEditorWorld(GEditor->GetEditorWorldContext().World());
		}
	}

	void HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues IVS)
	{
		EnsureDebugControllerForEditorWorld(World);
	}

	void EnsureDebugControllerForEditorWorld(UWorld* World) const
	{
		if (!ShouldAutoCreateForWorld(World))
		{
			return;
		}

		for (TActorIterator<AUOUDebugController> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				return;
			}
		}

		AUOUDebugController::FindOrCreateDebugController(World);
	}

	bool ShouldAutoCreateForWorld(const UWorld* World) const
	{
		return World != nullptr
			&& World->WorldType == EWorldType::Editor
			&& World->PersistentLevel != nullptr;
	}

	FDelegateHandle MapOpenedHandle;
	FDelegateHandle NewCurrentLevelHandle;
	FDelegateHandle PostWorldInitializationHandle;
#endif
};

IMPLEMENT_PRIMARY_GAME_MODULE(FUnderOneUmBrellaModule, UnderOneUmBrella, "UnderOneUmBrella");
 
