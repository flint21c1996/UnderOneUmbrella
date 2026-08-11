// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUDevelopmentDebugDrawSubsystem.h"

#include "Debug/UOUDevelopmentToolsBuild.h"
#include "Engine/World.h"
#include "UOUDevelopmentDebugControlSubsystem.h"

#if !UOU_WITH_DEVELOPMENT_TOOLS
#error UOUDevelopmentDebugDrawSubsystem must only be compiled when development tools are enabled.
#endif

bool UUOUDevelopmentDebugDrawSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World != nullptr
		&& World->IsGameWorld()
		&& World->GetNetMode() != NM_DedicatedServer;
}

void UUOUDevelopmentDebugDrawSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UUOUDevelopmentDebugControlSubsystem>();
	if (UWorld* World = GetWorld())
	{
		DebugControlSubsystem = World->GetSubsystem<UUOUDevelopmentDebugControlSubsystem>();
	}
}

void UUOUDevelopmentDebugDrawSubsystem::Deinitialize()
{
	DebugControlSubsystem.Reset();
	Super::Deinitialize();
}

void UUOUDevelopmentDebugDrawSubsystem::Tick(float DeltaTime)
{
	UE_UNUSED(DeltaTime);

	const UUOUDevelopmentDebugControlSubsystem* ControlSubsystem = DebugControlSubsystem.Get();
	if (ControlSubsystem == nullptr || !ControlSubsystem->IsDebugToolsEnabled())
	{
		return;
	}

	// 기존 디버그 렌더링은 이후 단계에서 카테고리별로 이관합니다.
}

TStatId UUOUDevelopmentDebugDrawSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UUOUDevelopmentDebugDrawSubsystem, STATGROUP_Tickables);
}
