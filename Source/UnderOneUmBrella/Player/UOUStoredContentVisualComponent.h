// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Player/UOUUmbrellaComponent.h"
#include "World/Pour/UOUPourContentProfile.h"
#include "UOUStoredContentVisualComponent.generated.h"

class UNiagaraComponent;
class USceneComponent;
class UUOUWaterContainerComponent;

// WaterContainerComponent에 저장된 내용물을 실제 화면에 보이도록 갱신하는 컴포넌트입니다.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class UUOUStoredContentVisualComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UUOUStoredContentVisualComponent();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Container", meta = (ToolTip = "저장량과 ContentProfile을 제공하는 WaterContainerComponent입니다. 비워두면 Owner에서 자동으로 찾습니다."))
	TObjectPtr<UUOUWaterContainerComponent> WaterContainerComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Container", meta = (ToolTip = "WaterContainerComponent를 자동으로 찾을 때 사용할 컴포넌트 이름 또는 태그입니다. None이면 Owner의 첫 WaterContainerComponent를 사용합니다."))
	FName WaterContainerComponentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Container")
	bool bAutoFindWaterContainerComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual", meta = (ToolTip = "저장량 비율에 따라 위치/파라미터가 갱신될 Mesh 또는 Niagara 컴포넌트입니다. 비워두면 자식 컴포넌트나 이름으로 자동 탐색합니다."))
	TObjectPtr<USceneComponent> StoredVisualComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual", meta = (ToolTip = "StoredVisualComponent를 자동으로 찾을 때 사용할 컴포넌트 이름 또는 태그입니다."))
	FName StoredVisualComponentName = TEXT("StoredWaterVisual");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual")
	bool bAutoFindStoredVisualComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual")
	bool bUpdateStoredVisual = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Visibility", meta = (ToolTip = "우산 상태를 확인해 저장 내용물 표시 여부를 결정하는 UmbrellaComponent입니다. 비워두면 Owner에서 자동으로 찾습니다."))
	TObjectPtr<UUOUUmbrellaComponent> UmbrellaComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Socket", meta = (ToolTip = "저장 내용물 소켓을 가진 컴포넌트 이름 또는 태그입니다. 기본값은 플레이어의 우산 스켈레탈 메시입니다."))
	FName SocketSourceComponentName = TEXT("UmbrellaSkeletalVisual");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Socket", meta = (ToolTip = "저장 내용물 visual의 시작 위치가 되는 소켓 이름입니다. 이 소켓 위치를 기준으로 fill offset이 적용됩니다."))
	FName StoredContentSocketName = TEXT("StoredWaterPoint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual", meta = (ClampMin = "0.0", ToolTip = "ContentProfile에서 별도 설정하지 않았을 때 사용할 fill ratio 보간 속도입니다. 0이면 즉시 목표값으로 이동합니다."))
	float FillVisualInterpSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual", meta = (ToolTip = "가득 찼을 때 StoredVisualComponent에 적용할 상대 위치 offset입니다. 보통 물 표면이 위로 차오르는 높이로 사용합니다."))
	FVector FullLocationOffset = FVector(0.0f, 0.0f, 12.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual", meta = (ToolTip = "Fallback scale multiplier at empty fill."))
	FVector EmptyScaleMultiplier = FVector(1.0f, 1.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual", meta = (ToolTip = "Fallback scale multiplier at full fill."))
	FVector FullScaleMultiplier = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Mesh", meta = (ToolTip = "Mesh 머티리얼에 전달할 fill ratio 스칼라 파라미터 이름입니다. None이면 머티리얼 파라미터를 갱신하지 않습니다."))
	FName MeshFillRatioParameterName = TEXT("FillRatio");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Niagara", meta = (ToolTip = "Niagara에 전달할 fill ratio float 변수 이름입니다. 예: User.FillRatio. None이면 Niagara 파라미터를 갱신하지 않습니다."))
	FName NiagaraFillRatioParameterName = TEXT("User.FillRatio");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Niagara", meta = (ToolTip = "켜면 visual이 보일 때 Niagara를 활성화하고, 숨겨질 때 즉시 비활성화합니다."))
	bool bAutoActivateNiagara = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Niagara", meta = (ToolTip = "켜면 Niagara visual의 초기 스케일을 유지하고 위치/파라미터만으로 차오름을 표현합니다."))
	bool bKeepNiagaraScaleForFill = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Content Visual|Mesh", meta = (ToolTip = "켜면 StaticMesh visual의 초기 스케일을 유지하고 위치/파라미터만으로 차오름을 표현합니다."))
	bool bKeepStaticMeshScaleForFill = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stored Content Visual|Runtime")
	bool bResolvedWaterContainerComponent = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stored Content Visual|Runtime")
	FString ResolvedWaterContainerComponentName = TEXT("None");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stored Content Visual|Runtime")
	bool bResolvedStoredVisualComponent = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stored Content Visual|Runtime")
	FString ResolvedStoredVisualComponentName = TEXT("None");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stored Content Visual|Runtime")
	bool bResolvedUmbrellaComponent = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stored Content Visual|Runtime")
	FString ResolvedUmbrellaComponentName = TEXT("None");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stored Content Visual|Runtime")
	bool bResolvedSocketSourceComponent = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stored Content Visual|Runtime")
	FString ResolvedSocketSourceComponentName = TEXT("None");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stored Content Visual|Runtime")
	float DisplayedFillVisualRatio = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Stored Content Visual")
	void RefreshStoredContentVisual(bool bSnapToTarget = false);

protected:
	void ResolveReferences();

	void ResolveWaterContainerComponent();
	UUOUWaterContainerComponent* FindWaterContainerComponent() const;

	void ResolveUmbrellaComponent();
	UUOUUmbrellaComponent* FindUmbrellaComponent() const;
	bool IsUmbrellaVisualStateAllowed() const;

	void ResolveStoredVisualComponent();
	USceneComponent* FindStoredVisualComponent() const;

	void ResolveSocketSourceComponent();
	USceneComponent* FindSocketSourceComponent() const;
	void UpdateSocketFollowLocation();

	void BindWaterContainerEvents();
	void UnbindWaterContainerEvents();
	void BindUmbrellaEvents();
	void UnbindUmbrellaEvents();

	void CaptureStoredVisualTransformIfNeeded();
	void ApplyStoredVisualCollisionSettings() const;
	void ApplyStoredVisualContentProfile();
	void UpdateStoredVisual(float DeltaTime, bool bSnapToTarget = false);
	void ApplyStoredVisualTransform(float FillRatio);
	void ApplyStoredVisualParameters(float FillRatio);
	bool ShouldShowStoredVisual() const;
	float GetTargetFillVisualRatio() const;
	const FUOUPourStoredVisualSettings* GetProfileStoredVisualSettings() const;
	const FUOUPourStoredVisualSettings* GetActiveMotionSettings() const;

	UFUNCTION()
	void HandleWaterAmountChanged(float NewAmount, float MaxAmount);

	UFUNCTION()
	void HandlePourContentProfileChanged(UUOUPourContentProfile* NewProfile);

	UFUNCTION()
	void HandleUmbrellaStateChanged(EUOUUmbrellaState NewState, bool bHasUmbrella);

private:
	UPROPERTY(Transient)
	TObjectPtr<UUOUWaterContainerComponent> BoundWaterContainerComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUOUUmbrellaComponent> BoundUmbrellaComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> SocketSourceComponent = nullptr;

	bool bCapturedStoredVisualTransform = false;
	FVector InitialStoredVisualRelativeLocation = FVector::ZeroVector;
	FVector InitialStoredVisualRelativeScale = FVector::OneVector;
};
