#include "World/Sky/UOUVirtualPerspectiveSkyMaterialFactory.h"
#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

void EnsureUOUPerspectiveSkyCompositeMaterial()
{
	const FString PackageName(TEXT("/Game/UOU/Materials/M_UOU_PerspectiveSkyComposite"));
	if (FPackageName::DoesPackageExist(PackageName)) return;
	UPackage* Package = CreatePackage(*PackageName);
	UMaterial* Material = NewObject<UMaterial>(Package, TEXT("M_UOU_PerspectiveSkyComposite"), RF_Public | RF_Standalone);
	Material->SetShadingModel(MSM_Unlit);
	Material->TwoSided = true;
	// Composite after world fog; keep depth testing so gameplay occludes it.
	Material->BlendMode = BLEND_Translucent;
	Material->bUseTranslucencyVertexFog = false;
	Material->bIsSky = false;
	UMaterialExpressionTextureObjectParameter* Texture = NewObject<UMaterialExpressionTextureObjectParameter>(Material);
	Texture->ParameterName = TEXT("SkyImage");
	Texture->Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	Texture->SamplerType = SAMPLERTYPE_Color;
	Material->GetExpressionCollection().AddExpression(Texture);
	UMaterialExpressionCustom* Output = NewObject<UMaterialExpressionCustom>(Material);
	Output->OutputType = CMOT_Float3;
	Output->Inputs.Reset();
	FCustomInput& Input = Output->Inputs.AddDefaulted_GetRef();
	Input.InputName = TEXT("SkyImage");
	Input.Input.Expression = Texture;
	Output->Code = TEXT("return SkyImage.SampleLevel(SkyImageSampler, GetViewportUV(Parameters), 0).rgb;");
	Material->GetExpressionCollection().AddExpression(Output);
	Material->GetEditorOnlyData()->EmissiveColor.Expression = Output;
	Material->PostEditChange();
	FAssetRegistryModule::AssetCreated(Material);
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	if (!UPackage::SavePackage(Package, Material, *Filename, Args))
		UE_LOG(LogTemp, Error, TEXT("[PerspectiveSky] Could not save %s"), *Filename);
}
#endif
