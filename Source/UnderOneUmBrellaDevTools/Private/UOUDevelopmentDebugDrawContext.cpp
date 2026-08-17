// Copyright Epic Games, Inc. All Rights Reserved.

#include "UOUDevelopmentDebugDrawContext.h"

#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

FUOUDevelopmentDebugDrawContext::FUOUDevelopmentDebugDrawContext(UWorld& InWorld)
	: World(&InWorld)
{
}

void FUOUDevelopmentDebugDrawContext::DrawPoint(
	const FVector& Location,
	float Size,
	const FColor& Color)
{
	DrawDebugPoint(World, Location, Size, Color, false, 0.0f);
}

void FUOUDevelopmentDebugDrawContext::DrawLine(
	const FVector& Start,
	const FVector& End,
	const FColor& Color,
	float Thickness)
{
	DrawDebugLine(World, Start, End, Color, false, 0.0f, 0, Thickness);
}

void FUOUDevelopmentDebugDrawContext::DrawArrow(
	const FVector& Start,
	const FVector& End,
	float ArrowSize,
	const FColor& Color,
	float Thickness)
{
	DrawDebugDirectionalArrow(World, Start, End, ArrowSize, Color, false, 0.0f, 0, Thickness);
}

void FUOUDevelopmentDebugDrawContext::DrawSphere(
	const FVector& Center,
	float Radius,
	int32 Segments,
	const FColor& Color,
	float Thickness)
{
	DrawDebugSphere(World, Center, Radius, Segments, Color, false, 0.0f, 0, Thickness);
}

void FUOUDevelopmentDebugDrawContext::DrawBox(
	const FVector& Center,
	const FVector& Extent,
	const FQuat& Rotation,
	const FColor& Color,
	float Thickness)
{
	DrawDebugBox(World, Center, Extent, Rotation, Color, false, 0.0f, 0, Thickness);
}

void FUOUDevelopmentDebugDrawContext::DrawCapsule(
	const FVector& Center,
	float HalfHeight,
	float Radius,
	const FQuat& Rotation,
	const FColor& Color,
	float Thickness)
{
	DrawDebugCapsule(World, Center, HalfHeight, Radius, Rotation, Color, false, 0.0f, 0, Thickness);
}

void FUOUDevelopmentDebugDrawContext::DrawCylinder(
	const FVector& Start,
	const FVector& End,
	float Radius,
	int32 Segments,
	const FColor& Color,
	float Thickness)
{
	DrawDebugCylinder(World, Start, End, Radius, Segments, Color, false, 0.0f, 0, Thickness);
}

void FUOUDevelopmentDebugDrawContext::DrawCoordinateSystem(
	const FVector& Location,
	const FRotator& Rotation,
	float Scale,
	float Thickness)
{
	DrawDebugCoordinateSystem(World, Location, Rotation, Scale, false, 0.0f, 0, Thickness);
}

void FUOUDevelopmentDebugDrawContext::DrawString(
	const FVector& Location,
	const FString& Text,
	const FColor& Color,
	float TextScale)
{
	DrawDebugString(World, Location, Text, nullptr, Color, 0.0f, true, TextScale);
}

void FUOUDevelopmentDebugDrawContext::AddOnScreenMessage(
	int32 Key,
	const FString& Text,
	const FColor& Color,
	const FVector2D& TextScale)
{
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(Key, 0.0f, Color, Text, false, TextScale);
	}
}
