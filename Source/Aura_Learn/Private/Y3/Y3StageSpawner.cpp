// Y3 刷怪计时器 Actor 实现
#include "Y3/Y3StageSpawner.h"
#include "Y3/Y3BattleGameMode.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Character/AuraEnemy.h"

AY3StageSpawner::AY3StageSpawner()
{
    PrimaryActorTick.bCanEverTick = false;
}

float AY3StageSpawner::GetSecondsToNextSpawn() const
{
    if (!GetWorld()) return -1.f;
    FTimerManager& TM = GetWorld()->GetTimerManager();
    // 未开始：返回 StartDelay 剩余
    if (TM.IsTimerActive(StartDelayHandle))
    {
        return TM.GetTimerRemaining(StartDelayHandle);
    }
    if (TM.IsTimerActive(LoopHandle))
    {
        return TM.GetTimerRemaining(LoopHandle);
    }
    return -1.f; // 已结束
}

void AY3StageSpawner::Init(AY3BattleGameMode* InOwner, const FY3TimerRow& InTimer, const FVector& InBase)
{
    OwnerGM = InOwner;
    TimerData = InTimer;
    BaseLocation = InBase;
    CurrentGroupSize = TimerData.countGrp;
    UE_LOG(LogTemp, Log, TEXT("[Y3 Spawner] Init unitID=%d timeStart=%d timeLoop=%d count=%d countGrp=%d base=%s"),
        TimerData.unitID, TimerData.timeStart, TimerData.timeLoop,
        TimerData.count, TimerData.countGrp, *BaseLocation.ToString());
}

void AY3StageSpawner::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Log, TEXT("[Y3 Spawner] BeginPlay unitID=%d"), TimerData.unitID);
    if (TimerData.timeStart <= 0)
    {
        OnStartDelay();
    }
    else
    {
        GetWorld()->GetTimerManager().SetTimer(StartDelayHandle, this, &AY3StageSpawner::OnStartDelay, (float)TimerData.timeStart, false);
    }
}

void AY3StageSpawner::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorld()->GetTimerManager().ClearTimer(StartDelayHandle);
    GetWorld()->GetTimerManager().ClearTimer(LoopHandle);
    Super::EndPlay(Reason);
}

void AY3StageSpawner::OnStartDelay()
{
    OnSpawnTick();
    if (TimerData.timeLoop > 0)
    {
        GetWorld()->GetTimerManager().SetTimer(LoopHandle, this, &AY3StageSpawner::OnSpawnTick, (float)TimerData.timeLoop, true);
    }
}

void AY3StageSpawner::OnSpawnTick()
{
    if (!OwnerGM) return;

    if (TimerData.timeEnd > 0 && ElapsedSinceStart >= TimerData.timeEnd)
    {
        GetWorld()->GetTimerManager().ClearTimer(LoopHandle);
        return;
    }

    // [Y3demo survivor-style] Spawn around player position, not fixed BaseLocation.
    // Falls back to BaseLocation if no player pawn (e.g. early BeginPlay).
    FVector SpawnCenter = BaseLocation;
    if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
    {
        SpawnCenter = PlayerPawn->GetActorLocation();
    }

    // Clamp radius to a sensible camera-edge range for survivor pacing.
    // DT_Y3_Timer.radius "6000|6000" was for original Y3's larger map; in our demo
    // we want monsters appearing just outside view (≈ 800-1500 units from player).
    TArray<float> Radii = ParseFloatPipeArray(TimerData.radius);
    float RadiusOuter = Radii.Num() > 1 ? Radii[1] : (Radii.Num() > 0 ? Radii[0] : 0.f);
    const float R = FMath::Clamp(RadiusOuter, 800.f, 1500.f);

    // 原表语义：count=本次刷几组，countGrp(=>CurrentGroupSize)=每组只数
    const int32 GroupCount = FMath::Max(1, TimerData.count);
    const int32 PerGroup = FMath::Max(1, CurrentGroupSize);
    int32 Total = GroupCount * PerGroup;

    // [安全钳制 1] 单次 tick 硬上限，防止 DT 配置过大(如 countGrp=30)导致主线程过载崩溃
    if (MaxSpawnPerTick > 0 && Total > MaxSpawnPerTick)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Y3 Spawn] unitID=%d total %d clamped to MaxSpawnPerTick=%d"),
            TimerData.unitID, Total, MaxSpawnPerTick);
        Total = MaxSpawnPerTick;
    }

    // [安全钳制 2] 场上存活怪上限，避免怪持续累积拖垮性能
    if (MaxAliveTotal > 0)
    {
        TArray<AActor*> Alive;
        UGameplayStatics::GetAllActorsOfClass(this, AAuraEnemy::StaticClass(), Alive);
        if (Alive.Num() >= MaxAliveTotal)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Y3 Spawn] unitID=%d skipped: alive %d >= cap %d"),
                TimerData.unitID, Alive.Num(), MaxAliveTotal);
            Total = 0;
        }
        else
        {
            Total = FMath::Min(Total, MaxAliveTotal - Alive.Num());
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[Y3 Spawn] unitID=%d total=%d radius=%.0f center=%s"),
        TimerData.unitID, Total, R, *SpawnCenter.ToString());

    int32 SuccessCount = 0;
    for (int32 i = 0; i < Total; ++i)
    {
        float Angle = FMath::FRandRange(0.f, 2.f * PI);
        float Dist = FMath::FRandRange(R * 0.5f, R);
        FVector Loc = SpawnCenter + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0);
        AActor* Spawned = OwnerGM->SpawnY3Enemy(TimerData.unitID, Loc);
        if (Spawned) ++SuccessCount;
    }
    UE_LOG(LogTemp, Log, TEXT("[Y3 Spawn] unitID=%d done: %d/%d spawned"),
        TimerData.unitID, SuccessCount, Total);

    ++LoopCount;
    if (TimerData.timeLoop > 0)
    {
        ElapsedSinceStart += (float)TimerData.timeLoop;
    }

    // countAdd: 每次循环组内数量+countAdd，但上限 = countGrp + countAdd*countAddGrp
    if (TimerData.countAdd > 0)
    {
        int32 NextSize = CurrentGroupSize + TimerData.countAdd;
        if (TimerData.countAddGrp > 0)
        {
            int32 Cap = TimerData.countGrp + TimerData.countAdd * TimerData.countAddGrp;
            NextSize = FMath::Min(NextSize, Cap);
        }
        CurrentGroupSize = NextSize;
    }
}

TArray<float> AY3StageSpawner::ParseFloatPipeArray(const FString& s)
{
    TArray<float> Out;
    if (s.IsEmpty()) return Out;
    TArray<FString> Parts;
    s.ParseIntoArray(Parts, TEXT("|"), true);
    for (const FString& P : Parts)
    {
        Out.Add(FCString::Atof(*P));
    }
    return Out;
}
