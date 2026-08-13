import unreal


ASSET_PATH = "/Game/UOU/Effects/StylizedLightFX/Materials"
ASSET_NAME = "M_SLF_StaticRay_Master_V3"


asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
material = unreal.load_asset(f"{ASSET_PATH}/{ASSET_NAME}")
if material is None:
    material = asset_tools.create_asset(
        ASSET_NAME,
        ASSET_PATH,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )

material.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property("two_sided", True)
material.set_editor_property("disable_depth_test", True)

mel = unreal.MaterialEditingLibrary
beam_color = mel.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -900, -220)
beam_color.set_editor_property("parameter_name", "BeamColor")
beam_color.set_editor_property("default_value", unreal.LinearColor.WHITE)

variation_color = mel.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -900, -20)
variation_color.set_editor_property("parameter_name", "VariationColor")
variation_color.set_editor_property("default_value", unreal.LinearColor.WHITE)

intensity = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -900, -420)
intensity.set_editor_property("parameter_name", "EmissiveIntensity")
intensity.set_editor_property("default_value", 1.0)

opacity = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -900, 360)
opacity.set_editor_property("parameter_name", "Opacity")
opacity.set_editor_property("default_value", 0.12)

variation_amount = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -900, 160)
variation_amount.set_editor_property("parameter_name", "VariationAmount")
variation_amount.set_editor_property("default_value", 0.0)

variation_speed = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -900, 560)
variation_speed.set_editor_property("parameter_name", "VariationSpeed")
variation_speed.set_editor_property("default_value", 1.0)

variation_scale = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -900, 700)
variation_scale.set_editor_property("parameter_name", "VariationScale")
variation_scale.set_editor_property("default_value", 10.0)

vertex_color = mel.create_material_expression(material, unreal.MaterialExpressionVertexColor, -900, 880)
time_node = mel.create_material_expression(material, unreal.MaterialExpressionTime, -900, 1020)
world_position = mel.create_material_expression(material, unreal.MaterialExpressionWorldPosition, -900, 1140)

noise = mel.create_material_expression(material, unreal.MaterialExpressionCustom, -500, 80)
noise.set_editor_property("description", "Unity Lumen Variation과 동일한 2중 시간 노이즈")
noise.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
noise.set_editor_property(
    "code",
    """
float2 uv = float2(WorldPosition.x + WorldPosition.y, WorldPosition.z);
float t = Time * VariationSpeed;

// Unity 원본처럼 서로 다른 속도와 스케일의 부드러운 노이즈 두 장을 겹칩니다.
float2 p1 = (uv - float2(t * 0.5, t * 0.5)) * (2.0 / max(VariationScale, 0.001));
float2 p2 = (uv + float2(t, t)) * (1.0 / max(VariationScale, 0.001));

float2 i1 = floor(p1);
float2 f1 = frac(p1);
f1 = f1 * f1 * (3.0 - 2.0 * f1);
float a1 = frac(sin(dot(i1, float2(12.9898, 78.233))) * 43758.5453);
float b1 = frac(sin(dot(i1 + float2(1, 0), float2(12.9898, 78.233))) * 43758.5453);
float c1 = frac(sin(dot(i1 + float2(0, 1), float2(12.9898, 78.233))) * 43758.5453);
float d1 = frac(sin(dot(i1 + float2(1, 1), float2(12.9898, 78.233))) * 43758.5453);
float n1 = lerp(lerp(a1, b1, f1.x), lerp(c1, d1, f1.x), f1.y);

float2 i2 = floor(p2);
float2 f2 = frac(p2);
f2 = f2 * f2 * (3.0 - 2.0 * f2);
float a2 = frac(sin(dot(i2, float2(39.3468, 11.135))) * 24634.6345);
float b2 = frac(sin(dot(i2 + float2(1, 0), float2(39.3468, 11.135))) * 24634.6345);
float c2 = frac(sin(dot(i2 + float2(0, 1), float2(39.3468, 11.135))) * 24634.6345);
float d2 = frac(sin(dot(i2 + float2(1, 1), float2(39.3468, 11.135))) * 24634.6345);
float n2 = lerp(lerp(a2, b2, f2.x), lerp(c2, d2, f2.x), f2.y);

return saturate(min(n1, n2) * VariationAmount);
""".strip(),
)
noise.set_editor_property(
    "inputs",
    [],
)
custom_inputs = []
for input_name in (
    "WorldPosition",
    "Time",
    "VariationSpeed",
    "VariationScale",
    "VariationAmount",
):
    custom_input = unreal.CustomInput()
    custom_input.set_editor_property("input_name", input_name)
    custom_inputs.append(custom_input)
noise.set_editor_property("inputs", custom_inputs)
mel.connect_material_expressions(world_position, "", noise, "WorldPosition")
mel.connect_material_expressions(time_node, "", noise, "Time")
mel.connect_material_expressions(variation_speed, "", noise, "VariationSpeed")
mel.connect_material_expressions(variation_scale, "", noise, "VariationScale")
mel.connect_material_expressions(variation_amount, "", noise, "VariationAmount")

color_lerp = mel.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate, -220, -120)
mel.connect_material_expressions(beam_color, "", color_lerp, "A")
mel.connect_material_expressions(variation_color, "", color_lerp, "B")
mel.connect_material_expressions(noise, "", color_lerp, "Alpha")

emissive_multiply = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, 40, -180)
mel.connect_material_expressions(color_lerp, "", emissive_multiply, "A")
mel.connect_material_expressions(intensity, "", emissive_multiply, "B")
mel.connect_material_property(emissive_multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

vertex_red = mel.create_material_expression(material, unreal.MaterialExpressionComponentMask, -500, 720)
vertex_red.set_editor_property("r", True)
vertex_red.set_editor_property("g", False)
vertex_red.set_editor_property("b", False)
vertex_red.set_editor_property("a", False)
mel.connect_material_expressions(vertex_color, "", vertex_red, "")

alpha_mul_1 = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, -180, 420)
alpha_mul_2 = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, 40, 460)
depth_fade = mel.create_material_expression(material, unreal.MaterialExpressionDepthFade, -180, 600)
depth_fade.set_editor_property("fade_distance_default", 300.0)
final_alpha = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, 280, 480)

mel.connect_material_expressions(opacity, "", alpha_mul_2, "A")
mel.connect_material_expressions(vertex_red, "", alpha_mul_2, "B")
mel.connect_material_expressions(alpha_mul_2, "", final_alpha, "A")
mel.connect_material_expressions(depth_fade, "", final_alpha, "B")
mel.connect_material_property(final_alpha, "", unreal.MaterialProperty.MP_OPACITY)

mel.layout_material_expressions(material)
mel.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
unreal.log("LUMEN_STATIC_RAY_MATERIAL_CREATED")
