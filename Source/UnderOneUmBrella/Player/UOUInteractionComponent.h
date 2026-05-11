// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUInteractionComponent.generated.h"

class UPrimitiveComponent;
class USceneComponent;

// ???대옒?ㅻ뒗 ?뚮젅?댁뼱 ?욎そ?먯꽌 ?곹샇?묒슜 ?꾨낫瑜??먯??섍퀬 罹먯떆?쒕떎.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class UUOUInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUInteractionComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bInteractionEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float InteractionRange = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float InteractionProbeRadius = 32.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FVector DetectionOffset = FVector(50.0f, 0.0f, 40.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<USceneComponent> DetectionOrigin = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UPrimitiveComponent> CurrentCandidateComponent = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void RefreshCandidate();

protected:
	FVector GetDetectionStart() const;
	FVector GetDetectionEnd() const;
};
