// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Reward/UOURewardCollectionMotionComponentDetails.h"
#include "World/Rewards/UOURewardAppearanceMotionComponent.h"
#include "World/Rewards/UOURewardCollectionMotionComponent.h"

class FUnderOneUmBrellaEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FPropertyEditorModule& PropertyEditorModule =
			FModuleManager::LoadModuleChecked<FPropertyEditorModule>(
				TEXT("PropertyEditor"));

		PropertyEditorModule.RegisterCustomClassLayout(
			UUOURewardCollectionMotionComponent::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(
				&FUOURewardCollectionMotionComponentDetails::MakeInstance));
		PropertyEditorModule.RegisterCustomClassLayout(
			UUOURewardAppearanceMotionComponent::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(
				&FUOURewardCollectionMotionComponentDetails::MakeInstance));
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}

	virtual void ShutdownModule() override
	{
		if (!FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
		{
			return;
		}

		FPropertyEditorModule& PropertyEditorModule =
			FModuleManager::GetModuleChecked<FPropertyEditorModule>(
				TEXT("PropertyEditor"));
		PropertyEditorModule.UnregisterCustomClassLayout(
			UUOURewardCollectionMotionComponent::StaticClass()->GetFName());
		PropertyEditorModule.UnregisterCustomClassLayout(
			UUOURewardAppearanceMotionComponent::StaticClass()->GetFName());
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}
};

IMPLEMENT_MODULE(
	FUnderOneUmBrellaEditorModule,
	UnderOneUmBrellaEditor)
