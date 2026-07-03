// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUInGameHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "UOUMenuPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
#include "Components/MultiLineEditableText.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/WidgetComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "Player/UOUUmbrellaComponent.h"
#include "UI/UOUDialogueBoxWidget.h"
#include "UI/UOUUmbrellaStatusWidget.h"
#include "UI/UOUSpeechBubbleWidget.h"
#include "UI/UOUUISubsystem.h"

namespace
{
	bool IsSpeechBubbleTextWidget(const UWidget* Widget)
	{
		return Widget != nullptr
			&& (Widget->IsA<UTextBlock>()
				|| Widget->IsA<URichTextBlock>()
				|| Widget->IsA<UEditableTextBox>()
				|| Widget->IsA<UEditableText>()
				|| Widget->IsA<UMultiLineEditableTextBox>()
				|| Widget->IsA<UMultiLineEditableText>());
	}

	bool SetSpeechBubbleTextWidgetValue(UWidget* Widget, const FText& BubbleText)
	{
		if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
		{
			TextBlock->SetText(BubbleText);
			return true;
		}

		if (URichTextBlock* RichTextBlock = Cast<URichTextBlock>(Widget))
		{
			RichTextBlock->SetText(BubbleText);
			return true;
		}

		if (UEditableTextBox* EditableTextBox = Cast<UEditableTextBox>(Widget))
		{
			EditableTextBox->SetText(BubbleText);
			return true;
		}

		if (UEditableText* EditableText = Cast<UEditableText>(Widget))
		{
			EditableText->SetText(BubbleText);
			return true;
		}

		if (UMultiLineEditableTextBox* MultiLineEditableTextBox = Cast<UMultiLineEditableTextBox>(Widget))
		{
			MultiLineEditableTextBox->SetText(BubbleText);
			return true;
		}

		if (UMultiLineEditableText* MultiLineEditableText = Cast<UMultiLineEditableText>(Widget))
		{
			MultiLineEditableText->SetText(BubbleText);
			return true;
		}

		return false;
	}

	UWidget* FindSpeechBubbleTextWidget(UUserWidget* UserWidget)
	{
		if (UserWidget == nullptr || UserWidget->WidgetTree == nullptr)
		{
			return nullptr;
		}

		if (UWidget* NamedWidget = UserWidget->WidgetTree->FindWidget(FName(TEXT("TXT_BubbleText"))))
		{
			if (IsSpeechBubbleTextWidget(NamedWidget))
			{
				return NamedWidget;
			}
		}

		UWidget* FirstSupportedTextWidget = nullptr;
		UserWidget->WidgetTree->ForEachWidget([&FirstSupportedTextWidget](UWidget* Widget)
		{
			if (FirstSupportedTextWidget == nullptr && IsSpeechBubbleTextWidget(Widget))
			{
				FirstSupportedTextWidget = Widget;
			}
		});

		return FirstSupportedTextWidget;
	}

	bool TrySetGenericSpeechBubbleText(UUserWidget* UserWidget, const FText& BubbleText)
	{
		UWidget* TextWidget = FindSpeechBubbleTextWidget(UserWidget);
		if (TextWidget == nullptr)
		{
			return false;
		}

		if (!SetSpeechBubbleTextWidgetValue(TextWidget, BubbleText))
		{
			return false;
		}

		UserWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UserWidget->SetRenderOpacity(1.0f);

		if (UserWidget->WidgetTree != nullptr)
		{
			if (UWidget* BubbleRoot = UserWidget->WidgetTree->FindWidget(FName(TEXT("BubbleRoot"))))
			{
				BubbleRoot->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				BubbleRoot->SetRenderOpacity(1.0f);
			}
		}

		return true;
	}
}

void UUOUInGameHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UUOUUISubsystem* UISubsystem = GetUISubsystem())
	{
		UISubsystem->RegisterHUD(this);
	}

	BindToPlayerUmbrella();
}

void UUOUInGameHUDWidget::NativeDestruct()
{
	if (UUOUUISubsystem* UISubsystem = GetUISubsystem())
	{
		UISubsystem->UnregisterHUD(this);
	}

	Super::NativeDestruct();
}

void UUOUInGameHUDWidget::OpenSettingsMenu()
{
	if (AUOUMenuPlayerController* MenuPlayerController = GetMenuPlayerController())
	{
		MenuPlayerController->OpenSettingsMenu();
	}
}

bool UUOUInGameHUDWidget::IsSettingsMenuOpen() const
{
	const AUOUMenuPlayerController* MenuPlayerController = GetMenuPlayerController();
	return MenuPlayerController != nullptr && MenuPlayerController->IsSettingsMenuOpen();
}

void UUOUInGameHUDWidget::ReturnToTitle()
{
	if (AUOUMenuPlayerController* MenuPlayerController = GetMenuPlayerController())
	{
		MenuPlayerController->ReturnToTitle();
	}
}

void UUOUInGameHUDWidget::RestartCurrentStage()
{
	if (AUOUMenuPlayerController* MenuPlayerController = GetMenuPlayerController())
	{
		MenuPlayerController->RestartCurrentStage();
	}
}

void UUOUInGameHUDWidget::GoToNextLevel()
{
	if (AUOUMenuPlayerController* MenuPlayerController = GetMenuPlayerController())
	{
		MenuPlayerController->GoToNextLevel();
	}
}

void UUOUInGameHUDWidget::GoToPreviousLevel()
{
	if (AUOUMenuPlayerController* MenuPlayerController = GetMenuPlayerController())
	{
		MenuPlayerController->GoToPreviousLevel();
	}
}

bool UUOUInGameHUDWidget::CanRestartCurrentStage() const
{
	const AUOUMenuPlayerController* MenuPlayerController = GetMenuPlayerController();
	return MenuPlayerController != nullptr && MenuPlayerController->CanRestartCurrentStage();
}

bool UUOUInGameHUDWidget::CanReturnToTitle() const
{
	const AUOUMenuPlayerController* MenuPlayerController = GetMenuPlayerController();
	return MenuPlayerController != nullptr && MenuPlayerController->CanReturnToTitle();
}

void UUOUInGameHUDWidget::BindToPlayerUmbrella()
{
	APlayerController* OwningPlayerController = GetOwningPlayer();
	APawn* OwningPawn = OwningPlayerController != nullptr ? OwningPlayerController->GetPawn() : nullptr;
	if (OwningPawn == nullptr)
	{
		return;
	}

	UUOUUmbrellaComponent* UmbrellaComponent = OwningPawn->FindComponentByClass<UUOUUmbrellaComponent>();
	if (UmbrellaComponent == nullptr)
	{
		return;
	}

	if (UUOUUISubsystem* UISubsystem = GetUISubsystem())
	{
		UISubsystem->BindUmbrellaComponent(UmbrellaComponent);
	}
}

void UUOUInGameHUDWidget::AdvanceDialogue()
{
	if (UUOUUISubsystem* UISubsystem = GetUISubsystem())
	{
		UISubsystem->AdvanceDialogue();
	}
}

void UUOUInGameHUDWidget::SetDialogueBoxWidget(UUOUDialogueBoxWidget* InDialogueBoxWidget)
{
	DialogueBoxWidget = InDialogueBoxWidget;
}

void UUOUInGameHUDWidget::SetUmbrellaStatusWidget(UUOUUmbrellaStatusWidget* InUmbrellaStatusWidget)
{
	UmbrellaStatusWidget = InUmbrellaStatusWidget;
}

void UUOUInGameHUDWidget::ShowTitle(const FUOUTitleDisplayData& TitleData)
{
	if (UUOUUISubsystem* UISubsystem = GetUISubsystem())
	{
		UISubsystem->ShowTitle(TitleData);
	}
}

void UUOUInGameHUDWidget::ApplyUmbrellaHUDState(const FUOUUmbrellaHUDState& State)
{
	if (UUOUUmbrellaStatusWidget* ResolvedUmbrellaStatusWidget = ResolveUmbrellaStatusWidget())
	{
		ResolvedUmbrellaStatusWidget->ApplyUmbrellaHUDState(State);
	}
}

void UUOUInGameHUDWidget::BeginDialoguePresentation_Implementation(AActor* SpeakerActor, UUOUDialogueSourceComponent* DialogueSource)
{
}

void UUOUInGameHUDWidget::ShowNPCSpeechBubble(AActor* SpeakerActor, FText BubbleText, float Duration)
{
	UWidgetComponent* SpeechBubbleWidgetComponent = ResolveSpeechBubbleWidgetComponent(SpeakerActor);
	if (SpeechBubbleWidgetComponent == nullptr)
	{
		return;
	}

	SpeechBubbleWidgetComponent->SetVisibility(true, true);
	SpeechBubbleWidgetComponent->SetHiddenInGame(false, true);
	SpeechBubbleWidgetComponent->InitWidget();

	UUserWidget* SpeechBubbleWidget = SpeechBubbleWidgetComponent->GetUserWidgetObject();
	if (SpeechBubbleWidget == nullptr)
	{
		return;
	}

	if (UUOUSpeechBubbleWidget* TypedSpeechBubbleWidget = Cast<UUOUSpeechBubbleWidget>(SpeechBubbleWidget))
	{
		TypedSpeechBubbleWidget->ShowBubble(BubbleText, Duration);
		return;
	}

	UFunction* ShowFunction = SpeechBubbleWidget->FindFunction(TEXT("ShowBubble"));
	if (ShowFunction == nullptr)
	{
		TrySetGenericSpeechBubbleText(SpeechBubbleWidget, BubbleText);
		return;
	}

	struct FShowBubbleParams
	{
		FText BubbleText;
		double Duration = 0.0;
	};

	FShowBubbleParams Params;
	Params.BubbleText = BubbleText;
	Params.Duration = Duration;
	SpeechBubbleWidget->ProcessEvent(ShowFunction, &Params);

	TrySetGenericSpeechBubbleText(SpeechBubbleWidget, BubbleText);
}

void UUOUInGameHUDWidget::ShowDialogueLine_Implementation(AActor* SpeakerActor, const FUOUDialogueLine& Line)
{
	if (DialogueBoxWidget != nullptr)
	{
		DialogueBoxWidget->ShowDialogueLine(SpeakerActor, Line);
	}
}

void UUOUInGameHUDWidget::HideDialogue_Implementation()
{
	if (DialogueBoxWidget != nullptr)
	{
		DialogueBoxWidget->HideDialogueBox();
	}
}

AUOUMenuPlayerController* UUOUInGameHUDWidget::GetMenuPlayerController() const
{
	return Cast<AUOUMenuPlayerController>(GetOwningPlayer());
}

UUOUUISubsystem* UUOUInGameHUDWidget::GetUISubsystem() const
{
	ULocalPlayer* OwningLocalPlayer = GetOwningLocalPlayer();
	return OwningLocalPlayer != nullptr ? OwningLocalPlayer->GetSubsystem<UUOUUISubsystem>() : nullptr;
}

UUOUUmbrellaStatusWidget* UUOUInGameHUDWidget::ResolveUmbrellaStatusWidget()
{
	if (UmbrellaStatusWidget != nullptr)
	{
		return UmbrellaStatusWidget;
	}

	if (WidgetTree == nullptr)
	{
		return nullptr;
	}

	if (UWidget* NamedWidget = WidgetTree->FindWidget(FName(TEXT("UmbrellaStatusWidget"))))
	{
		UmbrellaStatusWidget = Cast<UUOUUmbrellaStatusWidget>(NamedWidget);
		if (UmbrellaStatusWidget != nullptr)
		{
			return UmbrellaStatusWidget;
		}
	}

	UUOUUmbrellaStatusWidget* FirstUmbrellaStatusWidget = nullptr;
	WidgetTree->ForEachWidget([&FirstUmbrellaStatusWidget](UWidget* Widget)
	{
		if (FirstUmbrellaStatusWidget == nullptr)
		{
			FirstUmbrellaStatusWidget = Cast<UUOUUmbrellaStatusWidget>(Widget);
		}
	});

	UmbrellaStatusWidget = FirstUmbrellaStatusWidget;
	return UmbrellaStatusWidget;
}

UWidgetComponent* UUOUInGameHUDWidget::ResolveSpeechBubbleWidgetComponent(AActor* SpeakerActor) const
{
	if (SpeakerActor == nullptr)
	{
		return nullptr;
	}

	TArray<UWidgetComponent*> WidgetComponents;
	SpeakerActor->GetComponents<UWidgetComponent>(WidgetComponents);

	for (UWidgetComponent* WidgetComponent : WidgetComponents)
	{
		if (WidgetComponent != nullptr && WidgetComponent->GetFName() == FName(TEXT("SpeechBubbleWidget")))
		{
			return WidgetComponent;
		}
	}

	for (UWidgetComponent* WidgetComponent : WidgetComponents)
	{
		if (WidgetComponent != nullptr && WidgetComponent->GetName().Contains(TEXT("Bubble")))
		{
			return WidgetComponent;
		}
	}

	return WidgetComponents.Num() > 0 ? WidgetComponents[0] : nullptr;
}
