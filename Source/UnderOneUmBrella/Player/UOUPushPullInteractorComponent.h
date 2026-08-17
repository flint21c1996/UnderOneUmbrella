// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UOUPushPullInteractorComponent.generated.h"

class AUOUCharacter;
class UUOUInteractionComponent;
class UUOUCrankComponent;
class UUOUPushPullObjectComponent;
class UUOUUmbrellaComponent;
class UUOURotatableMirrorComponent;

// 플레이어가 퍼즐 블럭을 잡고 밀고 당기는 전용 흐름을 관리하는 컴포넌트다.
// 후보 탐색과 잡기 상태, 이동 입력 해석을 모두 이곳에서 맡는다.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class UNDERONEUMBRELLA_API UUOUPushPullInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 밀고 당기기 기본 규칙과 디버그 옵션을 초기화한다.
	UUOUPushPullInteractorComponent();

	// 시작 시 플레이어와 우산, 상호작용 컴포넌트를 연결한다.
	virtual void BeginPlay() override;

	// 현재 후보와 잡기 상태를 갱신하고 디버그를 그린다.
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 우산이 닫혀 있을 때만 손을 쓰게 제한할지 정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Rules")
	bool bRequireClosedUmbrella = true;

	// 공중 상태에서는 잡기를 막을지 정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Rules")
	bool bRequireGrounded = true;

	// 대상이 너무 멀어졌다고 판단할 추가 여유 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Rules", meta = (ClampMin = "0.0"))
	float ReleaseDistanceBuffer = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Movement", meta = (ClampMin = "0.0", ToolTip = "상자를 잡고 옮기는 동안 플레이어 최대 이동 속도입니다. 기존 속도가 더 낮으면 기존 속도를 유지합니다."))
	float PushPullWalkSpeed = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Movement", meta = (ToolTip = "잡은 순간의 플레이어-상자 수평 기준거리를 유지하도록 상자 속도를 보정합니다. Z축은 보정하지 않습니다."))
	bool bUseGrabDistanceCorrection = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Movement", meta = (ClampMin = "0.0", ToolTip = "기준거리 오차를 상자 보정 속도로 바꾸는 계수입니다. 값이 클수록 거리 차이를 빠르게 줄입니다."))
	float GrabDistanceCorrectionStrength = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Movement", meta = (ClampMin = "0.0", ToolTip = "기준거리 보정으로 추가하거나 줄일 수 있는 최대 수평 속도입니다."))
	float MaxGrabDistanceCorrectionSpeed = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Movement", meta = (ClampMin = "0.0", ToolTip = "이 거리 이하의 기준거리 오차는 보정하지 않습니다."))
	float GrabDistanceCorrectionDeadZone = 5.0f;

	// 후보를 찾을 때 플레이어 주변을 얼마나 넓게 볼지 정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Detection", meta = (ClampMin = "0.0"))
	float CandidateSearchRadius = 90.0f;

	// 일반 동적 오브젝트를 밀고 당기기 후보에 포함할지 정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Detection")
	bool bDetectWorldDynamic = true;

	// 물리 바디 오브젝트를 밀고 당기기 후보에 포함할지 정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Detection")
	bool bDetectPhysicsBody = true;

	// 퍼즐 무게 채널 오브젝트를 밀고 당기기 후보에 포함할지 정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPull|Detection")
	bool bDetectPuzzleWeight = true;

	// 화면 디버그 표시 여부는 이제 Debug Controller의 Player HUD가 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "PushPull|Debug", meta = (ToolTip = "이 값은 더 이상 화면 디버그 표시 여부를 결정하지 않습니다. Debug Controller의 Player HUD 옵션을 사용합니다."))
	bool bShowScreenDebug = false;

	// 월드 디버그 표시 여부는 이제 Debug Controller의 Interaction 카테고리가 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "PushPull|Debug", meta = (ToolTip = "이 값은 더 이상 월드 디버그 표시 여부를 결정하지 않습니다. Debug Controller의 Interaction 옵션을 사용합니다."))
	bool bShowWorldDebug = true;

	// 현재 가장 적합하다고 판단한 후보 오브젝트다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushPull|Runtime")
	TObjectPtr<UUOUPushPullObjectComponent> CurrentCandidateObject = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushPull|Runtime")
	TObjectPtr<UUOUCrankComponent> CurrentCandidateCrank = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushPull|Runtime")
	TObjectPtr<UUOURotatableMirrorComponent> CurrentCandidateMirror = nullptr;

	// 실제로 잡고 있는 오브젝트다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushPull|Runtime")
	TObjectPtr<UUOUPushPullObjectComponent> GrabbedObject = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushPull|Runtime")
	TObjectPtr<UUOUCrankComponent> GrabbedCrank = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushPull|Runtime")
	TObjectPtr<UUOURotatableMirrorComponent> GrabbedMirror = nullptr;

	// 지금 잡고 있는 오브젝트가 있는지 빠르게 확인한다.
	UFUNCTION(BlueprintPure, Category = "PushPull")
	bool IsGrabbing() const
	{
		return GrabbedObject != nullptr || GrabbedCrank != nullptr || GrabbedMirror != nullptr;
	}

	// 잡고 있는 동안에는 점프를 막기 위해 쓰는 헬퍼다.
	UFUNCTION(BlueprintPure, Category = "PushPull")
	bool BlocksJumping() const { return IsGrabbing(); }

	// 입력 시작 시 현재 후보를 기준으로 잡기를 시도한다.
	UFUNCTION(BlueprintCallable, Category = "PushPull")
	void HandleGrabPressed();

	// 입력 해제 시 현재 잡고 있는 오브젝트를 놓는다.
	UFUNCTION(BlueprintCallable, Category = "PushPull")
	void HandleGrabReleased();

	// 이동 입력을 밀기와 당기기 축으로 바꿔 현재 오브젝트에 전달한다.
	UFUNCTION(BlueprintCallable, Category = "PushPull")
	bool HandleMoveInput(const FVector2D& MovementVector, float MovementYaw);

	// 후보 탐색 범위를 사용하는 외부 시스템에 현재 설정된 반경을 제공합니다.
	float GetCandidateSearchRadius() const { return CandidateSearchRadius; }

	// 현재 플레이어 상태를 반영한 후보 탐색 중심 위치를 반환합니다.
	FVector GetCandidateDetectionOriginLocation() const { return GetDetectionOriginLocation(); }

	// 현재 후보 종류와 무관하게 잡기 기준 위치를 조회합니다.
	bool TryGetCurrentCandidateReferenceLocation(FVector& OutLocation) const;

	// 현재 잡은 대상 종류와 무관하게 잡기 기준 위치를 조회합니다.
	bool TryGetCurrentGrabbedReferenceLocation(FVector& OutLocation) const;

	// 통합 플레이어 디버그 HUD에서 현재 후보를 읽기 위한 접근자입니다.
	UFUNCTION(BlueprintPure, Category = "PushPull|Debug")
	UUOUPushPullObjectComponent* GetCurrentCandidateObject() const { return CurrentCandidateObject; }

	UFUNCTION(BlueprintPure, Category = "PushPull|Debug")
	UUOUCrankComponent* GetCurrentCandidateCrank() const { return CurrentCandidateCrank; }

	UFUNCTION(BlueprintPure, Category = "PushPull|Debug")
	UUOURotatableMirrorComponent* GetCurrentCandidateMirror() const { return CurrentCandidateMirror; }

	// 통합 플레이어 디버그 HUD에서 실제 잡은 대상을 읽기 위한 접근자입니다.
	UFUNCTION(BlueprintPure, Category = "PushPull|Debug")
	UUOUPushPullObjectComponent* GetGrabbedObject() const { return GrabbedObject; }

	UFUNCTION(BlueprintPure, Category = "PushPull|Debug")
	UUOUCrankComponent* GetGrabbedCrank() const { return GrabbedCrank; }

	UFUNCTION(BlueprintPure, Category = "PushPull|Debug")
	UUOURotatableMirrorComponent* GetGrabbedMirror() const { return GrabbedMirror; }

	// 통합 플레이어 디버그 HUD에서 grab 입력 유지 상태를 읽기 위한 접근자입니다.
	UFUNCTION(BlueprintPure, Category = "PushPull|Debug")
	bool IsGrabInputHeld() const { return bGrabInputHeld; }

	// 통합 플레이어 디버그 HUD에서 손을 쓸 수 있는지 확인하기 위한 접근자입니다.
	UFUNCTION(BlueprintPure, Category = "PushPull|Debug")
	bool CanUseHandsForDebug() const { return CanUseHands(); }

	// 통합 플레이어 디버그 HUD에서 잡은 대상의 거리 이탈 여부를 확인하기 위한 접근자입니다.
	UFUNCTION(BlueprintPure, Category = "PushPull|Debug")
	bool IsGrabbedObjectTooFarForDebug() const { return IsGrabbedObjectTooFar(); }

	// 통합 플레이어 디버그 HUD에서 현재 이동 축을 읽기 위한 접근자입니다.
	UFUNCTION(BlueprintPure, Category = "PushPull|Debug")
	FVector GetGrabbedMoveAxis() const { return GrabbedMoveAxis; }

	// 통합 플레이어 디버그 HUD에서 현재 축 입력 값을 읽기 위한 접근자입니다.
	UFUNCTION(BlueprintPure, Category = "PushPull|Debug")
	float GetCurrentAxisInput() const { return CurrentAxisInput; }

	// 통합 플레이어 디버그 HUD에서 마지막 실패 이유를 읽기 위한 접근자입니다.
	UFUNCTION(BlueprintPure, Category = "PushPull|Debug")
	FString GetLastFailureReason() const { return LastFailureReason; }

protected:
	// 소유 캐릭터를 빠르게 재사용하기 위한 캐시다.
	UPROPERTY(Transient)
	TObjectPtr<AUOUCharacter> OwnerCharacter = nullptr;

	// 일반 상호작용 센서를 같이 참고하기 위한 캐시다.
	UPROPERTY(Transient)
	TObjectPtr<UUOUInteractionComponent> InteractionComponent = nullptr;

	// 우산 상태에 따라 손 사용 가능 여부를 판단하기 위한 참조다.
	UPROPERTY(Transient)
	TObjectPtr<UUOUUmbrellaComponent> UmbrellaComponent = nullptr;

	// 현재 잡은 블럭을 어느 축으로만 움직일지 저장하는 값이다.
	UPROPERTY(Transient)
	FVector GrabbedMoveAxis = FVector::ZeroVector;

	// 마지막 이동 입력이 실제로 얼마였는지 디버그로 확인하기 위한 값이다.
	UPROPERTY(Transient)
	float CurrentAxisInput = 0.0f;

	// 현재 grab 입력이 눌린 상태인지 유지하는 플래그다.
	UPROPERTY(Transient)
	bool bGrabInputHeld = false;

	// 밀고 당기기 동안 캐릭터 회전 옵션을 복구하기 위해 원래 값을 저장한다.
	UPROPERTY(Transient)
	bool bCachedOrientRotationToMovement = true;

	UPROPERTY(Transient)
	float CachedMaxWalkSpeed = 0.0f;

	UPROPERTY(Transient)
	bool bHasCachedCharacterMovementSettings = false;

	UPROPERTY(Transient)
	float GrabbedReferenceDistance2D = 0.0f;

	UPROPERTY(Transient)
	bool bHasGrabbedReferenceDistance = false;

	// 가장 최근 실패 이유를 화면 디버그로 보여주기 위한 문자열이다.
	UPROPERTY(Transient)
	FString LastFailureReason = TEXT("None");

	// 현재 후보를 다시 평가한다.
	void RefreshCandidate();

	// 우산과 공중 여부를 보고 손을 쓸 수 있는지 판단한다.
	bool CanUseHands() const;

	// 잡은 블럭이 너무 멀어졌는지 검사해서 자동 해제에 쓴다.
	bool IsGrabbedObjectTooFar() const;

	// 현재 후보를 기반으로 실제 잡기 시작을 시도한다.
	void TryBeginGrab();

	// 잡기 상태를 정리하고 원래 이동 설정을 복구한다.
	void EndGrab();

	void ApplyGrabbedCharacterMovementSettings(bool bLimitWalkSpeed);
	void RestoreGrabbedCharacterMovementSettings();
	void CacheGrabbedReferenceDistance();
	void ClearGrabbedReferenceDistance();
	FVector BuildGrabbedObjectVelocity(float BaseMoveSpeed) const;

	// 잡은 동안 캐릭터 방향을 블럭 축에 맞게 보정한다.
	void ApplyGrabbedRotation() const;

	// 시작 시 필요한 플레이어 관련 참조를 한 번에 찾는다.
	void ResolveOwnerReferences();

	// 액션 입력이 빠졌을 때도 기본 이동값을 읽어 테스트 가능하게 보조한다.
	void UpdateMovementInputFallback();

	// 현재 후보와 실패 이유를 화면에 출력한다.
	void UpdateScreenDebug() const;

	// 후보 탐색 반경과 이동 축을 월드에 시각화한다.
	// 후보 블럭에서 실제로 사용할 수평 이동 축을 계산한다.
	bool TryResolveGrabAxis(UUOUPushPullObjectComponent* TargetObject, FVector& OutMoveAxis) const;

	// 주변 오브젝트 중 가장 적합한 밀고 당기기 후보를 찾는다.
	UUOUPushPullObjectComponent* FindBestCandidate() const;
	UUOUCrankComponent* FindBestCrankCandidate() const;
	UUOURotatableMirrorComponent* FindBestMirrorCandidate() const;

	// 후보 탐색의 중심 위치를 계산한다.
	FVector GetDetectionOriginLocation() const;
	FVector GetGrabbedReferenceLocation() const;

	// 플레이어 방향과 가장 잘 맞는 축을 고르기 위한 보조 함수다.
	static void CheckBetterAxis(const FVector& Axis, const FVector& ToPlayer, FVector& BestAxis, float& BestDot);

	// 축 후보를 수평면 기준으로 정리해서 쓴다.
	static FVector GetHorizontalAxis(FVector Axis, const FVector& Fallback);
};
