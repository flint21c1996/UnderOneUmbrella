// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/Pour/UOUPourContentProfile.h"
#include "UOUWaterContainerComponent.generated.h"

class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWaterAmountChangedSignature, float, NewAmount, float, MaxAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPourContentProfileChangedSignature, UUOUPourContentProfile*, NewProfile);

// 물 양을 저장하고 퍼즐이나 무게 계산에 넘길 수 있게 관리하는 범용 물 컨테이너다.
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class UUOUWaterContainerComponent : public UActorComponent
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Fill Visual", meta = (ToolTip = "Visual component driven by this container fill ratio. Mesh and Niagara components are both supported."))
	TObjectPtr<USceneComponent> FillVisualComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Fill Visual", meta = (ToolTip = "If FillVisualComponent is empty, search owner components by this name or component tag."))
	FName FillVisualComponentName = TEXT("StoredWaterVisual");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Fill Visual", meta = (ToolTip = "Automatically find FillVisualComponent by name or tag when it is not assigned."))
	bool bAutoFindFillVisualComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Fill Visual", meta = (ToolTip = "Enables fill visual updates from this container amount."))
	bool bUpdateFillVisual = true;

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

	void ResolveFillVisualComponent();

	USceneComponent* FindFillVisualComponent() const;

	void CaptureFillVisualTransformIfNeeded();

	void RefreshFillVisualTarget();

	void UpdateFillVisual(float DeltaTime, bool bSnapToTarget = false);

	bool ShouldShowFillVisual() const;

	const FUOUPourStoredVisualSettings* GetActiveStoredVisualSettings() const;

	void ApplyFillVisualContentProfile();

	bool bCapturedFillVisualTransform = false;

	FVector InitialFillVisualRelativeLocation = FVector::ZeroVector;

	FVector InitialFillVisualRelativeScale = FVector::OneVector;

	float TargetFillVisualRatio = 0.0f;

};
