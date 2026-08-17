// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Debug/UOUDevelopmentToolsBuild.h"

#if UOU_WITH_DEVELOPMENT_TOOLS

// 런타임 퍼즐 코드가 구체적인 DrawDebug 구현을 몰라도 개발용 도형을 요청할 수 있게 하는 명령 Context입니다.
class IUOUDevelopmentDebugDrawContext
{
public:
	virtual ~IUOUDevelopmentDebugDrawContext() = default;

	virtual void DrawPoint(
		const FVector& Location,
		float Size,
		const FColor& Color) = 0;
	virtual void DrawLine(
		const FVector& Start,
		const FVector& End,
		const FColor& Color,
		float Thickness = 1.0f) = 0;
	virtual void DrawArrow(
		const FVector& Start,
		const FVector& End,
		float ArrowSize,
		const FColor& Color,
		float Thickness = 1.0f) = 0;
	virtual void DrawSphere(
		const FVector& Center,
		float Radius,
		int32 Segments,
		const FColor& Color,
		float Thickness = 1.0f) = 0;
	virtual void DrawBox(
		const FVector& Center,
		const FVector& Extent,
		const FQuat& Rotation,
		const FColor& Color,
		float Thickness = 1.0f) = 0;
	virtual void DrawCapsule(
		const FVector& Center,
		float HalfHeight,
		float Radius,
		const FQuat& Rotation,
		const FColor& Color,
		float Thickness = 1.0f) = 0;
	virtual void DrawCylinder(
		const FVector& Start,
		const FVector& End,
		float Radius,
		int32 Segments,
		const FColor& Color,
		float Thickness = 1.0f) = 0;
	virtual void DrawCoordinateSystem(
		const FVector& Location,
		const FRotator& Rotation,
		float Scale,
		float Thickness = 1.0f) = 0;
	virtual void DrawString(
		const FVector& Location,
		const FString& Text,
		const FColor& Color,
		float TextScale = 1.0f) = 0;
	virtual void AddOnScreenMessage(
		int32 Key,
		const FString& Text,
		const FColor& Color,
		const FVector2D& TextScale = FVector2D(1.0f, 1.0f)) = 0;
};

#endif
