// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

// QA와 개발팀에 배포하는 내부 게임 Target입니다.
// Shipping 최적화를 유지하면서 UOU 개발 도구 모듈을 명시적으로 포함합니다.
public class UnderOneUmBrellaInternalTarget : TargetRules
{
	public UnderOneUmBrellaInternalTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;

		ExtraModuleNames.Add("UnderOneUmBrella");
		ExtraModuleNames.Add("UnderOneUmBrellaDevTools");
	}
}
