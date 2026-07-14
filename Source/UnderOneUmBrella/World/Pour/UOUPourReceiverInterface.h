// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "World/Pour/UOUPourDropTypes.h"
#include "UOUPourReceiverInterface.generated.h"

class AActor;

// PourDrop이 관측한 공통 입력 사실입니다. 수신 대상별 계산값은 각 구현체가 내부에서 만듭니다.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUPourInputContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input", meta = (ClampMin = "0.0", ToolTip = "이번 pour 입력으로 전달된 물의 양입니다."))
	float Volume = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input", meta = (ClampMin = "0.0", ToolTip = "이번 pour 입력이 지속된 시간입니다."))
	float Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input", meta = (ToolTip = "물이 떨어지는 월드 방향입니다."))
	FVector WorldDirection = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input", meta = (ToolTip = "물이 닿은 월드 위치입니다."))
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input", meta = (ToolTip = "WorldLocation이 실제 충돌 또는 위치 판정 지점인지 나타냅니다."))
	bool bHasValidWorldLocation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input", meta = (ClampMin = "0.0", ToolTip = "충돌 없이 위치만으로 수신자를 찾을 때 허용할 월드 거리 오차입니다."))
	float LocationAcceptanceTolerance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input", meta = (ToolTip = "pour 입력을 발생시킨 액터입니다."))
	TObjectPtr<AActor> InstigatorActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour Input", meta = (ToolTip = "수신자가 연결된 대상까지 입력을 전파할 수 있도록 전달하는 공통 정책입니다. 연결 개념이 없는 수신자는 무시합니다."))
	bool bPropagateToConnectedTargets = true;
};

// 수신 구현이 처리 여부와 실제 수신 대상을 DropActor에 돌려주는 공통 결과입니다.
USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUPourReceiveResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Pour Result")
	bool bAccepted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Pour Result", meta = (ClampMin = "0.0"))
	float AcceptedVolume = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Pour Result")
	FName ReceiverId = NAME_None;

	// 기존 충돌 델리게이트와 디버그 enum을 유지하기 위한 호환 정보입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Pour Result")
	EUOUPourDropReceiverType ReceiverType = EUOUPourDropReceiverType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Pour Result")
	TObjectPtr<UObject> ReceiverObject = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Pour Result")
	TObjectPtr<AActor> ReceiverActor = nullptr;
};

UINTERFACE(BlueprintType, meta = (ToolTip = "우산 PourDrop의 공통 입력을 받을 수 있는 액터 또는 컴포넌트가 구현하는 인터페이스입니다."))
class UUOUPourReceiver : public UInterface
{
	GENERATED_BODY()
};

class IUOUPourReceiver
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pour Receiver")
	bool CanAcceptPour(const FUOUPourInputContext& Context) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pour Receiver")
	FUOUPourReceiveResult TryReceivePour(const FUOUPourInputContext& Context);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pour Receiver")
	int32 GetPourReceivePriority() const;

	// 충돌 Actor를 찾지 못했을 때 현재 월드 위치만으로 이 수신자를 선택할 수 있는지 반환합니다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pour Receiver")
	bool CanAcceptPourAtLocation(const FUOUPourInputContext& Context) const;
};
