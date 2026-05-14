// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Light/UOULightReceivableInterface.h"

FVector IUOULightReceivableInterface::GetLightReceiverPosition_Implementation() const
{
	return FVector::ZeroVector;
}

void IUOULightReceivableInterface::ReceiveLightExposure_Implementation(const FUOULightExposureData& ExposureData)
{
}
