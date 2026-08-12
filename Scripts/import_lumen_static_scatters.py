import unreal


SOURCE_FBX = r"F:\Git\Umbrella\Umbrella\Docs\유니티 에셋\Lumen\Contents\Resources\Static Scatters.fbx"
DESTINATION_PATH = "/Game/UOU/Effects/StylizedLightFX/StaticScatters"


task = unreal.AssetImportTask()
task.filename = SOURCE_FBX
task.destination_path = DESTINATION_PATH
task.automated = True
task.replace_existing = True
task.save = True

options = unreal.FbxImportUI()
options.import_mesh = True
options.import_as_skeletal = False
options.import_animations = False
options.import_materials = False
options.import_textures = False
options.static_mesh_import_data.combine_meshes = False
options.static_mesh_import_data.generate_lightmap_u_vs = False
options.static_mesh_import_data.auto_generate_collision = False
task.options = options

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

assets = unreal.EditorAssetLibrary.list_assets(
    DESTINATION_PATH,
    recursive=True,
    include_folder=False,
)
for asset in assets:
    unreal.log(f"LUMEN_STATIC_SCATTER_IMPORTED {asset}")

unreal.log(f"LUMEN_STATIC_SCATTER_IMPORT_COUNT {len(assets)}")
