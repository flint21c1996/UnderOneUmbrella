// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/ActorComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Debug/UOUDebugSubsystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "Interaction/UOUInteractable.h"
#include "Player/UOUCameraControllerComponent.h"
#include "Player/UOUInteractionComponent.h"
#include "Player/UOUPlayerInteractionExecutorComponent.h"
#include "Player/UOUPushPullInteractorComponent.h"
#include "Player/UOUUmbrellaComponent.h"
#include "UI/UOUUISubsystem.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

namespace
{
	void LogPushPullComponentState(const AUOUCharacter* Character, const TCHAR* ContextLabel)
	{
		if (Character == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PushPullDebug][%s] Character is null."), ContextLabel);
			return;
		}

		const UClass* CharacterClass = Character->GetClass();
		const UClass* SuperClass = CharacterClass != nullptr ? CharacterClass->GetSuperClass() : nullptr;
		const UUOUPushPullInteractorComponent* FoundComponent = Character->FindComponentByClass<UUOUPushPullInteractorComponent>();

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[PushPullDebug][%s] Character=%s Class=%s Super=%s StoredPtr=%s FoundByClass=%s"),
			ContextLabel,
			*Character->GetName(),
			CharacterClass != nullptr ? *CharacterClass->GetName() : TEXT("None"),
			SuperClass != nullptr ? *SuperClass->GetName() : TEXT("None"),
			Character->GetPushPullInteractorComponent() != nullptr ? *Character->GetPushPullInteractorComponent()->GetName() : TEXT("None"),
			FoundComponent != nullptr ? *FoundComponent->GetName() : TEXT("None"));

		TInlineComponentArray<UActorComponent*> Components(Character);
		for (const UActorComponent* Component : Components)
		{
			if (Component == nullptr)
			{
				continue;
			}

			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[PushPullDebug][%s] Component=%s Class=%s"),
				ContextLabel,
				*Component->GetName(),
				*Component->GetClass()->GetName());
		}
	}
}

AUOUCharacter::AUOUCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bEnablePhysicsInteraction = false;
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
	InteractionExecutorComponent = CreateDefaultSubobject<UUOUPlayerInteractionExecutorComponent>(
		TEXT("InteractionExecutorComponent"));

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

	UmbrellaSkeletalVisual = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("UmbrellaSkeletalVisual"));
	UmbrellaSkeletalVisual->SetupAttachment(GetMesh());
	UmbrellaSkeletalVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UmbrellaSkeletalVisual->SetGenerateOverlapEvents(false);
	UmbrellaSkeletalVisual->SetCastShadow(false);
	UmbrellaSkeletalVisual->SetVisibility(false, true);

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

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetCastShadow(true);
		CharacterMesh->bCastDynamicShadow = true;
	}

	PushPullInteractorComponent = FindComponentByClass<UUOUPushPullInteractorComponent>();

	TInlineComponentArray<UArrowComponent*> ArrowComponents(this);
	for (UArrowComponent* DebugArrowComponent : ArrowComponents)
	{
		if (DebugArrowComponent != nullptr)
		{
			DebugArrowComponent->SetHiddenInGame(true);
		}
	}

	LogPushPullComponentState(this, TEXT("BeginPlay"));
}

void AUOUCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (PushPullInteractorComponent == nullptr && !bLoggedMissingPushPullComponent)
	{
		LogPushPullComponentState(this, TEXT("TickMissingPtr"));
		bLoggedMissingPushPullComponent = true;
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
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AUOUCharacter::Move);

		if (ContextInteractAction != nullptr)
		{
			EnhancedInputComponent->BindAction(ContextInteractAction, ETriggerEvent::Started, this, &AUOUCharacter::HandleContextInteractPressed);
			EnhancedInputComponent->BindAction(ContextInteractAction, ETriggerEvent::Completed, this, &AUOUCharacter::HandleContextInteractReleased);
		}

		if (UmbrellaToggleAction != nullptr)
		{
			EnhancedInputComponent->BindAction(UmbrellaToggleAction, ETriggerEvent::Started, this, &AUOUCharacter::HandleUmbrellaTogglePressed);
		}

		if (UmbrellaInvertAction != nullptr)
		{
			EnhancedInputComponent->BindAction(UmbrellaInvertAction, ETriggerEvent::Started, this, &AUOUCharacter::HandleUmbrellaInvertPressed);
		}

		if (UmbrellaDebugFillAction != nullptr)
		{
			EnhancedInputComponent->BindAction(UmbrellaDebugFillAction, ETriggerEvent::Started, this, &AUOUCharacter::HandleUmbrellaDebugFillPressed);
		}
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component."), *GetNameSafe(this));
	}

	PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AUOUCharacter::RotateCameraLeft);
	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AUOUCharacter::RotateCameraRight);
	PlayerInputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &AUOUCharacter::RotateCameraLeftByMouseWheel);
	PlayerInputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &AUOUCharacter::RotateCameraRightByMouseWheel);
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AUOUCharacter::HandleDialogueAdvancePressed);

	if (UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent())
	{
		if (UmbrellaToggleAction == nullptr)
		{
			PlayerInputComponent->BindKey(UmbrellaComponent->GetToggleUmbrellaKey(), IE_Pressed, this, &AUOUCharacter::HandleUmbrellaTogglePressed);
		}

		if (UmbrellaInvertAction == nullptr)
		{
			PlayerInputComponent->BindKey(UmbrellaComponent->GetInvertUmbrellaKey(), IE_Pressed, this, &AUOUCharacter::HandleUmbrellaInvertPressed);
		}

		if (ContextInteractAction == nullptr)
		{
			PlayerInputComponent->BindKey(UmbrellaComponent->GetPourKey(), IE_Pressed, this, &AUOUCharacter::HandleUmbrellaPourPressed);
			PlayerInputComponent->BindKey(UmbrellaComponent->GetPourKey(), IE_Released, this, &AUOUCharacter::HandleUmbrellaPourReleased);
		}

		if (UmbrellaComponent->IsDebugFillEnabled() && UmbrellaDebugFillAction == nullptr)
		{
			PlayerInputComponent->BindKey(UmbrellaComponent->GetDebugFillKey(), IE_Pressed, this, &AUOUCharacter::HandleUmbrellaDebugFillPressed);
		}
	}

}

void AUOUCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (IsPlayerInteractionInputBlocked())
	{
		GetCharacterMovement()->StopMovementImmediately();
		return;
	}

	if (PushPullInteractorComponent != nullptr)
	{
		const float MovementYaw = CameraControllerComponent != nullptr ? CameraControllerComponent->GetMovementYaw() : 0.0f;
		if (PushPullInteractorComponent->HandleMoveInput(MovementVector, MovementYaw))
		{
			return;
		}
	}

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
	if (IsPlayerInteractionInputBlocked())
	{
		return;
	}

	if (CameraControllerComponent != nullptr)
	{
		CameraControllerComponent->RotateCameraLeft();
	}
}

void AUOUCharacter::RotateCameraRight()
{
	if (IsPlayerInteractionInputBlocked())
	{
		return;
	}

	if (CameraControllerComponent != nullptr)
	{
		CameraControllerComponent->RotateCameraRight();
	}
}

void AUOUCharacter::RotateCameraLeftByMouseWheel()
{
	if (CameraControllerComponent != nullptr && CameraControllerComponent->IsSnapCameraRotationInProgress())
	{
		return;
	}

	RotateCameraLeft();
}

void AUOUCharacter::RotateCameraRightByMouseWheel()
{
	if (CameraControllerComponent != nullptr && CameraControllerComponent->IsSnapCameraRotationInProgress())
	{
		return;
	}

	RotateCameraRight();
}

void AUOUCharacter::ZoomCameraIn()
{
	if (IsPlayerInteractionInputBlocked())
	{
		return;
	}

	if (CameraControllerComponent != nullptr)
	{
		CameraControllerComponent->ZoomCameraIn();
	}
}

void AUOUCharacter::ZoomCameraOut()
{
	if (IsPlayerInteractionInputBlocked())
	{
		return;
	}

	if (CameraControllerComponent != nullptr)
	{
		CameraControllerComponent->ZoomCameraOut();
	}
}

void AUOUCharacter::HandleJumpStarted()
{
	if (IsPlayerInteractionInputBlocked())
	{
		return;
	}

	if (PushPullInteractorComponent != nullptr && PushPullInteractorComponent->BlocksJumping())
	{
		return;
	}

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
	if (IsPlayerInteractionInputBlocked())
	{
		return;
	}

	if (UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent())
	{
		UmbrellaComponent->HandleInputPressed(UmbrellaComponent->GetToggleUmbrellaKey());
	}
}

void AUOUCharacter::HandleUmbrellaInvertPressed()
{
	if (IsPlayerInteractionInputBlocked())
	{
		return;
	}

	if (UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent())
	{
		UmbrellaComponent->HandleInputPressed(UmbrellaComponent->GetInvertUmbrellaKey());
	}
}

void AUOUCharacter::HandleUmbrellaPourPressed()
{
	if (IsPlayerInteractionInputBlocked())
	{
		return;
	}

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
	if (IsPlayerInteractionInputBlocked())
	{
		return;
	}

	if (UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent())
	{
		UmbrellaComponent->HandleInputReleased(UmbrellaComponent->GetPourKey());
	}
}

void AUOUCharacter::HandleUmbrellaDebugFillPressed()
{
	if (IsPlayerInteractionInputBlocked())
	{
		return;
	}

	if (UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent())
	{
		UmbrellaComponent->HandleInputPressed(UmbrellaComponent->GetDebugFillKey());
	}
}

void AUOUCharacter::HandleContextInteractPressed()
{
	++ContextInteractPressedCount;

	if (IsPlayerInteractionInputBlocked())
	{
		return;
	}

	if (UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent())
	{
		if (UmbrellaComponent->HasUmbrella() && UmbrellaComponent->IsUpsideDown() && UmbrellaComponent->GetCurrentStoredWater() > 0.0f)
		{
			HandleUmbrellaPourPressed();
			return;
		}
	}

	if (TryContextInteractable())
	{
		return;
	}

	HandlePushPullPressed();
}

void AUOUCharacter::HandleContextInteractReleased()
{
	++ContextInteractReleasedCount;

	if (IsPlayerInteractionInputBlocked())
	{
		return;
	}

	if (UUOUUmbrellaComponent* UmbrellaComponent = FindUmbrellaComponent())
	{
		if (UmbrellaComponent->IsPouring())
		{
			HandleUmbrellaPourReleased();
		}
	}

	if (PushPullInteractorComponent != nullptr && PushPullInteractorComponent->IsGrabbing())
	{
		HandlePushPullReleased();
	}
}

void AUOUCharacter::HandlePushPullPressed()
{
	++PushPullPressedCount;

	if (IsPlayerInteractionInputBlocked())
	{
		return;
	}

	if (PushPullInteractorComponent == nullptr)
	{
		PushPullInteractorComponent = FindComponentByClass<UUOUPushPullInteractorComponent>();
		LogPushPullComponentState(this, TEXT("HandlePushPullPressed"));
	}

	if (PushPullInteractorComponent != nullptr)
	{
		PushPullInteractorComponent->HandleGrabPressed();
	}
}

void AUOUCharacter::HandlePushPullReleased()
{
	++PushPullReleasedCount;

	if (IsPlayerInteractionInputBlocked())
	{
		return;
	}

	if (PushPullInteractorComponent == nullptr)
	{
		PushPullInteractorComponent = FindComponentByClass<UUOUPushPullInteractorComponent>();
	}

	if (PushPullInteractorComponent != nullptr)
	{
		PushPullInteractorComponent->HandleGrabReleased();
	}
}

bool AUOUCharacter::TryContextInteractable()
{
	auto TryExecuteInteract = [this](UObject* TargetObject) -> bool
	{
		if (TargetObject == nullptr || !TargetObject->GetClass()->ImplementsInterface(UUOUInteractable::StaticClass()))
		{
			return false;
		}

		IUOUInteractable::Execute_Interact(TargetObject, this);
		return true;
	};

	UUOUInteractionComponent* InteractionComponent = FindComponentByClass<UUOUInteractionComponent>();
	if (InteractionComponent == nullptr)
	{
		return false;
	}

	InteractionComponent->RefreshCandidate();

	UPrimitiveComponent* CandidateComponent = InteractionComponent->CurrentCandidateComponent;
	if (TryExecuteInteract(CandidateComponent))
	{
		return true;
	}

	AActor* CandidateActor = CandidateComponent != nullptr ? CandidateComponent->GetOwner() : nullptr;
	if (TryExecuteInteract(CandidateActor))
	{
		return true;
	}

	if (CandidateActor != nullptr)
	{
		TInlineComponentArray<UActorComponent*> CandidateActorComponents(CandidateActor);
		for (UActorComponent* CandidateActorComponent : CandidateActorComponents)
		{
			if (TryExecuteInteract(CandidateActorComponent))
			{
				return true;
			}
		}
	}

	return false;
}

void AUOUCharacter::HandleDialogueAdvancePressed()
{
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (PlayerController == nullptr)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	UUOUUISubsystem* UISubsystem = LocalPlayer != nullptr ? LocalPlayer->GetSubsystem<UUOUUISubsystem>() : nullptr;
	if (UISubsystem != nullptr && UISubsystem->IsDialoguePlaying())
	{
		UISubsystem->AdvanceDialogue();
	}
}

UUOUUmbrellaComponent* AUOUCharacter::FindUmbrellaComponent() const
{
	return FindComponentByClass<UUOUUmbrellaComponent>();
}

bool AUOUCharacter::IsPlayerInteractionInputBlocked() const
{
	return InteractionExecutorComponent != nullptr && InteractionExecutorComponent->ShouldBlockPlayerInput();
}
