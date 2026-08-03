// Copyright Epic Games, Inc. All Rights Reserved.

#include "Reward/UOURewardCollectionMotionComponentDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "GameFramework/Actor.h"
#include "Reward/SUOURewardCueTimeline.h"
#include "Styling/AppStyle.h"
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
	AActor* RewardOwner = MotionComponent != nullptr
		? MotionComponent->GetOwner()
		: nullptr;
	if (RewardOwner == nullptr && MotionComponent != nullptr)
	{
		RewardOwner = MotionComponent->GetTypedOuter<AActor>();
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
	if (MotionComponent != nullptr && FeedbackComponent != nullptr)
	{
		TimelineContent = SNew(SUOURewardCueTimeline)
			.MotionComponent(MotionComponent)
			.FeedbackComponent(FeedbackComponent);
	}

	IDetailCategoryBuilder& TimelineCategory = DetailBuilder.EditCategory(
		TEXT("Reward|Motion|Timeline"),
		LOCTEXT("TimelineCategoryName", "Reward Motion Timeline"),
		ECategoryPriority::Important);

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
