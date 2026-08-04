// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/UOURewardPresentationWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
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

bool UUOURewardPresentationWidget::InitializePresentation(
	const FUOURewardPresentationData& InPresentationData)
{
	if (PresentationState != EUOURewardPresentationWidgetState::Uninitialized)
	{
		return false;
	}

	PresentationData = InPresentationData;
	PresentationState = EUOURewardPresentationWidgetState::Ready;
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

	return true;
}

bool UUOURewardPresentationWidget::RequestClose()
{
	if (PresentationState != EUOURewardPresentationWidgetState::Presenting)
	{
		return false;
	}

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

	StopAllAnimations();
	PresentationData = FUOURewardPresentationData();
	PresentationState = EUOURewardPresentationWidgetState::Uninitialized;
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

void UUOURewardPresentationWidget::HandleOutroAnimationFinished()
{
	if (PresentationState == EUOURewardPresentationWidgetState::Closing)
	{
		FinishPresentation();
	}
}
