// Copyright Epic Games, Inc. All Rights Reserved.

#include "Reward/UOURewardCollectionMotionComponentDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "GameFramework/Actor.h"
#include "Reward/SUOURewardCueTimeline.h"
#include "Styling/AppStyle.h"
#include "World/Rewards/UOURewardAppearanceMotionComponent.h"
#include "World/Rewards/UOURewardCollectionMotionComponent.h"
#include "World/Rewards/UOURewardFeedbackComponent.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "UOURewardCollectionMotionComponentDetails"

TSharedRef<IDetailCustomization>
FUOURewardCollectionMotionComponentDetails::MakeInstance()
{
	return MakeShared<FUOURewardCollectionMotionComponentDetails>();
}

void FUOURewardCollectionMotionComponentDetails::CustomizeDetails(
	IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
	DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);

	UUOURewardCollectionMotionComponent* MotionComponent =
		CustomizedObjects.Num() == 1
			? Cast<UUOURewardCollectionMotionComponent>(
				CustomizedObjects[0].Get())
			: nullptr;
	UUOURewardAppearanceMotionComponent* AppearanceMotionComponent =
		CustomizedObjects.Num() == 1
			? Cast<UUOURewardAppearanceMotionComponent>(
				CustomizedObjects[0].Get())
			: nullptr;
	UActorComponent* SelectedMotionComponent = MotionComponent != nullptr
		? static_cast<UActorComponent*>(MotionComponent)
		: static_cast<UActorComponent*>(AppearanceMotionComponent);
	AActor* RewardOwner = SelectedMotionComponent != nullptr
		? SelectedMotionComponent->GetOwner()
		: nullptr;
	if (RewardOwner == nullptr && SelectedMotionComponent != nullptr)
	{
		RewardOwner = SelectedMotionComponent->GetTypedOuter<AActor>();
	}
	UUOURewardFeedbackComponent* FeedbackComponent = RewardOwner != nullptr
		? RewardOwner->FindComponentByClass<UUOURewardFeedbackComponent>()
		: nullptr;

	TSharedRef<SWidget> TimelineContent = SNew(STextBlock)
		.Text(CustomizedObjects.Num() > 1
			? LOCTEXT(
				"MultipleSelectionMessage",
				"여러 MotionComponent를 선택한 상태에서는 타임라인을 편집할 수 없습니다.")
			: LOCTEXT(
				"MissingFeedbackMessage",
				"같은 RewardActor에서 FeedbackComponent를 찾을 수 없습니다."));
	if (SelectedMotionComponent != nullptr && FeedbackComponent != nullptr)
	{
		TimelineContent = SNew(SUOURewardCueTimeline)
			.MotionComponent(MotionComponent)
			.AppearanceMotionComponent(AppearanceMotionComponent)
			.FeedbackComponent(FeedbackComponent);
	}

	IDetailCategoryBuilder& TimelineCategory = DetailBuilder.EditCategory(
		TEXT("Reward|Motion|Timeline"),
		LOCTEXT("TimelineCategoryName", "Reward Motion Timeline"),
		ECategoryPriority::Important);

	TimelineCategory
		.AddCustomRow(LOCTEXT("TimelineMarkerGuideSearchText", "Presentation S O Intro Outro"))
		.WholeRowContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT(
				"TimelineMarkerGuideText",
				"Presentation 행의 S 마커는 Intro 시작 시점입니다. O 마커는 재생 중인 Intro를 중지하고 Outro를 시작하는 시점이며, Outro가 없으면 즉시 종료합니다."))
			.AutoWrapText(true)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];

	TimelineCategory
		.AddCustomRow(LOCTEXT("TimelineSearchText", "Reward Timeline"))
		.WholeRowContent()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
			.Padding(12.0f)
			[
				TimelineContent
			]
		];
}

#undef LOCTEXT_NAMESPACE
