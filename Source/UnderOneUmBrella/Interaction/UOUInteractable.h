// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UOUInteractable.generated.h"

class AActor;

// 플레이어의 Context Interact 입력을 받을 수 있는 공용 인터페이스입니다.
UINTERFACE(BlueprintType, meta = (ToolTip = "플레이어의 상호작용 입력을 받을 수 있는 오브젝트가 구현하는 인터페이스입니다."))
class UNDERONEUMBRELLA_API UUOUInteractable : public UInterface
{
	GENERATED_BODY()
};

class UNDERONEUMBRELLA_API IUOUInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction", meta = (ToolTip = "플레이어의 상호작용 입력이 들어왔을 때 실행됩니다."))
	void Interact(AActor* Interactor);
};
