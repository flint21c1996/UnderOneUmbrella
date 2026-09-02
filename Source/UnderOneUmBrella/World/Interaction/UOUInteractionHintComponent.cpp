// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Interaction/UOUInteractionHintComponent.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UI/UOUSpeechBubbleWidget.h"

namespace
{
	// ShowBubble 블루프린트 함수의 입력 핀 구성과 같은 임시 파라미터입니다.
	struct FUOUShowBubbleParams
	{
		FText BubbleText;
		double Duration = 0.0;
	};
}

UUOUInteractionHintComponent::UUOUInteractionHintComponent()
{
	InitSphereRadius(220.0f);
	SetCollisionProfileName(TEXT("Trigger"));
	SetGenerateOverlapEvents(true);
	PrimaryComponentTick.bCanEverTick = false;

	HintText = FText::FromString(TEXT("?"));
}

void UUOUInteractionHintComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddDynamic(this, &UUOUInteractionHintComponent::HandleBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &UUOUInteractionHintComponent::HandleEndOverlap);

	if (UWidgetComponent* ResolvedHintWidgetComponent = ResolveHintWidgetComponent())
	{
		// 처음 숨기기 전에 UserWidget을 생성해 첫 overlap에서도 ShowBubble을 호출할 수 있게 합니다.
		ResolvedHintWidgetComponent->InitWidget();
	}

	if (bHideHintOnBeginPlay)
	{
		HideHint();
	}
}

void UUOUInteractionHintComponent::ShowHint()
{
	UWidgetComponent* WidgetComponent = ResolveHintWidgetComponent();
	if (WidgetComponent == nullptr)
	{
		return;
	}

	SetWidgetComponentVisible(true);
	WidgetComponent->InitWidget();

	UUserWidget* UserWidget = WidgetComponent->GetUserWidgetObject();
	if (UserWidget == nullptr)
	{
		SetWidgetComponentVisible(false);
		return;
	}

	if (UUOUSpeechBubbleWidget* SpeechBubbleWidget = Cast<UUOUSpeechBubbleWidget>(UserWidget))
	{
		if (HintFontSizeOverride > 0)
		{
			SpeechBubbleWidget->SetBubbleFontSize(HintFontSizeOverride);
		}

		SpeechBubbleWidget->ShowBubble(HintText, HintDuration);
	}
	else
	{
		CallWidgetShowFunction(UserWidget);
	}

	bHintVisible = true;
}

void UUOUInteractionHintComponent::HideHint()
{
	UUserWidget* UserWidget = GetHintUserWidget();
	if (UserWidget != nullptr)
	{
		if (UUOUSpeechBubbleWidget* SpeechBubbleWidget = Cast<UUOUSpeechBubbleWidget>(UserWidget))
		{
			SpeechBubbleWidget->HideBubble();
		}
		else
		{
			CallWidgetHideFunction(UserWidget);
		}
	}

	SetWidgetComponentVisible(false);
	bHintVisible = false;
}

UWidgetComponent* UUOUInteractionHintComponent::ResolveHintWidgetComponent()
{
	if (HintWidgetComponent != nullptr)
	{
		return HintWidgetComponent;
	}

	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return nullptr;
	}

	TArray<UWidgetComponent*> WidgetComponents;
	OwnerActor->GetComponents<UWidgetComponent>(WidgetComponents);

	for (UWidgetComponent* WidgetComponent : WidgetComponents)
	{
		if (WidgetComponent != nullptr && WidgetComponent->GetFName() == HintWidgetComponentName)
		{
			HintWidgetComponent = WidgetComponent;
			return HintWidgetComponent;
		}
	}

	if (bAutoFindFirstWidgetComponent && WidgetComponents.Num() > 0)
	{
		HintWidgetComponent = WidgetComponents[0];
	}

	return HintWidgetComponent;
}

void UUOUInteractionHintComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (PassesOverlapRules(OtherActor))
	{
		ShowHint();
	}
}

void UUOUInteractionHintComponent::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (PassesOverlapRules(OtherActor))
	{
		HideHint();
	}
}

bool UUOUInteractionHintComponent::PassesOverlapRules(AActor* OtherActor) const
{
	if (OtherActor == nullptr || OtherActor == GetOwner())
	{
		return false;
	}

	const APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (bOnlyPawn && OtherPawn == nullptr)
	{
		return false;
	}

	if (bOnlyPlayerPawn)
	{
		const UWorld* World = GetWorld();
		const APawn* PlayerPawn = World != nullptr ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
		return OtherPawn == PlayerPawn;
	}

	return true;
}

void UUOUInteractionHintComponent::SetWidgetComponentVisible(bool bNewVisible) const
{
	if (!bControlWidgetComponentVisibility || HintWidgetComponent == nullptr)
	{
		return;
	}

	HintWidgetComponent->SetVisibility(bNewVisible, true);
	HintWidgetComponent->SetHiddenInGame(!bNewVisible, true);
}

void UUOUInteractionHintComponent::CallWidgetShowFunction(UUserWidget* UserWidget) const
{
	if (UserWidget == nullptr || ShowFunctionName.IsNone())
	{
		return;
	}

	UFunction* ShowFunction = UserWidget->FindFunction(ShowFunctionName);
	if (ShowFunction == nullptr)
	{
		return;
	}

	FUOUShowBubbleParams Params;
	Params.BubbleText = HintText;
	Params.Duration = HintDuration < 0.0f ? 9999.0f : HintDuration;
	UserWidget->ProcessEvent(ShowFunction, &Params);
}

void UUOUInteractionHintComponent::CallWidgetHideFunction(UUserWidget* UserWidget) const
{
	if (UserWidget == nullptr || HideFunctionName.IsNone())
	{
		return;
	}

	UFunction* HideFunction = UserWidget->FindFunction(HideFunctionName);
	if (HideFunction != nullptr)
	{
		UserWidget->ProcessEvent(HideFunction, nullptr);
	}
}

UUserWidget* UUOUInteractionHintComponent::GetHintUserWidget()
{
	UWidgetComponent* WidgetComponent = ResolveHintWidgetComponent();
	return WidgetComponent != nullptr ? WidgetComponent->GetUserWidgetObject() : nullptr;
}
