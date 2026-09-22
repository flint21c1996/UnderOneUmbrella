#include "World/Sky/UOUVirtualPerspectiveSkyActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AUOUVirtualPerspectiveSkyActor::AUOUVirtualPerspectiveSkyActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SkyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SkyMesh"));
	SetRootComponent(SkyMesh);
	SkyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkyMesh->SetCastShadow(false);
	SkyMesh->SetCanEverAffectNavigation(false);
	SkyMesh->bReceivesDecals = false;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Dome(TEXT("/Game/Fab/8k_Stylized_Cloudy_Skybox_010/skysphere/StaticMeshes/skysphere.skysphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DomeMaterial(TEXT("/Game/UOU/Materials/M_UOU_SkyDomeSurface.M_UOU_SkyDomeSurface"));
	if (Dome.Succeeded()) SkyMesh->SetStaticMesh(Dome.Object);
	if (DomeMaterial.Succeeded()) SkyMesh->SetMaterial(0, DomeMaterial.Object);
	SkyMesh->SetRelativeScale3D(FVector(0.18f));

	SkyCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SkyCapture"));
	SkyCapture->SetupAttachment(SkyMesh);
	SkyCapture->SetAbsolute(true, true, true);
	SkyCapture->ProjectionType = ECameraProjectionMode::Perspective;
	SkyCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	SkyCapture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	SkyCapture->bCaptureEveryFrame = false;
	SkyCapture->bCaptureOnMovement = false;
	SkyCapture->bAlwaysPersistRenderingState = true;
	// The sky-material pass is gated by Atmosphere, even for an unlit dome.
	SkyCapture->ShowFlags.SetAtmosphere(true);
	SkyCapture->ShowFlags.SetCloud(false);
	SkyCapture->ShowFlags.SetFog(false);
	SkyCapture->ShowFlags.SetVolumetricFog(false);
	SkyCapture->ShowFlags.SetDynamicShadows(false);
	SkyCapture->ShowFlags.SetTemporalAA(false);
	SkyCapture->ShowFlags.SetMotionBlur(false);

	BackgroundScreen = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackgroundScreen"));
	BackgroundScreen->SetupAttachment(SkyMesh);
	BackgroundScreen->SetAbsolute(true, true, true);
	BackgroundScreen->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BackgroundScreen->SetCastShadow(false);
	BackgroundScreen->SetCanEverAffectNavigation(false);
	BackgroundScreen->bReceivesDecals = false;
	BackgroundScreen->SetHiddenInSceneCapture(true);
	BackgroundScreen->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (Plane.Succeeded()) BackgroundScreen->SetStaticMesh(Plane.Object);
	CompositeMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/UOU/Materials/M_UOU_PerspectiveSkyComposite.M_UOU_PerspectiveSkyComposite")));
}

void AUOUVirtualPerspectiveSkyActor::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<AUOUVirtualPerspectiveSkyActor> It(GetWorld()); It; ++It)
	{
		if (*It != this && It->bOwnsBackground)
		{
			SkyMesh->SetVisibleInSceneCaptureOnly(true);
			SetActorTickEnabled(false);
			UE_LOG(LogTemp, Warning, TEXT("[PerspectiveSky] Duplicate %s disabled; %s owns the background."), *GetName(), *It->GetName());
			return;
		}
	}
	UMaterialInterface* Material = CompositeMaterial.LoadSynchronous();
	if (!Material)
	{
		UE_LOG(LogTemp, Error, TEXT("[PerspectiveSky] Missing composite material on %s."), *GetName());
		SetActorTickEnabled(false);
		return;
	}
	bOwnsBackground = true;
	// Keep the manual capture's view state rather than rebuilding it each frame.
	SkyCapture->bAlwaysPersistRenderingState = true;
	// Request all sky texture mips asynchronously during initial play, not on Q/E.
	// This is deliberately scoped to this dome, not every texture in the level.
	SkyMesh->PrestreamTextures(60.0f, true);
	SkyCapture->ShowFlags.SetAtmosphere(true);
	SkyCapture->ShowFlags.SetCloud(false);
	SkyMesh->SetVisibleInSceneCaptureOnly(true);
	SkyCapture->ShowOnlyComponent(SkyMesh);
	SkyRenderTarget = NewObject<UTextureRenderTarget2D>(this);
	SkyRenderTarget->RenderTargetFormat = RTF_RGBA16f;
	SkyRenderTarget->ClearColor = FLinearColor::Black;
	SkyRenderTarget->InitAutoFormat(1920, 1080);
	SkyCapture->TextureTarget = SkyRenderTarget;
	CompositeInstance = UMaterialInstanceDynamic::Create(Material, this);
	CompositeInstance->SetTextureParameterValue(TEXT("SkyImage"), SkyRenderTarget);
	BackgroundScreen->SetMaterial(0, CompositeInstance);
	UpdateBackground();
}

void AUOUVirtualPerspectiveSkyActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bOwnsBackground) UpdateBackground();
}

void AUOUVirtualPerspectiveSkyActor::UpdateBackground()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager || !SkyRenderTarget) return;
	const FMinimalViewInfo& View = PC->PlayerCameraManager->GetCameraCacheView();
	if (++ViewUpdateCount == 60)
	{
		UE_LOG(LogTemp, Display, TEXT("[PerspectiveSky] %s gameplay=%d rotation=%s sky=Perspective FOV=%g mesh=%s material=%s"), *GetName(), int32(View.ProjectionMode), *View.Rotation.ToString(), SkyVerticalFOV, *GetNameSafe(SkyMesh->GetStaticMesh()), *GetNameSafe(SkyMesh->GetMaterial(0)));
	}
	int32 Width = 0, Height = 0;
	PC->GetViewportSize(Width, Height);
	float Aspect = Width > 0 && Height > 0 ? float(Width) / Height : View.AspectRatio;
	if (View.bConstrainAspectRatio) Aspect = View.AspectRatio;
	Aspect = FMath::Max(0.1f, Aspect);
	const int32 TargetWidth = FMath::Clamp(CaptureWidth, 256, 4096);
	const int32 TargetHeight = FMath::Clamp(FMath::RoundToInt(TargetWidth / Aspect), 64, 4096);
	if (SkyRenderTarget->SizeX != TargetWidth || SkyRenderTarget->SizeY != TargetHeight)
		SkyRenderTarget->ResizeTarget(TargetWidth, TargetHeight);
	SkyCapture->SetWorldLocation(GetActorTransform().TransformPosition(CaptureLocalOffset));
	SkyCapture->SetWorldRotation(View.Rotation);
	SkyCapture->FOVAngle = FMath::RadiansToDegrees(2.0f * FMath::Atan(FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(SkyVerticalFOV, 10.0f, 150.0f)) * 0.5f) * Aspect));
	SkyCapture->CaptureScene();

	float Distance = FMath::Max(1000.0f, BackgroundDistance);
	const bool bOrtho = View.ProjectionMode == ECameraProjectionMode::Orthographic;
	if (bOrtho) Distance = FMath::Min(Distance, FMath::Max(1.0f, View.OrthoFarClipPlane * 0.9f));
	const float ScreenWidth = bOrtho ? View.OrthoWidth : 2.0f * Distance * FMath::Tan(FMath::DegreesToRadians(View.FOV) * 0.5f);
	BackgroundScreen->SetWorldLocation(View.Location + View.Rotation.Vector() * Distance);
	BackgroundScreen->SetWorldRotation(FRotationMatrix::MakeFromXY(View.Rotation.Quaternion().GetRightVector(), View.Rotation.Quaternion().GetUpVector()).Rotator());
	BackgroundScreen->SetWorldScale3D(FVector(ScreenWidth / 100.0f * 1.01f, ScreenWidth / Aspect / 100.0f * 1.01f, 1.0f));
	BackgroundScreen->SetVisibility(true);
}

void AUOUVirtualPerspectiveSkyActor::EndPlay(const EEndPlayReason::Type Reason)
{
	bOwnsBackground = false;
	SkyCapture->TextureTarget = nullptr;
	BackgroundScreen->SetVisibility(false);
	SkyMesh->SetVisibleInSceneCaptureOnly(false);
	Super::EndPlay(Reason);
}
