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

void UUOUDevelopmentDebugControlSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (const AUOUDebugController* DebugController = ResolveDebugController())
	{
		bDebugToolsEnabled = DebugController->bEnableDebugTools;
	}
}

bool UUOUDevelopmentDebugControlSubsystem::IsDebugToolsEnabled() const
{
	return bDebugToolsEnabled;
}

void UUOUDevelopmentDebugControlSubsystem::SetDebugToolsEnabled(bool bNewEnabled)
{
	bDebugToolsEnabled = bNewEnabled;
	ApplyMasterStateToLegacyController();
}

void UUOUDevelopmentDebugControlSubsystem::ApplyMasterStateToLegacyController() const
{
	if (AUOUDebugController* DebugController = ResolveDebugController())
	{
		DebugController->bEnableDebugTools = bDebugToolsEnabled;
	}
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
