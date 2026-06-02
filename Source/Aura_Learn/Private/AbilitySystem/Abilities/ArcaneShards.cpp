#include "AbilitySystem/Abilities/ArcaneShards.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilitieDescriptions.h"

FString UArcaneShards::GetDescription(const UAuraAbilitySystemComponent* AuraGAS, const FGameplayTag& GATag,
                                      const int32 Level)
{
	if (!AuraGAS)return FString{};

	const int32 Damage = GetDamageByDamageType(FAuraGmaeplayTags::GetInstance().Damage_Arcane, Level);
	FText CostofMana = FText::FromString(FString::Printf(TEXT("%.1f"), GetManaCost(Level)));
	FText CoolDown = FText::FromString(FString::Printf(TEXT("%.1f"), GetCooldown(Level)));

	auto Description = FText::Format(AuraGAS->AbilityDescriptions->FindDescriptionsByTagAndLevel(GATag, Level), CostofMana, CoolDown, Level, Damage, MaxNumArcane);
	return Description.ToString();
}
