// Y3 关卡战斗 GameMode 实现
#include "Y3/Y3BattleGameMode.h"
#include "Y3/Y3StageSpawner.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Game/AuraGameInstance.h"
#include "Character/AuraEnemy.h"
#include "Y3/Y3EnemyBase.h"
#include "Character/AuraCharacterBase.h"
#include "Interaction/CombatInterface.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DataTable.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemBPLibary.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Y3/UI/Y3SkillRegistry.h"
#include "Y3/Y3SkillTuning.h"
#include "Y3/Y3UpgradeChoice.h"
#include "AbilitySystem/Data/AttributeInfo.h"
#include "Player/AuraPlayerController.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "UI/WidgetController/AttributeMenuWgtController.h"
#include "Y3/Account/Y3AccountSubsystem.h"
#include "Framework/Application/SlateApplication.h"

// 技能中文名:查 DT_SkillRegistry 按 GA 类匹配,取 DisplayName 里空格后的中文段。
static FString Y3_SkillCnName(const FAuraAbilityInfo& I)
{
    // 单一真源：直接取 DA 的中文显示名
    if (!I.DisplayName.IsEmpty()) return I.DisplayName;
    // 兜底:tag 最后一段
    FString S = I.AbilityTag.ToString();
    int32 Dot; if (S.FindLastChar('.', Dot)) S = S.RightChop(Dot + 1);
    return S;
}
#include "Engine/Engine.h"

AY3BattleGameMode::AY3BattleGameMode()
{
}

void AY3BattleGameMode::BeginPlay()
{
    Super::BeginPlay();

    int32 LevelIdx = DebugForceLevel;
    if (LevelIdx < 0)
    {
        if (UAuraGameInstance* GI = GetGameInstance<UAuraGameInstance>())
        {
            FProperty* Prop = GI->GetClass()->FindPropertyByName(TEXT("Y3_SelectedLevelIndex"));
            if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
            {
                LevelIdx = IntProp->GetPropertyValue_InContainer(GI);
            }
        }
    }
    if (LevelIdx < 0) LevelIdx = 0;

    if (StageHUDClass)
    {
        StageHUDInstance = CreateWidget<UUserWidget>(GetWorld(), StageHUDClass);
        if (StageHUDInstance)
        {
            StageHUDInstance->AddToViewport(100);
            // 右侧图标栏：角色→属性面板 技能→图鉴 设置→退出确认；背包/强化未开发,按钮在WBP里禁用
            if (StageHUDInstance->WidgetTree)
            {
                if (UButton* B = Cast<UButton>(StageHUDInstance->WidgetTree->FindWidget(TEXT("Btn_Stats"))))
                    B->OnClicked.AddDynamic(this, &AY3BattleGameMode::OnStatsButtonClicked);
                if (UButton* B = Cast<UButton>(StageHUDInstance->WidgetTree->FindWidget(TEXT("Btn_Skill"))))
                    B->OnClicked.AddDynamic(this, &AY3BattleGameMode::OnSkillButtonClicked);
                if (UButton* B = Cast<UButton>(StageHUDInstance->WidgetTree->FindWidget(TEXT("Btn_Settings"))))
                    B->OnClicked.AddDynamic(this, &AY3BattleGameMode::OnSettingsButtonClicked);
            }
        }
    }

    if (bUseSimpleWaveMode)
    {
        // 简易三轨道模式：跳过 DT 驱动的 StartLevel，跑清晰的有限波次
        StartSimpleWaves();
    }
    else
    {
        StartLevel(LevelIdx);
        if (BossTimeLimit > 0.f)
        {
            GetWorld()->GetTimerManager().SetTimer(BossLimitTimer, this, &AY3BattleGameMode::OnBossTimeExpired, BossTimeLimit, false);
            UE_LOG(LogTemp, Log, TEXT("[Y3] Boss time limit started: %.1fs"), BossTimeLimit);
        }
    }

    // 引擎已通过 GetDefaultPawnClassForController 生成正确英雄(无需销毁/重生）。
    // 这里只绑定玩家死亡事件到当前 pawn。
    if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
    {
        if (ICombatInterface* CI = Cast<ICombatInterface>(PlayerPawn))
        {
            CI->GetOnDeathDel().AddDynamic(this, &AY3BattleGameMode::HandlePlayerDeath);
        }
        UE_LOG(LogTemp, Log, TEXT("[Y3] Player pawn ready: %s"), *PlayerPawn->GetClass()->GetName());
    }
}

UClass* AY3BattleGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
    if (UAuraGameInstance* GI = GetGameInstance<UAuraGameInstance>())
    {
        FProperty* Prop = GI->GetClass()->FindPropertyByName(TEXT("Y3_SelectedHeroClass"));
        if (FClassProperty* ClassProp = CastField<FClassProperty>(Prop))
        {
            UClass* HeroClass = Cast<UClass>(ClassProp->GetObjectPropertyValue_InContainer(GI));
            if (HeroClass && HeroClass->IsChildOf(APawn::StaticClass()))
            {
                UE_LOG(LogTemp, Log, TEXT("[Y3] DefaultPawnClass -> selected hero: %s"), *HeroClass->GetName());
                return HeroClass;
            }
        }
    }
    return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void AY3BattleGameMode::StartBattle(int32 LevelIdx)
{
    // 清理当前关卡状态
    ClearActiveSpawners();
    GetWorld()->GetTimerManager().ClearTimer(StageTickTimer);
    GetWorld()->GetTimerManager().ClearTimer(StageEndTimer);

    bLevelFinished = false;
    StartLevel(LevelIdx);
}

void AY3BattleGameMode::StartLevel(int32 LevelIdx)
{
    CurrentLevel = LevelIdx;
    LevelStages.Reset();
    if (!DT_Stage)
    {
        UE_LOG(LogTemp, Error, TEXT("[Y3] DT_Stage not set!"));
        return;
    }

    DT_Stage->ForeachRow<FY3StageRow>(TEXT("Y3 StartLevel"), [this, LevelIdx](const FName& Key, const FY3StageRow& Row)
    {
        if (Row.level == LevelIdx)
        {
            LevelStages.Add(Row);
        }
    });

    LevelStages.Sort([](const FY3StageRow& A, const FY3StageRow& B) { return A.stage < B.stage; });

    TotalStages = LevelStages.Num();
    UE_LOG(LogTemp, Log, TEXT("[Y3] Level %d loaded, %d stages"), LevelIdx, TotalStages);

    if (TotalStages == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[Y3] No stages for level %d"), LevelIdx);
        return;
    }

    StartStage(1);
}

void AY3BattleGameMode::StartStage(int32 StageIdx)
{
    ClearActiveSpawners();
    CurrentStageIdx = StageIdx;
    if (StageIdx > TotalStages)
    {
        bLevelFinished = true;
        CurrentStageInfo = FString::Printf(TEXT("关卡 %d 完成!"), CurrentLevel);
        OnStageInfoChanged.Broadcast(CurrentStageInfo);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage((uint64)this, 0.6f, FColor::Yellow, CurrentStageInfo);
    }
        UE_LOG(LogTemp, Log, TEXT("[Y3] Level %d finished"), CurrentLevel);
        return;
    }

    const FY3StageRow& Stage = LevelStages[StageIdx - 1];
    CurrentStageRemaining = Stage.stageTime;

    TArray<int32> TimerIDs = ParsePipeIntArray(Stage.timerID);
    FVector BaseCoord = ParseCoord(Stage.coord);

    if (!DT_Timer || !SpawnerClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[Y3] DT_Timer or SpawnerClass not set"));
    }
    else
    {
        for (int32 TimerID : TimerIDs)
        {
            FName Key = FName(*FString::FromInt(TimerID));
            FY3TimerRow* TimerRow = DT_Timer->FindRow<FY3TimerRow>(Key, TEXT("Y3 StartStage"));
            if (!TimerRow) continue;

            // 用 Deferred Spawn：先创建但不调用 BeginPlay，Init 注入数据后再 FinishSpawning
            FTransform SpawnTM(FRotator::ZeroRotator, BaseCoord);
            AY3StageSpawner* Spawner = GetWorld()->SpawnActorDeferred<AY3StageSpawner>(
                SpawnerClass, SpawnTM, this, nullptr,
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            if (Spawner)
            {
                Spawner->Init(this, *TimerRow, BaseCoord);
                Spawner->FinishSpawning(SpawnTM);
                ActiveSpawners.Add(Spawner);
            }
        }
    }

    if (Stage.stageBoss > 0)
    {
        SpawnY3Enemy(Stage.stageBoss, BaseCoord + FVector(0, 0, 100));
    }

    GetWorld()->GetTimerManager().SetTimer(StageTickTimer, this, &AY3BattleGameMode::OnStageTick, 0.5f, true);
    GetWorld()->GetTimerManager().SetTimer(StageEndTimer, this, &AY3BattleGameMode::EndCurrentStage, Stage.stageTime, false);

    CurrentStageInfo = FString::Printf(TEXT("关卡 %d · 阶段 %d/%d · 剩余 %ds"),
        CurrentLevel, StageIdx, TotalStages, FMath::FloorToInt(CurrentStageRemaining));
    OnStageInfoChanged.Broadcast(CurrentStageInfo);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage((uint64)this, 0.6f, FColor::Yellow, CurrentStageInfo);
    }

    UE_LOG(LogTemp, Log, TEXT("[Y3] Stage %d start, time=%.1f, timers=%d"), StageIdx, Stage.stageTime, TimerIDs.Num());
}

void AY3BattleGameMode::OnStageTick()
{
    if (bUseSimpleWaveMode)
    {
        CurrentStageInfo = GetWaveHUDText();
        OnStageInfoChanged.Broadcast(CurrentStageInfo);
        // 直接把波次文本推到 StageHUD 的 Txt_StageInfo，并折叠废弃面板
        if (StageHUDInstance && StageHUDInstance->WidgetTree)
        {
            if (UTextBlock* Info = Cast<UTextBlock>(StageHUDInstance->WidgetTree->FindWidget(TEXT("Txt_StageInfo"))))
                Info->SetText(FText::FromString(CurrentStageInfo));
            if (UWidget* BossPanel = StageHUDInstance->WidgetTree->FindWidget(TEXT("BossPanel")))
                BossPanel->SetVisibility(ESlateVisibility::Collapsed);
            if (UWidget* MobBar = StageHUDInstance->WidgetTree->FindWidget(TEXT("MobBar")))
                MobBar->SetVisibility(ESlateVisibility::Collapsed);
        }
        return;
    }

    CurrentStageRemaining = FMath::Max(0.f, CurrentStageRemaining - 0.5f);
    CurrentStageInfo = FString::Printf(TEXT("关卡 %d · 阶段 %d/%d · 剩余 %ds"),
        CurrentLevel, CurrentStageIdx, TotalStages, FMath::FloorToInt(CurrentStageRemaining));
    OnStageInfoChanged.Broadcast(CurrentStageInfo);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage((uint64)this, 0.6f, FColor::Yellow, CurrentStageInfo);
    }
}

void AY3BattleGameMode::EndCurrentStage()
{
    GetWorld()->GetTimerManager().ClearTimer(StageTickTimer);
    StartStage(CurrentStageIdx + 1);
}

void AY3BattleGameMode::ClearActiveSpawners()
{
    for (AY3StageSpawner* S : ActiveSpawners)
    {
        if (S) S->Destroy();
    }
    ActiveSpawners.Reset();
}

TSubclassOf<AAuraEnemy> AY3BattleGameMode::ResolveEnemyBPFromUnitID(int32 UnitID)
{
    // 仅返回 fallback（兼容老 API）
    if (!DT_Unit) return nullptr;
    FName Key = FName(*FString::FromInt(UnitID));
    FY3UnitRow* Row = DT_Unit->FindRow<FY3UnitRow>(Key, TEXT("Y3 Resolve"));
    if (!Row) return MobBP_Melee;
    if (Row->unitTag.Contains(TEXT("boss"))) return BossBP ? BossBP : MobBP_Melee;
    if (Row->atkType.ToLower().Contains(TEXT("ranged"))) return MobBP_Ranged ? MobBP_Ranged : MobBP_Melee;
    return MobBP_Melee;
}

UClass* AY3BattleGameMode::ResolveEnemyBPFromUnitIDAsClass(int32 UnitID)
{
    // [Y3demo] 131 个 BP_Mob_* 空壳已删除。现统一使用原版 Aura 怪
    // (BP_Goblin_Spear=近战 / BP_Goblin_Slingshot=远程 / BP_DemonR=boss),
    // 由 DT_UnitType 的 atkType / unitTag 决定映射到哪一个。
    // 未来若新增真实怪物 BP,在此按 unitID 增加专属解析即可。
    TSubclassOf<AAuraEnemy> Fallback = ResolveEnemyBPFromUnitID(UnitID);
    return Fallback ? Fallback.Get() : nullptr;
}

AActor* AY3BattleGameMode::SpawnY3Enemy(int32 UnitID, const FVector& Location)
{
    UClass* BP = ResolveEnemyBPFromUnitIDAsClass(UnitID);
    if (!BP)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Y3] SpawnY3Enemy: no BP for unitID=%d"), UnitID);
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    AActor* Spawned = GetWorld()->SpawnActor<AActor>(BP, Location, FRotator::ZeroRotator, Params);
    if (!Spawned)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Y3] SpawnY3Enemy: SpawnActor failed unitID=%d loc=%s class=%s"),
            UnitID, *Location.ToString(), *BP->GetName());
        return nullptr;
    }

    // 注入 Y3 属性（如果是 AY3EnemyBase）
    if (AY3EnemyBase* Y3E = Cast<AY3EnemyBase>(Spawned))
    {
        Y3E->Y3_UnitID = UnitID;
        if (DT_Unit)
        {
            FName UKey = FName(*FString::FromInt(UnitID));
            if (FY3UnitRow* URow = DT_Unit->FindRow<FY3UnitRow>(UKey, TEXT("Y3 SpawnInject")))
            {
                Y3E->Y3_Name = URow->name;
                Y3E->Y3_AtkType = URow->atkType;
                Y3E->Y3_UnitTag = URow->unitTag;
                Y3E->Y3_AttrID = URow->attrID;

                if (DT_Attr)
                {
                    FName AKey = FName(*FString::FromInt(URow->attrID));
                    if (FY3AttrRow* ARow = DT_Attr->FindRow<FY3AttrRow>(AKey, TEXT("Y3 AttrInject")))
                    {
                        Y3E->MaxHP = ARow->hp;
                        Y3E->CurrentHP = ARow->hp;
                        Y3E->ATK = ARow->atk;
                        Y3E->DEF = ARow->def;
                        Y3E->MoveSpeed = ARow->moveSpeed;
                    }
                }
            }
        }
    }

    return Spawned;
}

TArray<int32> AY3BattleGameMode::ParsePipeIntArray(const FString& s)
{
    TArray<int32> Out;
    if (s.IsEmpty()) return Out;
    TArray<FString> Parts;
    s.ParseIntoArray(Parts, TEXT("|"), true);
    for (const FString& P : Parts)
    {
        Out.Add(FCString::Atoi(*P));
    }
    return Out;
}

FVector AY3BattleGameMode::ParseCoord(const FString& Coord)
{
    if (Coord.IsEmpty()) return FVector::ZeroVector;
    FString X, Y;
    if (Coord.Split(TEXT("_"), &X, &Y))
    {
        return FVector(FCString::Atof(*X), FCString::Atof(*Y), 100.f);
    }
    return FVector::ZeroVector;
}

// ---- HUD 数据接口 ----

float AY3BattleGameMode::GetSecondsToNextBoss() const
{
    // 从当前阶段往后扫描，找到第一个 stageBoss > 0 的阶段
    if (LevelStages.Num() == 0) return -1.f;

    float Acc = CurrentStageRemaining;
    // 当前阶段：如果 stageBoss > 0，直接返回剩余
    int32 StartIdx = FMath::Clamp(CurrentStageIdx - 1, 0, LevelStages.Num() - 1);
    if (StartIdx < LevelStages.Num() && LevelStages[StartIdx].stageBoss > 0)
    {
        return Acc;
    }
    // 后续阶段：累加 stageTime
    for (int32 i = StartIdx + 1; i < LevelStages.Num(); ++i)
    {
        Acc += LevelStages[i].stageTime;
        if (LevelStages[i].stageBoss > 0)
        {
            return Acc;
        }
    }
    return -1.f; // 之后没有 Boss 阶段
}

FString AY3BattleGameMode::FormatSecondsAsMMSS(float Seconds)
{
    if (Seconds < 0.f) return TEXT("--:--");
    int32 Total = FMath::FloorToInt(Seconds);
    int32 M = Total / 60;
    int32 S = Total % 60;
    return FString::Printf(TEXT("%02d:%02d"), M, S);
}

TArray<FY3HUDMobEntry> AY3BattleGameMode::GetHUDMobEntries(int32 MaxCount) const
{
    TArray<FY3HUDMobEntry> Entries;
    TSet<int32> SeenUnitIDs;

    for (AY3StageSpawner* S : ActiveSpawners)
    {
        if (!S) continue;
        const int32 UID = S->GetUnitID();
        if (SeenUnitIDs.Contains(UID)) continue;
        SeenUnitIDs.Add(UID);

        FY3HUDMobEntry Entry;
        Entry.UnitID = UID;
        Entry.GroupSize = S->GetCurrentGroupSize();
        Entry.NextSpawnInSec = S->GetSecondsToNextSpawn();

        if (DT_Unit)
        {
            FName Key = FName(*FString::FromInt(UID));
            if (FY3UnitRow* URow = DT_Unit->FindRow<FY3UnitRow>(Key, TEXT("Y3 HUD")))
            {
                Entry.Name = URow->name;
                Entry.bIsBoss = URow->unitTag.ToLower().Contains(TEXT("boss")) || URow->unitType >= 2;
            }
        }

        Entries.Add(Entry);
        if (Entries.Num() >= MaxCount) break;
    }

    return Entries;
}

void AY3BattleGameMode::HandlePlayerDeath(AActor* DeadActor)
{
    UE_LOG(LogTemp, Warning, TEXT("[Y3] Player died -> show result (loss)"));
    if (bResultShown) return;
    const int32 StageReached = CurrentStageIdx + 1;
    FTimerDelegate D = FTimerDelegate::CreateLambda([this, StageReached]()
    {
        ShowResult(false, StageReached);
    });
    GetWorld()->GetTimerManager().SetTimer(ResultDelayTimer, D, FMath::Max(0.1f, ResultShowDelay), false);
}

void AY3BattleGameMode::OnBossTimeExpired()
{
    UE_LOG(LogTemp, Warning, TEXT("[Y3] Boss time expired -> show result (loss)"));
    if (bResultShown) return;
    ShowResult(false, CurrentStageIdx + 1);
}

void AY3BattleGameMode::ShowResult(bool bWin, int32 StageReached)
{
    if (bResultShown) return;
    bResultShown = true;

    const int32 AccountXPReward = bWin ? WinAccountXPReward : LossAccountXPReward;
    const int32 GoldReward = bWin ? WinGoldReward : LossGoldReward;
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UY3AccountSubsystem* Account = GI->GetSubsystem<UY3AccountSubsystem>())
        {
            Account->AddRunReward(AccountXPReward, GoldReward);
            Account->PrintCurrentAccount();
        }
    }

    // 停止所有计时器与刷怪
    GetWorld()->GetTimerManager().ClearTimer(StageTickTimer);
    GetWorld()->GetTimerManager().ClearTimer(StageEndTimer);
    GetWorld()->GetTimerManager().ClearTimer(BossLimitTimer);
    GetWorld()->GetTimerManager().ClearTimer(NormalWaveTimer);
    GetWorld()->GetTimerManager().ClearTimer(EliteWaveTimer);
    GetWorld()->GetTimerManager().ClearTimer(BossSpawnTimer);
    GetWorld()->GetTimerManager().ClearTimer(BossKillTimer);
    GetWorld()->GetTimerManager().ClearTimer(HudTickTimer);
    ClearActiveSpawners();

    // 移除 StageHUD
    if (StageHUDInstance)
    {
        StageHUDInstance->RemoveFromParent();
        StageHUDInstance = nullptr;
    }

    if (!ResultScreenClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[Y3] ResultScreenClass not set on GameMode!"));
        return;
    }

    ResultScreenInstance = CreateWidget<UUserWidget>(GetWorld(), ResultScreenClass);
    if (ResultScreenInstance)
    {
        ResultScreenInstance->AddToViewport(1000);

        // 模板复制自 WBP_MainMenu:TitleText / Btn_StartGame(返回选关)/ Btn_QuitGame(退出)
        if (UWidgetTree* WT = ResultScreenInstance->WidgetTree)
        {
            if (UTextBlock* Title = Cast<UTextBlock>(WT->FindWidget(TEXT("TitleText"))))
            {
                Title->SetText(FText::FromString(bWin
                    ? FString::Printf(TEXT("胜利!  (Stage %d)\n奖励: 账号经验 +%d  金币 +%d"), StageReached, AccountXPReward, GoldReward)
                    : FString::Printf(TEXT("失败  (Stage %d)\n奖励: 账号经验 +%d  金币 +%d"), StageReached, AccountXPReward, GoldReward)));
            }
            // 切换背景图(胜利/失败各一张)
            if (UImage* Bg = Cast<UImage>(WT->FindWidget(TEXT("BG_Image"))))
            {
                UTexture2D* Tex = bWin ? ResultBgWin : ResultBgLose;
                if (Tex) Bg->SetBrushFromTexture(Tex);
            }
            // "返回选关" 按钮 → 解除暂停并回选关
            if (UButton* BtnReturn = Cast<UButton>(WT->FindWidget(TEXT("Btn_StartGame"))))
            {
                BtnReturn->OnClicked.AddDynamic(this, &AY3BattleGameMode::OnConfirmResult);
            }
        }

        // 让结算界面可获得焦点(否则暂停下按钮可能收不到点击）
        ResultScreenInstance->SetIsFocusable(true);

        // 锁死角色操作 + UI 独占输入 + 显示鼠标
        if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
        {
            // 1) 彻底禁用玩家 pawn / controller 输入
            if (APawn* P = PC->GetPawn())
            {
                P->DisableInput(PC);
            }
            PC->DisableInput(PC);
            PC->SetIgnoreMoveInput(true);
            PC->SetIgnoreLookInput(true);

            // 2) UI 独占输入 + 鼠标可见(只有结算界面能交互)
            PC->bShowMouseCursor = true;
            FInputModeUIOnly Mode;
            Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PC->SetInputMode(Mode);
        }

        // 3) 暂停整个游戏(角色彻底无法操作；UMG 按钮在暂停下仍可点击)
        UGameplayStatics::SetGamePaused(this, true);
    }

    UE_LOG(LogTemp, Log, TEXT("[Y3] ShowResult bWin=%d stage=%d (paused, UI-only)"), bWin ? 1 : 0, StageReached);
}

void AY3BattleGameMode::OnConfirmResult()
{
    UE_LOG(LogTemp, Log, TEXT("[Y3] Confirm result -> OpenLevel %s"), *ReturnLevelName.ToString());
    UGameplayStatics::SetGamePaused(this, false);
    UGameplayStatics::OpenLevel(this, ReturnLevelName);
}

// ===================== 简易三轨道刷怪 =====================

void AY3BattleGameMode::StartSimpleWaves()
{
    NormalWaveIdx = 0;
    EliteWaveIdx = 0;
    bBossSpawned = false;

    UE_LOG(LogTemp, Log, TEXT("[Y3 Wave] SimpleWave start: Normal %d波/%.0fs, Elite %d波/%.0fs, Boss @%.0fs kill %.0fs"),
        NormalWaveCount, NormalInterval, EliteWaveCount, EliteInterval, BossSpawnDelay, BossKillTimeLimit);

    // 普通轨道：开局缓冲 FirstWaveDelay 后第一波,之后周期(InFirstDelay 让首波也延迟,不再立即刷)
    const float FirstDelay = FMath::Max(0.1f, FirstWaveDelay);
    if (NormalWaveCount > 0)
        GetWorld()->GetTimerManager().SetTimer(NormalWaveTimer, this, &AY3BattleGameMode::NormalWaveTick, NormalInterval, NormalWaveCount > 1, FirstDelay);

    // 精英轨道：比普通再晚一个缓冲出现(避免开局普通+精英同时围殴)
    if (EliteWaveCount > 0)
        GetWorld()->GetTimerManager().SetTimer(EliteWaveTimer, this, &AY3BattleGameMode::EliteWaveTick, EliteInterval, EliteWaveCount > 1, FirstDelay * 2.f);

    // Boss 轨道：延迟 BossSpawnDelay 后出现
    GetWorld()->GetTimerManager().SetTimer(BossSpawnTimer, this, &AY3BattleGameMode::SpawnBossWave, FMath::Max(0.1f, BossSpawnDelay), false);

    // HUD 刷新
    GetWorld()->GetTimerManager().SetTimer(HudTickTimer, this, &AY3BattleGameMode::OnStageTick, 0.5f, true);
}

void AY3BattleGameMode::NormalWaveTick()
{
    if (bResultShown) return;
    NormalWaveIdx++;
    UE_LOG(LogTemp, Log, TEXT("[Y3 Wave] 普通 %d/%d"), NormalWaveIdx, NormalWaveCount);
    if (NormalEnemyPool.Num() > 0) SpawnWaveFromPool(NormalEnemyPool, NormalPerWave, NormalScale);
    else SpawnWaveAroundPlayer(NormalUnitID, NormalPerWave, NormalScale);
    if (NormalWaveIdx >= NormalWaveCount)
        GetWorld()->GetTimerManager().ClearTimer(NormalWaveTimer);
}

void AY3BattleGameMode::EliteWaveTick()
{
    if (bResultShown) return;
    EliteWaveIdx++;
    UE_LOG(LogTemp, Log, TEXT("[Y3 Wave] 精英 %d/%d"), EliteWaveIdx, EliteWaveCount);
    if (EliteEnemyPool.Num() > 0) SpawnWaveFromPool(EliteEnemyPool, ElitePerWave, EliteScale);
    else SpawnWaveAroundPlayer(EliteUnitID, ElitePerWave, EliteScale);
    if (EliteWaveIdx >= EliteWaveCount)
        GetWorld()->GetTimerManager().ClearTimer(EliteWaveTimer);
}

void AY3BattleGameMode::SpawnBossWave()
{
    if (bResultShown) return;
    bBossSpawned = true;
    UE_LOG(LogTemp, Warning, TEXT("[Y3 Wave] BOSS 出现! 击杀倒计时 %.0fs"), BossKillTimeLimit);

    // 在玩家前方生成 boss
    FVector BossLoc = FVector::ZeroVector;
    if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
        BossLoc = P->GetActorLocation() + P->GetActorForwardVector() * 800.f;
    AActor* Boss = BossEnemyClass ? SpawnEnemyAt(BossEnemyClass.Get(), BossLoc)
                                  : SpawnY3Enemy(SimpleBossUnitID, BossLoc);

    if (Boss)
    {
        ApplyEnemyScale(Boss, BossScale);
        if (ICombatInterface* CI = Cast<ICombatInterface>(Boss))
        {
            CI->GetOnDeathDel().AddDynamic(this, &AY3BattleGameMode::HandleBossDeath);
        }
    }

    // 击杀倒计时：超时 = 失败
    GetWorld()->GetTimerManager().SetTimer(BossKillTimer, this, &AY3BattleGameMode::OnBossKillTimeExpired, FMath::Max(1.f, BossKillTimeLimit), false);
}

void AY3BattleGameMode::HandleBossDeath(AActor* DeadActor)
{
    if (bResultShown) return;
    GetWorld()->GetTimerManager().ClearTimer(BossKillTimer);
    UE_LOG(LogTemp, Warning, TEXT("[Y3 Wave] Boss 被击杀 -> 通关胜利"));
    ShowResult(true, NormalWaveCount);
}

void AY3BattleGameMode::OnBossKillTimeExpired()
{
    if (bResultShown) return;
    UE_LOG(LogTemp, Warning, TEXT("[Y3 Wave] Boss 击杀超时 -> 关卡失败"));
    ShowResult(false, NormalWaveCount);
}

AActor* AY3BattleGameMode::SpawnEnemyAt(UClass* Cls, const FVector& Loc)
{
    if (!Cls || !GetWorld()) return nullptr;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    return GetWorld()->SpawnActor<AActor>(Cls, Loc, FRotator::ZeroRotator, Params);
}

void AY3BattleGameMode::SpawnWaveFromPool(const TArray<TSubclassOf<AActor>>& Pool, int32 Count, float Scale)
{
    if (Pool.Num() == 0) return;
    FVector Center = FVector::ZeroVector;
    if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
        Center = P->GetActorLocation();

    const int32 Safe = FMath::Clamp(Count, 1, 12);
    for (int32 i = 0; i < Safe; ++i)
    {
        // 每只随机从池里挑一种
        TSubclassOf<AActor> Pick = Pool[FMath::RandRange(0, Pool.Num() - 1)];
        if (!Pick) continue;
        const float Angle = FMath::FRandRange(0.f, 2.f * PI);
        const float Dist = FMath::FRandRange(WaveSpawnRadius * 0.6f, WaveSpawnRadius);
        FVector Loc = Center + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
        if (AActor* Spawned = SpawnEnemyAt(Pick.Get(), Loc))
            ApplyEnemyScale(Spawned, Scale);
    }
}

void AY3BattleGameMode::ApplyEnemyScale(AActor* Enemy, float Scale)
{
    // 体型差异化已改为"烤进BP网格组件"(可靠渲染),运行时缩放不再使用,
    // 仅当显式传入非1缩放时才生效(默认参数都=1,即不动BP烤好的尺寸)。
    if (!Enemy || FMath::IsNearlyEqual(Scale, 1.0f)) return;
    Enemy->SetActorScale3D(FVector(Scale));
}

void AY3BattleGameMode::SpawnWaveAroundPlayer(int32 UnitID, int32 Count, float Scale)
{
    FVector Center = FVector::ZeroVector;
    if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
        Center = P->GetActorLocation();

    const int32 Safe = FMath::Clamp(Count, 1, 12);
    for (int32 i = 0; i < Safe; ++i)
    {
        const float Angle = FMath::FRandRange(0.f, 2.f * PI);
        const float Dist = FMath::FRandRange(WaveSpawnRadius * 0.6f, WaveSpawnRadius);
        FVector Loc = Center + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
        if (AActor* Spawned = SpawnY3Enemy(UnitID, Loc))
        {
            ApplyEnemyScale(Spawned, Scale);
        }
    }
}

FString AY3BattleGameMode::GetWaveHUDText() const
{
    FTimerManager& TM = GetWorld()->GetTimerManager();
    FString NormalStr, EliteStr, BossStr;

    if (NormalWaveIdx >= NormalWaveCount)
        NormalStr = FString::Printf(TEXT("普通 %d/%d 完成"), NormalWaveCount, NormalWaveCount);
    else
    {
        float nxt = TM.IsTimerActive(NormalWaveTimer) ? TM.GetTimerRemaining(NormalWaveTimer) : 0.f;
        NormalStr = FString::Printf(TEXT("普通 %d/%d · 下一波 %.0fs"), NormalWaveIdx, NormalWaveCount, nxt);
    }

    if (EliteWaveIdx >= EliteWaveCount)
        EliteStr = FString::Printf(TEXT("精英 %d/%d 完成"), EliteWaveCount, EliteWaveCount);
    else
    {
        float nxt = TM.IsTimerActive(EliteWaveTimer) ? TM.GetTimerRemaining(EliteWaveTimer) : 0.f;
        EliteStr = FString::Printf(TEXT("精英 %d/%d · 下一波 %.0fs"), EliteWaveIdx, EliteWaveCount, nxt);
    }

    if (!bBossSpawned)
    {
        float bd = TM.IsTimerActive(BossSpawnTimer) ? TM.GetTimerRemaining(BossSpawnTimer) : 0.f;
        BossStr = FString::Printf(TEXT("Boss 1/1 · %.0fs 后出现"), bd);
    }
    else if (TM.IsTimerActive(BossKillTimer))
    {
        BossStr = FString::Printf(TEXT("⚠ 击杀 Boss! 剩 %.0fs"), TM.GetTimerRemaining(BossKillTimer));
    }
    else
        BossStr = TEXT("Boss 已击杀");

    return NormalStr + TEXT("\n") + EliteStr + TEXT("\n") + BossStr;
}

// ===================== 升级三选一 =====================

static UAuraAbilitySystemComponent* Y3_GetPlayerASC(UObject* Ctx)
{
    if (APawn* P = UGameplayStatics::GetPlayerPawn(Ctx, 0))
        return Cast<UAuraAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(P));
    return nullptr;
}

FGameplayTag AY3BattleGameMode::Y3_FindFreeInputSlot() const
{
    const FAuraGmaeplayTags& T = FAuraGmaeplayTags::GetInstance();
    // 槽1=初始技能(火球),3选一升级技能装到 2~6 这 5 个槽(第7槽已砍,共6主动技能)。
    const FGameplayTag Slots[5] = { T.InputTag_2, T.InputTag_3, T.InputTag_4, T.InputTag_5, T.InputTag_6 };
    if (UAuraAbilitySystemComponent* ASC = Y3_GetPlayerASC(const_cast<AY3BattleGameMode*>(this)))
    {
        for (const FGameplayTag& S : Slots)
            if (ASC->SlotIsEmpty(S)) return S;
    }
    return FGameplayTag();
}

FGameplayTag AY3BattleGameMode::Y3_FindFreePassiveSlot() const
{
    const FAuraGmaeplayTags& T = FAuraGmaeplayTags::GetInstance();
    const FGameplayTag Slots[6] = { T.InputTag_Passive_1, T.InputTag_Passive_2, T.InputTag_Passive_3,
                                    T.InputTag_Passive_4, T.InputTag_Passive_5, T.InputTag_Passive_6 };
    if (UAuraAbilitySystemComponent* ASC = Y3_GetPlayerASC(const_cast<AY3BattleGameMode*>(this)))
    {
        for (const FGameplayTag& S : Slots)
            if (ASC->SlotIsEmpty(S)) return S;
    }
    return FGameplayTag();
}

int32 AY3BattleGameMode::Y3_GetSkillStock(const FGameplayTag& AbilityTag) const
{
    if (!SkillTuningTable) return 0;
    TArray<FY3SkillTuningRow*> Rows;
    SkillTuningTable->GetAllRows<FY3SkillTuningRow>(TEXT("Y3 Stock"), Rows);
    for (const FY3SkillTuningRow* R : Rows)
        if (R && R->AbilityTag.MatchesTagExact(AbilityTag)) return R->StockCount;
    return 0;
}

void AY3BattleGameMode::Y3_GiveAndEquip(FGameplayTag AbilityTag)
{
    UAuraAbilitySystemComponent* ASC = Y3_GetPlayerASC(this);
    if (!ASC || !AbilityTag.IsValid()) return;

    const FAuraGmaeplayTags& GTags = FAuraGmaeplayTags::GetInstance();
    FGameplayAbilitySpec* ExistingSpec = ASC->GetSpecFromAbilityTag(AbilityTag);
    if (!ExistingSpec)
    {
        // 没给过 → 新Give 一个,状态=Unlocked
        UAbilityInfo* Info = UAuraAbilitySystemBPLibary::GetAbilityInfo(this);
        if (!Info) return;
        FAuraAbilityInfo I = Info->FindAbilityInfoForTag(AbilityTag);
        if (!I.Ability)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Y3 Skill] no GA for %s"), *AbilityTag.ToString());
            return;
        }
        FGameplayAbilitySpec Spec(I.Ability, 1);
        Spec.DynamicAbilityTags.AddTag(GTags.Abilities_Status_Unlocked);
        ASC->GiveAbility(Spec);
    }
    else
    {
        const FGameplayTag CurStatus = ASC->GetStatusFromSpec(*ExistingSpec);
        const bool bAlreadyOwned =
            CurStatus.MatchesTagExact(GTags.Abilities_Status_Unlocked) ||
            CurStatus.MatchesTagExact(GTags.Abilities_Status_Equipped);
        if (bAlreadyOwned)
        {
            // 不放回抽卡:已拥有再抽到 = 升级(候选池已保证未满级)。Level++,不重新装槽(技能已在栏里)。
            // 主动技能下次施放自动读新等级;被动效果的实时刷新(GE 重应用)留作后续。
            ExistingSpec->Level += 1;
            ASC->MarkAbilitySpecDirty(*ExistingSpec);
            UE_LOG(LogTemp, Log, TEXT("[Y3 Skill] %s 升级 -> Lv%d"), *AbilityTag.ToString(), ExistingSpec->Level);
            ASC->AbilitiesGiveDel.Broadcast(); // 重播 AbilityInfo,刷新技能栏等级角标
            return;
        }
        // 原RPG预给的 Locked/Eligible → 升到 Unlocked(首次获得,继续走下面装槽)
        if (CurStatus.IsValid()) ExistingSpec->DynamicAbilityTags.RemoveTag(CurStatus);
        ExistingSpec->DynamicAbilityTags.AddTag(GTags.Abilities_Status_Unlocked);
        ASC->MarkAbilitySpecDirty(*ExistingSpec);
    }

    // 判断主动/被动:被动装到 Passive.1-6 槽并自动激活;主动装到 InputTag.2-7 槽。
    bool bPassive = AbilityTag.ToString().Contains(TEXT("Passive"));
    if (UAbilityInfo* Info2 = UAuraAbilitySystemBPLibary::GetAbilityInfo(this))
    {
        const FAuraAbilityInfo AI = Info2->FindAbilityInfoForTag(AbilityTag);
        if (AI.AbilityType.MatchesTagExact(GTags.Abilities_Type_Passive)) bPassive = true;
    }

    const FGameplayTag Slot = bPassive ? Y3_FindFreePassiveSlot() : Y3_FindFreeInputSlot();
    if (Slot.IsValid())
    {
        ASC->Y3_EquipAbilityToSlot(AbilityTag, Slot);
        UE_LOG(LogTemp, Log, TEXT("[Y3 Skill] %s -> slot %s (%s)"),
            *AbilityTag.ToString(), *Slot.ToString(), bPassive ? TEXT("被动") : TEXT("主动"));
        // 被动的激活由 ServerEquipAbility 内部负责(首次装槽 !AbilityHasAnySlot 时 TryActivateAbility)。
        // 这里【不要】再激活一次——重复激活会 reset 掉 InstancedPerActor 被动的定时器,
        // 正是自动追踪导弹走三选一后不开火的根因(Y3TestGive 不装槽只激活一次,所以能开火)。
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Y3 Skill] 无空槽,%s 已获得但未装备"), *AbilityTag.ToString());
    }
    // 首次获得/装备后重播 AbilityInfo,刷新技能栏等级角标(初始 Lv1)
    ASC->AbilitiesGiveDel.Broadcast();
}

void AY3BattleGameMode::ShowSkillChoice()
{
    if (!SkillChoiceClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[Y3 Skill] SkillChoiceClass 未设置!"));
        return;
    }
    UAuraAbilitySystemComponent* ASC = Y3_GetPlayerASC(this);
    UAbilityInfo* Info = UAuraAbilitySystemBPLibary::GetAbilityInfo(this);
    if (!ASC || !Info) return;

    // 候选池:有GA + 有图标(排除 ListenForEvent 等无图标的系统级被动) + 状态未被选。
    // 被动技能(Halo/虹吸/AutoMissile)也纳入,保证三选一有足够候选。
    const FAuraGmaeplayTags& GTags = FAuraGmaeplayTags::GetInstance();
    TArray<FAuraAbilityInfo> Pool;
    for (const FAuraAbilityInfo& I : Info->AbilityInfomation)
    {
        if (!I.Ability || !I.AbilityTag.IsValid()) continue;
        if (!I.AbilityIcon) continue;
        // 不放回抽卡:已拥有的技能,只有"已升级次数 < 库存"才继续进池(可升级);满级移出。
        // 未拥有的技能(Spec 为空)首次总能进池。封顶等级 = 1 + 库存。
        if (FGameplayAbilitySpec* Spec = ASC->GetSpecFromAbilityTag(I.AbilityTag))
        {
            const int32 Stock = Y3_GetSkillStock(I.AbilityTag);
            const int32 UpgradesDone = FMath::Max(0, Spec->Level - 1);
            if (UpgradesDone >= Stock) continue; // 满级(含 Stock=0:抽到即定级,不再出现)
        }
        Pool.Add(I);
    }
    // 洗牌
    for (int32 i = Pool.Num() - 1; i > 0; --i) { const int32 j = FMath::RandRange(0, i); Pool.Swap(i, j); }
    const int32 SkillN = FMath::Min(3, Pool.Num());

    // 技能池不足3张 → 用兜底升级卡(基础属性/金币)补位;技能全满级后三张全是兜底卡。
    const TArray<FName> UpgradeRows = Y3_PickUpgradeRows(3 - SkillN);
    const int32 N = SkillN + UpgradeRows.Num();
    if (N == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[Y3 Skill] 技能全满级且未配置 UpgradeChoiceTable,无可提供选项"));
        return;
    }

    SkillChoiceInstance = CreateWidget<UUserWidget>(GetWorld(), SkillChoiceClass);
    if (!SkillChoiceInstance) return;
    SkillChoiceInstance->AddToViewport(1100);
    SkillChoiceInstance->SetIsFocusable(true);

    CurrentChoices.Reset();
    UWidgetTree* WT = SkillChoiceInstance->WidgetTree;
    const TCHAR* BgN[3]   = { TEXT("Bg1"), TEXT("Bg2"), TEXT("Bg3") };
    const TCHAR* IconN[3] = { TEXT("Icon1"), TEXT("Icon2"), TEXT("Icon3") };
    const TCHAR* NameN[3] = { TEXT("Name1"), TEXT("Name2"), TEXT("Name3") };
    const TCHAR* BtnN[3]  = { TEXT("Btn_Card1"), TEXT("Btn_Card2"), TEXT("Btn_Card3") };

    for (int32 i = 0; i < 3; ++i)
    {
        if (i < SkillN)
        {
            // —— 技能卡 ——
            const FAuraAbilityInfo& I = Pool[i];
            CurrentChoices.Add({ I.AbilityTag, NAME_None });
            if (WT)
            {
                // 两层普通 Image 合成与技能栏一致的彩色技能图标：
                // BgN = 彩色球底材质(BackgroundMaterial)，IconN = 图标glyph(AbilityIcon)叠在上层。
                if (UImage* Bg = Cast<UImage>(WT->FindWidget(BgN[i])))
                {
                    if (I.BackgroundMaterial) Bg->SetBrushFromMaterial(I.BackgroundMaterial);
                }
                if (UImage* Ic = Cast<UImage>(WT->FindWidget(IconN[i])))
                {
                    if (I.AbilityIcon) Ic->SetBrushFromTexture(I.AbilityIcon);
                }
                if (UTextBlock* Nm = Cast<UTextBlock>(WT->FindWidget(NameN[i])))
                {
                    Nm->SetText(FText::FromString(Y3_SkillCnName(I)));
                }
            }
        }
        else if (i < N)
        {
            // —— 兜底升级卡(属性/金币) ——
            const FName RowName = UpgradeRows[i - SkillN];
            CurrentChoices.Add({ FGameplayTag(), RowName });
            const FY3UpgradeChoiceRow* Row = UpgradeChoiceTable
                ? UpgradeChoiceTable->FindRow<FY3UpgradeChoiceRow>(RowName, TEXT("Y3 Upgrade Card"))
                : nullptr;
            if (WT && Row)
            {
                // 兜底卡没有彩色球底,折叠 Bg 只显示方形图标。
                if (UWidget* Bg = WT->FindWidget(BgN[i])) Bg->SetVisibility(ESlateVisibility::Collapsed);
                if (UImage* Ic = Cast<UImage>(WT->FindWidget(IconN[i])))
                {
                    if (Row->Icon) Ic->SetBrushFromTexture(Row->Icon);
                }
                if (UTextBlock* Nm = Cast<UTextBlock>(WT->FindWidget(NameN[i])))
                {
                    Nm->SetText(FText::FromString(Row->DisplayName));
                }
            }
        }
        else
        {
            CurrentChoices.Add({ FGameplayTag(), NAME_None });
            if (WT)
            {
                // 候选不足3个时,隐藏多余的空卡(否则残留白方块)。
                if (UWidget* W = WT->FindWidget(BtnN[i]))  W->SetVisibility(ESlateVisibility::Collapsed);
                if (UWidget* W = WT->FindWidget(BgN[i]))   W->SetVisibility(ESlateVisibility::Collapsed);
                if (UWidget* W = WT->FindWidget(IconN[i])) W->SetVisibility(ESlateVisibility::Collapsed);
                if (UWidget* W = WT->FindWidget(NameN[i])) W->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }

    if (WT)
    {
        if (UButton* B = Cast<UButton>(WT->FindWidget(BtnN[0]))) B->OnClicked.AddDynamic(this, &AY3BattleGameMode::OnSkillCard1Clicked);
        if (UButton* B = Cast<UButton>(WT->FindWidget(BtnN[1]))) B->OnClicked.AddDynamic(this, &AY3BattleGameMode::OnSkillCard2Clicked);
        if (UButton* B = Cast<UButton>(WT->FindWidget(BtnN[2]))) B->OnClicked.AddDynamic(this, &AY3BattleGameMode::OnSkillCard3Clicked);
    }

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->bShowMouseCursor = true;
        FInputModeUIOnly Mode;
        PC->SetInputMode(Mode);
    }
    UGameplayStatics::SetGamePaused(this, true);
    UE_LOG(LogTemp, Log, TEXT("[Y3 Skill] 三选一弹出,候选=%d"), N);
}

void AY3BattleGameMode::OnSkillCard1Clicked() { SelectSkillCard(0); }
void AY3BattleGameMode::OnSkillCard2Clicked() { SelectSkillCard(1); }
void AY3BattleGameMode::OnSkillCard3Clicked() { SelectSkillCard(2); }

void AY3BattleGameMode::SelectSkillCard(int32 Index)
{
    UGameplayStatics::SetGamePaused(this, false);
    if (CurrentChoices.IsValidIndex(Index) && CurrentChoices[Index].IsValid())
    {
        if (CurrentChoices[Index].SkillTag.IsValid())
            Y3_GiveAndEquip(CurrentChoices[Index].SkillTag);
        else
            ApplyUpgradeChoice(CurrentChoices[Index].UpgradeRow);
    }

    if (SkillChoiceInstance) { SkillChoiceInstance->RemoveFromParent(); SkillChoiceInstance = nullptr; }
    CurrentChoices.Reset();

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->bShowMouseCursor = true;          // 保留鼠标(点击移动需要)
        FInputModeGameAndUI Mode;
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Mode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(Mode);
        // 三选一 widget(SetIsFocusable)抢走了键盘焦点,移除后必须还给游戏视口,
        // 否则数字键放技能失效(鼠标点击移动仍可,走视口指针)。
        FSlateApplication::Get().SetAllUserFocusToGameViewport();
    }
}

TArray<FName> AY3BattleGameMode::Y3_PickUpgradeRows(int32 Count) const
{
    TArray<FName> Result;
    if (Count <= 0 || !UpgradeChoiceTable) return Result;

    // 加权不放回抽样
    struct FCandidate { FName Row; int32 Weight; };
    TArray<FCandidate> Candidates;
    for (const auto& Pair : UpgradeChoiceTable->GetRowMap())
    {
        const FY3UpgradeChoiceRow* Row = reinterpret_cast<const FY3UpgradeChoiceRow*>(Pair.Value);
        if (Row && Row->Weight > 0) Candidates.Add({ Pair.Key, Row->Weight });
    }
    while (Result.Num() < Count && Candidates.Num() > 0)
    {
        int32 Total = 0;
        for (const FCandidate& C : Candidates) Total += C.Weight;
        int32 Roll = FMath::RandRange(1, Total);
        for (int32 i = 0; i < Candidates.Num(); ++i)
        {
            Roll -= Candidates[i].Weight;
            if (Roll <= 0)
            {
                Result.Add(Candidates[i].Row);
                Candidates.RemoveAtSwap(i);
                break;
            }
        }
    }
    return Result;
}

void AY3BattleGameMode::ApplyUpgradeChoice(FName RowName)
{
    if (!UpgradeChoiceTable || RowName.IsNone()) return;
    const FY3UpgradeChoiceRow* Row = UpgradeChoiceTable->FindRow<FY3UpgradeChoiceRow>(RowName, TEXT("Y3 Upgrade Apply"));
    if (!Row) return;

    // 属性卡:经 DA_AttributeInfo 把 tag 解析成 FGameplayAttribute,直接加 BaseValue(本局内永久)。
    // 主属性提升会经由派生 GE 自动联动副属性(MaxHealth 等)。
    if (Row->AttributeTag.IsValid() && Row->Magnitude != 0.f)
    {
        UAuraAbilitySystemComponent* ASC = Y3_GetPlayerASC(this);
        if (ASC && AttributeInfoSource)
        {
            const FAuraAttributeInfo AI = AttributeInfoSource->FindAttributeInfoForTag(Row->AttributeTag, true);
            if (AI.AttributeGetter.IsValid())
            {
                ASC->ApplyModToAttribute(AI.AttributeGetter, EGameplayModOp::Additive, Row->Magnitude);
                UE_LOG(LogTemp, Log, TEXT("[Y3 Upgrade] %s: %s +%.0f"),
                    *RowName.ToString(), *Row->AttributeTag.ToString(), Row->Magnitude);
            }
        }
        else if (!AttributeInfoSource)
        {
            UE_LOG(LogTemp, Error, TEXT("[Y3 Upgrade] AttributeInfoSource 未设置,属性卡 %s 无法应用"), *RowName.ToString());
        }
    }

    // 货币卡:直接进账号金币并落盘。
    if (Row->Gold > 0)
    {
        if (UGameInstance* GI = GetGameInstance())
        {
            if (UY3AccountSubsystem* Account = GI->GetSubsystem<UY3AccountSubsystem>())
            {
                Account->AddGold(Row->Gold);
                Account->SaveCurrentAccount();
                UE_LOG(LogTemp, Log, TEXT("[Y3 Upgrade] %s: 金币 +%d"), *RowName.ToString(), Row->Gold);
            }
        }
    }
}

void AY3BattleGameMode::Y3ToggleAttributeMenu()
{
    if (!AttributeMenuInstance)
    {
        if (!AttributeMenuClass)
        {
            UE_LOG(LogTemp, Error, TEXT("[Y3 HUD] AttributeMenuClass 未设置!"));
            return;
        }
        // WBP_AttributeMenu 在 Construct 时自取 AttributeMenuWgtController 并分发给子行,这里只管创建。
        AttributeMenuInstance = CreateWidget<UUserWidget>(GetWorld(), AttributeMenuClass);
        if (!AttributeMenuInstance) return;
        AttributeMenuInstance->AddToViewport(1050);
        // 旧链路里由 Overlay 菜单触发初始广播,瘦身后没人调 → 这里补一次,否则面板全是设计器占位值。
        if (UAttributeMenuWgtController* Ctrl = UAuraAbilitySystemBPLibary::GetAttributeMenuWgtController(this))
            Ctrl->BroadcastInitialValues();
        return;
    }
    // 面板自带的关闭按钮走 SetVisibility 隐藏自身,这里统一用可见性开关。
    const bool bShown = AttributeMenuInstance->IsVisible();
    AttributeMenuInstance->SetVisibility(bShown ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (!bShown)
    {
        // 重新打开时刷一遍初始值(属性变化平时有委托实时推,这里兜底)
        if (UAttributeMenuWgtController* Ctrl = UAuraAbilitySystemBPLibary::GetAttributeMenuWgtController(this))
            Ctrl->BroadcastInitialValues();
    }
}

void AY3BattleGameMode::OnStatsButtonClicked()
{
    Y3ToggleAttributeMenu();
}

void AY3BattleGameMode::OnSkillButtonClicked()
{
    if (AAuraPlayerController* PC = Cast<AAuraPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
        PC->Y3Atlas();
}

void AY3BattleGameMode::OnSettingsButtonClicked()
{
    // 复用 overlay 里现成的退出确认弹窗(QuiteGameTip 无参,直接按名触发)
    TArray<UUserWidget*> Widgets;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, UUserWidget::StaticClass(), true);
    for (UUserWidget* W : Widgets)
    {
        if (W && W->GetClass()->GetName().Contains(TEXT("WBP_Y3BattleOverlay")))
        {
            if (UFunction* F = W->FindFunction(TEXT("QuiteGameTip")))
            {
                W->ProcessEvent(F, nullptr);
                return;
            }
        }
    }
}
