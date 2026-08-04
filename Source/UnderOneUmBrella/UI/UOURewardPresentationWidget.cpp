// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOURewardPresentationWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Engine/World.h"
#include "MovieScene.h"

#if WITH_EDITOR
#include "Editor/WidgetCompilerLog.h"
#endif

namespace
{
	FName GetPresentationAnimationName(const UWidgetAnimation* Animation)
	{
		const UMovieScene* MovieScene =
			IsValid(Animation) ? Animation->GetMovieScene() : nullptr;
		return MovieScene != nullptr ? MovieScene->GetFName() : NAME_None;
	}
}

void UUOURewardPresentationWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ResolvedIntroAnimation = FindAnimationByName(IntroAnimationName);
	ResolvedOutroAnimation = FindAnimationByName(OutroAnimationName);

	if (!IntroAnimationName.IsNone() && ResolvedIntroAnimation == nullptr)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Reward Presentation Widget '%s'에서 선택한 Intro 애니메이션 '%s'을 찾을 수 없습니다."),
			*GetName(),
			*IntroAnimationName.ToString());
	}

	if (!OutroAnimationName.IsNone() && ResolvedOutroAnimation == nullptr)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Reward Presentation Widget '%s'에서 선택한 Outro 애니메이션 '%s'을 찾을 수 없습니다."),
			*GetName(),
			*OutroAnimationName.ToString());
	}

	if (ResolvedIntroAnimation != nullptr)
	{
		FWidgetAnimationDynamicEvent IntroFinishedEvent;
		IntroFinishedEvent.BindDynamic(
			this,
			&UUOURewardPresentationWidget::HandleIntroAnimationFinished);
		BindToAnimationFinished(ResolvedIntroAnimation, IntroFinishedEvent);
	}

	if (ResolvedOutroAnimation != nullptr)
	{
		FWidgetAnimationDynamicEvent OutroFinishedEvent;
		OutroFinishedEvent.BindDynamic(
			this,
			&UUOURewardPresentationWidget::HandleOutroAnimationFinished);
		BindToAnimationFinished(ResolvedOutroAnimation, OutroFinishedEvent);
	}
}

#if WITH_EDITOR
void UUOURewardPresentationWidget::ValidateCompiledDefaults(
	IWidgetCompilerLog& CompileLog) const
{
	Super::ValidateCompiledDefaults(CompileLog);

	const TArray<FName> AvailableAnimationNames =
		GetAvailableAnimationNames();

	auto ValidateSelection =
		[&AvailableAnimationNames, &CompileLog, this](
			const FName SelectedAnimationName,
			const FText& AnimationRole)
		{
			if (SelectedAnimationName.IsNone()
				|| AvailableAnimationNames.Contains(SelectedAnimationName))
			{
				return;
			}

			CompileLog.Error(
				FText::Format(
					NSLOCTEXT(
						"UOURewardPresentationWidget",
						"InvalidSelectedAnimation",
						"{0}의 {1} 애니메이션 '{2}'을 현재 WBP에서 찾을 수 없습니다."),
					FText::FromString(GetName()),
					AnimationRole,
					FText::FromName(SelectedAnimationName)));
		};

	ValidateSelection(
		IntroAnimationName,
		NSLOCTEXT(
			"UOURewardPresentationWidget",
			"IntroAnimationRole",
			"Intro"));
	ValidateSelection(
		OutroAnimationName,
		NSLOCTEXT(
			"UOURewardPresentationWidget",
			"OutroAnimationRole",
			"Outro"));
}
#endif

void UUOURewardPresentationWidget::NativeDestruct()
{
	ClearAutoCloseTimer();
	Super::NativeDestruct();
}

bool UUOURewardPresentationWidget::InitializePresentation(
	const FUOURewardPresentationData& InPresentationData)
{
	if (PresentationState != EUOURewardPresentationWidgetState::Uninitialized)
	{
		return false;
	}

	PresentationData = InPresentationData;
	PresentationState = EUOURewardPresentationWidgetState::Ready;
	bPresentationHoldStarted = false;
	ReceivePresentationInitialized(PresentationData);
	return true;
}

bool UUOURewardPresentationWidget::StartPresentation()
{
	if (PresentationState != EUOURewardPresentationWidgetState::Ready)
	{
		return false;
	}

	PresentationState = EUOURewardPresentationWidgetState::Presenting;
	if (ResolvedIntroAnimation != nullptr)
	{
		PlayAnimation(ResolvedIntroAnimation);
	}
	else
	{
		BeginPresentationHold();
	}

	return true;
}

bool UUOURewardPresentationWidget::HandlePresentationCue(
	const FUOURewardPresentationCue& Cue)
{
	if (PresentationState != EUOURewardPresentationWidgetState::Presenting)
	{
		return false;
	}

	ReceivePresentationCue(Cue);
	return true;
}

bool UUOURewardPresentationWidget::BeginPresentationHold()
{
	if (PresentationState != EUOURewardPresentationWidgetState::Presenting
		|| bPresentationHoldStarted)
	{
		return false;
	}

	ClearAutoCloseTimer();

	const float SafeDisplayDuration =
		FMath::Max(0.0f, PresentationData.DisplayDuration);
	if (SafeDisplayDuration <= KINDA_SMALL_NUMBER)
	{
		bPresentationHoldStarted = true;
		return true;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	bPresentationHoldStarted = true;
	World->GetTimerManager().SetTimer(
		AutoCloseTimerHandle,
		this,
		&UUOURewardPresentationWidget::HandleAutoCloseTimerElapsed,
		SafeDisplayDuration,
		false);
	return true;
}

bool UUOURewardPresentationWidget::RequestClose()
{
	if (PresentationState != EUOURewardPresentationWidgetState::Presenting)
	{
		return false;
	}

	ClearAutoCloseTimer();
	PresentationState = EUOURewardPresentationWidgetState::Closing;

	if (ResolvedIntroAnimation != nullptr
		&& IsAnimationPlaying(ResolvedIntroAnimation))
	{
		StopAnimation(ResolvedIntroAnimation);
	}

	if (ResolvedOutroAnimation != nullptr)
	{
		PlayAnimation(ResolvedOutroAnimation);
	}
	else
	{
		FinishPresentation();
	}

	return true;
}

bool UUOURewardPresentationWidget::FinishPresentation()
{
	if (PresentationState == EUOURewardPresentationWidgetState::Uninitialized
		|| PresentationState == EUOURewardPresentationWidgetState::Finished)
	{
		return false;
	}

	ClearAutoCloseTimer();
	PresentationState = EUOURewardPresentationWidgetState::Finished;
	OnPresentationFinished.Broadcast(this);
	return true;
}

bool UUOURewardPresentationWidget::ResetPresentation()
{
	if (PresentationState != EUOURewardPresentationWidgetState::Finished)
	{
		return false;
	}

	ClearAutoCloseTimer();
	StopAllAnimations();
	PresentationData = FUOURewardPresentationData();
	PresentationState = EUOURewardPresentationWidgetState::Uninitialized;
	bPresentationHoldStarted = false;
	ReceivePresentationReset();
	return true;
}

EUOURewardPresentationWidgetState
UUOURewardPresentationWidget::GetPresentationState() const
{
	return PresentationState;
}

TArray<FName> UUOURewardPresentationWidget::GetAvailableAnimationNames() const
{
	TArray<FName> AnimationNames;

	for (const UClass* CurrentClass = GetClass();
		CurrentClass != nullptr;
		CurrentClass = CurrentClass->GetSuperClass())
	{
		const UWidgetBlueprintGeneratedClass* WidgetClass =
			Cast<UWidgetBlueprintGeneratedClass>(CurrentClass);
		if (WidgetClass == nullptr)
		{
			continue;
		}

		for (const UWidgetAnimation* Animation : WidgetClass->Animations)
		{
			const FName AnimationName =
				GetPresentationAnimationName(Animation);
			if (!AnimationName.IsNone())
			{
				AnimationNames.AddUnique(AnimationName);
			}
		}
	}

	AnimationNames.Sort(
		[](const FName& Left, const FName& Right)
		{
			return Left.LexicalLess(Right);
		});
	AnimationNames.Insert(NAME_None, 0);
	return AnimationNames;
}

UWidgetAnimation* UUOURewardPresentationWidget::FindAnimationByName(
	const FName AnimationName) const
{
	if (AnimationName.IsNone())
	{
		return nullptr;
	}

	for (const UClass* CurrentClass = GetClass();
		CurrentClass != nullptr;
		CurrentClass = CurrentClass->GetSuperClass())
	{
		const UWidgetBlueprintGeneratedClass* WidgetClass =
			Cast<UWidgetBlueprintGeneratedClass>(CurrentClass);
		if (WidgetClass == nullptr)
		{
			continue;
		}

		for (UWidgetAnimation* Animation : WidgetClass->Animations)
		{
			if (GetPresentationAnimationName(Animation) == AnimationName)
			{
				return Animation;
			}
		}
	}

	return nullptr;
}

void UUOURewardPresentationWidget::HandleIntroAnimationFinished()
{
	if (PresentationState == EUOURewardPresentationWidgetState::Presenting)
	{
		BeginPresentationHold();
	}
}

void UUOURewardPresentationWidget::HandleOutroAnimationFinished()
{
	if (PresentationState == EUOURewardPresentationWidgetState::Closing)
	{
		FinishPresentation();
	}
}

void UUOURewardPresentationWidget::ClearAutoCloseTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoCloseTimerHandle);
	}
}

void UUOURewardPresentationWidget::HandleAutoCloseTimerElapsed()
{
	RequestClose();
}
