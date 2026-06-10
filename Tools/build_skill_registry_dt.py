# 在编辑器内创建 DT_SkillRegistry（行类型 FY3SkillRegistryRow）并用 JSON 填充。
import unreal, os

TOOLS = r"D:\UnrealProjects\GASAura\Tools"
DEST_DIR = "/Game/UI"
DT_NAME = "DT_SkillRegistry"
DT_PATH = f"{DEST_DIR}/{DT_NAME}"

row_struct = unreal.load_object(None, "/Script/Aura_Learn.Y3SkillRegistryRow")
assert row_struct, "找不到 FY3SkillRegistryRow（先确认 C++ 编译成功）"

if unreal.EditorAssetLibrary.does_asset_exist(DT_PATH):
    unreal.EditorAssetLibrary.delete_asset(DT_PATH)

factory = unreal.DataTableFactory()
factory.set_editor_property("struct", row_struct)
tools = unreal.AssetToolsHelpers.get_asset_tools()
dt = tools.create_asset(DT_NAME, DEST_DIR, unreal.DataTable, factory)
assert dt, "DataTable 创建失败"

json_str = open(os.path.join(TOOLS, "DT_SkillRegistry.json"), encoding="utf-8").read()
ok = unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(dt, json_str)
print("fill ok:", ok)

unreal.EditorAssetLibrary.save_loaded_asset(dt)
names = dt.get_row_names()
print("DT_SkillRegistry rows:", len(names))
