#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraDamageGameplayAbility.h"
#include "Y3ChainLightning.generated.h"

class UNiagaraSystem;
class UAuraAbilitySystemComponent;

/**
 * Y3 active skill: hits the nearest enemy, then chains lightning to nearby enemies.
 */
UCLASS()
class AURA_LEARN_API UY3ChainLightning : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()

public:
	UY3ChainLightning();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual FString GetDescription(const UAuraAbilitySystemComponent* AuraGAS,
		const FGameplayTag& GATag,
		const int32 Level) override;

	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Y3|ChainLightning", DisplayName = "首目标索敌半径")
	float AcquireRadius = 1600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Y3|ChainLightning", DisplayName = "弹射索敌半径")
	float BounceRadius = 700.f;

	UPROPERTY(EditDefaultsOnly, Category = "Y3|ChainLightning", DisplayName = "1级命中目标数")
	int32 BaseTargetCount = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Y3|ChainLightning", DisplayName = "每级增加目标数")
	int32 TargetsPerLevel = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Y3|ChainLightning", DisplayName = "最大命中目标数")
	int32 MaxTargetCount = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Y3|ChainLightning", DisplayName = "每次弹射伤害倍率")
	float DamageFalloffPerBounce = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "Y3|ChainLightning|VFX", DisplayName = "闪电束特效")
	TObjectPtr<UNiagaraSystem> BeamFX;

	UPROPERTY(EditDefaultsOnly, Category = "Y3|ChainLightning|VFX", DisplayName = "命中特效")
	TObjectPtr<UNiagaraSystem> ImpactFX;

	UPROPERTY(EditDefaultsOnly, Category = "Y3|ChainLightning|VFX", DisplayName = "命中点高度偏移")
	float TargetHeightOffset = 80.f;

private:
	UPROPERTY(Transient)
	mutable FGameplayTagContainer ChainLightningCooldownTags;

	int32 GetTargetCountForLevel() const;
	TArray<AActor*> BuildTargetChain(AActor* Avatar) const;
	void ApplyChainDamage(AActor* Target, float DamageScale);
	void SpawnChainVFX(AActor* Avatar, const TArray<AActor*>& Chain) const;
	FVector GetBeamSourceLocation(AActor* Avatar) const;
	FVector GetTargetVFXLocation(const AActor* Target) const;
};
