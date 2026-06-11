// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Debug/UOUPuzzleDebugInfoProvider.h"
#include "GameFramework/Actor.h"
#include "Puzzle/Core/UOUPuzzleResultReceiver.h"
#include "UOUFlyingSwarmEffectActor.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;

class AUOUFlyingSwarmEffectActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUOUFlyingSwarmEffectActorEvent, AUOUFlyingSwarmEffectActor*, EffectActor);

// 여러 메시 파티클이 출발 지점에서 목표 지점 주변으로 날아가는 Niagara 연출을 퍼즐 Result와 연결하는 액터입니다.
// 종이비행기, 종이 조각, 빛 조각처럼 실제 게임플레이 액터가 아닌 시각 연출을 제어할 때 사용합니다.
UCLASS(Blueprintable, meta=(DisplayName="UOU Flying Swarm Effect Actor"))
class UNDERONEUMBRELLA_API AUOUFlyingSwarmEffectActor
	: public AActor
	, public IUOUPuzzleResultReceiver
	, public IUOUPuzzleDebugInfoProvider
{
	GENERATED_BODY()

public:
	AUOUFlyingSwarmEffectActor();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flying Swarm")
	TObjectPtr<USceneComponent> RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Effect")
	TObjectPtr<UNiagaraComponent> SwarmEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Effect", meta = (DisplayName = "Swarm System", ToolTip = "날아다니는 군집 연출에 사용할 Niagara System입니다. 비워두면 SwarmEffect 컴포넌트에 직접 지정된 System을 사용합니다."))
	TObjectPtr<UNiagaraSystem> SwarmEffectSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Effect", meta = (ToolTip = "게임 시작과 동시에 연출을 시작할지 정합니다. 퍼즐 Result로만 켤 때는 꺼둡니다."))
	bool bActivateOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Effect", meta = (ToolTip = "Activate를 다시 받을 때 Niagara를 처음부터 다시 재생할지 정합니다."))
	bool bResetSystemOnActivate = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Effect", meta = (ToolTip = "Deactivate 시 컴포넌트를 즉시 숨길지 정합니다. Niagara 안에서 페이드아웃을 처리하려면 꺼둡니다."))
	bool bHideEffectWhenInactive = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Flying Swarm|Source", meta = (ToolTip = "비행체가 출발할 액터입니다. 비워두면 이 액터의 위치를 사용합니다. 우체통 액터를 넣는 용도입니다."))
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Source", meta = (ToolTip = "SourceActor 기준 로컬 출발 오프셋입니다. 우체통 입구 위치를 맞출 때 사용합니다."))
	FVector SourceLocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Source", meta = (ToolTip = "SourceActor 루트 컴포넌트에 있는 소켓 이름입니다. 지정하면 해당 소켓 기준으로 SourceLocalOffset을 더합니다."))
	FName SourceSocketName = NAME_None;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Flying Swarm|Target", meta = (ToolTip = "비행체가 감쌀 목표 액터입니다. 비워두고 Use Player Pawn When Target Missing을 켜면 첫 플레이어 Pawn을 사용합니다."))
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Target", meta = (ToolTip = "TargetActor 기준 로컬 목표 오프셋입니다. 캐릭터 허리나 머리 높이를 맞출 때 사용합니다."))
	FVector TargetLocalOffset = FVector(0.0f, 0.0f, 120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Target", meta = (ToolTip = "TargetActor 루트 컴포넌트에 있는 소켓 이름입니다. 지정하면 해당 소켓 기준으로 TargetLocalOffset을 더합니다."))
	FName TargetSocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Target", meta = (ToolTip = "TargetActor가 비어 있을 때 첫 플레이어 Pawn을 목표로 사용할지 정합니다."))
	bool bUsePlayerPawnWhenTargetMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Shape", meta = (ClampMin = "0", ToolTip = "Niagara가 생성할 비행체 수입니다. Niagara의 Spawn Burst Count에 이 값을 연결합니다."))
	int32 PlaneCount = 24;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Shape", meta = (ClampMin = "0.0", ToolTip = "목표 주변을 감싸는 기본 반경입니다."))
	float OrbitRadius = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Shape", meta = (ClampMin = "0.0", ToolTip = "목표 주변 궤도의 세로 높이입니다."))
	float OrbitHeight = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Timing", meta = (ClampMin = "0.01", ToolTip = "출발 지점에서 목표 주변까지 진입하는 시간입니다."))
	float TravelDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Timing", meta = (ClampMin = "0.0", ToolTip = "목표 주변을 감싸며 유지되는 시간입니다."))
	float HoldDuration = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Timing", meta = (ClampMin = "0.0", ToolTip = "연출 종료 시 Niagara 내부 페이드아웃에 넘겨줄 시간입니다."))
	float FadeOutDuration = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Random", meta = (ToolTip = "켜면 매번 같은 Seed를 Niagara에 전달합니다."))
	bool bUseFixedRandomSeed = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Random", meta = (EditCondition = "bUseFixedRandomSeed"))
	int32 RandomSeed = 2026;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Random", meta = (ToolTip = "고정 Seed를 쓰지 않을 때 Activate마다 Seed를 증가시켜 매번 다른 군집 패턴을 만듭니다."))
	bool bAdvanceRandomSeedOnActivate = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Debug", meta = (ToolTip = "게임 중 출발점, 목표점, 궤도 반경을 디버그로 표시합니다."))
	bool bDrawRuntimeDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Debug", meta = (ClampMin = "0.0", ToolTip = "디버그 선 두께입니다."))
	float RuntimeDebugThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Niagara Parameters")
	FName SourcePositionParameterName = TEXT("SourcePosition");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Niagara Parameters")
	FName SourceForwardParameterName = TEXT("SourceForward");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Niagara Parameters")
	FName SourceRightParameterName = TEXT("SourceRight");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Niagara Parameters")
	FName TargetPositionParameterName = TEXT("TargetPosition");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Niagara Parameters")
	FName TargetForwardParameterName = TEXT("TargetForward");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Niagara Parameters")
	FName TargetRightParameterName = TEXT("TargetRight");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Niagara Parameters")
	FName TargetUpParameterName = TEXT("TargetUp");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Niagara Parameters")
	FName PlaneCountParameterName = TEXT("PlaneCount");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Niagara Parameters")
	FName OrbitRadiusParameterName = TEXT("OrbitRadius");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Niagara Parameters")
	FName OrbitHeightParameterName = TEXT("OrbitHeight");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Niagara Parameters")
	FName TravelDurationParameterName = TEXT("TravelDuration");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Niagara Parameters")
	FName HoldDurationParameterName = TEXT("HoldDuration");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Niagara Parameters")
	FName FadeOutDurationParameterName = TEXT("FadeOutDuration");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Niagara Parameters")
	FName EffectAgeParameterName = TEXT("EffectAge");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Niagara Parameters")
	FName EffectActiveParameterName = TEXT("EffectActive");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Swarm|Niagara Parameters")
	FName RandomSeedParameterName = TEXT("RandomSeed");

	UPROPERTY(BlueprintAssignable, Category = "Flying Swarm|Events")
	FUOUFlyingSwarmEffectActorEvent OnEffectStarted;

	UPROPERTY(BlueprintAssignable, Category = "Flying Swarm|Events")
	FUOUFlyingSwarmEffectActorEvent OnEffectStopped;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Flying Swarm|Runtime")
	bool bIsEffectActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Flying Swarm|Runtime")
	float EffectElapsedTime = 0.0f;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Flying Swarm|Actions")
	void ActivateEffect();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Flying Swarm|Actions")
	void DeactivateEffect();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Flying Swarm|Actions")
	void RestartEffect();

	UFUNCTION(BlueprintCallable, Category = "Flying Swarm|Actions")
	void SetEffectActive(bool bNewActive);

	UFUNCTION(BlueprintPure, Category = "Flying Swarm|Runtime")
	bool IsEffectActive() const;

	virtual void ApplyPuzzleResult_Implementation(EOUUPuzzleResultAction Action) override;
	virtual TArray<FString> GetPuzzleDebugInfo_Implementation() const override;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif

private:
	int32 RuntimeRandomSeed = 2026;

	void ApplyEffectSystem();
	void ApplyNiagaraParameters();
	void DrawRuntimeDebug() const;

	FTransform ResolveSourceTransform() const;
	FTransform ResolveTargetTransform() const;
	FTransform ResolveReferenceTransform(const AActor* ReferenceActor, FName SocketName, const FVector& LocalOffset) const;
	AActor* ResolveTargetActor() const;

	static FVector GetSafeTransformAxis(const FTransform& Transform, EAxis::Type Axis, const FVector& Fallback);
};
