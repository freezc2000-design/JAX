# Y3 Target Architecture

Date: 2026-06-09

## Product Direction

Y3 is no longer a GASAura course-project variant. The target product is a single-player, top-down survivor / light-roguelite action RPG built on the useful parts of GASAura:

- GAS combat, attributes, effects, cooldowns, tags, and projectiles stay as the gameplay foundation.
- The old RPG menu loop, spell-point learning, attribute-point spending, multi-slot save screen, and manual spell equip flow are legacy systems.
- The final loop is:

```text
Main Menu
-> Hero Select
-> Run Prepare
-> Battle / Wave Loop
-> Level Up Choice
-> Boss / Stage Clear
-> Result
-> Account Progress
```

The commercial goal is a clear, repeatable run-based game loop: choose a hero, survive and grow during a run, earn rewards, unlock account progression, and start the next run with better strategic options.

UI ownership:

- Login / main menu controls only the active account: continue current account, create a new account, or login to an existing account.
- Stage / selection screen displays account progression: account level, current XP, gold, stage unlocks, hero unlocks, and future materials, equipment, hero growth, and account upgrades.
- Battle runtime handles only the current run.
- Result screen applies run rewards, then returns to the stage / selection screen where account data refreshes.

## Current Reality

The project already has real Y3 direction in place:

- `AY3BattleGameMode`
- `AY3StageSpawner`
- `FY3StageRow`, `FY3TimerRow`, `FY3UnitRow`, `FY3UnitAttrRow`
- `DT_Stage`, `DT_SpawnTimer`, `DT_UnitType`, `DT_UnitAttr`
- `WBP_MainMenu`, `WBP_HeroSelection`, `WBP_SkillChoice`, `WBP_ResultScreen`, `WBP_SkillAtlas`
- `WBP_StageHUD`, `WBP_Y3BattleOverlay_Clean`, `BP_Y3HUD`
- `DT_SkillRegistry`, `DT_SkillTuning`
- `UY3AutoMissile`, `UY3ChainLightning`
- Y3 skill metadata extensions in `AbilityInfo`

The main architectural problem is not lack of content. It is that Y3 runtime logic is mixed with old Aura RPG systems and too much new runtime responsibility is concentrated inside `AY3BattleGameMode`.

## Target Runtime Modules

### 1. App / Mode Layer

Purpose: decide which screen or map is active.

Target responsibilities:

- Main menu routing
- Hero selection routing
- Battle map entry
- Result screen return routing

Current files:

- `/Game/Core/GameModes/BP_MainMenuGameMode`
- `/Game/Core/GameModes/BP_SelectionGameMode`
- `/Game/Core/GameModes/BP_BattleGameMode`
- `AY3BattleGameMode`

Target change:

- Keep GameModes thin.
- Move persistent account data out of GameMode.
- Move run-state data out of GameMode.

### 2. Run State Layer

Purpose: own one battle run.

Target owner:

- `UY3RunState` or `AY3RunStateActor`

Target data:

- selected hero
- current run level
- elapsed time
- current wave / stage
- acquired active skills
- acquired passive skills
- skill levels
- kills
- damage dealt
- damage taken
- boss state
- win / loss result

Current state:

- Most of this is implicitly inside `AY3BattleGameMode`.

Target change:

- Introduce an explicit run-state object before the project grows further.
- `AY3BattleGameMode` should orchestrate, not own every subsystem.

### 3. Battle Flow Layer

Purpose: stage, wave, boss, result.

Target owner:

- `UY3BattleFlowComponent` or `UY3WaveDirector`

Target responsibilities:

- start run
- start wave
- spawn normal / elite / boss enemies
- track stage transitions
- detect victory and defeat
- notify HUD

Current files:

- `AY3BattleGameMode`
- `AY3StageSpawner`

Target change:

- Keep `AY3StageSpawner` if useful.
- Move simple wave mode and boss timer rules out of `AY3BattleGameMode`.

### 4. Skill Runtime Layer

Purpose: one source of truth for Y3 run skills.

Target owner:

- `UY3SkillRuntimeComponent` or `UY3UpgradeManager`

Target responsibilities:

- build three-choice offerings
- apply picked skill
- track active/passive skill levels
- auto-equip active skills
- apply passive modifiers
- expose acquired skills to HUD / result / run stats

Current files:

- `AY3BattleGameMode::ShowSkillChoice`
- `AY3BattleGameMode::Y3_GiveAndEquip`
- `UAuraAbilitySystemComponent::Y3_EquipAbilityToSlot`
- `DT_SkillTuning`
- `DA_AbilityInfo`
- `DT_SkillRegistry`

Target change:

- Keep ASC ability granting and slot tags.
- Retire old `USpellMenuWgtController` for Y3.
- Replace spell-point spending with Y3 upgrade choices.

### 5. Hero Layer

Purpose: define player archetypes.

Target data:

- hero id
- display name
- portrait
- pawn class
- starting active skill
- starting passive / trait
- base stats
- unlock state

Target data source:

- `DT_HeroRegistry` or `DA_Y3HeroDefinition`

Current files:

- `WBP_HeroSelection`
- demo hero assets under `/Game/Demo/HeroSelect`
- selected hero passed through GameInstance property reflection in `AY3BattleGameMode`

Target change:

- Stop using loose reflected GameInstance fields as the long-term contract.
- Create typed Y3 selection/account data.

### 6. UI Layer

Purpose: user-facing screens only. UI should not own game rules.

Target folders:

- `/Game/UI/Common`
- `/Game/UI/HUD`
- `/Game/UI/HUD/Widgets`
- `/Game/UI/Menus`
- `/Game/UI/Panels/Character`
- `/Game/UI/Panels/RunStats`
- `/Game/UI/SkillIcons`
- `/Game/UI/Textures`

Keep / build:

- main menu
- hero selection
- battle HUD
- skill choice
- skill atlas
- result screen
- character stats panel
- run stats panel
- confirm / exit dialog

Retire from Y3 runtime:

- old Aura spell tree
- old Aura spell equip menu
- old Aura attribute-point spending panel
- old Aura load-slot UI

### 7. Account Save Layer

Purpose: persistent single-account progress through a platform-agnostic Y3 account layer.

Target owner:

- `UY3AccountSaveGame`
- optional `UY3AccountSubsystem`
- later storage providers for Steam/Epic/backend

MVP scope:

- account level
- account XP
- gold

Identity scope:

- `Y3AccountId`
- provider type: Local / Steam / Epic / Console / Y3Backend
- provider user id
- save version
- local/cloud sync metadata

Target data:

- unlocked heroes
- unlocked skills
- currency
- meta upgrades
- best runs
- settings
- tutorial flags

Current files:

- `ULoadScreenSaveGame`
- `AAuraGameModeBase`
- `UMVVM_LoadScreen`
- `UMVVM_LoadSlot`
- `AAuraGameInstance::LoadSlotName`
- `AAuraGameInstance::LoadSlotIdx`

Target change:

- Do not hard-delete old save code first.
- Build Y3 account save in parallel, starting with account level, account XP, and gold.
- Switch menu/runtime to this single local account save.
- Retire old load-slot UI and slot save calls after Y3 no longer depends on them.

Long-term local account progression can extend the same save object with:

- hero stars
- hero equipment
- hero talents
- meta-upgrade nodes
- materials
- best run records
- achievement flags

Steam integration should initially use the same account subsystem with a Steam provider id and local save plus Steam Cloud. A custom server/database is only needed later if the product requires trusted leaderboards, online economy, cross-platform accounts, seasons, or server-authoritative anti-cheat.

Detailed account design:

- `Tools/Y3_Account_Architecture.md`

## What Stays From GASAura

Keep:

- GAS ASC and AttributeSet foundation
- GameplayTags
- GameplayEffects
- damage execution calculation
- projectile actors
- cooldown and debuff model
- health / mana / XP bars until replaced
- floating damage text
- enemy base classes where useful
- Niagara/passive/debuff visuals where useful

Rewrite or isolate:

- manual spell-point learning
- manual spell equip menu
- attribute-point spending
- multi-slot save UI
- checkpoint/map persistence
- multiplayer assumptions where they complicate single-player work

## Near-Term Architecture Rule

Do not delete large old Aura systems by folder.

Use this order:

```text
identify references
-> introduce Y3 replacement
-> repoint Y3 runtime
-> PIE smoke test
-> mark old system deprecated
-> delete only after referencers are gone
```

This avoids Blueprint GUID reference failures and asset-chain breakage.
