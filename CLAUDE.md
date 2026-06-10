# GASAura / Y3demo — Claude operating notes

This project was the GASAura GAS course project (Stephen Ulibarri / DruidMech), now converted into **Y3demo** — a single-player MOBA-roguelite survival game. C++ in `Source/Aura_Learn/`.

## Content layout (CANONICAL since 2026-06-10 restructure — 7 top dirs, see Tools/Y3_Folder_Structure.md)

| Top dir | What's there |
|---|---|
| `/Game/Core/` | Framework: `GameModes/` (BP_MainMenuGameMode / BP_SelectionGameMode / BP_BattleGameMode + BP_StageSpawner), `Player/` (BP_AuraGameInstance / BP_AuraPlayerController / BP_AuraPlayerState / BP_LoadScreenSaveGame[legacy]), `Input/` (IMC_AuraContext, IA_*, DA_AuraInputAction) |
| `/Game/Characters/` | `Heroes/` (6 BP_Hero_* + Aura/ BP_AuraCharacter+ABP + Demo/ mesh-swap set + Models/ SK_Hero_Default), `Enemies/` (enemy BPs per race + AI/ behavior trees + Art/ enemy models), `Models/` (Shared/Aura model set + AnimNotifies) |
| `/Game/Gameplay/` | `Abilities/` (all GAs+CD/Cost GEs by element, Enemy/ enemy GAS), `Effects/` (attribute GEs, Cues/ GameplayCueNotify — ini path!, Generic/GE_Damage), `Actors/` (potions/crystals/flame pillar/magic circles/pickups), `SharedData/` (DA_AbilityInfo, DA_CharacterClassInfo, DA_AttributeInfo, DA_LevelUpInfo, DA_LootTiers, CT_* curve tables) |
| `/Game/UI/` | `HUD/` (BP_Y3HUD, WBP_StageHUD, overlay, skill slots, controllers), `Menus/` (main/selection/skill-choice/result/atlas), `Panels/` (read-only attribute menu), `SkillIcons/` (833), `Fonts/`, `Art/` (UI textures/materials incl. login art), `Common/` |
| `/Game/Art/` | `Effects/` (28 Niagara systems), `Sounds/` (all audio), `Materials/`, `Spells/` (shard meshes) |
| `/Game/Data/` | All DT_* tables (Stage/SpawnTimer/UnitType/UnitAttr/SkillTuning/UpgradeChoice) + `CSVSource/` |
| `/Game/Maps/` | Map_Y3_MainMenu / Map_Y3_Selection / Map_Y3_Battle_01 |

**Old `/Game/Blueprints/`, `/Game/Assets/`, `/Game/Demo/` trees no longer exist** (restructured 2026-06-10, redirectors cleaned). Config hard paths already updated: `GameInstanceClass` → `/Game/Core/Player/...`, `GameplayCueNotifyPaths` → `/Game/Gameplay/Effects/Cues`. Note: the old BP_Mob_*/BP_Boss_* placeholder BPs were deleted in earlier slimming; wave spawning uses `NormalEnemyPool`/`EliteEnemyPool`/`BossEnemyClass` on the BP_BattleGameMode CDO.
**Asset-move gotcha (learned hard)**: editor folder moves DON'T fix BP node pin-default object paths and may silently fail on in-use assets — after any bulk move, binary-scan uassets for stale path strings and diff git D-vs-A pairs for losses.

Engine: **UE 5.7**. `.uproject` is `Aura_Learn.uproject`. C++ module name is `Aura_Learn`. Tag singleton has a typo: `FAuraGmaeplayTags` (not "Gameplay") — do not "fix" without coordinating, the project relies on this exact name.

## How to operate this project via AI

The bundled **UE_MCP_Bridge** plugin (v0.3.0, `Plugins/UE_MCP_Bridge/`) runs a JSON-RPC WebSocket server on **port 9877** whenever the editor is open. The matching MCP server (`ue-mcp` Node package) is registered in `.mcp.json` (project scope) — Claude Code launched in this directory will pick it up automatically.

When the editor is open you can also call the bridge directly from any Node script (no MCP needed) — Node 22+ has native `WebSocket`. Useful for one-off fixes when MCP isn't loaded.

**438-450 actions across 19 categories** per [db-lyon docs](https://db-lyon.github.io/ue-mcp/) (Asset, Level, Widget, GAS, Networking, Niagara, Animation, Sequencer, Material, Physics, Editor, Reflection, etc.). Source of truth for what's actually wired in this project: `Plugins/UE_MCP_Bridge/Source/UE_MCP_Bridge/Private/Handlers/*.cpp`.

## Current session state (handoff — read first)

**Last verified state (2026-05-18, this is what you're picking up):**
- Editor compiles & opens with **only `UE_MCP_Bridge` enabled** in `Plugins` array of `.uproject` (a second incompatible plugin `UnrealMCP` was removed — see "Things to avoid" below).
- `DA_AuraInputAction.AbilityInputActions` has **7 entries**: `IA_LMB→InputTag.LMB`, `IA_KeyNum1..6→InputTag.1..6`. Click-to-move works via LMB; number keys 1-6 fire equipped abilities.
- `WBP_EquippedSpellRow.Wrapbox_offensive` displays slots in order **1,2,3,4,5,6** (visual). Internal widget names `Skill_LMB`/`Skill_RMB` are retained but their `InputTag` BP variable is now `InputTag.5`/`InputTag.6` respectively.
- `WBP_HealthManaSpeels.SpellGolbBox` has the same retarget + reorder: visible 1-6 left to right, bound to `InputTag.1`..`InputTag.6` respectively.

**Currently in progress: Map_Y3_Battle_01 怪物刷新**
- Data tables are READY: `DT_Y3_Stage` / `DT_Y3_Unit` / `DT_Y3_Timer` contain rich Y3 game data (stages with timerID refs, units 10001-19xxx, wave timer rules). Already inspected last session.
- `BP_Y3_BattleGameMode` CDO uses `BP_AuraCharacter` as `default_pawn_class` (not a Y3 hero — flag for later, not blocking spawning).
- `BP_Y3_StageSpawner` CDO has no exposed UPROPERTY config — logic lives in BP Event Graph (not yet inspected).
- **Next concrete step**: user opens `Map_Y3_Battle_01` in editor, then run `get_world_outliner` to see what's placed (looking for: `BP_Y3_StageSpawner` instances, `PlayerStart`, `NavMeshBoundsVolume`, lighting, ground). Decide based on what's there.

## Critical API patterns (learned in session 2026-05-18)

These cost round-trips last time. Use them directly, do not re-explore.

### Read an asset property value
```js
// Returns one property's value as UE text format
rpc("read_asset_properties", { assetPath, propertyName });
// Or all values:
rpc("read_asset_properties", { assetPath, includeValues: true });
```

### Set a DataAsset's struct array (e.g. AbilityInputActions, GE modifiers, etc.)
```js
rpc("set_property", {
  objectPath: "/Game/Core/Input/DA_AuraInputAction.DA_AuraInputAction",
  propertyName: "AbilityInputActions",
  value: [
    { InputAction: "/Game/Core/Input/InputActions/IA_KeyNum1.IA_KeyNum1",
      InputTag:    "(TagName=\"InputTag.1\")" },
    // ...
  ],
});
```
`FObjectProperty` accepts asset-path string. `FStructProperty` (like `FGameplayTag`) accepts UE text-import string `(TagName="...")`. Do NOT try to construct GameplayTags in Python — the manager/library isn't exposed.

### Set a widget property (incl. BP variables on child widgets)
```js
rpc("set_widget_property", {
  assetPath:   "/Game/.../WBP_Foo.WBP_Foo",
  widgetName:  "Skill_LMB",
  propertyName:"InputTag",
  value:       "(TagName=\"InputTag.5\")",
});
```
This recompiles + saves the WBP after every call. **Batch many widget edits into one `execute_python` script** to avoid N compiles.

### Reorder children within the same parent panel
`move_widget` is a noop if `oldParent == newParent`. Use Python via `execute_python`:
```python
import unreal
wrap = unreal.find_object(None, "<wbp path>:WidgetTree.<panel name>")
kids = list(wrap.get_all_children())
by_name = {c.get_name(): c for c in kids}
for c in kids:
    wrap.remove_child(c)
for n in TARGET_ORDER:
    wrap.add_child(by_name[n])
unreal.EditorAssetLibrary.save_loaded_asset(wbp)
```
`unreal.find_object(None, "<asset>:WidgetTree.<name>")` is the only way to grab a widget by name from outside the editor — `wbp.WidgetTree` is protected.

### Python access for struct fields marked EditDefaultsOnly
`set_editor_property("input_action", ia_obj)` fails on instances. Workarounds:
1. Use the C++ struct constructor: `unreal.AuraInputAction(input_action=ia, input_tag=tag)` — bypasses the editor-flag check.
2. Use the bridge's `set_property` handler — it goes through `MCPJsonProperty::SetJsonOnProperty` which ignores edit flags (this is the preferred path).

## Input system overview (post-fix state)

- **InputTags** (native, defined in `AuraGameplayTags.cpp` `InitInputTags()`): `InputTag.LMB`, `InputTag.RMB`, `InputTag.1`–`InputTag.6`, `InputTag.Passive`, `InputTag.Passive.1`, `InputTag.Passive.2`.
- **IMC_AuraContext** (`Content/Core/Input/IMC_AuraContext.uasset`): WASD → IA_Move, keys 1-6 → IA_KeyNum1-6, LMB/RMB still mapped to IA_LMB/RMB, LeftShift → IA_Shift.
- **DA_AuraInputAction** (`Content/Core/Input/DA_AuraInputAction.uasset`): 7 entries — `IA_LMB→InputTag.LMB`, `IA_KeyNum1..6→InputTag.1..6`. RMB intentionally not bound (no ability uses it). LMB drives both `InputTag.LMB`-tagged abilities (none currently equipped) AND the click-to-move logic in `AAuraPlayerController::AbilityInputTagReleased`.
- **Skill UI** (`(已删除:旧SpellMenu链)`): `Wrapbox_offensive` displays 6 number slots (1-6) followed by passive row. The widgets internally are still named `Skill_LMB` / `Skill_RMB` for slots 5 and 6 (their `InputTag` BP variable was retargeted to `InputTag.5` / `InputTag.6`, and the adjacent TextBlock labels are "5"/"6"). The user accepted leaving the internal names to avoid breaking any BP graph that does `Get Widget By Name`.

## Monster design intent (Y3demo)

- **Visual placeholders, mechanical variety**: ~60 `BP_Y3_Mob_* / BP_Y3_Boss_* / BP_Y3_Bldg_*` BPs exist in `Content/Y3Demo/Mobs/`. They are NOT 60 distinct monsters visually — all share visuals from **3 working enemy meshes inherited from the upstream GASAura project** (placeholder art). Final art will replace these later.
- **Source of truth for mapping**: `DT_Y3_Unit` data table. Each row has `unitType` (e.g. 10001-19xxx) and `unitTypeTemplate` (which visual to reuse). Many distinct unitTypes share one template = same mesh, different stats / level / behavior. This is the survivor-like design — high throughput from a small art set.
- **What "spawning works" looks like in PIE**: dozens of mobs visually identical to one of the original 3 Aura enemy meshes, all swarming the player. Don't be confused by visual duplication — that's intentional.

## Conventions / preferences

- Two-stage edits when modifying assets: **functional fix → verify → UI fix → verify**. Don't bundle both into one mass mutation.
- Batch widget mutations into one Python script (one BP compile) rather than chained `set_widget_property` calls (N compiles).
- Probe before mutate. Read the current state of every asset before writing.
- Preserve a rollback path — snapshot pre-state in script output, and use the bridge's returned `rollback` payload when present.
- The user writes in Chinese; respond in Chinese.

## Roguelite architecture (5 modules — locked-in 2026-05-22)

Y3demo is a single-player **roguelite** (per-run roguelike loop + account-wide permanent progression). Reference: Hades / Dead Cells / Vampire Survivors.

| Module | Role | Persistence |
|---|---|---|
| 0. Boot/Login | Title screen → Hub. Future: platform SDK, hot-update | n/a |
| 1. Hub (HeroSelect + LevelSelect) | Pick hero + difficulty tier. **NO save-slot picking — single account** | account |
| 2. In-Run | Battle, 3-choice level-up, in-run shop, mob drops | per-run (cleared on end) |
| 3. Out-of-Run (Meta) | Permanent stat upgrades per hero, unlocks, account currency | account |
| 4. Reward Loop | Run-end reward = Difficulty × StageReached × Bonus → coins → Module 3 | account |

**Critical architectural decisions:**
- **One account, no save slots.** Replace `ULoadScreenSaveGame` slot system with single fixed slot keyed by account ID (local for now, cloud later).
- **One map, multi-difficulty via numerical scaling.** `DT_Difficulty` table (TODO) injects HP/damage/spawn-rate multipliers into `SpawnY3Enemy`.
- **3-choice upgrade pool** uses existing `DA_AbilityInfo` as candidate source. New `DT_UpgradeChoice` data table for non-ability stat upgrades.
- **Account state** schema (suggested): `Coins`, `UnlockedHeroIDs: Set<int>`, `MaxClearedDifficulty: Map<LevelID, int>`, `PermanentUpgrades: Map<HeroID, Map<StatTag, int>>`.

Full detail (with state-of-mapping, P0-P5 phase plan): see [[project-y3demo-mission]] in memory.

## Live Coding pitfalls (learned the hard way)

- **Live Coding patches are in-memory only — they do NOT survive editor crashes or restarts.** If the editor process dies, all `hot_reload` / `LiveCoding.Compile` changes vanish and the binary reverts to whatever was last built externally via `Build.bat`. To make C++ changes survive, you MUST do an **external full build** at least once with editor closed:
  ```powershell
  & "D:\UE_5.7\Engine\Build\BatchFiles\Build.bat" Aura_LearnEditor Win64 Development -Project="D:\UnrealProjects\GASAura\Aura_Learn.uproject" -waitmutex
  ```
  After this completes, reopen `.uproject` — the changes are now baked into `Binaries/Win64/UnrealEditor-Aura_Learn.dll`.
- **First Live Coding compile of a session takes 4-8 minutes** even for tiny changes (full link). Subsequent incremental compiles in the same session are seconds.
- `hot_reload` via bridge triggers `ILiveCoding::Compile()`. Poll `live_coding_status.compiling` — it flips false when done. But the bridge's status flag flipping doesn't always align with the actual compile finish in the LiveCoding Console — also check `Saved/Logs/Aura_Learn.log` for `Live coding succeeded` / `Live coding failed`.

## BP_Y3_BattleGameMode required data refs (CRASH ROOT)

`AY3BattleGameMode` inherits from `AAuraGameModeBase` which declares 3 critical `UPROPERTY(EditDefaultsOnly)` that MUST be set on the BP CDO or `UExecCalc_Damage::Execute_Implementation()` will crash with `EXCEPTION_ACCESS_VIOLATION` at `ExecCalc_Damage.cpp:158` the moment an enemy applies damage to the player:
- `CharacterClassInfo` → `/Game/Gameplay/SharedData/DA_CharacterClassInfo`
- `AbilityInfo` → `/Game/Gameplay/SharedData/DA_AbilityInfo`
- `LootTiersInfo` → `/Game/Gameplay/SharedData/DA_LootTiers`

`UAuraAbilitySystemBPLibary::GetCharacterClassInfo` returns `GameMode->CharacterClassInfo` with no null check, then `ExecCalc_Damage` derefs `DefaultClassInfo->DamageCalculationCoefficients->FindCurve(...)`. If you fork a new GameMode from `AAuraGameModeBase`, copy these 3 refs from `BP_AuraGameMode` immediately. Fixed via `set_property` (see [[reference-ue-mcp-api-quirks]] for the pattern) on 2026-05-22.

## Things to avoid

- **Don't install a second MCP plugin alongside `UE_MCP_Bridge`.** On 2026-05-18 a plugin called `UnrealMCP` (chongdashu/unreal-mcp lineage) was added and broke the Editor's compile because:
  - Its source uses `ANY_PACKAGE` (the `FindObject<UClass>(ANY_PACKAGE, ...)` pattern), which was **removed in UE 5.4** — modern equivalent is `FindFirstObject<UClass>(...)`. Compile errors: `error C2065: 'ANY_PACKAGE': undeclared identifier` × 9.
  - Its `MCPServerRunnable.cpp` shadows `BufferSize` in a way that the UE 5.7 toolchain rejects as `error C4459`.
  - **Two MCP plugins in parallel = functional duplication, not addition.** `UE_MCP_Bridge` alone provides 438+ actions, covering everything `chongdashu/unreal-mcp` does and more.
  - **If you're tempted by VibeUE / cwilcox0916/claude-ue-plugin** — they have advantages (in-editor chat panel, 953 methods, deeper Claude Code integration) but **not enough to justify a full migration mid-project**. Wait until current `UE_MCP_Bridge` hits a real feature wall.
- Don't run `npx ue-mcp init` casually — it overwrites `Plugins/UE_MCP_Bridge/` with whatever version is on npm. Plugin version drift would force a full rebuild.
- Don't "fix" the `FAuraGmaeplayTags` typo — it's load-bearing across the codebase.
- Don't rename `Skill_LMB` / `Skill_RMB` widgets in `WBP_EquippedSpellRow` without first scanning all BP graphs for `Get Widget By Name = "Skill_LMB"` / `"Skill_RMB"` — they're functionally slots 5 and 6 now.
