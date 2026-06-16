// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUPourReceiverComponent.generated.h"

class AActor;
class UUOUPourReceiverComponent;

USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUPourInputContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input", meta = (ClampMin = "0.0"))
	float Volume = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input", meta = (ClampMin = "0.0"))
	float Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input")
	FVector WorldDirection = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input")
	bool bHasValidWorldLocation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input")
	TObjectPtr<AActor> InstigatorActor = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUOUPourReceivedSignature, UUOUPourReceiverComponent*, Receiver, const FUOUPourInputContext&, PourContext);

UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Pour Receiver"))
class UNDERONEUMBRELLA_API UUOUPourReceiverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUPourReceiverComponent();

	UPROPERTY(BlueprintAssignable, Category = "Pour")
	FUOUPourReceivedSignature OnPourReceived;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour")
	bool bPourReceiverEnabled = true;

	UFUNCTION(BlueprintCallable, Category = "Pour")
	void ReceivePourInput(const FUOUPourInputContext& PourContext);

	UFUNCTION(BlueprintPure, Category = "Pour")
	bool CanReceivePour() const;
};
