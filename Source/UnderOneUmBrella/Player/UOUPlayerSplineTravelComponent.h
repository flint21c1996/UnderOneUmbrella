// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUPlayerSplineTravelComponent.generated.h"

class AUOUCharacter;
class USplineComponent;
class UUOUPlayerInteractionExecutorComponent;

/**
 * Executes one externally-authored spline travel request for the owning character.
 * The path remains owned by the requesting world actor; this component only keeps a weak reference.
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class UNDERONEUMBRELLA_API UUOUPlayerSplineTravelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUOUPlayerSplineTravelComponent();

	/**
	 * Queues travel so movement state is changed from this component's PostPhysics tick,
	 * rather than from the character jump callback.
	 */
	bool StartTravel(
		USplineComponent* Path,
		float Speed,
		float AcceptanceRadius,
		UObject* RequestSource,
		bool bAllowCameraRotation = true);

	// Cancels the current request. A supplied source can only cancel its own request.
	void CancelTravel(UObject* RequestSource = nullptr);

	bool IsTraveling() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool BeginPendingTravel();
	void UpdateTravel(float DeltaTime);
	void FinishTravel();
	void ResetTravelState();

	TWeakObjectPtr<USplineComponent> ActivePath;
	TWeakObjectPtr<UObject> ActiveRequestSource;
	TWeakObjectPtr<UUOUPlayerInteractionExecutorComponent> LockedInputExecutorComponent;

	float TravelSpeed = 0.0f;
	float EntryAcceptanceRadius = 0.0f;
	float CurrentDistanceAlongPath = 0.0f;

	bool bStartPending = false;
	bool bTravelActive = false;
	bool bMovingToPathStart = false;
	bool bAllowCameraRotation = true;
};
