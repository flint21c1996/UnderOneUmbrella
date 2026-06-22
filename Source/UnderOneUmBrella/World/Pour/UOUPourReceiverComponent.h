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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input", meta = (ClampMin = "0.0", ToolTip = "이번 pour 입력으로 전달된 물의 양입니다. 0 이하이면 회전 반응 쪽에서 무시될 수 있습니다."))
	float Volume = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input", meta = (ClampMin = "0.0", ToolTip = "이번 pour 입력이 지속된 시간입니다. Duration 방식 회전에서는 이 값에 비례해 회전량이 계산됩니다."))
	float Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input", meta = (ToolTip = "물이 떨어지는 월드 방향입니다. 수차처럼 붓는 방향에 따라 회전 방향을 정할 때 사용됩니다."))
	FVector WorldDirection = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input", meta = (ToolTip = "물이 닿은 월드 위치입니다. 토크 기반 회전에서는 중심점과 이 위치의 차이로 힘이 걸린 방향을 계산합니다."))
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input", meta = (ToolTip = "WorldLocation이 실제 충돌 위치를 의미하는지 여부입니다. false이면 토크 기반 회전 방향 계산이 실패할 수 있습니다."))
	bool bHasValidWorldLocation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input", meta = (ToolTip = "pour 입력을 발생시킨 액터입니다. 보통 우산을 소유한 플레이어 또는 우산 액터가 들어옵니다."))
	TObjectPtr<AActor> InstigatorActor = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUOUPourReceivedSignature, UUOUPourReceiverComponent*, Receiver, const FUOUPourInputContext&, PourContext);

UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Pour Receiver"))
class UNDERONEUMBRELLA_API UUOUPourReceiverComponent : public UActorComponent
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
};
