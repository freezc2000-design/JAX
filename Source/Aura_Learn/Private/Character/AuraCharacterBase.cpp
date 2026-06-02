// 学习使用
#include "Character/AuraCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Debuff/DebuffNiagaraComponent.h"
#include "AbilitySystem/Passive/PassiveNiagaraComponent.h"
#include "Aura_Learn/Aura_Learn.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/AuraPlayerController.h"

AAuraCharacterBase::AAuraCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false; 
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon"));

	//武器设置
	Weapon->SetupAttachment(GetMesh(), FName("WeaponHandSocket"));//武器绑定到武器插槽
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);//关闭武器碰撞

	GetCapsuleComponent()->SetGenerateOverlapEvents(false);

	GetMesh()->SetCollisionResponseToChannel(ECC_Projectile, ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);//生成重叠碰撞检测事件

	//debuff组件
	BurnDebuffComponent = CreateDefaultSubobject<UDebuffNiagaraComponent>("BurnDebuffComponent");
	BurnDebuffComponent->SetupAttachment(RootComponent);
	BurnDebuffComponent->DebuffTag = FAuraGmaeplayTags::GetInstance().Debuff_Burn;

	StunDebuffComponent = CreateDefaultSubobject<UDebuffNiagaraComponent>("StunDebuffComponent");
	StunDebuffComponent->SetupAttachment(RootComponent);
	StunDebuffComponent->DebuffTag = FAuraGmaeplayTags::GetInstance().Debuff_Stun;

	/* 被动技能特效 */
	EffectAttachComponent = CreateDefaultSubobject<USceneComponent>("EffectAttachPoint");
	EffectAttachComponent->SetupAttachment(GetRootComponent());

	//不想让特效套件槽 旋转 设置其旋转绝对 旋转为0
	EffectAttachComponent->SetUsingAbsoluteRotation(true);
	EffectAttachComponent->SetWorldRotation(FRotator::ZeroRotator);

	HaloOfProtectionNiagaraComponent = CreateDefaultSubobject<UPassiveNiagaraComponent>("HaloOfProtectionComponent");
	HaloOfProtectionNiagaraComponent->SetupAttachment(EffectAttachComponent);
	LifeSiphonNiagaraComponent = CreateDefaultSubobject<UPassiveNiagaraComponent>("LifeSiphonNiagaraComponent");
	LifeSiphonNiagaraComponent->SetupAttachment(EffectAttachComponent);
	ManaSiphonNiagaraComponent = CreateDefaultSubobject<UPassiveNiagaraComponent>("ManaSiphonNiagaraComponent");
	ManaSiphonNiagaraComponent->SetupAttachment(EffectAttachComponent);

}

void AAuraCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAuraCharacterBase,bStun)
	DOREPLIFETIME(AAuraCharacterBase, bIsBurned)
	DOREPLIFETIME(AAuraCharacterBase, bIsBeingShoked)
}

UAbilitySystemComponent* AAuraCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

FVector AAuraCharacterBase::GetCombatSocktLocation_Implementation(const FGameplayTag& AttackMontageTag)
{
	if (AttackMontageTag.MatchesTagExact(FAuraGmaeplayTags::GetInstance().CombatSocket_Weapon))
	{
		if (IsValid(Weapon))
		{
			return Weapon->GetSocketLocation(WeaponTipSocketName);
		}
	}
	else if (AttackMontageTag.MatchesTagExact(FAuraGmaeplayTags::GetInstance().CombatSocket_LeftHand))
	{
		return GetMesh()->GetSocketLocation(LeftHandSocketName);
	}
	else if (AttackMontageTag.MatchesTagExact(FAuraGmaeplayTags::GetInstance().CombatSocket_RightHand))
	{
		return GetMesh()->GetSocketLocation(RightHandTipSocketName);
	}
	else if(AttackMontageTag.MatchesTagExact(FAuraGmaeplayTags::GetInstance().CombatSocket_Tail))
	{
		return GetMesh()->GetSocketLocation(TailSocketName);
	}
	return FVector();
}

FHitResult* AAuraCharacterBase::GetCursorHitRes()
{
	return &(Cast<AAuraPlayerController>(GetController())->CursorHit);
}

UAnimMontage* AAuraCharacterBase::GetHitRecatMontag_Implementation() const
{
	return HitReactMontage;
}

bool AAuraCharacterBase::IsDead_Implementation() const
{
	return bDead;
}

AActor* AAuraCharacterBase::GetAvatar_Implementation()
{
	return this;
}

UNiagaraSystem* AAuraCharacterBase::GetBloodEffect_Implementation() const
{
	return BloodEffect;
}

FTaggedMontage AAuraCharacterBase::GetTaggedMontageByTag_Implementation(const FGameplayTag& MontageTag)
{
	for(const auto& it:AttackMontages)
	{
		if (it.MontageTag == MontageTag)return it;
	}
	return FTaggedMontage{};
}

int32 AAuraCharacterBase::GetMinionCount_Implementation()
{
	return MinionCount;
}

int32 AAuraCharacterBase::IncrementMinionCount_Implementation(int32 DeltaCount)
{
	MinionCount += DeltaCount;
	return MinionCount;
}

ECharacterClass AAuraCharacterBase::GetCharacterClassType_Implementation()
{
	return CharacterClass;
}

FOnASCRegistered& AAuraCharacterBase::GetOnASCRegisteredDel()
{
	return OnASCRegistered;
}

FOnDeathDel& AAuraCharacterBase::GetOnDeathDel()
{
	return OnSelfDead;
}

bool AAuraCharacterBase::IsHeroCharacter() const
{
	return bIsHero;
}

USkeletalMeshComponent* AAuraCharacterBase::GetWeapon_Implementation()
{
	return Weapon;
}

void AAuraCharacterBase::SetIsBeingShocked_Implementation(const bool ShockLoop)
{
	bIsBeingShoked= ShockLoop;
	if(ShockLoop)
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->MaxWalkSpeed = 0.f;
	}else
	{
		GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
	}
}

bool AAuraCharacterBase::IsBeingShocked_Implementation() const
{
	return bIsBeingShoked;
}


void AAuraCharacterBase::Die(const FVector& DeathImpulse)
{
	Weapon->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld,true));//分离武器

	MulticastHandleDeath(DeathImpulse);
}

TArray<FTaggedMontage> AAuraCharacterBase::GetAttackMontages_Implementation() const
{
	return AttackMontages;
}

void AAuraCharacterBase::MulticastHandleDeath_Implementation(const FVector& DeathImpulse)
{

	UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation(),GetActorRotation());

	//开启布娃娃
	Weapon->SetSimulatePhysics(true);
	Weapon->SetEnableGravity(true);
	Weapon->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);//仅物理通道碰撞
	Weapon->AddImpulse(DeathImpulse*0.1, NAME_None, true);

	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetEnableGravity(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	GetMesh()->AddImpulse(DeathImpulse,NAME_None,true);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	GetMesh()->SetGenerateOverlapEvents(false);
	Dissolve();

	//特效组件取消
	BurnDebuffComponent->Deactivate();
	StunDebuffComponent->Deactivate();

	bDead = true;
	OnSelfDead.Broadcast(this);
}

void AAuraCharacterBase::StunTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	bStun = NewCount > 0;
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->MaxWalkSpeed = bStun ? 0 : DefaultMaxWalkSpeed;
}

void AAuraCharacterBase::OnRep_Stunned()
{

}

void AAuraCharacterBase::OnRep_Burned()
{

}

void AAuraCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	DefaultMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
}

void AAuraCharacterBase::ApplyEffectToSelf(const TSubclassOf<UGameplayEffect>& InitializeGEClass, float Level) const
{
	if (!IsValid(InitializeGEClass) || !IsValid(AbilitySystemComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("DefaultSecondaryAtributes Not Setup Form AAuraCharacterBase!!!"))
		return;
	}
	auto InitGESpec = AbilitySystemComponent->MakeEffectContext();
	InitGESpec.AddSourceObject(this);
	auto InitGEHandle = AbilitySystemComponent->MakeOutgoingSpec(InitializeGEClass, Level, InitGESpec);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*InitGEHandle.Data.Get());
}

void AAuraCharacterBase::InitDefaultAttribute() const
{
	ApplyEffectToSelf(DefaultPrimaryAtributes);
	ApplyEffectToSelf(DefaultSecondaryAtributes);
	ApplyEffectToSelf(DefaultVitalAtributes);
}

void AAuraCharacterBase::AddCharacterAbilities()
{
	auto GAS = Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponent());
	if ((!HasAuthority())||!IsValid(GAS))return; //Actor是否有网络权限（就是是否为服务端的实例）
	GAS->AddCharacterAbilities(StartupAbilities);
	GAS->AddCharacterPassiveAbilities(StartupPassiveAbilities);
}

void AAuraCharacterBase::Dissolve()
{
	UMaterialInstanceDynamic* DynamicMatInst{ nullptr };
	if(IsValid(DissolveMaterialOfMesh))
	{
		DynamicMatInst = UMaterialInstanceDynamic::Create(DissolveMaterialOfMesh, this);
		GetMesh()->SetMaterial(0, DynamicMatInst);

		StartDissolveTimlineOfMesh(DynamicMatInst);
	}
	if(IsValid(DissolveMaterialOfWeapon))
	{
		DynamicMatInst = UMaterialInstanceDynamic::Create(DissolveMaterialOfWeapon, this);
		Weapon->SetMaterial(0, DynamicMatInst);

		StartDissolveTimlineOfWeapon(DynamicMatInst);
	}
}
