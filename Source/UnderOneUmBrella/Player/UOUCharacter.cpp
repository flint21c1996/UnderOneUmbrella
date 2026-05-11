// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "Player/UOUCameraControllerComponent.h"
#include "Player/UOUUmbrellaComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AUOUCharacter::AUOUCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	CameraControllerComponent = CreateDefaultSubobject<UUOUCameraControllerComponent>(TEXT("CameraControllerComponent"));

	UmbrellaAttachPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("UmbrellaAttachPoint"));
	UmbrellaAttachPoint->SetupAttachment(GetMesh());
	UmbrellaAttachPoint->SetRelativeLocation(FVector(0.0f, 24.0f, 60.0f));
	UmbrellaAttachPoint->SetRelativeRotation(FRotator::ZeroRotator);
	UmbrellaAttachPoint->ArrowSize = 0.5f;
	UmbrellaAttachPoint->ArrowColor = FColor::Yellow;

	UmbrellaHeldVisualAnchor = CreateDefaultSubobject<UArrowComponent>(TEXT("UmbrellaHeldVisualAnchor"));
	UmbrellaHeldVisualAnchor->SetupAttachment(UmbrellaAttachPoint);
	UmbrellaHeldVisualAnchor->SetRelativeLocation(FVector::ZeroVector);
	UmbrellaHeldVisualAnchor->SetRelativeRotation(FRotator::ZeroRotator);
	UmbrellaHeldVisualAnchor->ArrowSize = 0.6f;
	UmbrellaHeldVisualAnchor->ArrowColor = FColor::Cyan;

	PourOrigin = CreateDefaultSubobject<UArrowComponent>(TEXT("PourOrigin"));
	PourOrigin->SetupAttachment(UmbrellaHeldVisualAnchor);
	PourOrigin->SetRelativeLocation(FVector(40.0f, 0.0f, 0.0f));

	InteractionOrigin = CreateDefaultSubobject<UArrowComponent>(TEXT("InteractionOrigin"));
	InteractionOrigin->SetupAttachment(RootComponent);
	InteractionOrigin->SetRelativeLocation(FVector(50.0f, 0.0f, 40.0f));
}

void AUOUCharacter::BeginPlay()
{
	Super::BeginPlay();

	TInlineComponentArray<UArrowComponent*> ArrowComponents(this);
	for (UArrowComponent* DebugArrowComponent : ArrowComponents)
	{
		if (DebugArrowComponent != nullptr)
		{
			DebugArrowComponent->SetHiddenInGame(true);
		}
	}
}

void AUOUCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AUOUCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AUOUCharacter::HandleJumpStarted);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AUOUCharacter::Move);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component."), *GetNameSafe(this));
	}

	PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AUOUCharacter::RotateCameraLeft);
	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AUOUCharacter::RotateCameraRight);
	PlayerInputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &AUOUCharacter::ZoomCameraIn);
	PlayerInputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &AUOUCharacter::ZoomCameraOut);

	if (UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent())
	{
		PlayerInputComponent->BindKey(UmbrellaComponent->GetToggleUmbrellaKey(), IE_Pressed, this, &AUOUCharacter::HandleUmbrellaTogglePressed);
		PlayerInputComponent->BindKey(UmbrellaComponent->GetInvertUmbrellaKey(), IE_Pressed, this, &AUOUCharacter::HandleUmbrellaInvertPressed);
		PlayerInputComponent->BindKey(UmbrellaComponent->GetPourKey(), IE_Pressed, this, &AUOUCharacter::HandleUmbrellaPourPressed);
		PlayerInputComponent->BindKey(UmbrellaComponent->GetPourKey(), IE_Released, this, &AUOUCharacter::HandleUmbrellaPourReleased);

		if (UmbrellaComponent->IsDebugFillEnabled())
		{
			PlayerInputComponent->BindKey(UmbrellaComponent->GetDebugFillKey(), IE_Pressed, this, &AUOUCharacter::HandleUmbrellaDebugFillPressed);
		}
	}
}

void AUOUCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent())
	{
		if (UmbrellaComponent->IsPouring())
		{
			GetCharacterMovement()->StopMovementImmediately();
			return;
		}
	}

	if (Controller != nullptr)
	{
		const float MovementYaw = CameraControllerComponent != nullptr ? CameraControllerComponent->GetMovementYaw() : 0.0f;
		const FRotator YawRotation(0.0f, MovementYaw, 0.0f);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AUOUCharacter::Look(const FInputActionValue& Value)
{
}

void AUOUCharacter::RotateCameraLeft()
{
	if (CameraControllerComponent != nullptr)
	{
		CameraControllerComponent->RotateCameraLeft();
	}
}

void AUOUCharacter::RotateCameraRight()
{
	if (CameraControllerComponent != nullptr)
	{
		CameraControllerComponent->RotateCameraRight();
	}
}

void AUOUCharacter::ZoomCameraIn()
{
	if (CameraControllerComponent != nullptr)
	{
		CameraControllerComponent->ZoomCameraIn();
	}
}

void AUOUCharacter::ZoomCameraOut()
{
	if (CameraControllerComponent != nullptr)
	{
		CameraControllerComponent->ZoomCameraOut();
	}
}

void AUOUCharacter::HandleJumpStarted()
{
	if (UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent())
	{
		if (UmbrellaComponent->BlocksJumping())
		{
			return;
		}

		UmbrellaComponent->CloseUmbrella();
	}

	Jump();
}

void AUOUCharacter::HandleUmbrellaTogglePressed()
{
	if (UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent())
	{
		UmbrellaComponent->HandleInputPressed(UmbrellaComponent->GetToggleUmbrellaKey());
	}
}

void AUOUCharacter::HandleUmbrellaInvertPressed()
{
	if (UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent())
	{
		UmbrellaComponent->HandleInputPressed(UmbrellaComponent->GetInvertUmbrellaKey());
	}
}

void AUOUCharacter::HandleUmbrellaPourPressed()
{
	if (UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent())
	{
		UmbrellaComponent->HandleInputPressed(UmbrellaComponent->GetPourKey());

		if (UmbrellaComponent->IsPouring())
		{
			GetCharacterMovement()->StopMovementImmediately();
		}
	}
}

void AUOUCharacter::HandleUmbrellaPourReleased()
{
	if (UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent())
	{
		UmbrellaComponent->HandleInputReleased(UmbrellaComponent->GetPourKey());
	}
}

void AUOUCharacter::HandleUmbrellaDebugFillPressed()
{
	if (UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent())
	{
		UmbrellaComponent->HandleInputPressed(UmbrellaComponent->GetDebugFillKey());
	}
}

UUOUUmbrellaComponent* AUOUCharacter::FindUmbrellaComponent() const
{
	return FindComponentByClass<UUOUUmbrellaComponent>();
}
