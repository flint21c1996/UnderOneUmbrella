// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UOULadderActor.generated.h"

class UBoxComponent;
class UArrowComponent;
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

	// Returns the point and facing direction on the explicit climbing rail nearest to a world location.
	FVector GetClimbLocationNear(const FVector& WorldLocation) const;
	FVector GetClimbDirection() const;
	FRotator GetClimbingRotationNear(const FVector& WorldLocation) const;

	// Returns the exact actor-origin transforms authored with the editor anchor components.
	FVector GetBottomStandingLocation() const;
	FVector GetTopStandingLocation() const;
	FVector GetBottomClimbLocation() const;
	FVector GetTopClimbLocation() const;
	FRotator GetBottomStandingRotation() const;
	FRotator GetTopStandingRotation() const;
	FRotator GetBottomClimbRotation() const;
	FRotator GetTopClimbRotation() const;
	FVector GetBottomEntryDirection() const;
	FVector GetTopEntryDirection() const;

	// +X points away from the ladder face and the character faces the opposite direction while climbing.
	FVector GetOutwardNormal() const;

	float GetBottomEntryTolerance() const { return BottomEntryTolerance; }
	float GetTopEntryTolerance() const { return TopEntryTolerance; }
	float GetBottomClimbHeight() const;
	float GetTopClimbHeight() const;
	float GetBottomExitHeight() const;
	float GetTopExitHeight() const;
	float GetClimbSpeed() const { return ClimbSpeed; }
	UBoxComponent* GetDetectionVolume() const { return DetectionVolume; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder", meta=(ToolTip="사다리 액터의 로컬 좌표 기준입니다. 실제 캐릭터 위치는 각 앵커가 결정하므로 루트 원점을 반드시 바닥에 둘 필요는 없습니다."))
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder", meta=(ToolTip="캐릭터가 사다리 근처에 있는지 감지하는 영역입니다. 실제 등반 종료 지점으로는 사용하지 않습니다."))
	TObjectPtr<UBoxComponent> DetectionVolume = nullptr;

	// Editor-only guide showing the lower exit threshold used by the capsule center.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Exit", meta=(ToolTip="아래쪽 퇴장이 시작되는 캐릭터 ActorLocation 높이입니다. 박스의 로컬 Z 위치가 실제 런타임 판정에 그대로 사용되며 충돌은 발생시키지 않습니다."))
	TObjectPtr<UBoxComponent> BottomExitMarker = nullptr;

	// Editor-only guide showing the upper exit threshold used by the capsule center.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Exit", meta=(ToolTip="위쪽 퇴장이 시작되는 캐릭터 ActorLocation 높이입니다. 박스의 로컬 Z 위치가 실제 런타임 판정에 그대로 사용되며 충돌은 발생시키지 않습니다."))
	TObjectPtr<UBoxComponent> TopExitMarker = nullptr;

	// Optional ladder visual; its relative transform can align any mesh to the traversal origin.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder", meta=(ToolTip="화면에 표시할 사다리 메시입니다. 메시 원점이 맞지 않으면 이 컴포넌트의 상대 위치와 회전을 조절합니다."))
	TObjectPtr<UStaticMeshComponent> VisualMesh = nullptr;

	// These anchors represent the character ActorLocation and rotation, not its feet or mesh origin.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Anchors", meta=(ToolTip="아래에서 진입하기 전과 아래로 퇴장한 후 캐릭터 ActorLocation이 놓일 위치와 방향입니다. 캐릭터의 발 위치가 아니라 캡슐 중심 기준입니다."))
	TObjectPtr<UArrowComponent> BottomStandingAnchor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Anchors", meta=(ToolTip="아래쪽에서 사다리에 붙었을 때 캐릭터 ActorLocation이 놓일 위치와 방향입니다. 캐릭터 체형과 진입 애니메이션에 맞춰 직접 배치합니다."))
	TObjectPtr<UArrowComponent> BottomClimbAnchor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Anchors", meta=(ToolTip="위쪽 등반이 끝나는 캐릭터 ActorLocation 위치와 방향입니다. 위쪽 퇴장 애니메이션이 시작될 자세에 맞춰 배치합니다."))
	TObjectPtr<UArrowComponent> TopClimbAnchor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Anchors", meta=(ToolTip="위 플랫폼에 올라선 후 캐릭터 ActorLocation이 놓일 최종 위치와 방향입니다. 캡슐 높이 보정은 자동으로 추가되지 않습니다."))
	TObjectPtr<UArrowComponent> TopStandingAnchor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Dimensions", meta=(ClampMin="20.0", ToolTip="사다리 앞뒤 감지 범위의 절반 크기(cm)입니다. 전체 앞뒤 감지 길이는 이 값의 두 배입니다."))
	float DetectionDepth = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Dimensions", meta=(ClampMin="20.0", ToolTip="사다리 좌우 감지 범위의 절반 크기(cm)입니다. 넓은 사다리에서는 값을 키웁니다."))
	float DetectionHalfWidth = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Entry", meta=(ClampMin="0.0", ToolTip="Bottom Climb Anchor 높이 주변에서 아래쪽 자동 진입을 허용할 추가 범위(cm)입니다."))
	float BottomEntryTolerance = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Entry", meta=(ClampMin="0.0", ToolTip="Top Climb Anchor 높이 주변에서 위쪽 자동 하강 진입을 허용할 추가 범위(cm)입니다."))
	float TopEntryTolerance = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ladder|Movement", meta=(ClampMin="1.0", ToolTip="사다리를 오르내릴 때 캐릭터의 최대 이동 속도(cm/s)입니다."))
	float ClimbSpeed = 160.0f;

private:
	void ApplyDetectionVolumeSettings();
};
