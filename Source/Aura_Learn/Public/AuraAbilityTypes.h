#pragma once
#include "GameplayEffectTypes.h"
#include "AuraAbilityTypes.generated.h"

class UGameplayEffect;
USTRUCT(BlueprintType)
struct FDamageGEParamsByDamageType
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag DamageType = FGameplayTag();

	UPROPERTY()
	float DebuffDuration = 0.f;

	UPROPERTY()
	float DebuffFrequency = 1.f;//执行频率

	UPROPERTY()
	float DebuffDamage = 0.f;

	float BaseDamage = 0.f;

	float DebuffChance = 0.f;
};

USTRUCT(BlueprintType)
struct FDamageEffectParams
{
	GENERATED_BODY()

	FDamageEffectParams() {}

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UObject> WorldContextObject = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> DamageGameplayEffectClass = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> SourceAbilitySystemComponent;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> TargetAbilitySystemComponent;

	UPROPERTY(BlueprintReadWrite)
	float AbilityLevel = 1.f;

	UPROPERTY(BlueprintReadWrite)
	float DeathImpulseMagnitude{ 0.f };

	UPROPERTY(BlueprintReadWrite)
	FVector DeathImpulseVectro{ FVector::ZeroVector };

	UPROPERTY(BlueprintReadWrite)
	float KnockbackForceMagnitude = 0.f;//击退力度

	UPROPERTY(BlueprintReadWrite)
	float KnockbackChance = 0.f;//击退触发几率

	UPROPERTY(BlueprintReadWrite)
	FVector KnockbackForce = FVector::ZeroVector;//击退向量

	UPROPERTY(BlueprintReadWrite)
	bool bApplyHitReact{ true };

	UPROPERTY(BlueprintReadWrite)
	bool bIsRadialDamage{ false };//是否属于径向伤害

	UPROPERTY(BlueprintReadWrite)
	float RadialDamageInnerRadius{ 0.f };//径向伤害内圈半径

	UPROPERTY(BlueprintReadWrite)
	float RadialDamageOuterRadius{ 0.f };//径向伤害外圈半径

	/**
	 * 径向伤害中心点
	 * 需DamageGA类MakeDamageEffectParamsFromClassDefaults前设置DamageGA类的RadialDamageOrigin
	 */
	UPROPERTY(BlueprintReadWrite)
	FVector RadialDamageOrigin{ FVector::ZeroVector };

	UPROPERTY(BlueprintReadWrite)
	TMap<FGameplayTag, FDamageGEParamsByDamageType> DebuffMapGEParams;//DeBuff类型对应的GE相关数据
};

USTRUCT(BlueprintType)
struct FAuraGameEffectContext :public FGameplayEffectContext
{
	GENERATED_BODY()
public:
	virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); };
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;//网络序列化

	virtual FAuraGameEffectContext* Duplicate() const override
	{
		FAuraGameEffectContext* NewContext = new FAuraGameEffectContext();
		*NewContext = *this;
		if (GetHitResult())
		{
			// Does a deep copy of the hit result
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	};

	bool IsCriticalHit() const { return bIsCriticalHit; };
	bool IsBlockedHit() const { return bIsBlockedHit; };
	bool IsSuccessfulDebuff() const { return bIsSucessfulDebuff; }
	float GetDebuffDamage() const { return DebuffDamage; }
	float GetDebuffDuration() const { return DebuffDuration; }
	float GetDebuffFrequency() const { return DebuffFrequency; }
	TSharedPtr<FGameplayTag> GetDamageType() const { return DamageType; }
	FVector GetDeathImpulse() const { return DeathImpulse; }
	FVector GetKnockbackForce() const { return KnockbackForce; }
	bool GetHitReact() const { return bActiveHitReact; }
	bool IsRadialDamage() const { return bIsRadialDamage; }
	float GetRadialDamageInnerRadius() const { return RadialDamageInnerRadius; }
	float GetRadialDamageOuterRadius() const { return RadialDamageOuterRadius; }
	FVector GetRadialDamageOrigin() const { return RadialDamageOrigin; }

	void SetCriticalHit(const bool bValue) { bIsCriticalHit = bValue; };
	void SetBlockedHit(const bool bValue) { bIsBlockedHit = bValue; };
	void SetIsSuccessfulDebuff(bool bInIsDebuff) { bIsSucessfulDebuff = bInIsDebuff; }
	void SetDebuffDamage(float InDamage) { DebuffDamage = InDamage; }
	void SetDebuffDuration(float InDuration) { DebuffDuration = InDuration; }
	void SetDebuffFrequency(float InFrequency) { DebuffFrequency = InFrequency; }
	void SetDamageType(TSharedPtr<FGameplayTag> InDamageType) { DamageType = InDamageType; }
	void SetDeathImpulse(const FVector& InImpulse) { DeathImpulse = InImpulse; }
	void SetKnockbackForce(const FVector& InForce) { KnockbackForce = InForce; }
	void SetHitReact(const bool bIsActiv) { bActiveHitReact = bIsActiv; }
	void SetIsRadialDamage(bool bInIsRadialDamage) { bIsRadialDamage = bInIsRadialDamage; }
	void SetRadialDamageInnerRadius(float InRadialDamageInnerRadius) { RadialDamageInnerRadius = InRadialDamageInnerRadius; }
	void SetRadialDamageOuterRadius(float InRadialDamageOuterRadius) { RadialDamageOuterRadius = InRadialDamageOuterRadius; }
	void SetRadialDamageOrigin(const FVector& InRadialDamageOrigin) { RadialDamageOrigin = InRadialDamageOrigin; }

protected:
	UPROPERTY()
	bool bIsCriticalHit{ false };//是否暴击
	UPROPERTY()
	bool bIsBlockedHit{ false };//是否格挡

	UPROPERTY()
	bool bIsSucessfulDebuff{ false };//成功debuff?

	UPROPERTY()
	float DebuffDamage{ 0.f };

	UPROPERTY()
	float DebuffDuration{ 0.f };

	UPROPERTY()
	float DebuffFrequency{ 0.f };//执行频率


	TSharedPtr<FGameplayTag> DamageType;//伤害类型

	UPROPERTY()
	FVector DeathImpulse{ FVector::ZeroVector };

	UPROPERTY()
	FVector KnockbackForce = FVector::ZeroVector;

	UPROPERTY()
	bool bActiveHitReact{ true };//是否可以启用受击反应


	UPROPERTY()
	bool bIsRadialDamage = false;

	UPROPERTY()
	float RadialDamageInnerRadius = 0.f;

	UPROPERTY()
	float RadialDamageOuterRadius = 0.f;

	UPROPERTY()
	FVector RadialDamageOrigin = FVector::ZeroVector;

private:

};

template<>
struct  TStructOpsTypeTraits<FAuraGameEffectContext>:TStructOpsTypeTraitsBase2<FAuraGameEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true		// 以便将 TSharedPtr<FHitResult> 数据复制到各处
	};

};