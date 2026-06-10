# 从 HTML 图鉴解析：一级 tab(data-prim) + 二级分组(dim-title→data-tag)，精确字符串。
# 输出 JSON(给人读) + C++ 片段(给 BuildFilterChips 用，避免手敲中文出错)。
import re, json

HTML = r"C:/Users/yasuozheng/WorkBuddy/2026-06-03-16-43-12/Dota2_Ability_Icons/index.html"
OUT_JSON = r"D:/UnrealProjects/GASAura/Tools/skill_filter_struct.json"
OUT_CPP  = r"D:/UnrealProjects/GASAura/Tools/_filter_cpp_snippet.txt"

html = open(HTML, encoding="utf-8").read()

# 一级 tab：<button ... data-prim="KEY" ...>LABEL<span ...>N</span>  (取按钮可见文字里的中文标签)
prims = []
for m in re.finditer(r'data-prim="([^"]+)"[^>]*>(.*?)</button>', html, re.S):
    key = m.group(1)
    inner = m.group(2)
    label = re.sub(r'<[^>]+>.*?</[^>]+>', '', inner)  # 去掉 span(数量)等
    label = re.sub(r'<[^>]+>', '', label).strip()
    prims.append({"key": key, "label": label})

# 二级分组：<div class="dim"><div class="dim-title">GROUP</div> ... data-tag="TAG"
groups = []
for dm in re.finditer(r'<div class="dim"><div class="dim-title">(.*?)</div>(.*?)</div></div>', html, re.S):
    gname = dm.group(1).strip()
    body = dm.group(2)
    tags = re.findall(r'data-tag="([^"]+)"', body)
    if gname and tags:
        groups.append({"group": gname, "tags": tags})

json.dump({"prims": prims, "groups": groups}, open(OUT_JSON, "w", encoding="utf-8"), ensure_ascii=False, indent=1)

# 生成 C++ 片段
def esc(s): return s.replace('"', '\\"')
lines = []
lines.append("// ==== 一级 tab(属性/分类) ==== (key, 中文标签)")
lines.append("// AttrKey: all=全部, str/agi/int/all=英雄属性, monster/system=cat")
for p in prims:
    lines.append(f'//   prim key="{p["key"]}" label="{p["label"]}"')
lines.append("")
lines.append("// ==== 二级分组(组名 -> 标签) ====")
for g in groups:
    taglist = ", ".join(f'TEXT("{esc(t)}")' for t in g["tags"])
    lines.append(f'// 组「{g["group"]}」: {{ {taglist} }}')
open(OUT_CPP, "w", encoding="utf-8").write("\n".join(lines))

print("prims:", len(prims), "groups:", len(groups))
print("group sizes:", [(g["group"], len(g["tags"])) for g in groups])
print("prim keys:", [p["key"] for p in prims])
