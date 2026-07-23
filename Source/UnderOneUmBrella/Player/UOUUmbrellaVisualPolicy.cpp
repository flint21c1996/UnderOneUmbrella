// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UOUUmbrellaVisualPolicy.h"

#include "Player/UOUUmbrellaComponent.h"

EUOUUmbrellaVisualState FUOUUmbrellaVisualPolicy::ResolveVisualState(
	EUOUUmbrellaState State,
	bool bUseClosedReversedOverride)
{
	if (State == EUOUUmbrellaState::Closed)
	{
		return bUseClosedReversedOverride
			? EUOUUmbrellaVisualState::ClosedReversed
			: EUOUUmbrellaVisualState::Closed;
	}

	if (State == EUOUUmbrellaState::Open || State == EUOUUmbrellaState::LightReflecting)
	{
		return EUOUUmbrellaVisualState::Open;
	}

	return EUOUUmbrellaVisualState::OpenReversed;
}

bool FUOUUmbrellaVisualPolicy::ShouldFlipRuntimeVisual(
	bool bFlipRuntimeVisualWhenReversed,
	EUOUUmbrellaVisualState VisualState)
{
	return bFlipRuntimeVisualWhenReversed
		&& VisualState == EUOUUmbrellaVisualState::OpenReversed;
}

FUOUUmbrellaVisualVisibility FUOUUmbrellaVisualPolicy::ResolveVisibility(
	bool bHasUmbrella,
	EUOUUmbrellaVisualState VisualState,
	bool bHasDedicatedVisuals,
	bool bHasUpsideDownVisual,
	bool bHasRuntimeVisual,
	bool bFlipRuntimeVisualWhenReversed)
{
	FUOUUmbrellaVisualVisibility Visibility;
	if (!bHasUmbrella)
	{
		return Visibility;
	}

	const bool bShouldFlipRuntime = ShouldFlipRuntimeVisual(
		bFlipRuntimeVisualWhenReversed,
		VisualState);

	if (!bHasDedicatedVisuals)
	{
		Visibility.bShowRuntime = bHasRuntimeVisual;
		Visibility.bFlipRuntime = bHasRuntimeVisual && bShouldFlipRuntime;
		return Visibility;
	}

	const bool bUseRuntimeUpsideDownFallback = bHasRuntimeVisual
		&& !bHasUpsideDownVisual
		&& bShouldFlipRuntime;

	Visibility.bShowClosed = VisualState == EUOUUmbrellaVisualState::Closed
		|| VisualState == EUOUUmbrellaVisualState::ClosedReversed;
	Visibility.bShowOpen = VisualState == EUOUUmbrellaVisualState::Open
		|| (VisualState == EUOUUmbrellaVisualState::OpenReversed
			&& !bHasUpsideDownVisual
			&& !bUseRuntimeUpsideDownFallback);
	Visibility.bShowUpsideDown = VisualState == EUOUUmbrellaVisualState::OpenReversed;
	Visibility.bShowRuntime = bUseRuntimeUpsideDownFallback;
	Visibility.bFlipRuntime = bUseRuntimeUpsideDownFallback;
	return Visibility;
}
