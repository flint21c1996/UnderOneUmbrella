// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/Traversal/UOULadderActor.h"

#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "EditorWorldUtils.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUOULadderModularHeightTest,
	"UnderOneUmbrella.Traversal.Ladder.ModularHeightUpdatesTraversalLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUOULadderModularHeightTest::RunTest(const FString& Parameters)
{
	UWorld* UninitializedWorld = UWorld::CreateWorld(
		EWorldType::Editor,
		false,
		TEXT("UOULadderModularHeightWorld"),
		nullptr,
		false,
		ERHIFeatureLevel::Num,
		nullptr,
		true);
	FScopedEditorWorld ScopedWorld(
		UninitializedWorld,
		UWorld::InitializationValues()
			.RequiresHitProxies(false)
			.ShouldSimulatePhysics(false)
			.EnableTraceCollision(true)
			.CreateNavigation(false)
			.CreateAISystem(false)
			.AllowAudioPlayback(false)
			.CreatePhysicsScene(true));
	UWorld* World = ScopedWorld.GetWorld();
	TestNotNull(TEXT("사다리 모듈 테스트 월드를 생성한다"), World);
	if (World == nullptr)
	{
		return false;
	}

	AUOULadderActor* Ladder = World->SpawnActor<AUOULadderActor>();
	TestNotNull(TEXT("사다리 액터를 생성한다"), Ladder);
	if (Ladder == nullptr || Ladder->GetLadderSegments() == nullptr)
	{
		return false;
	}

	UStaticMesh* SegmentMesh = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	TestNotNull(TEXT("반복 생성 검증용 메시를 불러온다"), SegmentMesh);
	if (SegmentMesh == nullptr)
	{
		return false;
	}

	Ladder->GetLadderSegments()->SetStaticMesh(SegmentMesh);
	Ladder->SetLadderHeight(425.0f);

	TestTrue(
		TEXT("사다리 액터 자체 스케일은 높이와 무관하게 유지된다"),
		Ladder->GetActorScale3D().Equals(FVector::OneVector));
	TestEqual(
		TEXT("425cm 높이는 50cm 모듈 9개로 덮는다"),
		Ladder->GetLadderSegments()->GetInstanceCount(),
		9);
	TestEqual(
		TEXT("상단 등반 위치는 BP에 배치한 위치를 유지한다"),
		Ladder->GetTopClimbHeight(),
		400.0f);
	TestEqual(
		TEXT("위쪽 진입 위치는 BP에 배치한 위치를 유지한다"),
		Ladder->GetTopStandingLocation().Z,
		496.0);
	TestEqual(
		TEXT("사다리를 다 오른 뒤 최종 위치도 BP에 배치한 위치를 유지한다"),
		Ladder->GetTopExitLocation().Z,
		496.0);
	TestEqual(
		TEXT("상단 이탈 지점은 플랫폼보다 3cm 아래에 놓인다"),
		Ladder->GetTopExitHeight(),
		397.0f);

	FTransform LastInstanceTransform;
	const bool bHasLastInstance = Ladder->GetLadderSegments()->GetInstanceTransform(
		8,
		LastInstanceTransform,
		false);
	TestTrue(TEXT("마지막 사다리 모듈 인스턴스가 존재한다"), bHasLastInstance);
	if (bHasLastInstance)
	{
		TestEqual(
			TEXT("마지막 모듈은 스케일 변경 없이 400cm 지점에 배치된다"),
			LastInstanceTransform.GetLocation(),
			FVector(0.0f, 0.0f, 400.0f));
		TestTrue(
			TEXT("모든 모듈은 원본 스케일을 유지한다"),
			LastInstanceTransform.GetScale3D().Equals(FVector::OneVector));
	}

	const UBoxComponent* DetectionVolume = Ladder->GetDetectionVolume();
	TestNotNull(TEXT("사다리 중앙 등반 가이드가 존재한다"), DetectionVolume);
	if (DetectionVolume != nullptr)
	{
		const float DetectionTop =
			DetectionVolume->GetRelativeLocation().Z + DetectionVolume->GetUnscaledBoxExtent().Z;
		TestTrue(
			TEXT("중앙 등반 가이드는 갱신된 상단 등반 지점까지 포함한다"),
			DetectionTop >= Ladder->GetTopClimbHeight());
	}

	TestNotNull(TEXT("아래쪽 진입 영역이 별도로 존재한다"), Ladder->GetBottomEntryVolume());
	TestNotNull(TEXT("위쪽 진입 영역이 별도로 존재한다"), Ladder->GetTopEntryVolume());
	TestFalse(
		TEXT("중앙 등반 가이드는 진입 영역으로 사용하지 않는다"),
		Ladder->IsEntryVolume(Ladder->GetDetectionVolume()));
	TestTrue(
		TEXT("아래쪽 박스만 진입 영역으로 인식한다"),
		Ladder->IsEntryVolume(Ladder->GetBottomEntryVolume()));
	TestTrue(
		TEXT("위쪽 박스만 진입 영역으로 인식한다"),
		Ladder->IsEntryVolume(Ladder->GetTopEntryVolume()));

	if (DetectionVolume != nullptr)
	{
		const FVector CustomBottomExtent(333.0f, 222.0f, 111.0f);
		const FVector CustomTopExtent(321.0f, 210.0f, 99.0f);
		Ladder->GetBottomEntryVolume()->SetBoxExtent(CustomBottomExtent);
		Ladder->GetBottomEntryVolume()->SetRelativeLocation(FVector(44.0f, 55.0f, 66.0f));
		Ladder->GetBottomEntryVolume()->SetRelativeScale3D(FVector(1.25f, 1.5f, 1.75f));
		Ladder->GetTopEntryVolume()->SetBoxExtent(CustomTopExtent);
		Ladder->GetTopEntryVolume()->SetRelativeLocation(FVector(77.0f, 88.0f, 99.0f));
		Ladder->GetTopEntryVolume()->SetRelativeScale3D(FVector(1.1f, 1.2f, 1.3f));
		Ladder->GetDetectionVolume()->SetRelativeScale3D(FVector(1.0f, 1.0f, 2.0f));
		Ladder->OnConstruction(Ladder->GetActorTransform());

		TestEqual(
			TEXT("빨간 중앙 박스의 실제 높이가 Ladder Height의 원본이 된다"),
			Ladder->GetLadderHeight(),
			850.0f);
		TestEqual(
			TEXT("빨간 박스의 상대 Z 스케일은 설정값으로 유지된다"),
			Ladder->GetDetectionVolume()->GetRelativeScale3D().Z,
			2.0);
		TestEqual(
			TEXT("빨간 박스가 바뀌어도 아래쪽 등반 위치는 덮어쓰지 않는다"),
			Ladder->GetBottomClimbHeight(),
			96.0f);
		TestEqual(
			TEXT("빨간 박스가 바뀌어도 위쪽 등반 위치는 덮어쓰지 않는다"),
			Ladder->GetTopClimbHeight(),
			400.0f);
		TestEqual(
			TEXT("빨간 박스 높이가 바뀌어도 위쪽 진입 위치는 덮어쓰지 않는다"),
			Ladder->GetTopStandingLocation().Z,
			496.0);
		TestEqual(
			TEXT("빨간 박스 높이가 바뀌어도 위쪽 최종 위치는 덮어쓰지 않는다"),
			Ladder->GetTopExitLocation().Z,
			496.0);
		TestEqual(
			TEXT("늘어난 빨간 박스 높이를 50cm 모듈 17개로 덮는다"),
			Ladder->GetLadderSegments()->GetInstanceCount(),
			17);
		TestEqual(
			TEXT("초록 진입 박스의 BP 편집 크기를 유지한다"),
			Ladder->GetBottomEntryVolume()->GetUnscaledBoxExtent(),
			CustomBottomExtent);
		TestTrue(
			TEXT("초록 진입 박스의 BP 편집 스케일을 유지한다"),
			Ladder->GetBottomEntryVolume()->GetRelativeScale3D().Equals(FVector(1.25f, 1.5f, 1.75f)));
		TestEqual(
			TEXT("초록 진입 박스의 BP 편집 위치를 모두 유지한다"),
			Ladder->GetBottomEntryVolume()->GetRelativeLocation(),
			FVector(44.0f, 55.0f, 66.0f));
		TestEqual(
			TEXT("파란 진입 박스의 BP 편집 크기를 유지한다"),
			Ladder->GetTopEntryVolume()->GetUnscaledBoxExtent(),
			CustomTopExtent);
		TestTrue(
			TEXT("파란 진입 박스의 BP 편집 스케일을 유지한다"),
			Ladder->GetTopEntryVolume()->GetRelativeScale3D().Equals(FVector(1.1f, 1.2f, 1.3f)));
		TestEqual(
			TEXT("파란 진입 박스의 BP 편집 위치를 모두 유지한다"),
			Ladder->GetTopEntryVolume()->GetRelativeLocation(),
			FVector(77.0f, 88.0f, 99.0f));
	}

	return true;
}

#endif
