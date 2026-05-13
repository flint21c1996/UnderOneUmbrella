// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "UOUCharacter.generated.h"

class UArrowComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UUOUPushPullInteractorComponent;
class USceneComponent;
class USpringArmComponent;
class UUOUCameraControllerComponent;
class UUOUUmbrellaComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

// ???대옒?ㅻ뒗 ?뚮젅?댁뼱 ?대룞怨?移대찓?? ?곗궛 湲곗??먯쓣 ?④퍡 媛吏??湲곕낯 罹먮┃?곕? ?대떦?쒕떎.
UCLASS(config=Game)
class AUOUCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ContextInteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* UmbrellaToggleAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* UmbrellaInvertAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* UmbrellaDebugFillAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UUOUCameraControllerComponent* CameraControllerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	UArrowComponent* UmbrellaAttachPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	UArrowComponent* UmbrellaHeldVisualAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	UArrowComponent* PourOrigin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	UArrowComponent* InteractionOrigin;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	UUOUPushPullInteractorComponent* PushPullInteractorComponent;

public:
	AUOUCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void RotateCameraLeft();
	void RotateCameraRight();
	void ZoomCameraIn();
	void ZoomCameraOut();
	void HandleJumpStarted();
	void HandleUmbrellaTogglePressed();
	void HandleUmbrellaInvertPressed();
	void HandleUmbrellaPourPressed();
	void HandleUmbrellaPourReleased();
	void HandleUmbrellaDebugFillPressed();
	void HandleContextInteractPressed();
	void HandleContextInteractReleased();
	void HandlePushPullPressed();
	void HandlePushPullReleased();

	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UUOUUmbrellaComponent* FindUmbrellaComponent() const;

public:
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE class UUOUCameraControllerComponent* GetCameraControllerComponent() const { return CameraControllerComponent; }
	FORCEINLINE class UUOUPushPullInteractorComponent* GetPushPullInteractorComponent() const { return PushPullInteractorComponent; }

private:
	UPROPERTY(Transient)
	int32 ContextInteractPressedCount = 0;

	UPROPERTY(Transient)
	int32 ContextInteractReleasedCount = 0;

	UPROPERTY(Transient)
	int32 PushPullPressedCount = 0;

	UPROPERTY(Transient)
	int32 PushPullReleasedCount = 0;

	UPROPERTY(Transient)
	bool bLoggedMissingPushPullComponent = false;
};
