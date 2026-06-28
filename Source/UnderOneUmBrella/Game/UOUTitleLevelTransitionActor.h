// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUTitleLevelTransitionActor.generated.h"

class USceneComponent;
class UWorld;

UCLASS(meta = (DisplayName = "UOU Title Level Transition Actor"))
class UNDERONEUMBRELLA_API AUOUTitleLevelTransitionActor : public AActor
{
	GENERATED_BODY()

public:
	AUOUTitleLevelTransitionActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Title|Level Transition")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Level Transition", meta = (AllowedClasses = "/Script/Engine.World", ToolTip = "타이틀에서 시작 버튼을 눌렀을 때 열 레벨입니다. 페이드 시간과 문구는 각 맵의 UOU Level Transition Settings Actor에서 설정합니다."))
	TSoftObjectPtr<UWorld> TargetLevel;
};
