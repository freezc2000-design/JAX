# 批量导入 Dota2 技能图标 → /Game/UI/SkillIcons/
# 清洗文件名为合法资产名，分块导入，输出 文件名↔资产名 映射 JSON。
import unreal, os, re, json

SRC = r"C:\Users\yasuozheng\WorkBuddy\2026-06-03-16-43-12\Dota2_Ability_Icons\icons"
DEST = "/Game/UI/SkillIcons"
MAP_OUT = r"D:\UnrealProjects\GASAura\Tools\skill_icon_namemap.json"
CHUNK = 120

def sanitize(fn):
    base = re.sub(r'\.png$', '', fn, flags=re.I)
    base = re.sub(r'\s*(ability\s+)?icon$', '', base, flags=re.I)   # 去掉结尾 " icon"/" ability icon"
    base = re.sub(r'[^0-9A-Za-z]+', '_', base).strip('_')
    if not base:
        base = "Icon"
    if base[0].isdigit():
        base = "N_" + base
    return "T_" + base

files = sorted(f for f in os.listdir(SRC) if f.lower().endswith('.png'))
print("found png:", len(files))

used = {}
namemap = {}   # asset_name -> original_filename
tasks = []
for f in files:
    name = sanitize(f)
    n, i = name, 2
    while n in used:
        n = f"{name}_{i}"; i += 1
    used[n] = True
    namemap[n] = f
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.join(SRC, f))
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("destination_name", n)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    tasks.append(t)

# 先写映射(即便导入超时，映射也已落盘)
with open(MAP_OUT, "w", encoding="utf-8") as fp:
    json.dump(namemap, fp, ensure_ascii=False, indent=1)
print("wrote namemap:", MAP_OUT, "entries:", len(namemap))

tools = unreal.AssetToolsHelpers.get_asset_tools()
done = 0
for i in range(0, len(tasks), CHUNK):
    tools.import_asset_tasks(tasks[i:i+CHUNK])
    done = min(i + CHUNK, len(tasks))
    print("imported", done, "/", len(tasks))

print("IMPORT_DONE total", len(tasks))
