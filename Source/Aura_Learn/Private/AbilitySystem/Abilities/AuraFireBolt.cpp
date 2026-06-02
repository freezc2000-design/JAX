#include "AbilitySystem/Abilities/AuraFireBolt.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemBPLibary.h"
#include "Actor/AuraProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"

void UAuraFireBolt::SpawnProjectiles(const FVector& ProjectileTargetLocation, bool bOverridePitch, float PitchOverride, AActor* HomingTarget)
{
	//单人游戏或者是在服务器
	const bool bIsServer = GetAvatarActorFromActorInfo()->HasAuthority();
	if (!bIsServer)return;

	auto CombatInterface = Cast<ICombatInterface>(GetAvatarActorFromActorInfo());
	if (!CombatInterface) return;

	const auto SocketLocation = ICombatInterface::Execute_GetCombatSocktLocation
	(GetAvatarActorFromActorInfo(), FAuraGmaeplayTags::GetInstance().CombatSocket_Weapon);

	FRotator  Rotation = (ProjectileTargetLocation - SocketLocation).Rotation();//获取两个位置的旋转差
	//如果想用新的pitch旋转
	if (bOverridePitch)
	{
		Rotation.Pitch = PitchOverride;
	}
	
	const FVector Forward = Rotation.Vector();
	const FVector Left = Forward.RotateAngleAxis(-ProjectileSpread / 2.f, FVector::UpVector);
	const FVector Right = Forward.RotateAngleAxis(ProjectileSpread / 2.f, FVector::UpVector);

	int16 CurretnNum = FMath::Min(GetAbilityLevel(), NumProjectiles);

	const auto Rotators = UAuraAbilitySystemBPLibary::EvenlySpeacedRotators(Forward, FVector::UpVector, ProjectileSpread, CurretnNum);

	FTransform SpawnTransform;

	SpawnTransform.SetLocation(SocketLocation);

	for(const auto& Rotator:Rotators)
	{

		SpawnTransform.SetRotation(Rotator.Quaternion());

		//延期生成 调用finish 完成生成
		auto ProjectfileActor = GetWorld()->SpawnActorDeferred<AAuraProjectile>(ProjectfileClass, SpawnTransform, GetOwningActorFromActorInfo(),
			Cast<APawn>(GetOwningActorFromActorInfo()),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		ProjectfileActor->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();

		if (HomingTarget&& HomingTarget->Implements<UCombatInterface>())
		{
			ProjectfileActor->HomingTargetSceneCmpt = HomingTarget->GetRootComponent();
			ProjectfileActor->ProjectileMovement->HomingTargetComponent = HomingTarget->GetRootComponent();//追踪目标为对象敌人

		}
		else
		{
			ProjectfileActor->HomingTargetSceneCmpt = NewObject<USceneComponent>(USceneComponent::StaticClass());//主要是想new的对象被计入GC，因此寄托在飞射物
			ProjectfileActor->HomingTargetSceneCmpt->SetWorldLocation(ProjectileTargetLocation);
			ProjectfileActor->ProjectileMovement->HomingTargetComponent = ProjectfileActor->HomingTargetSceneCmpt;
		}

		ProjectfileActor->ProjectileMovement->HomingAccelerationMagnitude = FMath::RandRange(HomingMinSpeed,HomingMaxSpeed);//追寻目标的速度
		ProjectfileActor->ProjectileMovement->bIsHomingProjectile = bLaunchHoming;

		ProjectfileActor->HomingTargetLocation = ProjectileTargetLocation;

		ProjectfileActor->FinishSpawning(SpawnTransform);
	}


}
