import unreal


VISUAL_BLUEPRINT_PATH = "/Game/UOU/BluePrint/World/Lights/BP_LightBeamVisual_Custom"
DEFAULT_CONE_MESH_PATH = (
    "/Game/UOU/Effects/StylizedLightFX/StaticScatters/"
    "_LUMENRAY42_Spotlight_Scatter_1"
)


visual_blueprint = unreal.load_asset(VISUAL_BLUEPRINT_PATH)
cone_mesh = unreal.load_asset(DEFAULT_CONE_MESH_PATH)

if visual_blueprint is None:
    raise RuntimeError(f"Blueprint를 찾지 못했습니다: {VISUAL_BLUEPRINT_PATH}")

if cone_mesh is None:
    raise RuntimeError(f"원본 Lumen 원뿔 메시를 찾지 못했습니다: {DEFAULT_CONE_MESH_PATH}")

generated_class = visual_blueprint.generated_class()
class_defaults = unreal.get_default_object(generated_class)
class_defaults.set_editor_property("use_original_lumen_cone_mesh", True)
class_defaults.set_editor_property("original_lumen_cone_mesh", cone_mesh)

unreal.EditorAssetLibrary.save_loaded_asset(visual_blueprint, only_if_is_dirty=False)

configured_enabled = class_defaults.get_editor_property("use_original_lumen_cone_mesh")
configured_mesh = class_defaults.get_editor_property("original_lumen_cone_mesh")
if not configured_enabled or configured_mesh != cone_mesh:
    raise RuntimeError("원본 Lumen 원뿔 기본값 검증에 실패했습니다.")

unreal.log("LUMEN_STATIC_CONE_VISUAL_CONFIGURED")
