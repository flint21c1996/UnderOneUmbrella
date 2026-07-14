// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/Pour/UOUPourReceiverInterface.h"
#include "UOUPourReceiverComponent.generated.h"

class UUOUPourReceiverComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUOUPourReceivedSignature, UUOUPourReceiverComponent*, Receiver, const FUOUPourInputContext&, PourContext);

UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Pour Receiver"))
class UNDERONEUMBRELLA_API UUOUPourReceiverComponent : public UActorComponent, public IUOUPourReceiver
{
	GENERATED_BODY()

public:
	UUOUPourReceiverComponent();

	UPROPERTY(BlueprintAssignable, Category = "Pour", meta = (ToolTip = "이 컴포넌트가 pour 입력을 받을 때 호출되는 이벤트입니다. 회전, 이동, 퍼즐 조건 같은 외부 반응을 여기에 연결할 수 있습니다."))
	FUOUPourReceivedSignature OnPourReceived;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour", meta = (ToolTip = "false이면 우산의 pour가 이 컴포넌트에 닿아도 입력을 받지 않습니다."))
	bool bPourReceiverEnabled = true;

	UFUNCTION(BlueprintCallable, Category = "Pour")
	void ReceivePourInput(const FUOUPourInputContext& PourContext);

	UFUNCTION(BlueprintPure, Category = "Pour")
	bool CanReceivePour() const;

	virtual bool CanAcceptPour_Implementation(const FUOUPourInputContext& Context) const override;
	virtual FUOUPourReceiveResult TryReceivePour_Implementation(const FUOUPourInputContext& Context) override;
	virtual int32 GetPourReceivePriority_Implementation() const override;
	virtual bool CanAcceptPourAtLocation_Implementation(const FUOUPourInputContext& Context) const override;
};
