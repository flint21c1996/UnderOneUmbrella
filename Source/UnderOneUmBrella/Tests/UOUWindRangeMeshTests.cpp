// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "EditorWorldUtils.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "World/Wind/UOUWindEmitterActor.h"
#include "World/Wind/UOUWindInteractionSurfaceComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUOUWindRangeMeshTest,
	"UnderOneUmbrella.Wind.Visual.RangeMeshFollowsGameplayPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOUWindRangeMeshTest::RunTest(const FString& Parameters)
{
	UWorld* Uninitialized = UWorld::CreateWorld(EWorldType::Editor, false,
		TEXT("WindRangeMeshTestWorld"), nullptr, false, ERHIFeatureLevel::Num, nullptr, true);
	FScopedEditorWorld ScopedWorld(Uninitialized, UWorld::InitializationValues()
		.RequiresHitProxies(false).ShouldSimulatePhysics(false).EnableTraceCollision(true)
		.CreateNavigation(false).CreateAISystem(false).AllowAudioPlayback(false).CreatePhysicsScene(true));
	UWorld* World = ScopedWorld.GetWorld();
	if (!TestNotNull(TEXT("World"), World)) return false;
	AUOUWindEmitterActor* Emitter = World->SpawnActor<AUOUWindEmitterActor>();
	UStaticMesh* Asset = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Emitter"), Emitter) || !TestNotNull(TEXT("Cube asset"), Asset)) return false;
	UStaticMeshComponent* Cube = NewObject<UStaticMeshComponent>(Emitter, TEXT("Cube"));
	Cube->SetupAttachment(Emitter->WindOrigin);
	Cube->SetStaticMesh(Asset);
	Cube->SetMobility(EComponentMobility::Movable);
	Cube->RegisterComponent();
	Cube->SetRelativeLocation(FVector(1500, 0, 0));
	Cube->SetRelativeScale3D(FVector(15, 2.25, 2.25));
	Emitter->MaxWindDistance = 1700;
	Emitter->WindRadius = 120;
	Emitter->SetActorLocationAndRotation(FVector(100, 200, 300), FRotator(40, -110, -90));
	Emitter->SetActorScale3D(FVector(2, 3, 4));
	Emitter->RebuildWindPath();
	TestEqual(TEXT("One straight segment"), Emitter->WindPathSegments.Num(), 1);
	if (Emitter->WindPathSegments.Num() != 1) return false;
	const FUOUWindPathSegment Segment = Emitter->WindPathSegments[0];
	TestTrue(TEXT("World center matches debug center despite rotated/scaled parent"),
		Cube->Bounds.Origin.Equals((Segment.Start + Segment.End) * 0.5, 0.1));
	TestTrue(TEXT("World dimensions match 1700 x 240 x 240"),
		(Cube->GetComponentScale() * Asset->GetBounds().BoxExtent * 2.0).Equals(FVector(1700, 240, 240), 0.1));
	TestTrue(TEXT("Long axis follows wind direction"), Cube->GetForwardVector().Equals(Segment.Direction, 0.001));
	TestTrue(TEXT("Range display never blocks wind or players"), Cube->GetCollisionEnabled() == ECollisionEnabled::NoCollision);

	// A wall must shorten the mesh to the computed path, not the configured maximum.
	AActor* Wall = World->SpawnActor<AActor>();
	UBoxComponent* WallBox = NewObject<UBoxComponent>(Wall);
	Wall->SetRootComponent(WallBox);
	WallBox->SetBoxExtent(FVector(10, 500, 500));
	WallBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WallBox->SetCollisionResponseToAllChannels(ECR_Block);
	WallBox->RegisterComponent();
	WallBox->SetWorldLocationAndRotation(Segment.Start + Segment.Direction * 600, Segment.Direction.Rotation());
	Emitter->RebuildWindPath();
	TestEqual(TEXT("Wall leaves one segment"), Emitter->WindPathSegments.Num(), 1);
	if (Emitter->WindPathSegments.Num() == 1)
	{
		const FUOUWindPathSegment& Short = Emitter->WindPathSegments[0];
		TestTrue(TEXT("Wall truncates path"), Short.GetLength() < 650);
		TestTrue(TEXT("Cube length follows obstruction"),
			FMath::IsNearlyEqual(Cube->GetComponentScale().X * Asset->GetBounds().BoxExtent.X * 2.0,
				static_cast<double>(Short.GetLength()), 0.1));
	}
	Emitter->SetWindEnabled(false);
	TestFalse(TEXT("Disabled wind hides range mesh"), Cube->IsVisible());
	Emitter->SetWindEnabled(true);
	TestTrue(TEXT("Reenabled wind restores range mesh"), Cube->IsVisible());
	WallBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UUOUWindInteractionSurfaceComponent* Mirror = NewObject<UUOUWindInteractionSurfaceComponent>(Wall);
	Mirror->SetBoxExtent(FVector(10, 500, 500));
	Mirror->RegisterComponent();
	Mirror->SetWorldLocationAndRotation(Segment.Start + Segment.Direction * 600, Segment.Direction.Rotation());
	Emitter->RebuildWindPath();
	TestEqual(TEXT("Reflection creates two segments"), Emitter->WindPathSegments.Num(), 2);
	TInlineComponentArray<UStaticMeshComponent*> Meshes(Emitter);
	int32 VisibleRanges = 0;
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (Mesh->GetStaticMesh() == Asset && Mesh->IsVisible())
		{
			++VisibleRanges;
		}
	}
	TestEqual(TEXT("Each reflected segment has a range mesh"), VisibleRanges, 2);
	Emitter->SetWindEnabled(false);
	Meshes.Reset();
	Emitter->GetComponents(Meshes);
	TestEqual(TEXT("Disabling destroys generated reflection meshes"), Meshes.Num(), 2);
	TestFalse(TEXT("Original cube stays hidden after reflection cleanup"), Cube->IsVisible());
	return true;
}

#endif
