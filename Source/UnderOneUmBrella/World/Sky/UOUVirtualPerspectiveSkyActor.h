#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOUVirtualPerspectiveSkyActor.generated.h"

class UStaticMeshComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;

// Retain the reflected class name for saved Blueprint references.
// Edit a physical dome; at runtime capture only that dome with a perspective view.
UCLASS(Blueprintable, meta = (DisplayName = "UOU Perspective Sky"))
class UNDERONEUMBRELLA_API AUOUVirtualPerspectiveSkyActor : public AActor
{
	GENERATED_BODY()
public:
	AUOUVirtualPerspectiveSkyActor();
	virtual void Tick(float DeltaSeconds) override;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sky")
	TObjectPtr<UStaticMeshComponent> SkyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sky")
	TObjectPtr<USceneCaptureComponent2D> SkyCapture;

	// This vertical FOV affects the sky only, never the gameplay camera.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky", meta = (ClampMin = "10", ClampMax = "150", Units = "deg"))
	float SkyVerticalFOV = 70.0f;

	// Zero observes the dome from its centre.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky")
	FVector CaptureLocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky", meta = (ClampMin = "256", ClampMax = "4096"))
	int32 CaptureWidth = 1920;

	// Must be behind gameplay geometry and inside the camera far clip plane.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky|Advanced", meta = (ClampMin = "1000", Units = "cm"))
	float BackgroundDistance = 100000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Sky|Advanced")
	TSoftObjectPtr<UMaterialInterface> CompositeMaterial;

private:
	UPROPERTY(VisibleAnywhere, Category = "Sky|Internal")
	TObjectPtr<UStaticMeshComponent> BackgroundScreen;
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> SkyRenderTarget;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CompositeInstance;
	bool bOwnsBackground = false;
	int32 ViewUpdateCount = 0;
	void UpdateBackground();
};
