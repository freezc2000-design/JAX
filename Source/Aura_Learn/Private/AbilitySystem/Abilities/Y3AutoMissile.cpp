// Y3 roguelite skill archetype: auto-firing homing missile. See header.
#include "AbilitySystem/Abilities/Y3AutoMissile.h"

#include "AbilitySystem/AuraAbilitySystemBPLibary.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"

UY3AutoMissile::UY3AutoMissile()
{
	// 需要实例化（定时器绑定 this、保存 FireTimerHandle）。
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UY3AutoMissile::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// SpawnProjectiles 仅在服务器权威端生成，故循环也只在服务器跑。
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (Avatar && Avatar->HasAuthority())
	{
		FireOnce(); // 激活立即来一发
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(FireTimerHandle, this, &UY3AutoMissile::FireOnce,
				FMath::Max(0.05f, FireInterval), /*bLoop=*/true);
		}
	}
	// 注意：本能力作为“自动武器”持续激活，不在此 EndAbility；
	// 由角色死亡/能力移除时走 EndAbility 清理定时器。
}

void UY3AutoMissile::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimerHandle);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

int32 UY3AutoMissile::GetNumTargetsForLevel() const
{
	const int32 Lvl = FMath::Max(1, GetAbilityLevel());
	return FMath::Clamp(BaseNumTargets + (Lvl - 1) * TargetsPerLevel, 1, FMath::Max(1, MaxNumTargets));
}

void UY3AutoMissile::FireOnce()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(Avatar))
	{
		return;
	}
	const FVector Origin = Avatar->GetActorLocation();

	// 1) 取索敌半径内的活敌人（GetLivePlayersWithRadius 不分队伍，按 CombatInterface+未死筛选，忽略自己）
	TArray<AActor*> Enemies;
	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(Avatar);
	UAuraAbilitySystemBPLibary::GetLivePlayersWithRadius(Avatar, Enemies, IgnoreActors, Origin, DetectRadius);
	if (Enemies.Num() == 0)
	{
		return; // 范围内没敌人，本次不开火
	}

	// 2) 取最近的 N 个敌人（N 随等级增长 = 多目标）
	TArray<AActor*> ClosestTargets;
	UAuraAbilitySystemBPLibary::GetClosesTarget(Origin, GetNumTargetsForLevel(), Enemies, ClosestTargets);

	// 3) 对每个目标发射追踪导弹（每次 SpawnProjectiles 喷 min(等级,NumProjectiles) 发，追踪该敌人）
	for (AActor* Target : ClosestTargets)
	{
		if (IsValid(Target))
		{
			SpawnProjectiles(Target->GetActorLocation(), /*bOverridePitch=*/false, /*PitchOverride=*/0.f, Target);
		}
	}
}
