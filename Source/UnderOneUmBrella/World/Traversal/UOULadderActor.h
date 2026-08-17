// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOULadderActor.generated.h"

class UBoxComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;

// Defines the detection volume and alignment points used by player ladder traversal.
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="UOU Ladder Actor"))
class UNDERONEUMBRELLA_API AUOULadderActor : public AActor
{
	GENERATED_BODY()

public:
	AUOULadderActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	// Returns the point and facing direction on the explicit climbing rail nearest to a world location.
	FVector GetClimbLocationNear(const FVector& WorldLocation) const;
	FVector GetClimbDirection() const;
	FRotator GetClimbingRotationNear(const FVector& WorldLocation) const;

	// Returns world transforms calculated from the lightweight editor transform properties.
	FVector GetBottomStandingLocation() const;
	FVector GetTopStandingLocation() const;
	FVector GetBottomExitLocation() const;
	FVector GetTopExitLocation() const;
	FVector GetBottomClimbLocation() const;
	FVector GetTopClimbLocation() const;
	FRotator GetBottomStandingRotation() const;
	FRotator GetTopStandingRotation() const;
	FRotator GetBottomExitRotation() const;
	FRotator GetTopExitRotation() const;
	FRotator GetBottomClimbRotation() const;
	FRotator GetTopClimbRotation() const;
	FVector GetBottomEntryDirection() const;
	FVector GetTopEntryDirection() const;

	// +X points away from the ladder face and the character faces the opposite direction while climbing.
	FVector GetOutwardNormal() const;

	// Rebuilds the modular visual and traversal anchors without scaling the actor itself.
	UFUNCTION(BlueprintCallable, Category = "Ladder|Dimensions")
	void SetLadderHeight(float NewLadderHeight);

	float GetLadderHeight() const { return LadderHeight; }
	float GetLadderSegmentHeight() const { return LadderSegmentHeight; }
	float GetBottomClimbHeight() const;
	float GetTopClimbHeight() const;
	float GetBottomExitHeight() const;
	float GetTopExitHeight() const;
	float GetClimbSpeed() const { return ClimbSpeed; }
	UBoxComponent* GetDetectionVolume() const { return DetectionVolume; }
	UBoxComponent* GetBottomEntryVolume() const { return BottomEntryVolume; }
	UBoxComponent* GetTopEntryVolume() const { return TopEntryVolume; }
	bool IsEntryVolume(const UPrimitiveComponent* Component) const;
	UHierarchicalInstancedStaticMeshComponent* GetLadderSegments() const { return LadderSegments; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder", meta=(ToolTip="사다리 액터의 로컬 좌표 기준입니다. 실제 캐릭터 위치는 각 앵커가 결정하므로 루트 원점을 반드시 바닥에 둘 필요는 없습니다."))
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Volumes", meta=(ToolTip="사다리를 오르는 중앙 구간을 보여 주는 빨간색 가이드입니다. 캐릭터 진입 감지에는 사용하지 않습니다."))
	TObjectPtr<UBoxComponent> DetectionVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Volumes", meta=(ToolTip="아래쪽에서 사다리 진입을 허용하는 초록색 영역입니다."))
	TObjectPtr<UBoxComponent> BottomEntryVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Volumes", meta=(ToolTip="위쪽에서 사다리 진입을 허용하는 파란색 영역입니다."))
	TObjectPtr<UBoxComponent> TopEntryVolume = nullptr;

	// Optional ladder visual; its relative transform can align any mesh to the traversal origin.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder", meta=(ToolTip="화면에 표시할 사다리 메시입니다. 메시 원점이 맞지 않으면 이 컴포넌트의 상대 위치와 회전을 조절합니다."))
	TObjectPtr<UStaticMeshComponent> VisualMesh = nullptr;

	// Optional modular visual. Assign a single ladder section mesh to repeat it without stretching.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Modular", meta=(ToolTip="한 칸짜리 사다리 모듈을 반복 배치하는 HISM 컴포넌트입니다. 메시를 지정하면 기존 Visual Mesh 대신 사용합니다."))
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> LadderSegments = nullptr;

	// These values represent the character ActorLocation and rotation, not its feet or mesh origin.
	// MakeEditWidget keeps the component tree small while still allowing viewport authoring.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Transforms|Bottom", meta=(MakeEditWidget, ToolTip="아래에서 진입 애니메이션을 시작하기 전 캐릭터 위치와 방향입니다."))
	FTransform BottomEntryTransform = FTransform(
		FRotator(0.0f, 180.0f, 0.0f),
		FVector(100.0f, 0.0f, 96.0f),
		FVector::OneVector);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Transforms|Bottom", meta=(MakeEditWidget, ToolTip="아래쪽에서 사다리에 붙었을 때 캐릭터 위치와 방향입니다."))
	FTransform BottomClimbTransform = FTransform(
		FRotator(0.0f, 180.0f, 0.0f),
		FVector(55.0f, 0.0f, 96.0f),
		FVector::OneVector);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Transforms|Bottom", meta=(MakeEditWidget, ToolTip="아래로 내려온 뒤 퇴장 애니메이션이 끝나는 캐릭터 위치와 방향입니다."))
	FTransform BottomExitTransform = FTransform(
		FRotator(0.0f, 180.0f, 0.0f),
		FVector(100.0f, 0.0f, 96.0f),
		FVector::OneVector);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Transforms|Top", meta=(MakeEditWidget, ToolTip="위쪽 등반이 끝나거나 위에서 내려가기 시작할 때의 사다리 자세 위치입니다."))
	FTransform TopClimbTransform = FTransform(
		FRotator(0.0f, 180.0f, 0.0f),
		FVector(55.0f, 0.0f, 400.0f),
		FVector::OneVector);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Transforms|Top", meta=(MakeEditWidget, ToolTip="위에서 내려가기 애니메이션을 시작하기 전 캐릭터 위치와 방향입니다."))
	FTransform TopEntryTransform = FTransform(
		FRotator(0.0f, 180.0f, 0.0f),
		FVector(-80.0f, 0.0f, 496.0f),
		FVector::OneVector);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Transforms|Top", meta=(MakeEditWidget, ToolTip="사다리를 다 오른 뒤 퇴장 애니메이션이 끝나는 캐릭터 위치와 방향입니다."))
	FTransform TopExitTransform = FTransform(
		FRotator(0.0f, 180.0f, 0.0f),
		FVector(-80.0f, 0.0f, 496.0f),
		FVector::OneVector);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Dimensions", meta=(ForceUnits="cm", ToolTip="빨간 중앙 등반 박스의 Box Extent Z와 상대 Scale Z에서 자동 계산되는 읽기 전용 값입니다."))
	float LadderHeight = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Modular", meta=(ClampMin="1.0", UIMin="1.0", ForceUnits="cm", ToolTip="HISM에 지정한 한 칸짜리 사다리 모듈의 실제 높이입니다. Ladder Height를 덮도록 필요한 개수를 계산합니다."))
	float LadderSegmentHeight = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Exit", meta=(ClampMin="0.0", ForceUnits="cm", ToolTip="상단 플랫폼에 도달하기 전에 퇴장 전환을 시작할 거리입니다."))
	float TopExitInset = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Exit", meta=(ClampMin="0.0", ForceUnits="cm", ToolTip="하단 사다리 위치에 도달하기 전에 퇴장 전환을 시작할 추가 거리입니다."))
	float BottomExitOffset = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Dimensions", meta=(ClampMin="20.0", ToolTip="사다리 앞뒤 감지 범위의 절반 크기(cm)입니다. 전체 앞뒤 감지 길이는 이 값의 두 배입니다."))
	float DetectionDepth = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Dimensions", meta=(ClampMin="20.0", ToolTip="사다리 좌우 감지 범위의 절반 크기(cm)입니다. 넓은 사다리에서는 값을 키웁니다."))
	float DetectionHalfWidth = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Movement", meta=(ClampMin="1.0", ToolTip="사다리를 오르내릴 때 캐릭터의 최대 이동 속도(cm/s)입니다."))
	float ClimbSpeed = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder|Debug", meta=(DisplayName="Show Traversal Debug In Game", ToolTip="체크하면 플레이 중 여섯 개의 캐릭터 Transform과 세 개의 사다리 영역을 색상별 디버그 도형으로 표시합니다."))
	bool bShowTraversalDebugInGame = false;
private:
	void RefreshLadderLayout();
	void UpdateLadderDimensionsFromVolume();
	void RebuildLadderSegments();
	void ApplyDetectionVolumeSettings();
	void DrawTraversalDebug() const;
};
