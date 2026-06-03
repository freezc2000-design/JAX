# Y3 Architecture Slimming Plan

## Goal

Make Y3demo the readable main game structure:

- Keep runtime-critical GASAura systems as shared foundation.
- Stop adding new Y3 work under `/Game/Blueprints`.
- Move reused UI/assets into small Y3-facing folders.
- Delete old course UI/save/skill-menu assets only after referencer checks.

## Canonical Content Layout

Use these folders for active Y3 work:

- `/Game/Core` - GameModes, spawners, game-level blueprints.
- `/Game/Characters` - Y3 heroes and enemy gameplay blueprints.
- `/Game/Data` - Y3 DataTables and future data assets.
- `/Game/UI` - all active Y3 widgets, HUD, menus, shared UI widgets.
- `/Game/Maps` - Y3 maps.
- `/Game/Assets` - shared art/audio/VFX still used by GAS/Y3. Reduce later by usage group.

Treat these as legacy/source-material folders:

- `/Game/Blueprints` - original GASAura course framework and legacy UI.
- `/Game/Assets/UI/SpellTree` - old spell-tree art, delete after old spell UI is gone.
- `/Game/Blueprints/UI/LoadMenu` and `/Game/Blueprints/UI/ViewModel` - old slot-save UI.

## Completed In This Pass

- Created `/Game/UI/HUD/WBP_Y3BattleOverlay_Clean`.
- Removed old menu state variables from the Clean overlay:
  - `Wgt_AttributeMenu`
  - `SpellMenu`
- Replaced old menu-opening functions with empty handlers so existing button delegates compile but do nothing:
  - `OnBtnAttributeMenuClicked`
  - `OnBtnSpellMenuCilicked`
- Updated `/Game/UI/HUD/BP_Y3HUD`:
  - `OverlayWidgetClass` -> `WBP_Y3BattleOverlay_Clean`
  - `AttributeMenuWgtControllerClass` -> `None`
  - `SpellMenuWgtControllerClass` -> `None`
- Deleted obsolete `/Game/UI/HUD/WBP_Y3BattleOverlay_Lite`.
- PIE smoke passed on `/Game/Maps/Map_Y3_Battle_01`:
  - pawn: `BP_Hero_Furi`
  - viewport widgets: `WBP_StageHUD`, `WBP_Y3BattleOverlay_Clean`

## Immediate Migration Targets

Move active reused UI out of `/Game/Blueprints/UI`:

- `/Game/Blueprints/UI/Button/*` -> `/Game/UI/Common/Buttons`
- `/Game/Blueprints/UI/AreYouSure/WBP_AreYouSure` -> `/Game/UI/Common/Dialogs`
- `/Game/Blueprints/UI/FloatingText/*` -> `/Game/UI/HUD/FloatingText`
- `/Game/Blueprints/UI/ProgressBar/WBP_XPBar` -> `/Game/UI/HUD/Widgets`
- `/Game/Blueprints/UI/ProgressBar/WBP_Health`, `WBP_Mana`, `WBP_Globe_ProgressBar` -> `/Game/UI/HUD/Widgets`
- `/Game/Blueprints/UI/SpellGlobs/*` -> `/Game/UI/HUD/SkillSlots`
- `/Game/Blueprints/UI/SubWidget/WBP_HealthManaSpeels`, `WBP_PictureFrame`, `WBP_LevelUpMessage`, `WBP_EffectMessage` -> `/Game/UI/HUD/Widgets`
- `/Game/Blueprints/UI/WIdgetController/BP_OverlayWgtController` -> `/Game/UI/HUD/Controllers`

Use UE asset move/fixup, not filesystem moves.

## Delete Candidates

Delete only after `get_referencers` returns no Y3 runtime referencers:

- `/Game/Blueprints/UI/SpellMenu/*`
- `/Game/Blueprints/UI/AttributeMenu/WBP_AttributePointRow`
- `/Game/Blueprints/UI/AttributeMenu/WBP_TextValueBtnRow`
- `/Game/Blueprints/UI/LoadMenu/*`
- `/Game/Blueprints/UI/ViewModel/*`
- `/Game/Blueprints/UI/MainMenu/WBP_MainMenu`
- `/Game/Blueprints/MainMenu/*`
- old `BP_LoadScreenHUD`, `BP_LoadGameScreenGameMode`, `BP_LoadScreenSaveGame`

Keep or redesign later:

- `WBP_AttributeMenu` can become a future read-only stats screen, but remove point-spend logic first.
- `LoadScreenSaveGame` C++ stays until a Y3 single-account save class replaces it.

## C++ Cleanup Order

Do not delete C++ controllers first. Order:

1. Move active widget assets into `/Game/UI`.
2. Remove Y3 HUD dependencies on old menus/controllers.
3. Delete unused old WBP assets.
4. Remove or deprecate `USpellMenuWgtController`.
5. Remove or redesign `UAttributeMenuWgtController`.
6. Replace slot-save code with Y3 account save.

## Working Rule

Every destructive pass needs:

1. `get_referencers` before delete or move.
2. Compile changed blueprints.
3. Save dirty packages.
4. PIE smoke on `Map_Y3_Battle_01`.
5. Check `list_dirty_packages`.
