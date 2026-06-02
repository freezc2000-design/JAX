// Y3 刷怪计时器 Actor
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Y3/Y3DataTypes.h"
#include "Y3StageSpawner.generated.h"

class AY3BattleGameMode;

UCLASS()
class AURA_LEARN_API AY3StageSpawner : public AActor
{
    GENERATED_BODY()

public:
    AY3StageSpawner();

    void Init(AY3BattleGameMode* InOwner, const FY3TimerRow& InTimer, const FVector& InBaseLocation);

    /** 安全钳制：单次 tick 最多刷多少只(防止 DT 配置过大导致主线程过载崩溃) */
    UPROPERTY(EditDefaultsOnly, Category="Y3|Safety")
    int32 MaxSpawnPerTick = 8;

    /** 安全钳制：场上同时存活的本类怪上限(全局粗略,<=0 关闭) */
    UPROPERTY(EditDefaultsOnly, Category="Y3|Safety")
    int32 MaxAliveTotal = 40;

    /** HUD 用：当前 Spawner 关联的 UnitID */
    UFUNCTION(BlueprintPure, Category="Y3|HUD")
    int32 GetUnitID() const { return TimerData.unitID; }

    /** HUD 用：距下一次刷怪还剩多少秒（未开始则返回 timeStart - elapsed） */
    UFUNCTION(BlueprintPure, Category="Y3|HUD")
    float GetSecondsToNextSpawn() const;

    /** HUD 用：返回 Timer 行的副本，蓝图可直接读取所有字段 */
    UFUNCTION(BlueprintPure, Category="Y3|HUD")
    FY3TimerRow GetTimerRow() const { return TimerData; }

    /** HUD 用：当前每组数量（考虑了 countAdd 递增） */
    UFUNCTION(BlueprintPure, Category="Y3|HUD")
    int32 GetCurrentGroupSize() const { return CurrentGroupSize; }

    /** HUD 用：已循环次数 */
    UFUNCTION(BlueprintPure, Category="Y3|HUD")
    int32 GetLoopCount() const { return LoopCount; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    UPROPERTY()
    TObjectPtr<AY3BattleGameMode> OwnerGM;

    FY3TimerRow TimerData;
    FVector BaseLocation = FVector::ZeroVector;

    FTimerHandle StartDelayHandle;
    FTimerHandle LoopHandle;

    float ElapsedSinceStart = 0.f;
    int32 LoopCount = 0;
    int32 CurrentGroupSize = 0;

    static TArray<float> ParseFloatPipeArray(const FString& s);

    void OnStartDelay();
    void OnSpawnTick();
};
