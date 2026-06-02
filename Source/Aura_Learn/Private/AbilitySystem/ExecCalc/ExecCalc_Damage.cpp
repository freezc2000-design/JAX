// 学习使用
#include "AbilitySystem/ExecCalc/ExecCalc_Damage.h"
#include "AbilitySystemComponent.h"
#include "AuraAbilityTypes.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemBPLibary.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "Interaction/CombatInterface.h"

static TMap<FGameplayTag, FGameplayEffectAttributeCaptureDefinition> TagToCaptureDef; //抗性类型映射相应的属性获取

AuraDamageStatics::AuraDamageStatics()
{
	DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, Armor, Target, false);
	DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, ArmorPenetration, Source, false);
	DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, BlockChance, Target, false);
	DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, CriticalHitChance, Source, false);
	DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, CriticalHitDamage, Source, false)
	DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, CriticalHitResistance, Target, false)
	DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, FireResistance, Target, false)
	DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, LightningResistance, Target, false)
	DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, ArcaneResistance, Target, false)
	DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet, PhysicalResistance, Target, false)

}

UExecCalc_Damage::UExecCalc_Damage()
{
	RelevantAttributesToCapture.Add(GetDamageStatics().ArmorDef);
	RelevantAttributesToCapture.Add(GetDamageStatics().ArmorPenetrationDef);
	RelevantAttributesToCapture.Add(GetDamageStatics().BlockChanceDef);
	RelevantAttributesToCapture.Add(GetDamageStatics().CriticalHitChanceDef);
	RelevantAttributesToCapture.Add(GetDamageStatics().CriticalHitDamageDef);
	RelevantAttributesToCapture.Add(GetDamageStatics().CriticalHitResistanceDef);
	RelevantAttributesToCapture.Add(GetDamageStatics().FireResistanceDef);
	RelevantAttributesToCapture.Add(GetDamageStatics().LightningResistanceDef);
	RelevantAttributesToCapture.Add(GetDamageStatics().ArcaneResistanceDef);
	RelevantAttributesToCapture.Add(GetDamageStatics().PhysicalResistanceDef);
}

void UExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);

	//初始化抗性类型映射相应的属性获取
	
	static bool bInitMap{false};
	if (!bInitMap)
	{
		const auto& Tags = FAuraGmaeplayTags::GetInstance();
		TagToCaptureDef.Emplace(Tags.Attributes_Secondary_Armor, GetDamageStatics().ArmorDef);
		TagToCaptureDef.Emplace(Tags.Attributes_Secondary_BlockChance, GetDamageStatics().BlockChanceDef);
		TagToCaptureDef.Emplace(Tags.Attributes_Secondary_ArmorPenetration, GetDamageStatics().ArmorPenetrationDef);
		TagToCaptureDef.Emplace(Tags.Attributes_Secondary_CriticalHitChance, GetDamageStatics().CriticalHitChanceDef);
		TagToCaptureDef.Emplace(Tags.Attributes_Secondary_CriticalHitResistance,
		                        GetDamageStatics().CriticalHitResistanceDef);
		TagToCaptureDef.Emplace(Tags.Attributes_Secondary_CriticalHitDamage, GetDamageStatics().CriticalHitDamageDef);
		TagToCaptureDef.Emplace(Tags.Attributes_Resistance_Arcane, GetDamageStatics().ArcaneResistanceDef);
		TagToCaptureDef.Emplace(Tags.Attributes_Resistance_Fire, GetDamageStatics().FireResistanceDef);
		TagToCaptureDef.Emplace(Tags.Attributes_Resistance_Lightning, GetDamageStatics().LightningResistanceDef);
		TagToCaptureDef.Emplace(Tags.Attributes_Resistance_Physical, GetDamageStatics().PhysicalResistanceDef);

		bInitMap = true;
	}


	//获取ASC
	const UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();

	//获取目标和源Actor
	AActor* SourceAvatar = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	AActor* TargetAvatar = TargetASC ? TargetASC->GetAvatarActor() : nullptr;

	//获取挂载此类的GE实例
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	//设置评估参数
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	//GE 设置上下文内容
	auto GEContext = Spec.GetContext();

	//Debuff
	CalcDebuff(Spec,ExecutionParams,EvaluationParameters);

	//通过caller获取伤害值
	float Damage{0.f};

	for (const auto& it : FAuraGmaeplayTags::GetInstance().DamageTagToResistanceTag)
	{
		const auto& DamageTypeTag = it.Key;
		const auto& DamageResistanceTag = it.Value;

		if (!TagToCaptureDef.Contains(DamageResistanceTag))
		{
			auto temp = GetDamageStatics();
			UE_LOG(LogTemp, Error, TEXT("TagToCaptureDef 不存在Tag: [%s]"), *DamageResistanceTag.ToString());
			continue;
		}
		//当前的抗性属性
		const auto& CaptureDef = TagToCaptureDef[DamageResistanceTag];//直接拿到当前的属性捕获类型 下面就方便直接获取值
		if(float CurentTypeDamge = Spec.GetSetByCallerMagnitude(it.Key,false)) //从GE中找寻相应Tag的Modfiy 修饰符 获取其值,没找到不给警告
		{
			float CurResistance{ 0.f };
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(CaptureDef, EvaluationParameters, CurResistance);

			CurResistance = FMath::Clamp(CurResistance, 0.f, 100.f);
			CurentTypeDamge *= 100.f/(100.f + CurResistance*4.f) ;

			/**
			 * 径向伤害处理
			 * 替代原项目的 重写TakeDamage和使用代理的形式
			 */
			if (UAuraAbilitySystemBPLibary::IsRadialDamage(GEContext))
			{
				CurentTypeDamge = UAuraAbilitySystemBPLibary::GetRadialDamageWithFalloff(TargetAvatar, CurentTypeDamge, 1.f,
				                                                                         UAuraAbilitySystemBPLibary::GetRadialDamageOrigin(GEContext),
				                                                                         UAuraAbilitySystemBPLibary::GetRadialDamageInnerRadius(GEContext),
				                                                                         UAuraAbilitySystemBPLibary::GetRadialDamageOuterRadius(GEContext),
				                                                                         1.f);
			}

			Damage += CurentTypeDamge;
		}

	}

	//将格挡机会参与计算
	float BlockChance{ 0.f };
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetDamageStatics().BlockChanceDef, EvaluationParameters, BlockChance);
	BlockChance=FMath::Max(0.f, BlockChance);
	bool bBlock= BlockChance>FMath::RandRange(0, 100);
	Damage = bBlock ? Damage / 2 : Damage;


	UAuraAbilitySystemBPLibary::SetIsBlockedHit(GEContext, bBlock);

	/**
	 *  护甲穿透将影响护甲的防护效果
	 */
	float TargetArmor{ 0.f };
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetDamageStatics().ArmorDef, EvaluationParameters, TargetArmor);
	TargetArmor = FMath::Max(0.f, TargetArmor);

	float SourceArmorPenetration{ 0.f };
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetDamageStatics().ArmorPenetrationDef, EvaluationParameters, SourceArmorPenetration);
	SourceArmorPenetration = FMath::Max(0.f, SourceArmorPenetration);

	auto DefaultClassInfo=UAuraAbilitySystemBPLibary::GetCharacterClassInfo(SourceAvatar);

	//获取表格曲线从表格曲线读值 分别是目标护甲的系数 和 源穿甲的系数
	auto ArmorPenetrationCurve = DefaultClassInfo->DamageCalculationCoefficients->FindCurve(FName("ArmorPenetration"), FString());//获取对应曲线
	auto SourceInterface=Cast<ICombatInterface>(SourceAvatar);
	const float  ArmorPenetrationRatio = ArmorPenetrationCurve->Eval(ICombatInterface::Execute_GetPlayerLevel(SourceAvatar));//从给定X读取Y

	const float EffectiveArmor = TargetArmor * (100 - SourceArmorPenetration* ArmorPenetrationRatio) / 100.f;

	auto ArmorCurve= DefaultClassInfo->DamageCalculationCoefficients->FindCurve(FName("EffectArmor"), FString());
	auto TargetInterface = Cast<ICombatInterface>(TargetAvatar);
	const float ArmorRatio = ArmorCurve->Eval(ICombatInterface::Execute_GetPlayerLevel(TargetAvatar));

	Damage *= (100-EffectiveArmor* ArmorRatio) / 100.f;

	/**
	 *  暴击
	 */
	float CriticalHitChance{ 0.f };
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetDamageStatics().CriticalHitChanceDef, EvaluationParameters, CriticalHitChance);
	CriticalHitChance = FMath::Max(0.f, CriticalHitChance);
	const bool bCriticalHit = CriticalHitChance > FMath::RandRange(0, 100);

	if(bCriticalHit)
	{
		UAuraAbilitySystemBPLibary::SetIsCriticalHit(GEContext, true);

		float CriticalHitDamage{ 0.f };
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetDamageStatics().CriticalHitDamageDef, EvaluationParameters, CriticalHitDamage);
		CriticalHitDamage = FMath::Max(0.f, CriticalHitDamage);

		float CriticalHitResistance{ 0.f };
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetDamageStatics().CriticalHitResistanceDef, EvaluationParameters, CriticalHitResistance);
		CriticalHitResistance = FMath::Max(0.f, CriticalHitResistance);

		float CriticalHitResistanceRatio = CriticalHitResistance/(100 + CriticalHitResistance);//对数增长形式的系数，最终只能无限接近1
		CriticalHitDamage = CriticalHitDamage - (CriticalHitDamage * CriticalHitResistanceRatio);
		Damage += CriticalHitDamage*2.f;
	}

	FGameplayModifierEvaluatedData EvaluatedData(UAuraAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, Damage);
	OutExecutionOutput.AddOutputModifier(EvaluatedData);
}

void UExecCalc_Damage::CalcDebuff(const FGameplayEffectSpec& Spec, const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FAggregatorEvaluateParameters& EvaluationParameters)
{
	for (const auto& it : FAuraGmaeplayTags::GetInstance().DamageTypesToDebuffs)
	{
		const float TypeDamage = Spec.GetSetByCallerMagnitude(it.Key, false, -1.f);
		if (TypeDamage > -0.01f)
		{
			const float DebuffChance = Spec.GetSetByCallerMagnitude(FAuraGmaeplayTags::GetInstance().Debuff_Chance, false, -1.f);
			
			if (DebuffChance <= 0.f || !(DebuffChance > FMath::RandRange(1, 100)))continue;

			auto CotextHandle = Spec.GetContext();

			UAuraAbilitySystemBPLibary::SetIsSuccessfulDebuff(CotextHandle, true);
			UAuraAbilitySystemBPLibary::SetDamageType(CotextHandle, it.Key);

			const float DebuffDuration = Spec.GetSetByCallerMagnitude(FAuraGmaeplayTags::GetInstance().Debuff_Duration, false, -1.f);
			const float DebuffFrequency = Spec.GetSetByCallerMagnitude(FAuraGmaeplayTags::GetInstance().Debuff_Frequency, false, -1.f);
			float DebuffDamage = Spec.GetSetByCallerMagnitude(FAuraGmaeplayTags::GetInstance().Debuff_Damage, false, -1.f);

			//减伤
			float TargetDebuffResistance{ 0.f };
			const auto& ResistanceTag = FAuraGmaeplayTags::GetInstance().DamageTagToResistanceTag[it.Key];
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(TagToCaptureDef[ResistanceTag], EvaluationParameters, TargetDebuffResistance);
			TargetDebuffResistance = FMath::Max(TargetDebuffResistance, 0.f);

			float ResistanceRatio = TargetDebuffResistance / (100 + TargetDebuffResistance);//对数增长形式的系数，最终只能无限接近1
			DebuffDamage = DebuffDamage - (DebuffDamage * ResistanceRatio);
			DebuffDamage = FMath::Max(DebuffDamage, 0.5f);//给个保底吧

			UAuraAbilitySystemBPLibary::SetDebuffDamage(CotextHandle, DebuffDamage);
			UAuraAbilitySystemBPLibary::SetDebuffDuration(CotextHandle, DebuffDuration);
			UAuraAbilitySystemBPLibary::SetDebuffFrequency(CotextHandle, DebuffFrequency);
		}
	}
}
