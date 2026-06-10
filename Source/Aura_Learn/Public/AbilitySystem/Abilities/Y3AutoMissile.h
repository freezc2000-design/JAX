// Y3 roguelite skill archetype: auto-firing homing missile.
// Subclass of UAuraFireBolt — reuses SpawnProjectiles (fan of homing projectiles,
// projectile count = min(AbilityLevel, NumProjectiles), damage scales by level).
// This class adds: auto target-acquisition (nearest enemies) + auto-fire on a timer,
// so it plays like a Vampire-Survivors weapon (player only moves).
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraFireBolt.h"
#include "Y3AutoMissile.generated.h"

/**
 * 自动追踪导弹（肉鸽技能类型模板）
 * 激活后按 FireInterval 自动开火，自动锁定半径内最近的 N 个敌人发射追踪导弹。
 * 升级杠杆：伤害（基类按等级走曲线）、单目标弹数（基类 min(等级,NumProjectiles)）、
 *          同时目标数（本类 BaseNumTargets + (等级-1)*TargetsPerLevel）。
 */
UCLASS()
class AURA_LEARN_API UY3AutoMissile : public UAuraFireBolt
{
	GENERATED_BODY()

public:
	UY3AutoMissile();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 自动开火间隔（秒）
	UPROPERTY(EditDefaultsOnly, Category = "Y3|AutoMissile", DisplayName = "开火间隔(秒)")
	float FireInterval = 1.0f;

	// 索敌半径
	UPROPERTY(EditDefaultsOnly, Category = "Y3|AutoMissile", DisplayName = "索敌半径")
	float DetectRadius = 1500.f;

	// 基础同时打击的目标数
	UPROPERTY(EditDefaultsOnly, Category = "Y3|AutoMissile", DisplayName = "基础同时目标数")
	int32 BaseNumTargets = 1;

	// 每级额外目标数（升级实现“多目标追踪”）
	UPROPERTY(EditDefaultsOnly, Category = "Y3|AutoMissile", DisplayName = "每级额外目标数")
	int32 TargetsPerLevel = 0;

	// 同时目标数上限
	UPROPERTY(EditDefaultsOnly, Category = "Y3|AutoMissile", DisplayName = "同时目标数上限")
	int32 MaxNumTargets = 5;

private:
	FTimerHandle FireTimerHandle;

	// 单次开火：索敌 + 朝最近的若干敌人发射追踪导弹
	void FireOnce();

	// 当前技能等级对应的同时目标数
	int32 GetNumTargetsForLevel() const;
};
