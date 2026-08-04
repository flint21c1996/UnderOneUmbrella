// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUInGameHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "UOUMenuPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
#include "Components/MultiLineEditableText.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/WidgetComponent.h"
#include "Engine/DataTable.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "Player/UOUUmbrellaComponent.h"
#include "UI/UOUDialogueBoxWidget.h"
#include "UI/UOURewardPresentationLayoutRow.h"
#include "UI/UOURewardPresentationWidget.h"
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

	InitializeRewardPresentationWidgets();

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

	ClearRewardPresentationWidgets();

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

bool UUOUInGameHUDWidget::ProcessRewardPresentationCue(
	const FUOURewardPresentationData& PresentationData,
	const FUOURewardPresentationCue& Cue)
{
	const FDataTableRowHandle& PresentationRow = Cue.PresentationRow;
	const FName PresentationKey = Cue.GetPresentationKey();
	const UDataTable* SelectedPresentationTable =
		PresentationRow.DataTable.Get();
	const UDataTable* HUDPresentationTable =
		RewardPresentationLayoutTable.Get();
	if (SelectedPresentationTable == nullptr || PresentationKey.IsNone())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Reward Presentation Cue has no DataTable Row selected."));
		return false;
	}

	if (SelectedPresentationTable != HUDPresentationTable)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Reward Presentation Row '%s' uses DataTable '%s', but the HUD uses '%s'."),
			*PresentationKey.ToString(),
			*GetNameSafe(SelectedPresentationTable),
			*GetNameSafe(HUDPresentationTable));
		return false;
	}

	TObjectPtr<UUOURewardPresentationWidget>* FoundWidget =
		RewardPresentationWidgets.Find(PresentationKey);
	if (FoundWidget == nullptr || !IsValid(FoundWidget->Get()))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Reward Presentation Key '%s' was not found in the HUD layout table."),
			*PresentationKey.ToString());
		return false;
	}

	UUOURewardPresentationWidget* PresentationWidget = FoundWidget->Get();
	const EUOURewardPresentationWidgetState CurrentState =
		PresentationWidget->GetPresentationState();
	if (Cue.PresentationPhase == EUOURewardPresentationCuePhase::Close)
	{
		if (CurrentState == EUOURewardPresentationWidgetState::Presenting)
		{
			return PresentationWidget->RequestClose();
		}
		if (CurrentState == EUOURewardPresentationWidgetState::Closing
			|| CurrentState == EUOURewardPresentationWidgetState::Finished)
		{
			return true;
		}

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Reward Presentation Key '%s' received an Outro request before it was started."),
			*PresentationKey.ToString());
		return false;
	}

	if (CurrentState == EUOURewardPresentationWidgetState::Presenting)
	{
		return true;
	}

	if (CurrentState == EUOURewardPresentationWidgetState::Closing)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Reward Presentation Key '%s' was requested while its Widget was closing."),
			*PresentationKey.ToString());
		return false;
	}

	if (CurrentState == EUOURewardPresentationWidgetState::Finished
		&& !PresentationWidget->ResetPresentation())
	{
		return false;
	}

	if (PresentationWidget->GetPresentationState()
			== EUOURewardPresentationWidgetState::Uninitialized
		&& !PresentationWidget->InitializePresentation(PresentationData))
	{
		return false;
	}

	PresentationWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (!PresentationWidget->StartPresentation())
	{
		PresentationWidget->SetVisibility(ESlateVisibility::Collapsed);
		return false;
	}

	return true;
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

void UUOUInGameHUDWidget::InitializeRewardPresentationWidgets()
{
	if (!RewardPresentationWidgets.IsEmpty())
	{
		return;
	}

	if (RewardResultRoot == nullptr)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Reward Presentation initialization failed: WBP_InGameHUD has no RewardResultRoot panel."));
		return;
	}

	if (RewardPresentationLayoutTable == nullptr)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Reward Presentation initialization failed: no layout DataTable was assigned."));
		return;
	}

	if (RewardPresentationLayoutTable->GetRowStruct()
		!= FUOURewardPresentationLayoutRow::StaticStruct())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Reward Presentation initialization failed: '%s' uses an unexpected row structure."),
			*GetNameSafe(RewardPresentationLayoutTable));
		return;
	}

	RewardResultRoot->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	TMap<UClass*, UUOURewardPresentationWidget*> WidgetInstancesByClass;
	for (const TPair<FName, uint8*>& RowPair : RewardPresentationLayoutTable->GetRowMap())
	{
		const FName PresentationKey = RowPair.Key;
		const FUOURewardPresentationLayoutRow* LayoutRow =
			reinterpret_cast<const FUOURewardPresentationLayoutRow*>(RowPair.Value);

		if (PresentationKey.IsNone()
			|| LayoutRow == nullptr
			|| LayoutRow->WidgetClass.Get() == nullptr)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Reward Presentation layout table '%s' has an invalid row '%s'."),
				*GetNameSafe(RewardPresentationLayoutTable),
				*PresentationKey.ToString());
			continue;
		}

		UClass* WidgetClass = LayoutRow->WidgetClass.Get();
		UUOURewardPresentationWidget* PresentationWidget =
			WidgetInstancesByClass.FindRef(WidgetClass);

		if (PresentationWidget == nullptr)
		{
			PresentationWidget = CreateWidget<UUOURewardPresentationWidget>(
				GetOwningPlayer(),
				LayoutRow->WidgetClass);
			if (PresentationWidget == nullptr)
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("Failed to create Reward Presentation Widget for key '%s'."),
					*PresentationKey.ToString());
				continue;
			}

			PresentationWidget->SetVisibility(ESlateVisibility::Collapsed);
			UOverlaySlot* PresentationSlot =
				RewardResultRoot->AddChildToOverlay(PresentationWidget);
			if (PresentationSlot == nullptr)
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("Failed to add Reward Presentation Widget for key '%s' to RewardResultRoot."),
					*PresentationKey.ToString());
				continue;
			}

			PresentationSlot->SetHorizontalAlignment(HAlign_Fill);
			PresentationSlot->SetVerticalAlignment(VAlign_Fill);

			PresentationWidget->OnPresentationFinished.AddUniqueDynamic(
				this,
				&UUOUInGameHUDWidget::HandleRewardPresentationFinished);

			WidgetInstancesByClass.Add(WidgetClass, PresentationWidget);
			CreatedRewardPresentationWidgets.Add(PresentationWidget);
		}

		RewardPresentationWidgets.Add(PresentationKey, PresentationWidget);
	}
}

void UUOUInGameHUDWidget::ClearRewardPresentationWidgets()
{
	for (UUOURewardPresentationWidget* PresentationWidget : CreatedRewardPresentationWidgets)
	{
		if (!IsValid(PresentationWidget))
		{
			continue;
		}

		PresentationWidget->OnPresentationFinished.RemoveDynamic(
			this,
			&UUOUInGameHUDWidget::HandleRewardPresentationFinished);
		PresentationWidget->RemoveFromParent();
	}

	RewardPresentationWidgets.Empty();
	CreatedRewardPresentationWidgets.Empty();
}

void UUOUInGameHUDWidget::HandleRewardPresentationFinished(
	UUOURewardPresentationWidget* PresentationWidget)
{
	if (!IsValid(PresentationWidget))
	{
		return;
	}

	const FName RewardId = PresentationWidget->GetPresentationRewardId();
	PresentationWidget->SetVisibility(ESlateVisibility::Collapsed);

	if (UUOUUISubsystem* UISubsystem = GetUISubsystem())
	{
		UISubsystem->NotifyRewardPresentationFinished(RewardId);
	}
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
