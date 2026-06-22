// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Player/UOUUmbrellaComponent.h"
#include "UOUUmbrellaAnimInstance.generated.h"

UCLASS(BlueprintType, Blueprintable)
class UNDERONEUMBRELLA_API UUOUUmbrellaAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Umbrella")
	void SetUmbrellaState(
		bool bNewHasUmbrella,
		EUOUUmbrellaState NewUmbrellaState,
		EUOUUmbrellaDirectionState NewDirectionState,
		EUOUUmbrellaVisualState NewVisualState);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella")
	bool bHasUmbrella = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella")
	EUOUUmbrellaState UmbrellaState = EUOUUmbrellaState::Closed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella")
	EUOUUmbrellaDirectionState DirectionState = EUOUUmbrellaDirectionState::Normal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella")
	EUOUUmbrellaVisualState VisualState = EUOUUmbrellaVisualState::Closed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella")
	bool bIsClosed = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella")
	bool bIsOpen = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella")
	bool bIsClosedReversed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella")
	bool bIsOpenReversed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella")
	bool bIsPouring = false;

	// 우산의 Close 모프 타겟에 전달할 값입니다. 0이면 펼침, 1이면 접힘입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Morph Target")
	float CloseAlpha = 1.0f;

	// 현재 상태에서 목표로 하는 Close 모프 타겟 값입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Umbrella|Morph Target")
	float TargetCloseAlpha = 1.0f;

	// CloseAlpha가 목표 값으로 따라가는 속도입니다. 0 이하이면 즉시 바뀝니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Morph Target", meta = (ClampMin = "0.0"))
	float CloseInterpSpeed = 8.0f;

	// CloseAlpha를 스켈레탈 메쉬 컴포넌트의 모프 타겟에 직접 반영합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Morph Target")
	bool bApplyCloseMorphTargetDirectly = true;

	// 우산 접힘/펼침에 사용하는 모프 타겟 이름입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Morph Target")
	FName CloseMorphTargetName = TEXT("Close");
};
