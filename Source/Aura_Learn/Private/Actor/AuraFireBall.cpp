#include "Actor/AuraFireBall.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "GameplayCueManager.h"
#include "AbilitySystem/AuraAbilitySystemBPLibary.h"
#include "Components/AudioComponent.h"

void AAuraFireBall::OnHit()
{
	if(IsValid(GetInstigator()))
	{
		/**
		 * 父类启用了网络复制 OnHit已经处于复制
		 * 如果想使用GC的方式播放效果 没必要使用GC的默认执行（GC会使用RPC），因此使用NonReplicated 执行效果
		 */
		FGameplayCueParameters GameplayCueParameters;
		GameplayCueParameters.Location = GetActorLocation();
		UGameplayCueManager::ExecuteGameplayCue_NonReplicated(GetInstigator(),FAuraGmaeplayTags::GetInstance().GameplayCue_FireBlast, GameplayCueParameters);
	}
	if (IsValid(ProjectileLoopSoundCmpt))
	{
		ProjectileLoopSoundCmpt->Stop();
		ProjectileLoopSoundCmpt->DestroyComponent();
	}

	bHit = true;
}

void AAuraFireBall::BeginPlay()
{
	Super::BeginPlay();
}

void AAuraFireBall::OnSphereOverlap(UPrimitiveComponent* OverlapPrimitiveComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                    bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValidOverlay(OtherActor))return;

	if (HasAuthority())
	{
		if (auto TargetAsc = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
		{
			const FVector DeathImpulse = GetActorForwardVector() * DamageEffectParams.DeathImpulseMagnitude;
			DamageEffectParams.DeathImpulseVectro = DeathImpulse;
			DamageEffectParams.TargetAbilitySystemComponent = TargetAsc;

			//击退
			const bool bKnockback = FMath::RandRange(1, 100) < DamageEffectParams.KnockbackChance;
			if (bKnockback)
			{
				FRotator Rotation = GetActorRotation();
				Rotation.Pitch = 45.f; //向量些微偏转，让敌人小飞

				const FVector KnockbackDirection = Rotation.Vector();
				const FVector KnockbackForce = KnockbackDirection * DamageEffectParams.KnockbackForceMagnitude;
				DamageEffectParams.KnockbackForce = KnockbackForce;
			}

			UAuraAbilitySystemBPLibary::ApplyDamageEffect(DamageEffectParams);
		}

	}
}
