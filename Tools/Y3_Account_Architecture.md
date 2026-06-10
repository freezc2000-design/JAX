# Y3 Account Architecture

Date: 2026-06-09

## Direction

Y3 should have its own platform-agnostic account layer.

Current implementation can be local-only, but the account model must not be hard-wired to "one local save file forever". It should be able to evolve into:

- Steam account
- Epic account
- console platform account
- custom Y3 account
- server-authoritative cloud account

The design rule:

```text
Account system first.
Storage backend second.
Platform provider third.
```

Local `USaveGame` is only the first storage backend.

## Conceptual Layers

```text
Platform Identity
-> Y3 Account Identity
-> Account Progress
-> Storage Backend
```

### 1. Platform Identity

External login provider.

Examples:

- Local developer profile
- SteamID
- Epic Account ID
- console platform user id
- custom Y3 backend user id

Target data:

```text
ProviderType: Local / Steam / Epic / Console / Y3Backend
ProviderUserId: provider-specific user id
DisplayName: optional
AuthTicket: optional runtime-only token
```

This data identifies who is playing, but does not directly define the save schema.

### 2. Y3 Account Identity

Y3's internal account key.

Target data:

```text
Y3AccountId
ProviderType
ProviderUserId
CreatedAt
LastLoginAt
SaveVersion
```

Rules:

- `Y3AccountId` is the stable internal key.
- Platform IDs are linked to it.
- Local-only development can synthesize a fake `Y3AccountId`.
- Future backend can return the official `Y3AccountId` after login.

### 3. Account Progress

Persistent game progression.

MVP:

```text
AccountLevel
AccountXP
Gold
```

Near future:

```text
Materials
UnlockedHeroes
UnlockedSkills
HeroProgress
MetaUpgrades
BestRuns
Settings
Achievements
Statistics
```

Long-term:

```text
HeroStars
HeroEquipment
HeroTalents
Inventory
SeasonProgress
Entitlements
MailRewards
```

### 4. Storage Backend

Where account progress is stored.

MVP:

```text
LocalSaveGameBackend
```

Future:

```text
SteamCloudSaveBackend
Y3BackendSaveBackend
HybridBackend
```

The game logic should talk to `UY3AccountSubsystem`, not directly to `UGameplayStatics::SaveGameToSlot`.

## MVP Implementation Shape

### UY3AccountSaveGame

Local serialized account snapshot.

Fields:

```text
int32 SaveVersion
FString Y3AccountId
EY3AccountProvider ProviderType
FString ProviderUserId
int64 CreatedAtUtc
int64 LastLoginAtUtc
int64 LastSavedAtUtc

int32 AccountLevel
int32 AccountXP
int32 Gold

bool bHasPendingCloudSync
int64 LastCloudSyncAtUtc
```

### UY3AccountSubsystem

Runtime account entry point.

Responsibilities:

```text
Initialize account identity
Load or create account save
Expose account progress to UI
Apply account rewards
Save local snapshot
Prepare future cloud sync boundary
```

First functions:

```text
LoadOrCreateAccount()
LoadOrCreateDevProfile(FString DevProfileId)
SaveAccount()
AddAccountXP(int32 Amount)
AddGold(int32 Amount)
GetXPRequiredForLevel(int32 Level)
GetAccountLevel()
GetGold()
```

### Development Profiles

Development needs multiple local test accounts, but the product can still behave as one account per platform user.

Dev save slot names:

```text
Y3Account_Local_Dev_Default
Y3Account_Local_Dev_New
Y3Account_Local_Dev_Rich
Y3Account_Local_Dev_HighLevel
```

Release save slot names:

```text
Y3Account_Steam_<SteamID>
Y3Account_Epic_<EpicAccountId>
Y3Account_Local_Default
```

Future backend account:

```text
Y3Account_Backend_<Y3AccountId>
```

## Future Server Model

When Y3 needs server-side account authority, the backend should own the canonical account progress.

Recommended backend concepts:

```text
PlayerAccount
PlayerWallet
PlayerHeroProgress
PlayerInventory
PlayerRunRecord
PlayerEntitlement
```

Server-authoritative fields:

```text
AccountLevel
AccountXP
Gold
PremiumCurrency
Materials
HeroStars
HeroTalents
HeroEquipment
Inventory
Entitlements
LeaderboardRecords
```

Client-local or sync-tolerant fields:

```text
Settings
GraphicsOptions
InputBindings
LocalTutorialFlags
CachedLastRunSummary
```

## Migration Path

### Phase 1 - Local Account

```text
UY3AccountSubsystem
-> UY3AccountSaveGame
-> local slot
```

Use this now.

### Phase 2 - Platform Identity

```text
Steam/Epic/local provider
-> provider id
-> Y3AccountId
-> local slot named by provider id
```

Still local-first. Steam Cloud can sync the local file.

### Phase 3 - Hybrid Cloud

```text
Login provider
-> Y3 backend
-> cloud account progress
-> local cached snapshot
```

Local save becomes cache/offline buffer.

### Phase 4 - Server-Authoritative Economy

```text
Run result
-> upload
-> server validation
-> server applies account rewards
-> client refreshes account snapshot
```

Use this only if Y3 needs trusted economy, leaderboards, seasons, online events, or anti-cheat.

## Important Boundary

Do not let gameplay systems call platform APIs directly.

Wrong:

```text
ResultScreen -> SteamID -> SaveGameToSlot
```

Right:

```text
ResultScreen
-> UY3AccountSubsystem::AddAccountXP / AddGold
-> account subsystem chooses storage backend
```

This keeps the project ready for Steam now and other platforms later.

## UI Ownership

Account UI is split by player intent.

### Login / Main Menu

Purpose:

```text
Who am I playing as?
```

Responsibilities:

```text
show current account id / display name
continue with current account
create a new local account
login to an existing local account
future platform login entry
```

Do not put progression management here.

The login / main menu should not become the long-term growth screen. It only controls account identity and high-level routing.

### Stage / Selection Screen

Purpose:

```text
What progress does this account have, and what can I play or upgrade?
```

Responsibilities:

```text
show account level
show current account XP
show gold
show stage unlock state
show hero unlock state
future materials
future equipment
future hero stars
future hero talents
future meta progression
enter battle
```

This screen is the account-progress hub.

The current MVP display should be:

```text
Account Level
Account XP / XP Required
Gold
```

Later, the same screen can expand into:

```text
Stage Select
Hero Select
Hero Growth
Inventory
Meta Upgrade
```

### Battle Runtime

Purpose:

```text
Play the current run.
```

Responsibilities:

```text
combat
wave state
skill choices
run-only stats
```

Battle should not continuously write account progression. It only writes account rewards at the run result boundary.

### Result Screen

Purpose:

```text
Convert a run result into account rewards.
```

Responsibilities:

```text
show win/loss
show reward XP
show reward gold
apply rewards to UY3AccountSubsystem
return to the selection screen
```

After returning to the stage / selection screen, account level, XP, gold, unlocks, and future growth data must refresh from `UY3AccountSubsystem`.
