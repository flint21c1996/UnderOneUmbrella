import unreal


BLUEPRINT_PATH = "/Game/UOU/Effects/StylizedLightFX/Blueprints"
BLUEPRINT_NAME = "BP_LumenStaticRayVisual"
SPOT_BLUEPRINT_PATH = "/Game/UOU/BluePrint/World/Lights/BP_LS01_DynamicRay_Spot"
CYLINDER_BLUEPRINT_PATH = "/Game/UOU/BluePrint/World/Lights/BP_LS01_DynamicRay_Cylinder"
STATIC_RAY_MATERIAL_PATH = (
    "/Game/UOU/Effects/StylizedLightFX/Materials/M_SLF_StaticRay_Master_V3"
)


parent_class = unreal.load_class(
    None,
    "/Script/UnderOneUmBrella.UOULumenStaticRayVisualActor",
)
if parent_class is None:
    raise RuntimeError("UOULumenStaticRayVisualActor 클래스를 찾지 못했습니다.")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
visual_blueprint = unreal.load_asset(f"{BLUEPRINT_PATH}/{BLUEPRINT_NAME}")
if visual_blueprint is None:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    visual_blueprint = asset_tools.create_asset(
        BLUEPRINT_NAME,
        BLUEPRINT_PATH,
        unreal.Blueprint,
        factory,
    )

if visual_blueprint is None:
    raise RuntimeError("Static Ray Blueprint 생성에 실패했습니다.")

visual_defaults = unreal.get_default_object(visual_blueprint.generated_class())
static_ray_material = unreal.load_asset(STATIC_RAY_MATERIAL_PATH)
if static_ray_material is None:
    raise RuntimeError(f"Static Ray 머티리얼을 찾지 못했습니다: {STATIC_RAY_MATERIAL_PATH}")

visual_defaults.set_editor_property("preset", 8)
visual_defaults.set_editor_property("emissive_intensity_scale", 2.2)
visual_defaults.set_editor_property("opacity_scale", 1.6)
visual_defaults.set_editor_property("beam_width_scale", 0.75)
visual_defaults.set_editor_property("use_variation", True)
visual_defaults.set_editor_property("face_camera_around_beam_axis", True)
visual_defaults.set_editor_property("use_camera_distance_fade", False)
visual_defaults.set_editor_property("ray_material", static_ray_material)
unreal.EditorAssetLibrary.save_loaded_asset(visual_blueprint, only_if_is_dirty=False)

spot_blueprint = unreal.load_asset(SPOT_BLUEPRINT_PATH)
if spot_blueprint is None:
    raise RuntimeError(f"Spot Blueprint를 찾지 못했습니다: {SPOT_BLUEPRINT_PATH}")

spot_defaults = unreal.get_default_object(spot_blueprint.generated_class())
visual_components = spot_defaults.get_components_by_class(
    unreal.UOULightBeamVisualComponent,
)
if not visual_components:
    raise RuntimeError("Spot Blueprint에서 Light Beam Visual Component를 찾지 못했습니다.")

for component in visual_components:
    component.set_editor_property("vfx_actor_class", visual_blueprint.generated_class())
    component.set_editor_property("lumen_static_ray_preset", 16)
    component.set_editor_property("enable_reflection_vfx", False)

unreal.EditorAssetLibrary.save_loaded_asset(spot_blueprint, only_if_is_dirty=False)

cylinder_blueprint = unreal.load_asset(CYLINDER_BLUEPRINT_PATH)
if cylinder_blueprint is None:
    raise RuntimeError(f"Cylinder Blueprint를 찾지 못했습니다: {CYLINDER_BLUEPRINT_PATH}")

cylinder_defaults = unreal.get_default_object(cylinder_blueprint.generated_class())
cylinder_visual_components = cylinder_defaults.get_components_by_class(
    unreal.UOULightBeamVisualComponent,
)
if not cylinder_visual_components:
    raise RuntimeError("Cylinder Blueprint에서 Light Beam Visual Component를 찾지 못했습니다.")

for component in cylinder_visual_components:
    component.set_editor_property("vfx_actor_class", visual_blueprint.generated_class())
    component.set_editor_property("lumen_static_ray_preset", 17)

unreal.EditorAssetLibrary.save_loaded_asset(cylinder_blueprint, only_if_is_dirty=False)
unreal.log("LUMEN_STATIC_RAY_BLUEPRINT_CREATED_AND_CONNECTED")
