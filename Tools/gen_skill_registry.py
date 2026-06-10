# 生成 DT_SkillRegistry 的导入 JSON：833 个未开发图标行 + 已开发技能行
import json, os, re

BASE = r"C:\Users\yasuozheng\WorkBuddy\2026-06-03-16-43-12\Dota2_Ability_Icons"
TOOLS = r"D:\UnrealProjects\GASAura\Tools"

tags = json.load(open(os.path.join(BASE, "icon_tags.json"), encoding="utf-8"))
namemap = json.load(open(os.path.join(TOOLS, "skill_icon_namemap.json"), encoding="utf-8"))
file2asset = {v: k for k, v in namemap.items()}   # 原文件名 -> 资产名

rows = []
seen = set()
for e in tags:
    asset = file2asset.get(e.get("file"))
    if not asset or asset in seen:
        continue
    seen.add(asset)
    rows.append({
        "Name": asset,
        "Icon": f"/Game/UI/SkillIcons/{asset}.{asset}",
        "DisplayName": e.get("label") or asset,
        "Hero": e.get("hero") or "",
        "Attr": e.get("attr") or "",
        "Tags": e.get("tags") or [],
        "Ability": "",   # 空 = 未开发
    })

# 全项目技能图标统一为 SkillIcons 里的方形彩色图（任何地方只用这一套）。
# DA_AbilityInfo.AbilityIcon 也指到这些同名图。AutoMissile 只在图鉴里。
ABIL = "/Game/Blueprints/AbilitySystem/Aura/Abilities"
SQ = "/Game/UI/SkillIcons"
devs = [
    ("AutoMissile 自动追踪导弹", f"{ABIL}/Auto/GA_AutoMissile.GA_AutoMissile_C",            f"{SQ}/T_Homing_Missile.T_Homing_Missile"),
    ("FireBolt 火球",           f"{ABIL}/Fire/FireBolt/GA_FireBolt.GA_FireBolt_C",          f"{SQ}/T_Fireball.T_Fireball"),
    ("Electrocute 闪电链",       f"{ABIL}/Lightning/GA_Electrocute.GA_Electrocute_C",        f"{SQ}/T_Arc_Lightning.T_Arc_Lightning"),
    ("FireBlast 火焰新星",       f"{ABIL}/Fire/FireBlast/GA_FireBlast.GA_FireBlast_C",       f"{SQ}/T_Fireblast.T_Fireblast"),
    ("ArcaneShards 奥术碎片",     f"{ABIL}/Arcane/ArcaneShards/GA_ArcaneShards.GA_ArcaneShards_C", f"{SQ}/T_Arcane_Orb.T_Arcane_Orb"),
    ("HaloOfProtection 守护光环", f"{ABIL}/PassiveSpells/GA_HaloOfProtection.GA_HaloOfProtection_C", f"{SQ}/T_Guardian_Angel.T_Guardian_Angel"),
    ("LifeSiphon 生命虹吸",       f"{ABIL}/PassiveSpells/GA_LifeSiphon.GA_LifeSiphon_C",      f"{SQ}/T_Life_Drain.T_Life_Drain"),
    ("ManaSiphon 魔力虹吸",       f"{ABIL}/PassiveSpells/GA_ManaSiphon.GA_ManaSiphon_C",      f"{SQ}/T_Mana_Drain.T_Mana_Drain"),
]
for name, abil, icon in devs:
    rows.append({
        "Name": "DEV_" + re.sub(r'[^0-9A-Za-z]', '_', name),
        "Icon": icon,
        "DisplayName": name,
        "Hero": "",
        "Attr": "dev",
        "Tags": ["已开发"],
        "Ability": abil,
    })

out = os.path.join(TOOLS, "DT_SkillRegistry.json")
json.dump(rows, open(out, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
print("total rows:", len(rows), "| developed:", len(devs), "| ->", out)
