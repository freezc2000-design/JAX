// 学习使用

#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemBPLibary.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Interaction/CombatInterface.h"
#include "Interaction/PlayerInterface.h"
#include "Player/AuraPlayerController.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UAuraAttributeSet::UAuraAttributeSet()
{
	InitHealth(GetMaxHealth());
	InitMana(GetMaxMana());

	auto GameplayTags = FAuraGmaeplayTags::GetInstance();

	/* TagsToAttributes添加回调函数 被FAuraAttributeInfo中AttributeGetter 替代*/
}


void UAuraAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}

	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
	}
	
}

void UAuraAttributeSet::SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props)
{
	Props.EffectContextHandle = Data.EffectSpec.GetContext();
	//Source= GE 发出者 , Target= 应用GE的对象
	Props.SourceASC = Props.EffectContextHandle.GetOriginalInstigatorAbilitySystemComponent();
	if (IsValid(Props.SourceASC) && Props.SourceASC->AbilityActorInfo.IsValid() && Props.SourceASC->AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.SourceAvatarActor = Props.SourceASC->AbilityActorInfo->AvatarActor.Get();
		Props.SourceController = Props.SourceASC->AbilityActorInfo->PlayerController.Get();
		if(Props.SourceAvatarActor!=nullptr&& Props.SourceController==nullptr)
		{
			Props.SourceController = (Cast<APawn>(Props.SourceAvatarActor))->GetController();
		}

		if(IsValid(Props.SourceController))
		{
			Props.SourceCharacter = Cast<ACharacter>(Props.SourceController->GetPawn());
		}
	}

	Props.TargetASC = Data.Target;
	if (IsValid(Props.TargetASC) && Props.TargetASC->AbilityActorInfo.IsValid() 
		&& Props.TargetASC->AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.TargetAvatarActor = Props.TargetASC->AbilityActorInfo->AvatarActor.Get();
		Props.TargetController = Props.TargetASC->AbilityActorInfo->PlayerController.Get();
		Props.TargetCharacter = Cast<ACharacter>(Props.TargetAvatarActor);
	}
}

void UAuraAttributeSet::ShowFloatingText(const FEffectProperties& Prop, float Damage, const bool bBlocked, const bool bCriticalHit)
{

	if (Prop.SourceCharacter != Prop.TargetCharacter)
	{
		if (auto PlayCtrlSource = Cast<AAuraPlayerController>(Prop.SourceCharacter->GetController()))//要注意只让客户端执行
		{
			PlayCtrlSource->ShowDamage(Damage, Prop.TargetCharacter, bBlocked, bCriticalHit);
		}else if (auto PlayCtrlTarget = Cast<AAuraPlayerController>(Prop.TargetCharacter->GetController()))//玩家被攻击也显示
		{
			PlayCtrlTarget->ShowDamage(Damage, Prop.TargetCharacter, bBlocked, bCriticalHit);
		}
	}
}

void UAuraAttributeSet::SendXPEvent(const FEffectProperties& Prop)
{
	auto CombatInterface = Cast<ICombatInterface>(Prop.TargetCharacter);
	if (!CombatInterface)return;
	const auto XPReward = UAuraAbilitySystemBPLibary::
		GetXpRewardForClassAndLevel(Prop.TargetCharacter,
		                            ICombatInterface::Execute_GetCharacterClassType(Prop.TargetCharacter),
		                            ICombatInterface::Execute_GetPlayerLevel(Prop.TargetCharacter));

	//数据装载
	FGameplayEventData Payload;
	Payload.EventTag = FAuraGmaeplayTags::GetInstance().Attributes_Meta_IncomingXP;
	Payload.EventMagnitude = XPReward;

	//向等待事件tag的任务出发出通知
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Prop.SourceAvatarActor, FAuraGmaeplayTags::GetInstance().Attributes_Meta_IncomingXP, Payload);
}

void UAuraAttributeSet::HandleIncomingDamage(const FEffectProperties& Props)
{
	//伤害值元属性 让当前生命值减伤害值
	const auto LocalIncomingDamage = GetIncomingDamage();
	SetIncomingDamage(0.f);

	if (LocalIncomingDamage > 0)
	{
		const float NewHelath = FMath::Max(0.f, GetHealth() - LocalIncomingDamage);
		SetHealth(NewHelath);

		const bool bFatal = NewHelath <= 0.f; //挂了吗

		if (!bFatal)
		{
			/* 受击反应 
			 *  由AuraDamageabilityBase 中 MakeEffectParams 时已经进行击退应用计算
			 *  如果想不受影响 也可以调用库函数设置 但是首先应该考虑在编辑器中将GA取消应用 受击反应
			 */
			if(UAuraAbilitySystemBPLibary::IsActiveHitReact(Props.EffectContextHandle))
			{
				FGameplayTagContainer ActiveTags;
				ActiveTags.AddTag(FAuraGmaeplayTags::GetInstance().EffectHitReact);
				Props.TargetASC->TryActivateAbilitiesByTag(ActiveTags);

				const FVector& KnockbackForce = UAuraAbilitySystemBPLibary::GetKnockbackForce(Props.EffectContextHandle);
				if (!KnockbackForce.IsNearlyZero(1.f))
				{
					Props.TargetCharacter->LaunchCharacter(KnockbackForce, true, true);
				}

			}

		}
		else
		{

			//死亡
			if (auto CombatInterface = Cast<ICombatInterface>(Props.TargetAvatarActor))
			{
				CombatInterface->Die(UAuraAbilitySystemBPLibary::GetDeathImpulse(Props.EffectContextHandle));
			}
			SendXPEvent(Props);
		}

		if(UAuraAbilitySystemBPLibary::IsSuccessfulDebuff(Props.EffectContextHandle))
		{
			Debuff(Props);
		}

		/* 通过timer 处理飘字 */
		AccumulatedDamage += LocalIncomingDamage;// 累加伤害
		if (!GetWorld()->GetTimerManager().IsTimerActive(FloatingTextTimerHandle))// 如果计时器未启动，启动一个0.1秒的延迟调用
		{
			const bool bBlockedHit = UAuraAbilitySystemBPLibary::IsBlockedHit(Props.EffectContextHandle);
			const bool bCriticalHit = UAuraAbilitySystemBPLibary::IsCriticalHit(Props.EffectContextHandle);

			GetWorld()->GetTimerManager().SetTimer(FloatingTextTimerHandle, FTimerDelegate::CreateUObject(this, &UAuraAttributeSet::OnShowFloatingText, Props, bBlockedHit, bCriticalHit), 0.1f, false);
		}
	}
}

void UAuraAttributeSet::HandleIncomingXP(const FEffectProperties& Props)
{
	const float LocalIncomingXP = GetIncomingXP();
	SetIncomingXP(0.f);

	//SourceActor为拥有者，从GA_ListenForEvent 应用 GE_EventBasedEffect , 添加 到 IncomingXP
	if(Props.SourceAvatarActor->Implements<UPlayerInterface>())
	{

		//检测升级
		const int32 CurrentXP = IPlayerInterface::Execute_GetXP(Props.SourceAvatarActor);
		const auto CurrentLevel = ICombatInterface::Execute_GetPlayerLevel(Props.SourceAvatarActor);
		const auto NewLevel=IPlayerInterface::Execute_FindLevelFromXP(Props.SourceAvatarActor, CurrentXP+LocalIncomingXP);

		int8 LevelUpNum = NewLevel - CurrentLevel;
		//升级
		for (; LevelUpNum > 0; LevelUpNum--)
		{

			//升级奖励赋予
			IPlayerInterface::Execute_AddToAttributePoints(Props.SourceAvatarActor, IPlayerInterface::Execute_GetAttributePointReward(Props.SourceAvatarActor,ICombatInterface::Execute_GetPlayerLevel(Props.SourceAvatarActor)));
			IPlayerInterface::Execute_AddToSpellPoints(Props.SourceAvatarActor, IPlayerInterface::Execute_GetSpellPointReward(Props.SourceAvatarActor, ICombatInterface::Execute_GetPlayerLevel(Props.SourceAvatarActor)));

			IPlayerInterface::Execute_LevelUp(Props.SourceAvatarActor);

			//需要刷新的状态 PostAttributeChange 进行刷新
			bTopOffHealth = true;
			bTopOffMana = true;

		}
		IPlayerInterface::Execute_AddToXP(Props.SourceAvatarActor, LocalIncomingXP);
	}
}

void UAuraAttributeSet::Debuff(const FEffectProperties& Props)
{
	auto GEContenxt = Props.SourceASC->MakeEffectContext();
	GEContenxt.AddSourceObject(Props.SourceAvatarActor);

	const auto DamageTypeTag = UAuraAbilitySystemBPLibary::GetDamageType(Props.EffectContextHandle);
	FString DebuffName = FString::Printf(TEXT("DebuffGE_%s"), *DamageTypeTag.ToString());

	/*
	 *  UE 所有对象都基于UObject，这些对象必须归属于一个包（UPackage）。包类似于一个容器，负责组织和管理对象。
	 *	GetTransientPackage() 返回一个临时的 UPackage,来放置那些不需要被保存的临时对象。
	 *	这些对象的生命周期通常仅限于游戏运行时，它们不需要被保存到硬盘中，也不需要与关卡数据等一起持久化。
	 */
	auto GE = NewObject<UGameplayEffect>(GetTransientPackage(), FName(DebuffName));

	//设置时长和周期
	GE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	const float DebuffDuration = UAuraAbilitySystemBPLibary::GetDebuffDuration(Props.EffectContextHandle);
	const float DebuffFrequency = UAuraAbilitySystemBPLibary::GetDebuffFrequency(Props.EffectContextHandle);
	GE->Period = DebuffFrequency;
	GE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(DebuffDuration));

	//添加Debuff 标签
	FInheritedTagContainer TagContainer = FInheritedTagContainer();
	UTargetTagsGameplayEffectComponent& TargetTagsComponent = GE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	auto DeBuffTag = FAuraGmaeplayTags::GetInstance().DamageTypesToDebuffs[DamageTypeTag];
	TagContainer.Added.AddTag(DeBuffTag);

	//额外效果

	//眩晕 
	if(DeBuffTag.MatchesTagExact(FAuraGmaeplayTags::GetInstance().Debuff_Stun))
	{
		//禁用输入 通过上标签的方式
		TagContainer.Added.AddTag(FAuraGmaeplayTags::GetInstance().Player_Block_CursorTrace);
		TagContainer.Added.AddTag(FAuraGmaeplayTags::GetInstance().Player_Block_InputHeld);
		TagContainer.Added.AddTag(FAuraGmaeplayTags::GetInstance().Player_Block_InputPressed);
		TagContainer.Added.AddTag(FAuraGmaeplayTags::GetInstance().Player_Block_InputReleased);
	}

	TargetTagsComponent.SetAndApplyTargetTagChanges(TagContainer);

	//设置堆叠聚合
	GE->StackingType = EGameplayEffectStackingType::AggregateBySource;
	GE->StackLimitCount = 1;

	//向这个GE最后添加伤害修饰器
	GE->Modifiers.Add(FGameplayModifierInfo());
	auto& ModifierInfo = GE->Modifiers.Last();
	const float DebuffDamage = UAuraAbilitySystemBPLibary::GetDebuffDamage(Props.EffectContextHandle);
	ModifierInfo.ModifierMagnitude = FScalableFloat(DebuffDamage);
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;
	ModifierInfo.Attribute = UAuraAttributeSet::GetIncomingDamageAttribute();//设置目标属性为即将到来的伤害

	if(auto MutableSpec = new FGameplayEffectSpec(GE, GEContenxt, 1.f))
	{
		if(auto AuraGEContext = static_cast<FAuraGameEffectContext*>(MutableSpec->GetContext().Get()))
		{
			TSharedPtr<FGameplayTag> DebuffDamageType = MakeShareable(new FGameplayTag(DamageTypeTag));//创建指向已存在对象的 TSharedPtr
			AuraGEContext->SetDamageType(DebuffDamageType);
			AuraGEContext->SetHitReact(false);
			Props.TargetASC->ApplyGameplayEffectSpecToSelf(*MutableSpec);
		}
	}
}

void UAuraAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	FEffectProperties Props;
	SetEffectProperties(Data, Props);

	if(Props.TargetCharacter->Implements<UCombatInterface>())
	{
		if(ICombatInterface::Execute_IsDead(Props.TargetCharacter))
		{

			return;
		}
	}

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		HandleIncomingDamage(Props);
	}

	if (Data.EvaluatedData.Attribute == GetIncomingXPAttribute())
	{
		HandleIncomingXP(Props);
	}
}

void UAuraAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	if (Attribute == GetMaxHealthAttribute() && bTopOffHealth)
	{
		SetHealth(GetMaxHealth());
		bTopOffHealth = false;
	}
	else if (Attribute == GetMaxManaAttribute() && bTopOffMana)
	{
		SetMana(GetMaxMana());
		bTopOffMana = false;
	}
}

void UAuraAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//指定属性的网络复制、条件复制以及通知机制
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Health, COND_None, REPNOTIFY_Always) //对该类的该变量无条件复制，并且每次都触发通知
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, MaxMana, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Mana, COND_None, REPNOTIFY_Always)

	/*RPG 拓展属性*/
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Intelligence, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Strength, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Resilience, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Vigor, COND_None, REPNOTIFY_Always)

	/*次要属性*/
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, ArmorPenetration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, BlockChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, CriticalHitChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, CriticalHitDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, CriticalHitResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, HealthRegeneration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, ManaRegeneration, COND_None, REPNOTIFY_Always);

	/*抗性*/
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, FireResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, LightningResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, ArcaneResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, PhysicalResistance, COND_None, REPNOTIFY_Always);
}

/*
 * 该宏在属性变化时自动通知客户端，并处理属性的网络复制。 CPP 仅定义减工作量
 *
 */
#define ATTRIBUTE_ONREP(ClassName, PropertyName) \
void ClassName::OnRep_##PropertyName(const FGameplayAttributeData& OldValue) const \
{ \
GAMEPLAYATTRIBUTE_REPNOTIFY(ClassName, PropertyName, OldValue); \
}

ATTRIBUTE_ONREP(UAuraAttributeSet, Health)
ATTRIBUTE_ONREP(UAuraAttributeSet, Mana)
ATTRIBUTE_ONREP(UAuraAttributeSet, MaxHealth)
ATTRIBUTE_ONREP(UAuraAttributeSet, MaxMana)

ATTRIBUTE_ONREP(UAuraAttributeSet, Intelligence)
ATTRIBUTE_ONREP(UAuraAttributeSet, Strength)
ATTRIBUTE_ONREP(UAuraAttributeSet, Resilience)
ATTRIBUTE_ONREP(UAuraAttributeSet, Vigor)

ATTRIBUTE_ONREP(UAuraAttributeSet, Armor)
ATTRIBUTE_ONREP(UAuraAttributeSet, ArmorPenetration)
ATTRIBUTE_ONREP(UAuraAttributeSet, BlockChance)
ATTRIBUTE_ONREP(UAuraAttributeSet, CriticalHitChance)
ATTRIBUTE_ONREP(UAuraAttributeSet, CriticalHitDamage)
ATTRIBUTE_ONREP(UAuraAttributeSet, CriticalHitResistance)
ATTRIBUTE_ONREP(UAuraAttributeSet, HealthRegeneration)
ATTRIBUTE_ONREP(UAuraAttributeSet, ManaRegeneration)

ATTRIBUTE_ONREP(UAuraAttributeSet, FireResistance)
ATTRIBUTE_ONREP(UAuraAttributeSet, LightningResistance)
ATTRIBUTE_ONREP(UAuraAttributeSet, ArcaneResistance)
ATTRIBUTE_ONREP(UAuraAttributeSet, PhysicalResistance)

void UAuraAttributeSet::OnShowFloatingText(FEffectProperties Props, bool bBlockedHit, bool bCriticalHit)
{
	// 调用显示浮动伤害文本的函数
	ShowFloatingText(Props, AccumulatedDamage, bBlockedHit, bCriticalHit);

	// 重置累加的伤害值
	AccumulatedDamage = 0.f;
}

