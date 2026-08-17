// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Debug/UOUDebugProvider.h"
#include "Engine/EngineTypes.h"
#include "UOURotatableMirrorComponent.generated.h"

class APawn;
class UAnimMontage;
class UPrimitiveComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FUOUMirrorRotationChangedSignature,
	float,
	AngleDegrees);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FUOUMirrorPushStartedSignature,
	APawn*,
	Pusher,
	USceneComponent*,
	PushHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FUOUMirrorPushEndedSignature,
	APawn*,
	Pusher);

// 플레이어가 거울 가장자리를 미는 방향을 회전축 기준 각도로 변환합니다.
UCLASS(ClassGroup=(Light), meta=(BlueprintSpawnableComponent, DisplayName="UOU Rotatable Mirror", ToolTip = "플레이어가 Push Volume 안에서 거울을 밀면 지정한 축을 중심으로 회전시킵니다."))
class UNDERONEUMBRELLA_API UUOURotatableMirrorComponent
	: public UActorComponent
	, public IUOUDebugProvider
{
	GENERATED_BODY()

public:
	UUOURotatableMirrorComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual EUOUDebugCategory GetDebugCategory_Implementation() const override;
	virtual FText GetDebugSummaryText_Implementation() const override;

#if UOU_WITH_DEVELOPMENT_TOOLS
	virtual bool ShouldDrawDevelopmentDebugLabel() const override { return false; }
	virtual void GatherDevelopmentDebugDraw(IUOUDevelopmentDebugDrawContext& Context) const override;
#endif

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Rotation", meta = (ToolTip = "플레이어 밀기에 의한 거울 회전을 사용할지 여부입니다."))
	bool bRotationEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Components", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.SceneComponent", ToolTip = "거울 메시와 빛 반사면의 부모가 되는 회전 컴포넌트입니다. 비워두면 Root Component를 사용합니다."))
	FComponentReference RotatingComponentReference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Components", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.PrimitiveComponent", ToolTip = "플레이어가 거울을 미는 영역입니다. Pawn과 Overlap하도록 별도 Box Component를 사용하는 것을 권장합니다."))
	FComponentReference PushVolumeReference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Components", meta = (ToolTip = "Push Volume을 지정하지 않았을 때 이름으로 자동 탐색합니다."))
	bool bAutoFindPushVolume = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Components", meta = (EditCondition = "bAutoFindPushVolume", ToolTip = "자동 탐색할 Push Volume의 컴포넌트 이름입니다."))
	FName PreferredPushVolumeName = TEXT("PushVolume");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Components", meta = (ToolTip = "찾은 Push Volume을 Query Only 및 Pawn Overlap으로 자동 설정합니다."))
	bool bConfigurePushVolumeCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Rotation", meta = (ToolTip = "회전 컴포넌트 로컬 공간에서 사용할 회전축입니다. 세워진 거울은 기본값인 Z축을 사용합니다."))
	FVector LocalRotationAxis = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Rotation", meta = (ToolTip = "회전 컴포넌트 원점에서 실제 회전축까지의 로컬 위치 보정값입니다."))
	FVector LocalPivotOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Rotation", meta = (ClampMax = "0.0", Units = "deg", ToolTip = "초기 배치 각도를 기준으로 허용할 최소 회전각입니다."))
	float MinimumAngle = -90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Rotation", meta = (ClampMin = "0.0", Units = "deg", ToolTip = "초기 배치 각도를 기준으로 허용할 최대 회전각입니다."))
	float MaximumAngle = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Rotation", meta = (ClampMin = "0.0", Units = "deg/s", ToolTip = "플레이어가 가장 강하게 밀 때의 최대 회전 속도입니다."))
	float MaximumRotationSpeed = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Push", meta = (ClampMin = "0.0", Units = "cm/s", ToolTip = "이 속도보다 느린 플레이어 이동은 밀기로 처리하지 않습니다."))
	float MinimumPushSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Push", meta = (ToolTip = "활성화하면 플레이어가 조종하는 Pawn만 거울을 돌릴 수 있습니다."))
	bool bPlayerControlledOnly = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Push", meta = (ToolTip = "켜면 잡지 않아도 Push Volume 안의 플레이어 이동만으로 회전합니다. 우클릭 고정 조작에서는 끄는 것을 권장합니다."))
	bool bAllowProximityPush = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Push|Grab", meta = (ToolTip = "우클릭으로 거울 손잡이에 붙는 조작을 사용합니다."))
	bool bEnableGrabPush = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Push|Grab", meta = (ToolTip = "거울 액터에서 손잡이 후보를 찾을 때 사용하는 컴포넌트 태그입니다."))
	FName PushHandleTag = TEXT("MirrorPushHandle");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Push|Grab", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "플레이어가 손잡이를 잡을 수 있는 최대 수평 거리입니다."))
	float MaximumGrabDistance = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Push|Grab", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "잡는 순간 플레이어 중심을 손잡이에서 떨어뜨릴 거리입니다."))
	float PlayerAttachDistance = 65.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Push|Grab", meta = (ClampMin = "0.0", Units = "cm", DisplayName = "거울 표면 최대 간격", ToolTip = "플레이어 캡슐과 거울 메시 표면 사이의 허용 간격입니다. 거울에 붙어 있는 상태에서만 잡히도록 제한합니다."))
	float MaximumGrabSurfaceGap = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Push|Grab", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg", DisplayName = "잡기 시작 최대 방향 보정각", ToolTip = "플레이어가 거울을 바라보도록 상호작용 시작 시 보정할 수 있는 최대 각도입니다. 이보다 크게 돌아야 하면 잡기를 시작하지 않습니다."))
	float MaximumGrabFacingCorrectionAngle = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Push|Grab", meta = (ToolTip = "잡는 순간 플레이어를 손잡이 앞 위치로 정렬합니다."))
	bool bSnapPlayerOnGrab = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Push|Grab", meta = (ToolTip = "회전하는 동안 플레이어가 손잡이를 바라보게 합니다."))
	bool bFacePushHandle = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Push|Animation", meta = (ToolTip = "거울을 잡는 동안 재생할 선택적 밀기 Montage입니다. 비워두면 위치와 방향만 고정합니다."))
	TObjectPtr<UAnimMontage> PushMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Push|Animation", meta = (ClampMin = "0.01", ToolTip = "밀기 Montage 재생 속도입니다."))
	float PushMontagePlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Push", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "회전축에서 이 거리보다 안쪽에 있는 플레이어는 거울을 돌릴 수 없습니다."))
	float MinimumLeverArm = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mirror|Push", meta = (ClampMin = "1.0", Units = "cm", ToolTip = "이 거리 이상 가장자리를 밀면 최대 회전력이 적용됩니다."))
	float FullTorqueLeverArm = 100.0f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Mirror|Runtime", meta = (Units = "deg", ToolTip = "초기 배치 각도를 기준으로 한 현재 회전각입니다."))
	float CurrentAngle = 0.0f;

	UPROPERTY(BlueprintAssignable, Category = "Mirror|Events", meta = (ToolTip = "거울 회전각이 변경될 때 호출됩니다."))
	FUOUMirrorRotationChangedSignature OnMirrorRotationChanged;

	UPROPERTY(BlueprintAssignable, Category = "Mirror|Events", meta = (ToolTip = "플레이어가 거울 손잡이를 잡았을 때 호출됩니다."))
	FUOUMirrorPushStartedSignature OnMirrorPushStarted;

	UPROPERTY(BlueprintAssignable, Category = "Mirror|Events", meta = (ToolTip = "플레이어가 거울 손잡이를 놓았을 때 호출됩니다."))
	FUOUMirrorPushEndedSignature OnMirrorPushEnded;

	UFUNCTION(BlueprintCallable, Category = "Mirror|Rotation", meta = (ToolTip = "허용 범위 안에서 거울의 상대 회전각을 지정합니다."))
	void SetMirrorAngle(float NewAngle);

	UFUNCTION(BlueprintCallable, Category = "Mirror|Rotation", meta = (ToolTip = "거울을 처음 배치된 각도로 되돌립니다."))
	void ResetMirrorAngle();

	UFUNCTION(BlueprintPure, Category = "Mirror|Push")
	bool CanBeginMirrorPush(const APawn* Pusher) const;

	UFUNCTION(BlueprintCallable, Category = "Mirror|Push")
	bool TryBeginMirrorPush(APawn* Pusher);

	UFUNCTION(BlueprintCallable, Category = "Mirror|Push")
	void EndMirrorPush(APawn* Pusher);

	UFUNCTION(BlueprintCallable, Category = "Mirror|Push")
	float ApplyMirrorPushInput(float AxisInput, float DeltaTime);

	UFUNCTION(BlueprintPure, Category = "Mirror|Push")
	bool IsBeingPushed() const { return CurrentPusher != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Mirror|Push")
	FVector GetGrabReferenceLocation() const;

	UFUNCTION(BlueprintPure, Category = "Mirror|Push")
	FVector GetWorldInputAxisForInteractor(const AActor* Interactor) const;

	UFUNCTION(BlueprintPure, Category = "Mirror|Rotation")
	USceneComponent* GetRotatingComponent() const { return RotatingComponent.Get(); }

	UFUNCTION(BlueprintPure, Category = "Mirror|Push")
	UPrimitiveComponent* GetPushVolume() const { return PushVolume.Get(); }

protected:
	void ValidateSettings();
	void ResolveComponents();
	void ConfigurePushVolume();
	void CaptureInitialTransform();
	void ApplyCurrentRotation();
	float CalculatePushInput(const APawn* Pusher) const;
	FVector GetPivotWorldLocation() const;
	FVector GetRotationAxisWorld() const;
	USceneComponent* FindNearestPushHandle(const AActor* Interactor) const;
	bool FindClosestGrabSurfacePoint(const APawn* Pusher, FVector& OutClosestPoint, float& OutSurfaceGap) const;
	float CalculateGrabSurfaceGap(const APawn* Pusher) const;
	bool UpdateAttachedPlayerTransform(bool bSweepMovement);
	void ApplyPusherFacing() const;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> RotatingComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> PushVolume = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<APawn> CurrentPusher = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> ActivePushHandle = nullptr;

	FVector AttachedPlayerLocalLocation = FVector::ZeroVector;
	FQuat AttachedPlayerLocalRotation = FQuat::Identity;

	FQuat InitialRelativeRotation = FQuat::Identity;
	FVector InitialRelativeLocation = FVector::ZeroVector;
	bool bInitialTransformCaptured = false;
};
