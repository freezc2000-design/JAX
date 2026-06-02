#include "Actor/AuraProjectile.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "AbilitySystem/AuraAbilitySystemBPLibary.h"
#include "Aura_Learn/Aura_Learn.h"
#include "Components/AudioComponent.h"
#include "Net/UnrealNetwork.h"

AAuraProjectile::AAuraProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	SetRootComponent(Sphere);

	//仅查询 仅对 场景动态 场景静态 pawn 检测
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	Sphere->SetCollisionObjectType(ECC_Projectile);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	ProjectileMovement->InitialSpeed = 550.f;
	ProjectileMovement->MaxSpeed = 550.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;//没有重力
}

void AAuraProjectile::Destroyed()
{
	if (IsValid(ProjectileLoopSoundCmpt))
	{
		ProjectileLoopSoundCmpt->Stop();
		ProjectileLoopSoundCmpt->DestroyComponent();
	}

	if(!bHit&&!HasAuthority())//如果客户端没有应用击中产生的效果，那么此时需要应用
	{
		OnHit();
	}
	
	Super::Destroyed();
}

void AAuraProjectile::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(LifeSpan);

	if (bHomingProjectile)
	{
		ProjectileMovement->HomingAccelerationMagnitude = HomingAccelerationMagnitude;
		ProjectileMovement->bIsHomingProjectile = bHomingProjectile;

		//无效制导对象，手动创建一个
		if (IsValid(HomingTargetSceneCmpt))
		{
			HomingTargetSceneCmpt = NewObject<USceneComponent>(USceneComponent::StaticClass());
			HomingTargetSceneCmpt->SetWorldLocation(HomingTargetLocation);
			ProjectileMovement->HomingTargetComponent = HomingTargetSceneCmpt;
		}
		ProjectileMovement->HomingTargetComponent = HomingTargetSceneCmpt;
	}
	else
	{
		SetReplicateMovement(true);
	}

	ProjectileLoopSoundCmpt =UGameplayStatics::SpawnSoundAttached(ProjectileSound, RootComponent);
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &AAuraProjectile::OnSphereOverlap);
	
}

void AAuraProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAuraProjectile, HomingTargetSceneCmpt);
	DOREPLIFETIME(AAuraProjectile, bHomingProjectile);
	DOREPLIFETIME(AAuraProjectile, HomingAccelerationMagnitude);
	DOREPLIFETIME(AAuraProjectile, HomingTargetLocation);
}

void AAuraProjectile::OnSphereOverlap(UPrimitiveComponent* OverlapPrimitiveComponent, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValidOverlay(OtherActor))return;

	if(!bHit)//防止服务端已经击中，生成音效 复制给客户端时 客户端还生成音效
	{
		OnHit();
	}

	if(HasAuthority())
	{
		if(auto TargetAsc=UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
		{
			const FVector DeathImpulse = GetActorForwardVector() * DamageEffectParams.DeathImpulseMagnitude;
			DamageEffectParams.DeathImpulseVectro = DeathImpulse;
			DamageEffectParams.TargetAbilitySystemComponent = TargetAsc;

			//击退
			const bool bKnockback = FMath::RandRange(1, 100) < DamageEffectParams.KnockbackChance;
			if (bKnockback)
			{
				FRotator Rotation = GetActorRotation();
				Rotation.Pitch = 45.f;//向量些微偏转，让敌人小飞

				const FVector KnockbackDirection = Rotation.Vector();
				const FVector KnockbackForce = KnockbackDirection * DamageEffectParams.KnockbackForceMagnitude;
				DamageEffectParams.KnockbackForce = KnockbackForce;
			}

			UAuraAbilitySystemBPLibary::ApplyDamageEffect(DamageEffectParams);
		}

		Destroy();//服务器上销毁这个

	}else
	{
		bHit = true;//在服务端销毁前 客户端 可能还没有触发到这个事件， 因此标记一下已经完成触发，后续不再触发 
	}
}

void AAuraProjectile::OnHit()
{
	UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation(), FRotator::ZeroRotator);
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, GetActorLocation());
	if (IsValid(ProjectileLoopSoundCmpt))
	{
		ProjectileLoopSoundCmpt->Stop();
		ProjectileLoopSoundCmpt->DestroyComponent();
	}
	bHit = true;
}

bool AAuraProjectile::IsValidOverlay(AActor* OtherActor) const
{
	if (!IsValid(DamageEffectParams.SourceAbilitySystemComponent)) return false;

	const AActor* SourceAvatarActor = DamageEffectParams.SourceAbilitySystemComponent->GetAvatarActor();

	if (SourceAvatarActor == OtherActor)return false;//仅客户端运行会打中自己

	if (!UAuraAbilitySystemBPLibary::IsNotFriend(OtherActor, SourceAvatarActor, "Enemy"))return false;

	return true;
}

