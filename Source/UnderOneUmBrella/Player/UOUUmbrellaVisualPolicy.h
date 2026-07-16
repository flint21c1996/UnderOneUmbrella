// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

enum class EUOUUmbrellaState : uint8;
enum class EUOUUmbrellaVisualState : uint8;

// 정적/런타임 우산 비주얼에 적용할 최종 표시 결정을 담습니다.
struct FUOUUmbrellaVisualVisibility
{
	bool bShowClosed = false;
	bool bShowOpen = false;
	bool bShowUpsideDown = false;
	bool bShowRuntime = false;
	bool bFlipRuntime = false;
};

// 게임플레이 상태를 렌더링 결정으로 변환하는 순수 정책입니다.
// 컴포넌트와 에셋을 직접 변경하지 않으므로 월드 없이 단위 테스트할 수 있습니다.
class FUOUUmbrellaVisualPolicy
{
public:
	static EUOUUmbrellaVisualState ResolveVisualState(
		EUOUUmbrellaState State,
		bool bUseClosedReversedOverride);

	static bool ShouldFlipRuntimeVisual(
		bool bFlipRuntimeVisualWhenReversed,
		EUOUUmbrellaVisualState VisualState);

	static FUOUUmbrellaVisualVisibility ResolveVisibility(
		bool bHasUmbrella,
		EUOUUmbrellaVisualState VisualState,
		bool bHasDedicatedVisuals,
		bool bHasUpsideDownVisual,
		bool bHasRuntimeVisual,
		bool bFlipRuntimeVisualWhenReversed);
};
