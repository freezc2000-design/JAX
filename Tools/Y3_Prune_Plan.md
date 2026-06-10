# Y3 Prune Plan

Date: 2026-06-09

## Goal

Reduce the project from a GASAura RPG course architecture into a Y3 survivor / roguelite game architecture without breaking the playable loop.

The plan favors staged isolation over hard deletion.

## Current Git Risk

The working tree already contains many unrelated or user-driven changes:

- modified Y3 GameModes, UI assets, skill data, input assets, and C++ files
- added demo hero assets
- added skill atlas / registry / tuning assets
- added Y3 auto missile and chain lightning code
- deleted some old spell icon assets

Before any destructive cleanup, make a local commit or branch snapshot.

Recommended checkpoint:

```text
git status
git add -A
git commit -m "Checkpoint before Y3 architecture pruning"
```

## Phase 0 - Baseline Verification

Purpose: know what currently works.

Actions:

- Verify current main menu map opens.
- Verify battle map PIE starts.
- Verify player pawn spawns.
- Verify `WBP_StageHUD` and `WBP_Y3BattleOverlay_Clean` appear.
- Verify skill choice can open.
- Verify result screen can open.
- Verify no dirty packages after saving.

Exit criteria:

- Main menu smoke test passes.
- Battle smoke test passes.
- Git status is understood.

## Phase 1 - Draw The Y3 Boundary

Purpose: separate "Y3 runtime" from "Aura legacy support".

Keep as Y3 runtime:

- `/Game/UI/Menus/WBP_MainMenu`
- `/Game/UI/Menus/WBP_HeroSelection`
- `/Game/UI/Menus/WBP_SkillChoice`
- `/Game/UI/Menus/WBP_ResultScreen`
- `/Game/UI/Menus/WBP_SkillAtlas`
- `/Game/UI/Menus/WBP_FilterChip`
- `/Game/UI/Menus/WBP_SkillAtlasEntry`
- `/Game/UI/HUD/WBP_StageHUD`
- `/Game/UI/HUD/WBP_Y3BattleOverlay_Clean`
- `/Game/UI/HUD/BP_Y3HUD`
- `/Game/UI/HUD/Widgets/WBP_HealthManaSpeels`
- `/Game/UI/HUD/SkillSlots/WBP_SpellGlobBase`
- `/Game/UI/HUD/ProgressBars/*`
- `/Game/UI/Common/*`
- `/Game/UI/SkillIcons/*`
- `/Game/Data/DT_Stage`
- `/Game/Data/DT_SpawnTimer`
- `/Game/Data/DT_UnitType`
- `/Game/Data/DT_UnitAttr`
- `/Game/Data/DT_SkillTuning`
- `/Game/UI/DT_SkillRegistry`

Keep as legacy support for now:

- `UAuraAbilitySystemComponent`
- `UAuraAttributeSet`
- `AAuraCharacter`
- `AAuraPlayerState`
- `AAuraHUD`
- `UOverlayWidgetController`
- `UAttributeMenuWgtController`
- original projectile / enemy / damage systems

Mark as retire candidates:

- `USpellMenuWgtController`
- old spell tree widgets
- old spell-point spending
- old manual spell equip flow
- old attribute-point spending
- old MVVM load-slot UI
- old `ULoadScreenSaveGame` runtime usage

Exit criteria:

- Y3 docs identify which systems are official runtime dependencies and which are legacy.

## Phase 2 - Retire SpellMenu Flow First

Reason: this conflicts most directly with Y3's three-choice auto-equip loop.

Current code references:

- `USpellMenuWgtController`
- `AAuraHUD::GetSpellMenuController`
- `UAuraAbilitySystemBPLibary::GetSpellMenuWgtController`
- `UAuraAbilitySystemComponent::ServerSpendSpellPoint`
- `UAuraAbilitySystemComponent::ServerEquipAbility`
- `UAuraAbilitySystemComponent::Y3_EquipAbilityToSlot`
- `AAuraPlayerState::SpellPoints`
- `AAuraAttributeSet::HandleIncomingXP` awards spell points
- `AAuraCharacter::AddToSpellPoints_Implementation`

Desired Y3 replacement:

- `UY3UpgradeManager`
- `FY3RunSkillState`
- `FY3SkillOffering`
- `WBP_SkillChoice` calls the Y3 upgrade manager
- ASC only grants, activates, and equips abilities

Safe steps:

1. Confirm Y3 HUD no longer opens old SpellMenu.
2. Move Y3 choice generation out of `AY3BattleGameMode`.
3. Replace direct spell-point upgrades with Y3 skill stock / level rules.
4. Stop broadcasting spell-point UI state in Y3 runtime.
5. Keep `ServerEquipAbility` temporarily if Y3 auto-equip still uses slot tags.
6. Delete old SpellMenu assets only after referencer scan is clean.

Do not delete yet:

- `ServerEquipAbility`
- slot tags
- `AbilityEquippedDel`

These still support Y3 HUD skill slots and passive visual behavior.

## Phase 3 - Convert Attribute Menu Into Character Stats

Reason: attributes are still useful, manual attribute points are not.

Current code references:

- `AAuraPlayerState::AttributePoints`
- `UAttributeMenuWgtController`
- `UAuraAbilitySystemComponent::UpgradeAttribute`
- `UAuraAbilitySystemComponent::ServerUpgradeAttribute`
- `AAuraAttributeSet::HandleIncomingXP` awards attribute points
- `WBP_AttributeMenu`
- `WBP_AttributePointRow`
- `WBP_TextValueBtnRow`

Desired Y3 replacement:

- `WBP_CharacterStats`
- display-only stat rows
- optional build summary: hero, active skills, passives, derived stats

Safe steps:

1. Keep AttributeSet.
2. Hide or remove attribute-point button UI from the Y3 panel.
3. Stop awarding attribute points in Y3 level-up.
4. Leave old AttributeMenu available only for Aura legacy maps if needed.
5. Delete point-spending widgets after Y3 stats panel exists.

## Phase 4 - Replace Save Slots With Single Account Save

Reason: the final product needs account progression, not RPG save slots.

Current code references:

- `AAuraGameModeBase`
- `ULoadScreenSaveGame`
- `UMVVM_LoadScreen`
- `UMVVM_LoadSlot`
- `ALoadScreenHUD`
- `AAuraGameInstance::LoadSlotName`
- `AAuraGameInstance::LoadSlotIdx`
- `AAuraCharacter::SaveProgress`
- `AAuraCharacter::LoadProgress`

Desired Y3 replacement:

- `UY3AccountSaveGame`
- `UY3AccountSubsystem`
- `FY3RunResult`
- `FY3AccountHeroState`
- `FY3AccountSkillState`

MVP fields:

- `AccountLevel`
- `AccountXP`
- `Gold`

Deferred fields:

- hero stars
- hero equipment
- hero talents
- materials
- unlocked heroes
- unlocked skills
- meta upgrades
- best runs
- account statistics

Safe steps:

1. Add Y3 account save class without deleting old save code.
2. Main menu loads or creates one account save automatically.
3. Result screen writes account XP and gold rewards into account save.
4. Account level is recalculated or advanced from accumulated account XP.
5. Login / main menu only controls the current account and account switching.
6. Stage / selection screen displays account level, account XP, gold, stage unlocks, hero unlocks, and future account growth data.
7. Hero selection can keep using current demo data until unlocks are introduced.
8. Once Y3 maps no longer call Aura slot save APIs, retire old load-slot UI.

Recommended first implementation:

```text
UY3AccountSaveGame
- int32 SaveVersion
- FString Y3AccountId
- EY3AccountProvider ProviderType
- FString ProviderUserId
- int32 AccountLevel
- int32 AccountXP
- int32 Gold

UY3AccountSubsystem
- LoadOrCreate()
- LoadOrCreateDevProfile(FString DevProfileId)
- Save()
- AddAccountXP(int32 Amount)
- AddGold(int32 Amount)
- GetXPRequiredForLevel(int32 Level)
```

The account subsystem is the public API. Local `USaveGame` is only the first storage backend, so future Steam/Epic/backend integration does not require gameplay systems to be rewritten.

Current implementation files:

```text
Source/Aura_Learn/Public/Y3/Account/Y3AccountSaveGame.h
Source/Aura_Learn/Private/Y3/Account/Y3AccountSaveGame.cpp
Source/Aura_Learn/Public/Y3/Account/Y3AccountSubsystem.h
Source/Aura_Learn/Private/Y3/Account/Y3AccountSubsystem.cpp
```

## Phase 5 - Split AY3BattleGameMode

Reason: `AY3BattleGameMode` currently owns too much.

Move out:

- wave rules -> `UY3WaveDirector`
- skill choices -> `UY3UpgradeManager`
- run stats -> `UY3RunState`
- result data -> `FY3RunResult`
- HUD binding helpers -> widgets or a UI presenter

Keep in GameMode:

- map-level orchestration
- initial spawn
- connect selected hero to pawn class
- create/own the runtime modules

Exit criteria:

- `AY3BattleGameMode` reads like a coordinator, not the gameplay database.

## Phase 6 - Asset Deletion

Only delete after referencer checks are clean and PIE passes.

Likely delete candidates:

- old Aura SpellMenu widgets
- old Aura SpellMenu widget controller blueprint
- old LoadScreen widgets and viewmodels
- old Aura main menu
- old attribute point button rows
- unused old spell icons already replaced by `/Game/UI/SkillIcons`

Deletion checklist:

1. Run asset referencer scan through UE.
2. Remove C++ references or leave class compiled but unused.
3. Compile Blueprints.
4. Save all packages.
5. PIE main menu.
6. PIE battle map.
7. Commit.

## Recommended Immediate Next Task

Implement Phase 2 foundation:

- create `UY3UpgradeManager`
- define `FY3RunSkillState`
- move `ShowSkillChoice`, candidate generation, and selected-skill application out of `AY3BattleGameMode`
- keep old ASC equip function as a low-level service

This gives Y3 its own skill-growth spine before further UI or content work.
