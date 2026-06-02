// 学习使用


#include "AuraGameplayTags.h"
#include "GameplayTagsManager.h"

FAuraGmaeplayTags& FAuraGmaeplayTags::GetInstance()
{
	static FAuraGmaeplayTags Instance{};
	return Instance;
}

void FAuraGmaeplayTags::InitNaviveGameplayTags()
{
	//Primary Attributes
	GetInstance().InitPrimaryAttributeTags();

	//Secondary Attributes
	GetInstance().InitSecondaryAttributeTags();

	/*输入*/
	GetInstance().InitInputTags();

	/*效果*/
	GetInstance().InitEffectTags();

	GetInstance().InitDamageTypesTags();

	/*技能*/
	GetInstance().InitSkillTags();
}

void FAuraGmaeplayTags::InitPrimaryAttributeTags()
{
	GetInstance().Attributes_Primary_Strength = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Primary.Strength"),
		FString("增加物理伤害")
	);

	GetInstance().Attributes_Primary_Intelligence = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Primary.Intelligence"),
		FString("增加魔法伤害")
	);

	GetInstance().Attributes_Primary_Resilience = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Primary.Resilience"),
		FString("增加护甲和护甲穿透")
	);

	GetInstance().Attributes_Primary_Vigor = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Primary.Vigor"),
		FString("增强血量")
	);
}

void FAuraGmaeplayTags::InitSecondaryAttributeTags()
{

	GetInstance().Attributes_Secondary_Armor = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.Armor"),
		FString("减少受到的伤害，提高格挡几率")
	);

	GetInstance().Attributes_Secondary_ArmorPenetration = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.ArmorPenetration"),
		FString("无视敌方百分比护甲，增加暴击几率")
	);

	GetInstance().Attributes_Secondary_BlockChance = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.BlockChance"),
		FString("有几率将受到的伤害减半")
	);

	GetInstance().Attributes_Secondary_CriticalHitChance = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.CriticalHitChance"),
		FString("有几率使伤害加倍并获得重击加成")
	);

	GetInstance().Attributes_Secondary_CriticalHitDamage = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.CriticalHitDamage"),
		FString("Bonus damage added when a critical hit is scored")
	);

	GetInstance().Attributes_Secondary_CriticalHitResistance = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.CriticalHitResistance"),
		FString("命中要害时增加额外伤害")
	);

	GetInstance().Attributes_Secondary_HealthRegeneration = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.HealthRegeneration"),
		FString("每 1 秒再生的健康值")
	);

	GetInstance().Attributes_Secondary_ManaRegeneration = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.ManaRegeneration"),
		FString("每 1 秒再生的法力值")
	);

	GetInstance().Attributes_Secondary_MaxHealth = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.MaxHealth"),
		FString("可获得的最大健康值")
	);

	GetInstance().Attributes_Secondary_MaxMana = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.MaxMana"),
		FString("可获得的最大法力值")
	);

	GetInstance().Attributes_Meta_IncomingXP = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Meta.IncomingXP"),
		FString("即将增加的经验")
	);
}

void FAuraGmaeplayTags::InitInputTags()
{
	GetInstance().InputTag = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag"),
		FString("输入")
	);

	GetInstance().InputTag_LMB = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.LMB"),
		FString("鼠标左键")
	);
	GetInstance().InputTag_RMB = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.RMB"),
		FString("鼠标右键")
	);
	GetInstance().InputTag_1 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.1"),
		FString("键盘1键")
	);
	GetInstance().InputTag_2 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.2"),
		FString("键盘2键")
	);
	GetInstance().InputTag_3 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.3"),
		FString("键盘3键")
	);
	GetInstance().InputTag_4 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.4"),
		FString("键盘4键")
	);
	GetInstance().InputTag_5 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.5"),
		FString("键盘5键")
	);
	GetInstance().InputTag_6 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.6"),
		FString("键盘6键")
	);

	GetInstance().InputTag_Passive = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.Passive"),
		FString("被动技能")
	);

	GetInstance().InputTag_Passive_1 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.Passive.1"),
		FString("被动技能1")
	);
	GetInstance().InputTag_Passive_2 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.Passive.2"),
		FString("被动技能2")
	);

	/*交互状态*/
	GetInstance().Player_Block_CursorTrace = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Player.Block.CursorTrace"),
		FString("Block tracing under the cursor")
	);
	GetInstance().Player_Block_InputHeld = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Player.Block.InputHeld"),
		FString("Block Input Held callback for input")
	);
	GetInstance().Player_Block_InputPressed = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Player.Block.InputPressed"),
		FString("Block Input Pressed callback for input")
	);
	GetInstance().Player_Block_InputReleased = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Player.Block.InputReleased"),
		FString("Block Input Released callback for input")
	);
}

void FAuraGmaeplayTags::InitDamageTypesTags()
{
	GetInstance().Damage = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage"),
		FString("伤害标签")
	);

	/*伤害类型*/
	GetInstance().Damage_Fire = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage.Fire"),
		FString("火性伤害")
	);
	GetInstance().Damage_Lightning = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage.Lightning"),
		FString("带雷伤害")
	);
	GetInstance().Damage_Arcane = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage.Arcane"),
		FString("元素伤害")
	);
	GetInstance().Damage_Physical = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage.Physical"),
		FString("物理伤害")
	);


	/*伤害类型抗性*/
	GetInstance().Attributes_Resistance = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Resistance"),
		FString("抗性")
	);

	GetInstance().Attributes_Resistance_Fire = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Resistance.Fire"),
		FString("火性伤害抗性")
	);
	GetInstance().Attributes_Resistance_Lightning = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Resistance.Lightning"),
		FString("带雷伤害抗性")
	);
	GetInstance().Attributes_Resistance_Arcane = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Resistance.Arcane"),
		FString("元素伤害抗性")
	);
	GetInstance().Attributes_Resistance_Physical = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Resistance.Physical"),
		FString("物理伤害抗性")
	);

	//Debuff
	GetInstance().Debuff = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Debuff"),
		FString("Debuff for Arcane damage")
	);

	GetInstance().Debuff_Arcane = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Debuff.Arcane"),
		FString("Debuff for Arcane damage")
	);
	GetInstance().Debuff_Burn = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Debuff.Burn"),
		FString("Debuff for Fire damage")
	);
	GetInstance().Debuff_Physical = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Debuff.Physical"),
		FString("Debuff for Physical damage")
	);
	GetInstance().Debuff_Stun = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Debuff.Stun"),
		FString("Debuff for Lightning damage")
	);

	GetInstance().Debuff_Chance = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Debuff.Chance"),
		FString("Debuff Chance")
	);
	GetInstance().Debuff_Damage = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Debuff.Damage"),
		FString("Debuff Damage")
	);
	GetInstance().Debuff_Duration = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Debuff.Duration"),
		FString("Debuff Duration")
	);
	GetInstance().Debuff_Frequency = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Debuff.Frequency"),
		FString("Debuff Frequency")
	);

	/*伤害类型映射相应抵抗*/
	GetInstance().DamageTagToResistanceTag.Emplace(GetInstance().Damage_Fire, GetInstance().Attributes_Resistance_Fire);
	GetInstance().DamageTagToResistanceTag.Emplace(GetInstance().Damage_Lightning, GetInstance().Attributes_Resistance_Lightning);
	GetInstance().DamageTagToResistanceTag.Emplace(GetInstance().Damage_Arcane, GetInstance().Attributes_Resistance_Arcane);
	GetInstance().DamageTagToResistanceTag.Emplace(GetInstance().Damage_Physical, GetInstance().Attributes_Resistance_Physical);

	/*伤害类型映射相应Debuff*/
	GetInstance().DamageTypesToDebuffs.Emplace(GetInstance().Damage_Fire, GetInstance().Debuff_Burn);
	GetInstance().DamageTypesToDebuffs.Emplace(GetInstance().Damage_Lightning, GetInstance().Debuff_Stun);
	GetInstance().DamageTypesToDebuffs.Emplace(GetInstance().Damage_Arcane, GetInstance().Debuff_Arcane);
	GetInstance().DamageTypesToDebuffs.Emplace(GetInstance().Damage_Physical, GetInstance().Debuff_Physical);

}

void FAuraGmaeplayTags::InitEffectTags()
{
	GetInstance().EffectHitReact = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Effects.HitReact"),
		FString("伤害标签")
	);

	GetInstance().GameplayCue_FireBlast= UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("GameplayCue.FireBlast"),
		FString("GameplayCue FireBlast")
	);
}

void FAuraGmaeplayTags::InitSkillTags()
{
	GetInstance().Abilities = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities"),
		FString("技能")
	);

	GetInstance().Abilities_None = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.None"),
		FString("技能")
	);

	GetInstance().Abilities_Attack = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Attack"),
		FString("基础攻击")
	);

	GetInstance().Abilities_Summon = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Summon"),
		FString("召唤")
	);

	GetInstance().Abilities_Fire_FireBlot = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Fire.FireBlot"),
		FString("火焰火球")
	);

	GetInstance().Abilities_Fire_FireBlast = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Fire.FireBlast"),
		FString("火焰火球")
	);

	GetInstance().Abilities_Lightning_Electrocute = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Lightning.Electrocute"),
		FString("闪电链路")
	);

	GetInstance().Abilities_Arcane= UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Arcane"),
		FString("Arcane Shards Ability Tag")
	);

	GetInstance().Abilities_Arcane_ArcaneShards = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Arcane.ArcaneShards"),
		FString("Arcane Shards Ability Tag")
	);

	GetInstance().Abilities_Passive = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Passive"),
		FString("Life Siphon")
	);

	GetInstance().Abilities_Passive_LifeSiphon = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Passive.LifeSiphon"),
		FString("Life Siphon")
	);
	GetInstance().Abilities_Passive_ManaSiphon = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Passive.ManaSiphon"),
		FString("Mana Siphon")
	);
	GetInstance().Abilities_Passive_HaloOfProtection = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Passive.HaloOfProtection"),
		FString("Halo Of Protection")
	);

	GetInstance().CombatSocket_Weapon = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("CombatSocket.Weapon"),
		FString("武器攻击")
	);
	GetInstance().CombatSocket_LeftHand = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("CombatSocket.LeftHand"),
		FString("左手攻击")
	);
	GetInstance().CombatSocket_RightHand = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("CombatSocket.RightHand"),
		FString("右手攻击")
	);
	GetInstance().CombatSocket_Tail = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("CombatSocket.Tail"),
		FString("尾巴攻击")
	);

	GetInstance().Montage_Attack_1 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Montage.Attack.1"),
		FString("攻击类蒙太奇1")
	);
	GetInstance().Montage_Attack_2 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Montage.Attack.2"),
		FString("攻击类蒙太奇2")
	);
	GetInstance().Montage_Attack_3 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Montage.Attack.3"),
		FString("攻击类蒙太奇3")
	);
	GetInstance().Montage_Attack_4 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Montage.Attack.4"),
		FString("攻击类蒙太奇4")
	);

	GetInstance().Abilities_HitReact = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.HitReact"),
		FString("Hit React Ability")
	);

	GetInstance().Abilities_Status = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Status"),
		FString("能够解锁")
	);

	GetInstance().Abilities_Status_Eligible = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Status.Eligible"),
		FString("能够解锁")
	);

	GetInstance().Abilities_Status_Equipped = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Status.Equipped"),
		FString("装备上了")
	);

	GetInstance().Abilities_Status_Locked = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Status.Locked"),
		FString("Locked Status")
	);

	GetInstance().Abilities_Status_Unlocked = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Status.Unlocked"),
		FString("Unlocked Status")
	);

	GetInstance().Abilities_Type_None = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Type.None"),
		FString("Type None")
	);

	GetInstance().Abilities_Type_Offensive = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Type.Offensive"),
		FString("Type Offensive")
	);

	GetInstance().Abilities_Type_Passive = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Type.Passive"),
		FString("Type Passive")
	);


	/**
	 * 冷却Tag
	 */
	GetInstance().Cooldown_Fire_FireBlot = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Cooldown.Fire.FireBlot"),
		FString("火焰火球冷却")
	);

	GetInstance().Cooldown_Fire_FireBlast = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Cooldown.Fire.FireBlast"),
		FString("火焰火球冷却")
	);

	GetInstance().Cooldown_Lightning_Electrocute = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Cooldown.Lightning.Electrocute"),
		FString("闪链冷却")
	);
}