// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUPushPullObjectComponent.generated.h"

class AActor;
class UPrimitiveComponent;

// ???대옒?ㅻ뒗 諛湲곗? ?밴린湲???곸쓽 ?≫옒 ?곹깭? ?섑룊 ?대룞 ?쒖뼱瑜??대떦?쒕떎.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent))
class UUOUPushPullObjectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUPushPullObjectComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|PushPull")
	bool bAutoFindTargetPrimitive = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|PushPull")
	TObjectPtr<UPrimitiveComponent> TargetPrimitive = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|PushPull", meta = (ClampMin = "0.0"))
	float MaxHorizontalSpeed = 250.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	bool bIsGrabbed = false;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|PushPull")
	bool CanGrab(AActor* Interactor) const;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|PushPull")
	bool TryBeginGrab(AActor* Interactor);

	UFUNCTION(BlueprintCallable, Category = "Puzzle|PushPull")
	void EndGrab(AActor* Interactor);

	UFUNCTION(BlueprintCallable, Category = "Puzzle|PushPull")
	FVector SetHorizontalVelocity(FVector HorizontalVelocity);

protected:
	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentGrabber = nullptr;

	void ResolveTargetPrimitive();
	void StopHorizontalMotion();
};
