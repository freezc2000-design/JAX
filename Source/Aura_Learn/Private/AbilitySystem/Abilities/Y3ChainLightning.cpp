#include "AbilitySystem/Abilities/Y3ChainLightning.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemBPLibary.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Interaction/CombatInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

UY3ChainLightning::UY3ChainLightning()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UY3ChainLightning::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	TArray<AActor*> TargetChain;
	if (IsValid(Avatar) && Avatar->HasAuthority())
	{
		TargetChain = BuildTargetChain(Avatar);
		if (TargetChain.Num() == 0)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (IsValid(Avatar) && Avatar->HasAuthority())
	{
		float DamageScale = 1.f;
		for (AActor* Target : TargetChain)
		{
			ApplyChainDamage(Target, DamageScale);
			DamageScale *= FMath::Clamp(DamageFalloffPerBounce, 0.f, 1.f);
		}
		SpawnChainVFX(Avatar, TargetChain);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

FString UY3ChainLightning::GetDescription(const UAuraAbilitySystemComponent* AuraGAS,
	const FGameplayTag& GATag,
	const int32 Level)
{
	const int32 Damage = FMath::RoundToInt(GetDamageByDamageType(FAuraGmaeplayTags::GetInstance().Damage_Lightning, Level));
	const int32 TargetCount = FMath::Clamp(BaseTargetCount + (FMath::Max(1, Level) - 1) * TargetsPerLevel, 1, FMath::Max(1, MaxTargetCount));
	const int32 FalloffPercent = FMath::RoundToInt(FMath::Clamp(DamageFalloffPerBounce, 0.f, 1.f) * 100.f);
	const FString ManaCost = FString::Printf(TEXT("%.1f"), GetManaCost(Level));
	const FString Cooldown = FString::Printf(TEXT("%.1f"), GetCooldown(Level));

	return FString::Printf(TEXT("<Title>闪电链</>\n<Small>法力消耗: %s  冷却: %s秒</>\n<Default>释放一道闪电命中最近敌人，并在附近敌人之间弹射。</>\n<Damage>伤害: %d</>\n<Default>最多命中 %d 个目标，每次弹射保留 %d%% 伤害。</>"),
		*ManaCost,
		*Cooldown,
		Damage,
		TargetCount,
		FalloffPercent);
}

const FGameplayTagContainer* UY3ChainLightning::GetCooldownTags() const
{
	ChainLightningCooldownTags.Reset();
	ChainLightningCooldownTags.AddTag(FAuraGmaeplayTags::GetInstance().Cooldown_Lightning_ChainLightning);
	return &ChainLightningCooldownTags;
}

void UY3ChainLightning::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownGE->GetClass(), GetAbilityLevel());
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->DynamicGrantedTags.AddTag(FAuraGmaeplayTags::GetInstance().Cooldown_Lightning_ChainLightning);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}

int32 UY3ChainLightning::GetTargetCountForLevel() const
{
	const int32 Level = FMath::Max(1, GetAbilityLevel());
	return FMath::Clamp(BaseTargetCount + (Level - 1) * TargetsPerLevel, 1, FMath::Max(1, MaxTargetCount));
}

TArray<AActor*> UY3ChainLightning::BuildTargetChain(AActor* Avatar) const
{
	TArray<AActor*> Chain;
	if (!IsValid(Avatar))
	{
		return Chain;
	}

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(Avatar);

	TArray<AActor*> Candidates;
	UAuraAbilitySystemBPLibary::GetLivePlayersWithRadius(Avatar, Candidates, IgnoreActors, Avatar->GetActorLocation(), AcquireRadius);

	TArray<AActor*> ClosestTargets;
	UAuraAbilitySystemBPLibary::GetClosesTarget(Avatar->GetActorLocation(), 1, Candidates, ClosestTargets);
	if (ClosestTargets.Num() == 0 || !IsValid(ClosestTargets[0]))
	{
		return Chain;
	}

	Chain.Add(ClosestTargets[0]);

	const int32 DesiredTargetCount = GetTargetCountForLevel();
	while (Chain.Num() < DesiredTargetCount)
	{
		AActor* CurrentTarget = Chain.Last();
		if (!IsValid(CurrentTarget))
		{
			break;
		}

		IgnoreActors.Reset();
		IgnoreActors.Add(Avatar);
		IgnoreActors.Append(Chain);

		Candidates.Reset();
		UAuraAbilitySystemBPLibary::GetLivePlayersWithRadius(Avatar, Candidates, IgnoreActors, CurrentTarget->GetActorLocation(), BounceRadius);

		ClosestTargets.Reset();
		UAuraAbilitySystemBPLibary::GetClosesTarget(CurrentTarget->GetActorLocation(), 1, Candidates, ClosestTargets);
		if (ClosestTargets.Num() == 0 || !IsValid(ClosestTargets[0]))
		{
			break;
		}

		Chain.Add(ClosestTargets[0]);
	}

	return Chain;
}

void UY3ChainLightning::ApplyChainDamage(AActor* Target, float DamageScale)
{
	if (!IsValid(Target))
	{
		return;
	}

	FDamageEffectParams DamageParams = MakeDamageEffectParamsFromClassDefaults(Target);
	for (auto& Pair : DamageParams.DebuffMapGEParams)
	{
		Pair.Value.BaseDamage *= DamageScale;
		Pair.Value.DebuffDamage *= DamageScale;
	}

	UAuraAbilitySystemBPLibary::ApplyDamageEffect(DamageParams);
}

void UY3ChainLightning::SpawnChainVFX(AActor* Avatar, const TArray<AActor*>& Chain) const
{
	if (Chain.Num() == 0)
	{
		return;
	}

	FVector BeamStart = GetBeamSourceLocation(Avatar);
	for (AActor* Target : Chain)
	{
		if (!IsValid(Target))
		{
			continue;
		}

		const FVector BeamEnd = GetTargetVFXLocation(Target);
		if (BeamFX)
		{
			UNiagaraComponent* BeamComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				this,
				BeamFX,
				BeamStart,
				FRotator::ZeroRotator,
				FVector(1.f),
				true,
				false);
			if (BeamComponent)
			{
				BeamComponent->SetNiagaraVariableVec3(TEXT("User.Beam Start"), BeamStart);
				BeamComponent->SetNiagaraVariableVec3(TEXT("User.Beam End"), BeamEnd);
				BeamComponent->Activate(true);
			}
		}

		if (ImpactFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactFX, BeamEnd);
		}

		BeamStart = BeamEnd;
	}
}

FVector UY3ChainLightning::GetBeamSourceLocation(AActor* Avatar) const
{
	if (IsValid(Avatar) && Avatar->Implements<UCombatInterface>())
	{
		if (USceneComponent* Weapon = ICombatInterface::Execute_GetWeapon(Avatar))
		{
			return Weapon->GetSocketLocation(FName("TipSocket"));
		}
	}

	return IsValid(Avatar) ? Avatar->GetActorLocation() + FVector(0.f, 0.f, TargetHeightOffset) : FVector::ZeroVector;
}

FVector UY3ChainLightning::GetTargetVFXLocation(const AActor* Target) const
{
	return IsValid(Target) ? Target->GetActorLocation() + FVector(0.f, 0.f, TargetHeightOffset) : FVector::ZeroVector;
}
