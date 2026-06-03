# Y3 UI Architecture Audit

Date: 2026-06-03

## Current Control Baseline

- UE MCP is reachable through `Tools/Invoke-UeMcp.ps1`.
- Current battle map: `/Game/Maps/Map_Y3_Battle_01`.
- `Map_Y3_Battle_01` uses `/Game/Core/GameModes/BP_BattleGameMode.BP_BattleGameMode_C`.
- `BP_BattleGameMode` required refs are set:
  - `DT_Stage` -> `/Game/Data/DT_Stage`
  - `DT_Timer` -> `/Game/Data/DT_SpawnTimer`
  - `DT_Unit` -> `/Game/Data/DT_UnitType`
  - `DT_Attr` -> `/Game/Data/DT_UnitAttr`
  - `StageHUDClass` -> `/Game/UI/HUD/WBP_StageHUD`
  - `ResultScreenClass` -> `/Game/UI/Menus/WBP_ResultScreen`
  - `SkillChoiceClass` -> `/Game/UI/Menus/WBP_SkillChoice`
  - Aura inherited refs: `DA_CharacterClassInfo`, `DA_AbilityInfo`, `DA_LootTiers`

Short PIE smoke test passed:
- Player pawn: `/Game/Characters/Heroes/BP_Hero_Furi`
- Runtime viewport widgets include `WBP_StageHUD_C_0` and old `WBP_Overlay_C_0`.

## High-Level State

The project currently has two UI roots:

- New Y3 UI: `/Game/UI/...`
- Original Aura UI: `/Game/Blueprints/UI/...`

Do not delete by folder. Some original Aura UI is still used by Y3 battle because `BP_AuraHUD` still creates `WBP_Overlay`, and `WBP_Overlay` still contains the health/mana/skill bar stack that Y3 uses.

## Keep As Y3 Runtime UI

These are active or structurally useful for the current Y3 loop.

### New Y3 UI

- `/Game/UI/Menus/WBP_MainMenu`
- `/Game/UI/Menus/WBP_HeroSelection`
- `/Game/UI/Menus/WBP_SkillChoice`
- `/Game/UI/Menus/WBP_ResultScreen`
- `/Game/UI/HUD/WBP_StageHUD`
- `/Game/UI/Textures/T_ResultWin`
- `/Game/UI/Textures/T_ResultLose`
- `/Game/UI/Menus/Heroes/*`

### Original Aura UI Still Used

- `/Game/Blueprints/UI/HUD/BP_AuraHUD`
- `/Game/Blueprints/UI/OverLay/WBP_Overlay`
- `/Game/Blueprints/UI/SubWidget/WBP_HealthManaSpeels`
- `/Game/Blueprints/UI/ProgressBar/WBP_Health`
- `/Game/Blueprints/UI/ProgressBar/WBP_Mana`
- `/Game/Blueprints/UI/ProgressBar/WBP_XPBar`
- `/Game/Blueprints/UI/ProgressBar/WBP_EnemyHealth`
- `/Game/Blueprints/UI/SpellGlobs/WBP_SpellGlobBase`
- `/Game/Blueprints/UI/SpellGlobs/WBP_ValueGlob`
- `/Game/Blueprints/UI/Button/WBP_BtnBase`
- `/Game/Blueprints/UI/Button/WBP_BtnWide`
- `/Game/Blueprints/UI/AreYouSure/WBP_AreYouSure`
- `/Game/Blueprints/UI/FloatingText/WBP_DamageText`
- `/Game/Blueprints/UI/FloatingText/BP_DamageTextComponent`

Reason:
- `WBP_Overlay` currently contains:
  - `WBP_HealthManaSpeels`
  - `WBP_XPBar`
  - `WBP_PictureFrame`
  - `WBPValue_Level`
  - `Btn_AttributeMenu`
  - `Btn_SpellMenu`
  - `Btn_Close`
- `WBP_HealthManaSpeels` contains active health/mana, six active skill slots, and passive slots.

## Convert / Redesign Instead Of Delete

### Character / Attribute Info

Current old assets:
- `/Game/Blueprints/UI/AttributeMenu/WBP_AttributeMenu`
- `/Game/Blueprints/UI/AttributeMenu/WBP_TextValueRow`
- `/Game/Blueprints/UI/AttributeMenu/WBP_TextValueBtnRow`
- `/Game/Blueprints/UI/AttributeMenu/WBP_FrameValue`
- `/Game/Blueprints/UI/AttributeMenu/WBP_AttributePointRow`
- `UAttributeMenuWgtController`

Future direction:
- Keep the idea of a character/stat page.
- Remove attribute-point spending.
- Convert primary/secondary stat display into a read-only or build-summary panel.
- Replace `WBP_TextValueBtnRow` plus-button rows with display-only rows.
- Retire `WBP_AttributePointRow`.
- Replace or shrink `UAttributeMenuWgtController::UpgradeAttribute`.

### Run Skill Summary / Statistics

New UI needed:
- A run statistics panel showing:
  - Active skills acquired this run
  - Passive skills acquired this run
  - Possibly skill levels, damage contribution, kills, uptime, or run modifiers later

Recommended new asset:
- `/Game/UI/Menus/WBP_RunStats`

Recommended data owner:
- `Y3RunState` or `Y3UpgradeManager`, not `USpellMenuWgtController`.

## Retire / Delete Candidates

These are aligned with the old RPG spell-tree flow and should be retired after references are removed.

### Old Spell Menu Tree

- `/Game/Blueprints/UI/SpellMenu/WBP_SpellMenu`
- `/Game/Blueprints/UI/SpellMenu/WBP_OffensiveSpellTree`
- `/Game/Blueprints/UI/SpellMenu/WBP_PassiveSpellTree`
- `/Game/Blueprints/UI/SpellMenu/WBP_EquippedSpellRow`
- `/Game/Blueprints/UI/SpellGlobs/WBP_EquelRow_Btn`
- `/Game/Blueprints/UI/SpellGlobs/WBP_BTN_Glob`
- `/Game/Blueprints/UI/WIdgetController/BP_SpellMenuWgtController`
- `USpellMenuWgtController`

Reason:
- Old flow is skill points -> learn/unlock -> manually equip.
- Y3 flow is three-choice upgrade -> auto give/equip.
- `USpellMenuWgtController` still implements old `SpellPoint`, `SpendPointBtnPressed`, `EquipBtnPressed`, slot selection, and spell-tree description flow.

### Old Load-Slot Save UI

- `/Game/Blueprints/UI/LoadMenu/WBP_LoadScreen`
- `/Game/Blueprints/UI/LoadMenu/WBP_LoadScreenWgt_Base`
- `/Game/Blueprints/UI/LoadMenu/WBP_LoadSlot_*`
- `/Game/Blueprints/UI/ViewModel/BP_LoadScreenViewModel_Base`
- `/Game/Blueprints/UI/ViewModel/BP_LoadSlotViewModel`
- `/Game/Blueprints/UI/HUD/BP_LoadScreenHUD`
- `UMVVM_LoadScreen`
- `UMVVM_LoadSlot`
- `ULoadScreenSaveGame` slot-selection UI usage

Reason:
- Y3 target is single account state, no save-slot picking.
- Keep old save code temporarily only if required by inherited `AAuraGameModeBase` until `Y3AccountSaveGame` replaces it.

### Old Aura Main Menu

- `/Game/Blueprints/UI/MainMenu/WBP_MainMenu`

Reason:
- Y3 has `/Game/UI/Menus/WBP_MainMenu`.
- Confirm no map or GameMode still creates the old one before deletion.

## Code To Retire Or Rewrite

### Attribute Points

Current code paths:
- `AAuraPlayerState::AttributePoints`
- `AAuraPlayerState::AddToAttributePoints`
- `AAuraCharacter::GetAttributePointReward_Implementation`
- `AAuraCharacter::AddToAttributePoints_Implementation`
- `UAuraAbilitySystemComponent::UpgradeAttribute`
- `UAuraAbilitySystemComponent::ServerUpgradeAttribute`
- `UAttributeMenuWgtController::UpgradeAttribute`
- `UAuraAttributeSet::HandleIncomingXP` awards attribute points on level-up

Future:
- Remove manual attribute-point reward/spend from Y3.
- Keep raw attributes until the new Y3 stat model is designed.
- Replace old point-spending UI with a read-only stats page.

### Spell Points

Current code paths:
- `AAuraPlayerState::SpellPoints`
- `AAuraPlayerState::AddToSpellPoints`
- `AAuraCharacter::GetSpellPointReward_Implementation`
- `AAuraCharacter::AddToSpellPoints_Implementation`
- `UAuraAbilitySystemComponent::ServerSpendSpellPoint`
- `USpellMenuWgtController::*`
- `UAuraAttributeSet::HandleIncomingXP` awards spell points on level-up

Future:
- Remove spell-point reward/spend from Y3.
- Keep ability specs, ability tags, and input slot assignment.
- Replace old spell menu with:
  - `Y3UpgradeManager` for choices
  - `Y3RunStats` / `WBP_RunStats` for acquired active/passive summary

### Save Slots

Current code paths:
- `AAuraGameModeBase`
- `ULoadScreenSaveGame`
- `UMVVM_LoadScreen`
- `UMVVM_LoadSlot`
- `AAuraCharacter::SaveProgress`
- `AAuraCharacter::LoadProgress`
- `UAuraAbilitySystemComponent::AddCharacterAbilitiesFromSaveData`

Future:
- Introduce `UY3AccountSaveGame`.
- Keep old save-slot code isolated until Y3 account state replaces it.

## Recommended Folder Migration

Target Y3 UI layout:

- `/Game/UI/Common`
  - buttons, confirm dialogs, shared text/value rows
- `/Game/UI/HUD`
  - `WBP_StageHUD`
  - Y3 battle overlay root
- `/Game/UI/HUD/Widgets`
  - health/mana, XP, skill slots, enemy health
- `/Game/UI/Menus`
  - main menu, hero select, result, skill choice
- `/Game/UI/Panels`
  - character stats
  - run stats
- `/Game/UI/Textures`
  - Y3 menu/result textures

Migration approach:
1. Duplicate/move the still-used old widgets into `/Game/UI/...`.
2. Repoint `BP_AuraHUD` or new `BP_Y3HUD` to the new widget paths.
3. Verify PIE and runtime widgets.
4. Only then delete old `/Game/Blueprints/UI/...` assets that no longer have referencers.

## First Safe Cleanup Phase

Do this before deleting assets:

1. Create a Y3-specific HUD class or blueprint (`BP_Y3HUD`) instead of continuing to use `BP_AuraHUD` everywhere.
2. Split `WBP_Overlay` into:
   - retained `WBP_Y3BattleOverlay`
   - removed old menu buttons: `Btn_AttributeMenu`, `Btn_SpellMenu`
   - retained close button if still needed
3. Stop creating old spell/attribute menu controllers in Y3 runtime.
4. Create `WBP_RunStats`.
5. Create `Y3UpgradeManager`.
6. Remove spell-point and attribute-point rewards from the Y3 level-up path.
7. Delete old UI only after reference checks pass.
