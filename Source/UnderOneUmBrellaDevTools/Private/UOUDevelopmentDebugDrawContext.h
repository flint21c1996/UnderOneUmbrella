// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Debug/UOUDevelopmentDebugDrawContext.h"

class UWorld;

// Provider가 요청한 개발용 도형을 현재 월드의 DrawDebug 호출로 변환하는 DevTools 구현체입니다.
class FUOUDevelopmentDebugDrawContext final : public IUOUDevelopmentDebugDrawContext
{
public:
	explicit FUOUDevelopmentDebugDrawContext(UWorld& InWorld);

	virtual void DrawPoint(const FVector& Location, float Size, const FColor& Color) override;
	virtual void DrawLine(
		const FVector& Start,
		const FVector& End,
		const FColor& Color,
		float Thickness) override;
	virtual void DrawArrow(
		const FVector& Start,
		const FVector& End,
		float ArrowSize,
		const FColor& Color,
		float Thickness) override;
	virtual void DrawSphere(
		const FVector& Center,
		float Radius,
		int32 Segments,
		const FColor& Color,
		float Thickness) override;
	virtual void DrawBox(
		const FVector& Center,
		const FVector& Extent,
		const FQuat& Rotation,
		const FColor& Color,
		float Thickness) override;
	virtual void DrawCapsule(
		const FVector& Center,
		float HalfHeight,
		float Radius,
		const FQuat& Rotation,
		const FColor& Color,
		float Thickness) override;
	virtual void DrawCylinder(
		const FVector& Start,
		const FVector& End,
		float Radius,
		int32 Segments,
		const FColor& Color,
		float Thickness) override;
	virtual void DrawCoordinateSystem(
		const FVector& Location,
		const FRotator& Rotation,
		float Scale,
		float Thickness) override;
	virtual void DrawString(
		const FVector& Location,
		const FString& Text,
		const FColor& Color,
		float TextScale) override;
	virtual void AddOnScreenMessage(
		int32 Key,
		const FString& Text,
		const FColor& Color,
		const FVector2D& TextScale) override;

private:
	// Context가 생성된 Subsystem Tick 동안 실제 디버그 명령을 받을 월드입니다.
	UWorld* World = nullptr;
};
