// Y3 关卡战斗 GameMode
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Game/AuraGameModeBase.h"
#include "Y3/Y3DataTypes.h"
#include "Y3BattleGameMode.generated.h"

class UDataTable;
class AY3StageSpawner;
class AAuraEnemy;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnY3StageInfoChanged, FString, StageInfo);

UCLASS()
class AURA_LEARN_API AY3BattleGameMode : public AAuraGameModeBase
{
    GENERATED_BODY()

public:
    AY3BattleGameMode();

    UPROPERTY(EditDefaultsOnly, Category="Y3|Data")
    TObjectPtr<UDataTable> DT_Stage;

    UPROPERTY(EditDefaultsOnly, Category="Y3|Data")
    TObjectPtr<UDataTable> DT_Timer;

    UPROPERTY(EditDefaultsOnly, Category="Y3|Data")
    TObjectPtr<UDataTable> DT_Unit;

    UPROPERTY(EditDefaultsOnly, Category="Y3|Data")
    TObjectPtr<UDataTable> DT_Attr;

    UPROPERTY(EditDefaultsOnly, Category="Y3|Mapping")
    TSubclassOf<AAuraEnemy> MobBP_Melee;

    UPROPERTY(EditDefaultsOnly, Category="Y3|Mapping")
    TSubclassOf<AAuraEnemy> MobBP_Ranged;

    UPROPERTY(EditDefaultsOnly, Category="Y3|Mapping")
    TSubclassOf<AAuraEnemy> BossBP;

    UPROPERTY(EditDefaultsOnly, Category="Y3|HUD")
    TSubclassOf<class UUserWidget> StageHUDClass;

    UPROPERTY(BlueprintReadOnly, Category="Y3|HUD")
    TObjectPtr<class UUserWidget> StageHUDInstance;

    UPROPERTY(EditDefaultsOnly, Category="Y3|HUD")
    TSubclassOf<class UUserWidget> ResultScreenClass;

    UPROPERTY(BlueprintReadOnly, Category="Y3|HUD")
    TObjectPtr<class UUserWidget> ResultScreenInstance;

    /** 结算背景:胜利/失败各一张(应用到 BG_Image) */
    UPROPERTY(EditDefaultsOnly, Category="Y3|HUD")
    TObjectPtr<class UTexture2D> ResultBgWin;

    UPROPERTY(EditDefaultsOnly, Category="Y3|HUD")
    TObjectPtr<class UTexture2D> ResultBgLose;

    // ===== 升级三选一 =====
    UPROPERTY(EditDefaultsOnly, Category="Y3|SkillChoice")
    TSubclassOf<class UUserWidget> SkillChoiceClass;

    UPROPERTY(BlueprintReadOnly, Category="Y3|SkillChoice")
    TObjectPtr<class UUserWidget> SkillChoiceInstance;

    /** 技能数值调优表(行类型 FY3SkillTuningRow;第一版含库存,后续加弹数/范围等)。在 BP_BattleGameMode 里设。 */
    UPROPERTY(EditDefaultsOnly, Category="Y3|SkillChoice")
    TObjectPtr<class UDataTable> SkillTuningTable;

    /** 兜底升级卡表(行类型 FY3UpgradeChoiceRow)。技能池不足3张时用属性/金币卡补位;全满级后三选一只出这些。 */
    UPROPERTY(EditDefaultsOnly, Category="Y3|SkillChoice")
    TObjectPtr<class UDataTable> UpgradeChoiceTable;

    /** Tag→FGameplayAttribute 解析源(配 DA_AttributeInfo);属性卡应用时查询 */
    UPROPERTY(EditDefaultsOnly, Category="Y3|SkillChoice")
    TObjectPtr<class UAttributeInfo> AttributeInfoSource;

    /** 升级时弹三选一(由 AuraCharacter::LevelUp 调用) */
    UFUNCTION(BlueprintCallable, Category="Y3|SkillChoice")
    void ShowSkillChoice();

    /** 给玩家一个技能并自动装到下一个空快捷栏槽(InputTag.1~6) */
    UFUNCTION(BlueprintCallable, Category="Y3|SkillChoice")
    void Y3_GiveAndEquip(FGameplayTag AbilityTag);

    /** 读技能库存(可升级份数);表未配该技能则返回 0(抽到即定级、不升级) */
    int32 Y3_GetSkillStock(const FGameplayTag& AbilityTag) const;

    /** 应用一张兜底强化卡(属性/金币)。三选一选中、图鉴点击测试共用。 */
    UFUNCTION(BlueprintCallable, Category="Y3|SkillChoice")
    void ApplyUpgradeChoice(FName RowName);

    UFUNCTION() void OnSkillCard1Clicked();
    UFUNCTION() void OnSkillCard2Clicked();
    UFUNCTION() void OnSkillCard3Clicked();

    // ===== 角色属性面板（HUD"属性"按钮开关） =====
    UPROPERTY(EditDefaultsOnly, Category="Y3|HUD")
    TSubclassOf<class UUserWidget> AttributeMenuClass;

    UPROPERTY(BlueprintReadOnly, Category="Y3|HUD")
    TObjectPtr<class UUserWidget> AttributeMenuInstance;

    /** 开/关角色属性详情面板(WBP_AttributeMenu 自取控制器,无需喂参) */
    UFUNCTION(BlueprintCallable, Category="Y3|HUD")
    void Y3ToggleAttributeMenu();

    UFUNCTION() void OnStatsButtonClicked();
    UFUNCTION() void OnSkillButtonClicked();    // 右侧栏"技能"图标 → 技能图鉴
    UFUNCTION() void OnSettingsButtonClicked(); // 右侧栏"设置"图标 → 退出确认弹窗(复用overlay QuiteGameTip)

    /** 整局 Boss 限时（秒）；<=0 表示不启用（简易波次模式下不使用） */
    UPROPERTY(EditDefaultsOnly, Category="Y3|Rules")
    float BossTimeLimit = 300.f;

    // ===== 简易三轨道刷怪模式（清晰的有限波次，替代复杂 DT 驱动） =====
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave")
    bool bUseSimpleWaveMode = true;

    // 各档敌人池(随机刷;在BP_BattleGameMode里设)。空则回退 unitID
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave|Class") TArray<TSubclassOf<AActor>> NormalEnemyPool;
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave|Class") TArray<TSubclassOf<AActor>> EliteEnemyPool;
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave|Class") TSubclassOf<AActor> BossEnemyClass;

    // 普通怪轨道
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave|Normal") int32 NormalUnitID = 10001;
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave|Normal") int32 NormalWaveCount = 6;
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave|Normal") float NormalInterval = 10.f;
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave|Normal") int32 NormalPerWave = 5;

    // 精英怪轨道
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave|Elite") int32 EliteUnitID = 10002;
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave|Elite") int32 EliteWaveCount = 3;
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave|Elite") float EliteInterval = 20.f;
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave|Elite") int32 ElitePerWave = 3;

    // Boss 轨道
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave|Boss") int32 SimpleBossUnitID = 13001;
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave|Boss") float BossSpawnDelay = 60.f;
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave|Boss") float BossKillTimeLimit = 20.f;

    // 简易模式刷怪半径
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave") float WaveSpawnRadius = 1200.f;

    // 体型差异化已改为烤进BP网格;运行时缩放保留接口但默认全=1(不覆盖BP尺寸)
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave") float NormalScale = 1.0f;
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave") float EliteScale = 1.0f;
    UPROPERTY(EditDefaultsOnly, Category="Y3|SimpleWave") float BossScale = 1.0f;

    // 运行时波次计数（HUD 读取）
    UPROPERTY(BlueprintReadOnly, Category="Y3|SimpleWave") int32 NormalWaveIdx = 0;
    UPROPERTY(BlueprintReadOnly, Category="Y3|SimpleWave") int32 EliteWaveIdx = 0;
    UPROPERTY(BlueprintReadOnly, Category="Y3|SimpleWave") bool bBossSpawned = false;

    /** HUD：返回三轨道波次显示文本 */
    UFUNCTION(BlueprintPure, Category="Y3|SimpleWave")
    FString GetWaveHUDText() const;

    /** 玩家死亡延迟显示结果（秒） */
    UPROPERTY(EditDefaultsOnly, Category="Y3|Rules")
    float ResultShowDelay = 2.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Y3|AccountReward")
    int32 LossAccountXPReward = 100;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Y3|AccountReward")
    int32 LossGoldReward = 100;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Y3|AccountReward")
    int32 WinAccountXPReward = 2000;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Y3|AccountReward")
    int32 WinGoldReward = 2000;

    UFUNCTION(BlueprintCallable, Category="Y3")
    void ShowResult(bool bWin, int32 StageReached);

    UFUNCTION()
    void HandlePlayerDeath(AActor* DeadActor);

    UFUNCTION()
    void OnBossTimeExpired();

    UFUNCTION()
    void OnConfirmResult();

    // 简易波次内部
    void StartSimpleWaves();
    void NormalWaveTick();
    void EliteWaveTick();
    void SpawnBossWave();
    void OnBossKillTimeExpired();
    UFUNCTION()
    void HandleBossDeath(AActor* DeadActor);
    void SpawnWaveAroundPlayer(int32 UnitID, int32 Count, float Scale = 1.0f);
    void SpawnWaveFromPool(const TArray<TSubclassOf<AActor>>& Pool, int32 Count, float Scale);
    AActor* SpawnEnemyAt(UClass* Cls, const FVector& Loc);
    void ApplyEnemyScale(AActor* Enemy, float Scale);

    /** 失败/胜利后,确认按钮跳转到此关卡(默认 Map_HeroSelection) */
    UPROPERTY(EditDefaultsOnly, Category="Y3|Rules")
    FName ReturnLevelName = TEXT("Map_Y3_Selection");

    UPROPERTY(EditDefaultsOnly, Category="Y3|Mapping")
    TSubclassOf<AY3StageSpawner> SpawnerClass;

    UPROPERTY(EditDefaultsOnly, Category="Y3|Debug")
    int32 DebugForceLevel = -1;

    UPROPERTY(BlueprintReadOnly, Category="Y3|Runtime")
    int32 CurrentLevel = 0;
    UPROPERTY(BlueprintReadOnly, Category="Y3|Runtime")
    int32 CurrentStageIdx = 0;
    UPROPERTY(BlueprintReadOnly, Category="Y3|Runtime")
    int32 TotalStages = 0;
    UPROPERTY(BlueprintReadOnly, Category="Y3|Runtime")
    float CurrentStageRemaining = 0.f;
    UPROPERTY(BlueprintReadOnly, Category="Y3|Runtime")
    FString CurrentStageInfo;
    UPROPERTY(BlueprintReadOnly, Category="Y3|Runtime")
    bool bLevelFinished = false;

    UPROPERTY(BlueprintAssignable, Category="Y3|Events")
    FOnY3StageInfoChanged OnStageInfoChanged;

    UFUNCTION(BlueprintCallable, Category="Y3")
    TSubclassOf<AAuraEnemy> ResolveEnemyBPFromUnitID(int32 UnitID);

    /** 任意类（含 Y3EnemyBase）解析：优先返回 Y3 专属 BP */
    UFUNCTION(BlueprintCallable, Category="Y3")
    UClass* ResolveEnemyBPFromUnitIDAsClass(int32 UnitID);

    UFUNCTION(BlueprintCallable, Category="Y3")
    AActor* SpawnY3Enemy(int32 UnitID, const FVector& Location);

    /** 开始指定关卡（供 UI 调用） */
    UFUNCTION(BlueprintCallable, Category="Y3")
    void StartBattle(int32 LevelIdx);

    // ---- HUD 数据接口 ----

    /** HUD 用：当前阶段剩余时间（秒，<0 表示无限或未开始） */
    UFUNCTION(BlueprintPure, Category="Y3|HUD")
    float GetCurrentStageRemaining() const { return CurrentStageRemaining; }

    /** HUD 用：距下一个 Boss 还有多少秒（找到当前关卡里 stageBoss>0 的下一个阶段） */
    UFUNCTION(BlueprintPure, Category="Y3|HUD")
    float GetSecondsToNextBoss() const;

    /** HUD 用：把秒数格式化为 "MM:SS" */
    UFUNCTION(BlueprintPure, Category="Y3|HUD")
    static FString FormatSecondsAsMMSS(float Seconds);

    /** HUD 用：当前阶段所有活跃 Spawner 的条目（按 UnitID 去重，最多取前 N 个） */
    UFUNCTION(BlueprintCallable, Category="Y3|HUD")
    TArray<FY3HUDMobEntry> GetHUDMobEntries(int32 MaxCount = 4) const;

protected:
    virtual void BeginPlay() override;
    /** 让引擎一开始就 spawn 选定英雄(避免销毁/重生默认 pawn 导致 HUD 悬空引用） */
    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

private:
    TArray<FY3StageRow> LevelStages;

    UPROPERTY()
    TArray<AY3StageSpawner*> ActiveSpawners;

    FTimerHandle StageTickTimer;
    FTimerHandle StageEndTimer;
    FTimerHandle BossLimitTimer;
    FTimerHandle ResultDelayTimer;
    bool bResultShown = false;

    // 三选一当前候选:技能卡(SkillTag 有效)或兜底升级卡(UpgradeRow 有效)
    struct FY3ChoiceEntry
    {
        FGameplayTag SkillTag;
        FName UpgradeRow;
        bool IsValid() const { return SkillTag.IsValid() || !UpgradeRow.IsNone(); }
    };
    TArray<FY3ChoiceEntry> CurrentChoices;
    TArray<FName> Y3_PickUpgradeRows(int32 Count) const;
    void SelectSkillCard(int32 Index);
    FGameplayTag Y3_FindFreeInputSlot() const;
    FGameplayTag Y3_FindFreePassiveSlot() const;

    // 简易波次计时器
    FTimerHandle NormalWaveTimer;
    FTimerHandle EliteWaveTimer;
    FTimerHandle BossSpawnTimer;
    FTimerHandle BossKillTimer;
    FTimerHandle HudTickTimer;

    void StartLevel(int32 LevelIdx);
    void StartStage(int32 StageIdx);
    void EndCurrentStage();
    void OnStageTick();
    void ClearActiveSpawners();

    static TArray<int32> ParsePipeIntArray(const FString& s);
    static FVector ParseCoord(const FString& Coord);
};
