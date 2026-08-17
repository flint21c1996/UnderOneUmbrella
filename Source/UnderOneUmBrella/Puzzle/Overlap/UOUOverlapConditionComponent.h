// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Puzzle/Core/UOUPuzzleConditionSourceComponent.h"
#include "UOUOverlapConditionComponent.generated.h"

class UPrimitiveComponent;

UENUM(BlueprintType)
enum class EUOUOverlapConditionMode : uint8
{
	StayWhileOverlapping UMETA(DisplayName = "겹치는 동안 만족", ToolTip = "대상 Actor가 감지 영역 안에 있는 동안만 조건을 만족합니다."),
	LatchOnEnter UMETA(DisplayName = "진입 시 고정", ToolTip = "대상 Actor가 한 번 진입하면 조건 만족 상태를 유지합니다.")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUOverlapConditionActorSignature, AActor*, OverlapActor);

// 지정한 Actor가 감지 영역에 들어왔는지를 ConditionGroup에서 사용할 수 있는 조건 상태로 변환합니다.
UCLASS(ClassGroup=(Puzzle), meta=(BlueprintSpawnableComponent, DisplayName="UOU Overlap Condition"))
class UNDERONEUMBRELLA_API UUOUOverlapConditionComponent : public UUOUPuzzleConditionSourceComponent
{
	GENERATED_BODY()

public:
	UUOUOverlapConditionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual FText GetDebugSummaryText_Implementation() const override;
	virtual void GetPuzzleDebugInputActors_Implementation(TArray<AActor*>& OutInputActors) const override;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Overlap")
	FUOUOverlapConditionActorSignature OnTargetEntered;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Overlap")
	FUOUOverlapConditionActorSignature OnTargetExited;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Overlap", meta = (ToolTip = "게임 시작 시 조건 만족 여부입니다."))
	bool bInitialSatisfied = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Overlap", meta = (ToolTip = "영역 진입 조건을 유지하는 방식입니다."))
	EUOUOverlapConditionMode ConditionMode = EUOUOverlapConditionMode::LatchOnEnter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Overlap", meta = (ToolTip = "TargetActors가 비어 있으면 0번 플레이어 Pawn을 대상으로 사용합니다."))
	bool bUsePlayerPawnWhenTargetActorsEmpty = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle|Overlap", meta = (ToolTip = "비어 있으면 플레이어 Pawn 또는 클래스/태그 필터를 사용합니다. 값이 있으면 여기에 등록된 Actor만 조건 대상입니다."))
	TArray<TObjectPtr<AActor>> TargetActors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Overlap", meta = (ToolTip = "값이 있으면 해당 클래스 계열 Actor만 조건 대상입니다."))
	TArray<TSubclassOf<AActor>> TargetActorClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Overlap", meta = (ToolTip = "값이 있으면 해당 태그 중 하나를 가진 Actor만 조건 대상입니다."))
	TArray<FName> TargetActorTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Overlap", meta = (ClampMin = "1", ToolTip = "이 수 이상 대상 Actor가 영역 안에 있으면 조건을 만족합니다."))
	int32 RequiredOverlapCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Overlap", meta = (ToolTip = "소유 Actor 자신과의 overlap은 무시합니다."))
	bool bIgnoreOwner = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Sensor", meta = (ToolTip = "같은 Actor 안의 감지 볼륨을 이름으로 자동 탐색합니다."))
	bool bAutoFindOverlapVolume = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Sensor")
	FName PreferredOverlapVolumeName = TEXT("OverlapVolume");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Sensor")
	FComponentReference OverlapVolumeReference;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Sensor")
	TObjectPtr<UPrimitiveComponent> OverlapVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Runtime")
	int32 OverlappingTargetCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Puzzle|Runtime")
	TObjectPtr<AActor> LastEnteredActor = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Overlap")
	void RefreshOverlapState();

	UFUNCTION(BlueprintCallable, Category = "Puzzle|Overlap")
	void ResetOverlapCondition();

protected:
	UFUNCTION()
	void HandleBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void ResolveOverlapVolume();
	void BindOverlapVolume();
	void UnbindOverlapVolume();
	void RegisterOverlappingActor(AActor* OtherActor);
	void UnregisterOverlappingActor(AActor* OtherActor);
	void RecalculateSatisfiedState();
	bool IsConditionTargetActor(AActor* OtherActor) const;
	bool MatchesClassFilter(AActor* OtherActor) const;
	bool MatchesTagFilter(AActor* OtherActor) const;

	TMap<TObjectPtr<AActor>, int32> OverlapActorCounts;
};
