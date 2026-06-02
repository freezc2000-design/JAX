#include "AbilitySystem/Abilities/AuraFireBlast.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemBPLibary.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilitieDescriptions.h"
#include "Actor/AuraFireBall.h"

FString UAuraFireBlast::GetDescription(const UAuraAbilitySystemComponent* AuraGAS, const FGameplayTag& GATag,
                                       const int32 Level)
{
	if (!AuraGAS)return FString{};

	const int32 Damage = GetDamageByDamageType(FAuraGmaeplayTags::GetInstance().Damage_Fire, Level);
	FText CostofMana = FText::FromString(FString::Printf(TEXT("%.1f"), GetManaCost(Level)));
	FText CoolDown = FText::FromString(FString::Printf(TEXT("%.1f"), GetCooldown(Level)));

	auto Description = FText::Format(AuraGAS->AbilityDescriptions->FindDescriptionsByTagAndLevel(GATag, Level),
	                                 FMath::Min(Level, NumFireBalls), Damage, Level, CostofMana, CoolDown);
	return Description.ToString();
}

TArray<AAuraFireBall*> UAuraFireBlast::SpawnFireBall()
{
	TArray<AAuraFireBall*> FireBalls;

	auto Rotators = UAuraAbilitySystemBPLibary::EvenlySpeacedRotators(
		GetAvatarActorFromActorInfo()->GetActorForwardVector(), FVector::UpVector, 360.f, NumFireBalls);

	FTransform Transform;
	Transform.SetLocation(GetAvatarActorFromActorInfo()->GetActorLocation());
	for (const auto& Rotator : Rotators)
	{
		Transform.SetRotation(Rotator.Quaternion());

		auto Fireball = GetWorld()->SpawnActorDeferred<AAuraFireBall>(
			FireballClass, Transform,
			GetOwningActorFromActorInfo(),
			CurrentActorInfo->PlayerController->GetPawn(),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		Fireball->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
		Fireball->ExplosionDamageParams = Fireball->DamageEffectParams;
		Fireball->CallerAbility = this;
		//重叠伤害不用径向和击退
		Fireball->DamageEffectParams.bIsRadialDamage = false;
		Fireball->DamageEffectParams.KnockbackChance = 0.f;

		FireBalls.Emplace(Fireball);

		Fireball->FinishSpawning(Transform);
	}

	return FireBalls;
}
