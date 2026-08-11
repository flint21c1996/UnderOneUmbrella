// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUDevelopmentDebugControlSubsystem.h"

#include "Debug/UOUDebugController.h"
#include "Debug/UOUDebugSubsystem.h"
#include "Debug/UOUDevelopmentToolsBuild.h"
#include "Engine/World.h"

#if !UOU_WITH_DEVELOPMENT_TOOLS
#error UOUDevelopmentDebugControlSubsystem must only be compiled when development tools are enabled.
#endif

bool UUOUDevelopmentDebugControlSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World != nullptr && World->IsGameWorld();
}

bool UUOUDevelopmentDebugControlSubsystem::IsDebugToolsEnabled() const
{
	const AUOUDebugController* DebugController = ResolveDebugController();
	return DebugController != nullptr && DebugController->bEnableDebugTools;
}

bool UUOUDevelopmentDebugControlSubsystem::SetDebugToolsEnabled(bool bNewEnabled)
{
	AUOUDebugController* DebugController = ResolveDebugController();
	if (DebugController == nullptr)
	{
		return false;
	}

	DebugController->bEnableDebugTools = bNewEnabled;
	return true;
}

AUOUDebugController* UUOUDevelopmentDebugControlSubsystem::ResolveDebugController() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	UUOUDebugSubsystem* DebugSubsystem = World->GetSubsystem<UUOUDebugSubsystem>();
	return DebugSubsystem != nullptr ? DebugSubsystem->GetActiveDebugController() : nullptr;
}
