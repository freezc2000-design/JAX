# Y3 目录结构规范（2026-06-10 定稿待执行）

> 维护原则：**一级目录 = 职责层**，回答"这东西是什么"；**二级目录 = 功能域**，回答"它属于哪个系统"。
> 新资产入库时先问：是框架？是角色？是玩法规则？是界面？是纯资源？是策划表？是关卡？——七问定位，不允许新建一级目录。

## 一级目录（7 个，职责唯一）

| 目录 | 职责 | 一句话判断标准 |
|---|---|---|
| `Core/` | 框架层：游戏怎么"跑起来" | 删了它游戏无法启动/无法进局 |
| `Characters/` | 角色层：一切会动的单位（定义+美术绑一起） | 有血条或会攻击的东西 |
| `Gameplay/` | 玩法层：技能、效果、可交互物 | 决定"打起来是什么手感" |
| `UI/` | 界面层：玩家看到点到的一切 2D | 挂在屏幕上的 |
| `Art/` | 资源层：非角色非UI的纯美术/音频 | 特效、声音、通用材质 |
| `Data/` | 数值层：策划调参的唯一入口 | 改数值不碰蓝图，只来这里 |
| `Maps/` | 关卡层 | .umap |

## 二级目录明细（约 20 个）

### Core/（3）
- `GameModes/` — BP_MainMenuGameMode / BP_SelectionGameMode / BP_BattleGameMode + BP_StageSpawner（流程编排）
- `Player/` — BP_AuraGameInstance（跨图账号宿主）、BP_AuraPlayerController、BP_AuraPlayerState（ASC宿主）
- `Input/` — IMC_AuraContext、IA_*、DA_AuraInputAction（按键→InputTag 映射）

### Characters/（3）
- `Heroes/` — 6 个英雄 BP + 选人 Demo 资产；**未来每英雄一个子目录**（BP+专属模型+动画+武器）
- `Enemies/` — 敌人 BP + AI（行为树/黑板）+ 怪物模型/动画/材质（Goblin/Ghoul/Shaman/Demon/Shroom）
- `Models/` — 共享骨骼模型：Aura 主角全套（mesh/骨骼/AnimBP/物理/动画）、SK_Hero_Default

### Gameplay/（4）
- `Abilities/` — 全部 GA + 各自的 CD/Cost GE + 弹道 Actor，按元素分子目录（Fire/Lightning/Arcane/Passive/Auto）
- `Effects/` — 通用 GE（属性初始化/派生公式/伤害系数 CT_*）+ GameplayCueNotify（⚠️ 挪动必须同步 DefaultGame.ini 的 GameplayCueNotifyPaths）
- `Actors/` — 火柱/火区/药水/拾取/法阵（地面交互物=未来危害区技能的原型）
- `SharedData/` — DA_AbilityInfo（技能唯一真源）、DA_CharacterClassInfo、DA_AttributeInfo、DA_LootTiers、DA_LevelUpInfo

### UI/（6）
- `HUD/` — 战斗内常驻：BP_Y3HUD、overlay、技能槽控件、Widget 控制器
- `Menus/` — 全屏界面：主菜单/选人/三选一/结算/图鉴
- `Panels/` — 局内弹出：属性面板（只读）
- `Icons/` — SkillIcons 833 张方图（图鉴+技能栏+三选一共用）
- `Fonts/` — 全部字体（中文雅黑/隶书 + 课程字体合一）
- `Art/` — 界面贴图材质：球底 MI、方框、登录美术、按钮三态

### Art/（3）
- `Effects/` — 28 个 Niagara + 配套材质（火/电/奥术/灵魂/光环/龙卷/召唤阵）
- `Sounds/` — 技能/敌人/UI/脚步/音乐（MetaSound + wav）
- `Materials/` — 跨系统共享材质

### Data/（平铺 + 1）
- `DT_*` 全部数据表平铺 + `CSVSource/`（再导入用的原始 CSV）

### Maps/（平铺）
- Map_Y3_MainMenu / Map_Y3_Selection / Map_Y3_Battle_01

## 未来必建的二级/三级模块（按一级归位）

| 一级 | 未来模块 | 对应架构模块 | 优先级 |
|---|---|---|---|
| `Data/` | `DT_HeroAttr` 英雄初始属性表 | 解决属性写死 | ★★★ |
| `Data/` | `DT_Difficulty` 难度倍率表 | 模块2 一图多难度 | ★★★ |
| `Data/` | `DT_MetaUpgrade` 永久升级项表 | 模块3 | ★★ |
| `UI/` | `Hub/` 难度选择/英雄详情界面 | 模块1 | ★★ |
| `UI/` | `Meta/` 永久升级面板 | 模块3 | ★★ |
| `UI/Panels/` | `WBP_RunStats` 本局构筑面板 | B4 | ★ |
| `Characters/Heroes/` | `<英雄名>/` 每英雄子目录 | BP-per-hero 真模型管线 | ★★（等模型） |
| `Gameplay/` | `Upgrades/` 强化卡扩展+UpgradeManager 配套 | 三选一深化 | ★ |
| `Art/` | `ThirdParty/` 进口资产包统一落点（Fab 包导入后搬进来） | VFX 包 | ★★ |
| `Maps/` | `Map_Y3_Hub`、`Map_Y3_Battle_02+` | 模块1/2 | ★★ |

## 迁移执行规程（每批必走）
bulk_rename 移动（自动修引用）→ 受影响 BP 全量编译 → 改配置硬路径（GameInstanceClass / GameplayCueNotifyPaths / C++ LoadObject×1+重编译）→ PIE 主菜单+战斗图冒烟 → commit。任一步失败 git reset 回滚本批。
