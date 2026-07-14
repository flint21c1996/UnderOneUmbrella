// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/Pour/UOUPourContentProfile.h"
#include "World/Pour/UOUPourReceiverInterface.h"
#include "UOUWaterContainerComponent.generated.h"

class UMaterialInterface;
class UMeshComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWaterAmountChangedSignature, float, NewAmount, float, MaxAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPourContentProfileChangedSignature, UUOUPourContentProfile*, NewProfile);

// 물 양을 저장하고 퍼즐이나 무게 계산에 넘길 수 있게 관리하는 범용 물 컨테이너다.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class UUOUWaterContainerComponent : public UActorComponent, public IUOUPourReceiver
{
	GENERATED_BODY()

public:
	// 저장량 최대치와 초기값을 설정한다.
	UUOUWaterContainerComponent();

	// 시작 시 현재 물 양을 초기값으로 맞춘다.
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "Water")
	FOnWaterAmountChangedSignature OnWaterAmountChanged;

	UPROPERTY(BlueprintAssignable, Category = "Water|Content")
	FOnPourContentProfileChangedSignature OnPourContentProfileChanged;

	// 이 컨테이너가 가질 수 있는 최대 물 양이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water", meta = (ClampMin = "0.0"))
	float MaxAmount = 5.0f;

	// 시작 시 디버그나 테스트용으로 미리 채워둘 물 양이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water", meta = (ClampMin = "0.0"))
	float InitialAmount = 0.0f;

	// 현재 물 양이 퍼즐 무게에 얼마나 기여할지 정하는 배수다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water", meta = (ClampMin = "0.0"))
	float WeightMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water|Content", meta = (ToolTip = "Runtime content profile for this container. Use SetPourContentProfile or AddAmountWithContent to change it during play."))
	TObjectPtr<UUOUPourContentProfile> PourContentProfile = nullptr;

	// 현재 실제로 저장된 물 양이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	float CurrentAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Container Material", meta = (ToolTip = "물 저장 상태에 따라 머티리얼을 교체할 대상 MeshComponent입니다. 비워두면 Owner에서 자동으로 찾습니다."))
	TObjectPtr<UMeshComponent> MaterialVisualComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Container Material", meta = (ToolTip = "MaterialVisualComponent를 자동으로 찾을 때 사용할 컴포넌트 이름 또는 태그입니다. None이면 Root Mesh 또는 첫 MeshComponent를 사용합니다."))
	FName MaterialVisualComponentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Container Material", meta = (ToolTip = "MaterialVisualComponent가 비어 있을 때 Owner에서 MeshComponent를 자동으로 찾습니다."))
	bool bAutoFindMaterialVisualComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Container Material", meta = (ToolTip = "켜면 물이 들어있는지 여부에 따라 대상 메시의 머티리얼을 교체합니다."))
	bool bUpdateMaterialVisual = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Container Material", meta = (ToolTip = "물이 들어있을 때 적용할 머티리얼 목록입니다. 인덱스가 슬롯 번호와 대응됩니다."))
	TArray<TObjectPtr<UMaterialInterface>> FilledMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Container Material", meta = (ToolTip = "비어 있을 때 적용할 머티리얼 목록입니다. 비워두면 시작 시점의 원래 머티리얼로 복구합니다."))
	TArray<TObjectPtr<UMaterialInterface>> EmptyMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Container Material", meta = (ToolTip = "EmptyMaterials가 비어 있을 때 시작 시점의 원래 머티리얼로 복구합니다."))
	bool bRestoreOriginalMaterialsWhenEmpty = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water|Container Material")
	bool bResolvedMaterialVisualComponent = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water|Container Material")
	FString ResolvedMaterialVisualComponentName = TEXT("None");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Fill Visual", meta = (ToolTip = "Visual component driven by this container fill ratio. Mesh and Niagara components are both supported."))
	TObjectPtr<USceneComponent> FillVisualComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Fill Visual", meta = (ToolTip = "If FillVisualComponent is empty, search owner components by this name or component tag."))
	FName FillVisualComponentName = TEXT("StoredWaterVisual");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Fill Visual", meta = (ToolTip = "Automatically find FillVisualComponent by name or tag when it is not assigned."))
	bool bAutoFindFillVisualComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Fill Visual", meta = (ToolTip = "Legacy fallback. Prefer UOUStoredContentVisualComponent for content-profile driven stored visuals."))
	bool bUpdateFillVisual = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Fill Visual", meta = (ClampMin = "0.0", ToolTip = "Fill ratio interpolation speed. Set to 0 to snap to the stored amount."))
	float FillVisualInterpSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Fill Visual", meta = (ToolTip = "Relative location offset applied at full fill. The initial component relative location is used as empty."))
	FVector FillVisualFullLocationOffset = FVector(0.0f, 0.0f, 12.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Fill Visual", meta = (ToolTip = "Scale multiplier at empty fill."))
	FVector FillVisualEmptyScaleMultiplier = FVector(1.0f, 1.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Fill Visual", meta = (ToolTip = "Scale multiplier at full fill."))
	FVector FillVisualFullScaleMultiplier = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Fill Visual", meta = (ToolTip = "Hide the fill visual when the displayed fill ratio is almost empty."))
	bool bHideFillVisualWhenEmpty = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Fill Visual", meta = (ToolTip = "Optional scalar parameter updated on mesh materials. Leave None to disable material parameter updates."))
	FName MeshFillRatioParameterName = TEXT("FillRatio");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Fill Visual", meta = (ToolTip = "Optional Niagara float variable updated from fill ratio. Use a User parameter such as User.FillRatio. Leave None to disable Niagara parameter updates."))
	FName NiagaraFillRatioParameterName = TEXT("User.FillRatio");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water|Fill Visual")
	bool bResolvedFillVisualComponent = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water|Fill Visual")
	FString ResolvedFillVisualComponentName = TEXT("None");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water|Fill Visual")
	float DisplayedFillVisualRatio = 0.0f;

	// 현재 물 양에 값을 더하고 실제 더해진 양을 돌려준다.
	UFUNCTION(BlueprintCallable, Category = "Water")
	float AddAmount(float AmountToAdd);

	virtual bool CanAcceptPour_Implementation(const FUOUPourInputContext& Context) const override;
	virtual FUOUPourReceiveResult TryReceivePour_Implementation(const FUOUPourInputContext& Context) override;
	virtual int32 GetPourReceivePriority_Implementation() const override;
	virtual bool CanAcceptPourAtLocation_Implementation(const FUOUPourInputContext& Context) const override;

	UFUNCTION(BlueprintCallable, Category = "Water|Content")
	float AddAmountWithContent(float AmountToAdd, UUOUPourContentProfile* NewContentProfile, bool bReplaceCurrentContent = true);

	// 현재 물 양에서 값을 빼고 실제 빠진 양을 돌려준다.
	UFUNCTION(BlueprintCallable, Category = "Water")
	float RemoveAmount(float AmountToRemove);

	// 물 양을 직접 설정하고 범위를 안전하게 제한한다.
	UFUNCTION(BlueprintCallable, Category = "Water")
	void SetAmount(float NewAmount);

	UFUNCTION(BlueprintCallable, Category = "Water|Content")
	void SetPourContentProfile(UUOUPourContentProfile* NewContentProfile);

	UFUNCTION(BlueprintPure, Category = "Water|Content")
	UUOUPourContentProfile* GetPourContentProfile() const;

	// 현재 물 양을 최대치 대비 비율로 돌려준다.
	UFUNCTION(BlueprintPure, Category = "Water")
	float GetFillRatio() const;

	// 현재 물 양이 퍼즐 무게에 더해질 값을 계산한다.
	UFUNCTION(BlueprintPure, Category = "Water")
	float GetWeightContribution() const;

protected:
	// 내부 물 양이 바뀐 뒤 변경 이벤트를 한 번에 방송한다.
	void BroadcastAmountChanged();

	void BroadcastPourContentProfileChanged();

	void ResolveMaterialVisualComponent();

	UMeshComponent* FindMaterialVisualComponent() const;

	void CaptureOriginalMaterialVisualMaterialsIfNeeded();

	void RefreshMaterialVisual();

	void ResolveFillVisualComponent();

	USceneComponent* FindFillVisualComponent() const;

	void CaptureFillVisualTransformIfNeeded();

	void RefreshFillVisualTarget();

	void UpdateFillVisual(float DeltaTime, bool bSnapToTarget = false);

	bool ShouldShowFillVisual() const;

	const FUOUPourStoredVisualSettings* GetActiveStoredVisualSettings() const;

	void ApplyFillVisualContentProfile();

	bool bCapturedOriginalMaterialVisualMaterials = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterialVisualMaterials;

	bool bCapturedFillVisualTransform = false;

	FVector InitialFillVisualRelativeLocation = FVector::ZeroVector;

	FVector InitialFillVisualRelativeScale = FVector::OneVector;

	float TargetFillVisualRatio = 0.0f;

};
