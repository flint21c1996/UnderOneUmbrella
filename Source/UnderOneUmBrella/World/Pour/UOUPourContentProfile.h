// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraSystem.h"
#include "World/Pour/UOUPourDropActor.h"
#include "UOUPourContentProfile.generated.h"

USTRUCT(BlueprintType)
struct UNDERONEUMBRELLA_API FUOUPourStoredVisualSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual", meta = (ToolTip = "켜면 WaterContainer의 기본 표시 설정 대신 이 ContentProfile의 Stored Visual 설정을 사용합니다."))
	bool bOverrideContainerFillVisual = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual|Mesh", meta = (ToolTip = "StoredVisualComponent가 StaticMeshComponent일 때 적용할 메쉬입니다. 비워두면 컴포넌트의 기존 메쉬를 유지합니다."))
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual|Mesh", meta = (ToolTip = "저장 내용물 메쉬에 적용할 머티리얼 목록입니다. 비어 있는 슬롯은 무시됩니다."))
	TArray<TObjectPtr<UMaterialInterface>> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual|Niagara", meta = (ToolTip = "StoredVisualComponent가 NiagaraComponent일 때 적용할 Niagara 시스템입니다. 비워두면 기존 에셋을 유지합니다."))
	TObjectPtr<UNiagaraSystem> NiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual", meta = (ClampMin = "0.0", ToolTip = "이 내용물이 저장량 변화에 반응하는 보간 속도입니다. 0이면 즉시 목표 fill ratio로 이동합니다."))
	float FillVisualInterpSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual", meta = (ToolTip = "가득 찼을 때 StoredVisualComponent가 이동할 상대 위치 offset입니다. 물 표면이 위로 차오르는 연출에 사용합니다."))
	FVector FullLocationOffset = FVector(0.0f, 0.0f, 12.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual", meta = (ToolTip = "비어 있을 때 적용할 스케일 배율입니다. 현재 mesh/Niagara 유지 스케일 옵션을 쓰면 위치/파라미터 중심으로 표현됩니다."))
	FVector EmptyScaleMultiplier = FVector(1.0f, 1.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual", meta = (ToolTip = "가득 찼을 때 적용할 스케일 배율입니다."))
	FVector FullScaleMultiplier = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual", meta = (ToolTip = "이전 호환용 설정입니다. 현재 StoredContentVisualComponent는 저장량이 없으면 기본적으로 숨깁니다."))
	bool bHideWhenEmpty = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual|Mesh", meta = (ToolTip = "Mesh 머티리얼에 전달할 fill ratio 스칼라 파라미터 이름입니다. None이면 갱신하지 않습니다."))
	FName MeshFillRatioParameterName = TEXT("FillRatio");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stored Visual|Niagara", meta = (ToolTip = "Niagara에 전달할 fill ratio float 변수 이름입니다. 예: User.FillRatio. None이면 갱신하지 않습니다."))
	FName NiagaraFillRatioParameterName = TEXT("User.FillRatio");
};

// Defines how one pourable content type is represented while it is being poured.
UCLASS(BlueprintType, Const, meta = (DisplayName = "UOU Pour Content Profile"))
class UNDERONEUMBRELLA_API UUOUPourContentProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content")
	TSubclassOf<AUOUPourDropActor> DropActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Drop Visual")
	FUOUPourDropVisualSettings DropVisual;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Stored Visual")
	FUOUPourStoredVisualSettings StoredVisual;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Stream Visual", meta = (ToolTip = "붓는 동안 계속 재생할 Niagara 시스템입니다. 실제 물줄기/꽃가루 같은 시각 표현은 이 에셋이 담당합니다."))
	TObjectPtr<UNiagaraSystem> StreamEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Stream Visual", meta = (ToolTip = "Stream Niagara에 추가로 적용할 회전입니다. 기본 방향은 소켓 위치에서 pour direction을 향하도록 맞춘 뒤, 여기서는 주로 Yaw 보정에 사용합니다."))
	FRotator StreamRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pour Content|Stream Visual", meta = (ClampMin = "0.0", ToolTip = "Stream Niagara의 월드 스케일입니다. 부모 소켓/스켈레탈 메쉬 scale의 영향을 받지 않고 이 값이 최종 크기로 적용됩니다."))
	FVector StreamRelativeScale = FVector::OneVector;
};
