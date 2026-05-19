// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "UOUCharacter.generated.h"

class UArrowComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UUOUPushPullInteractorComponent;
class USceneComponent;
class USpringArmComponent;
class UUOUCameraControllerComponent;
class UUOUUmbrellaComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

// 플레이어 이동, 카메라, 우산, 퍼즐 상호작용을 한데 묶는 기본 캐릭터다.
// 게임 중에 자주 오가는 입력 분기와 공용 진입점이 이 클래스에 모여 있다.
UCLASS(config=Game)
class AUOUCharacter : public ACharacter
{
	GENERATED_BODY()

	// 뒤따라오는 카메라 거리와 각도를 관리하는 스프링암이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	// 실제 화면을 그리는 플레이 카메라다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	// 캐릭터 시작 시 적용할 입력 매핑 컨텍스트다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	// 점프 입력에 대응하는 액션이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	// 이동 벡터를 받아 캐릭터를 움직이는 액션이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	// 카메라 회전을 전달하는 액션이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	// 우클릭처럼 문맥에 따라 다른 동작으로 갈라지는 상호작용 액션이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ContextInteractAction;

	// 우산을 열고 닫는 액션이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* UmbrellaToggleAction;

	// 우산을 뒤집는 액션이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* UmbrellaInvertAction;

	// 우산 테스트용 물 채우기 액션이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* UmbrellaDebugFillAction;

	// 8방향 카메라와 가림 처리를 담당하는 전용 카메라 컴포넌트다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UUOUCameraControllerComponent* CameraControllerComponent;

	// 우산을 손 근처에 붙일 때 쓰는 기준점이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	UArrowComponent* UmbrellaAttachPoint;

	// 손에 든 우산 메시를 조금 더 세밀하게 보정하는 앵커다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	UArrowComponent* UmbrellaHeldVisualAnchor;

	// 물 붓기 시작점과 방향을 정하는 기준점이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	UArrowComponent* PourOrigin;

	// 앞쪽 상호작용 후보를 찾을 때 기준으로 쓰는 위치다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	UArrowComponent* InteractionOrigin;

	// 블루프린트에서 붙인 밀고 당기기 컴포넌트를 런타임에 캐싱하는 포인터다.
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	UUOUPushPullInteractorComponent* PushPullInteractorComponent;

public:
	// 기본 이동 세팅과 공용 컴포넌트 구성을 초기화한다.
	AUOUCharacter();

protected:
	// 시작 시점에 입력과 필수 참조가 제대로 연결됐는지 확인한다.
	virtual void BeginPlay() override;

	// 런타임 참조 복구와 디버그 보조 갱신을 처리한다.
	virtual void Tick(float DeltaSeconds) override;

	// 이동 입력을 카메라 기준 방향으로 바꿔 실제 이동에 반영한다.
	void Move(const FInputActionValue& Value);

	// 시점 입력을 받아 카메라 제어 컴포넌트에 넘긴다.
	void Look(const FInputActionValue& Value);

	// 카메라를 왼쪽 방향으로 한 단계 회전시킨다.
	void RotateCameraLeft();

	// 카메라를 오른쪽 방향으로 한 단계 회전시킨다.
	void RotateCameraRight();

	// 카메라를 플레이어 쪽으로 더 당긴다.
	void ZoomCameraIn();

	// 카메라를 더 멀리 보낸다.
	void ZoomCameraOut();

	// 점프 직전에 우산 상태와 밀고 당기기 상태를 확인한다.
	void HandleJumpStarted();

	// 우산 열기와 닫기 입력을 처리한다.
	void HandleUmbrellaTogglePressed();

	// 우산 뒤집기 입력을 처리한다.
	void HandleUmbrellaInvertPressed();

	// 우산 붓기 시작 입력을 처리한다.
	void HandleUmbrellaPourPressed();

	// 우산 붓기 종료 입력을 처리한다.
	void HandleUmbrellaPourReleased();

	// 우산 테스트용 물 채우기 입력을 처리한다.
	void HandleUmbrellaDebugFillPressed();

	// 문맥 상호작용 시작을 우산과 밀고 당기기 쪽으로 분기한다.
	void HandleContextInteractPressed();

	// 문맥 상호작용 종료를 현재 상태에 맞게 분기한다.
	void HandleContextInteractReleased();

	// 밀고 당기기 잡기 시작 요청을 인터랙터에 넘긴다.
	void HandlePushPullPressed();

	// 밀고 당기기 잡기 종료 요청을 인터랙터에 넘긴다.
	void HandlePushPullReleased();

	// 컨트롤러가 바뀔 때 입력 매핑을 다시 적용한다.
	virtual void NotifyControllerChanged() override;

	// 액션 입력과 캐릭터 함수들을 한 번에 바인딩한다.
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 현재 캐릭터에 붙어 있는 우산 컴포넌트를 편하게 찾는다.
	UUOUUmbrellaComponent* FindUmbrellaComponent() const;

public:
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE class UUOUCameraControllerComponent* GetCameraControllerComponent() const { return CameraControllerComponent; }
	FORCEINLINE class UUOUPushPullInteractorComponent* GetPushPullInteractorComponent() const { return PushPullInteractorComponent; }

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Debug, meta = (AllowPrivateAccess = "true"))
	bool bShowContextInputDebug = false;

	// 문맥 상호작용 눌림 횟수를 화면 디버그로 추적하기 위한 값이다.
	UPROPERTY(Transient)
	int32 ContextInteractPressedCount = 0;

	// 문맥 상호작용 해제 횟수를 화면 디버그로 추적하기 위한 값이다.
	UPROPERTY(Transient)
	int32 ContextInteractReleasedCount = 0;

	// 밀고 당기기 눌림 이벤트가 실제로 들어오는지 확인하기 위한 값이다.
	UPROPERTY(Transient)
	int32 PushPullPressedCount = 0;

	// 밀고 당기기 해제 이벤트가 실제로 들어오는지 확인하기 위한 값이다.
	UPROPERTY(Transient)
	int32 PushPullReleasedCount = 0;

	// 밀고 당기기 컴포넌트가 비었을 때 같은 로그를 반복하지 않기 위한 플래그다.
	UPROPERTY(Transient)
	bool bLoggedMissingPushPullComponent = false;
};
