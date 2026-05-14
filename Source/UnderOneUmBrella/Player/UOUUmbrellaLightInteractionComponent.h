// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/UOUUmbrellaComponent.h"
#include "UOUUmbrellaLightInteractionComponent.generated.h"

class UUOULightInteractionSurfaceComponent;
class USceneComponent;

UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent, DisplayName="UOU Umbrella Light Interaction"))
class UUOUUmbrellaLightInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUUmbrellaLightInteractionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	bool bAutoFindUmbrellaComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	bool bAutoFindLightSurfaceComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	bool bSpreadUmbrellaBlocksLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	bool bUpsideDownUmbrellaReflectsLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	bool bCreateRuntimeLightSurfaceWhenMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light", meta = (ClampMin = "0.0"))
	FVector RuntimeSurfaceBoxExtent = FVector(70.0f, 70.0f, 6.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	FVector RuntimeSurfaceRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|Light")
	FRotator RuntimeSurfaceRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UUOUUmbrellaComponent> UmbrellaComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Umbrella|References")
	TObjectPtr<UUOULightInteractionSurfaceComponent> LightSurfaceComponent = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Umbrella|Light")
	void RefreshLightInteractionMode();

protected:
	void ResolveReferences();
	void EnsureRuntimeLightSurface();
	USceneComponent* GetLightSurfaceAttachParent() const;
	void ApplyRuntimeLightSurfacePlacement() const;

	UFUNCTION()
	void HandleUmbrellaStateChanged(EUOUUmbrellaState NewState, bool bHasUmbrella);
};
