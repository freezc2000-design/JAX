#include "AbilitySystem/Abilities/Electrocute.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilitieDescriptions.h"

FString UElectrocute::GetDescription(const UAuraAbilitySystemComponent* AuraGAS, const FGameplayTag& GATag,
                                     const int32 Level)
{
	if (!AuraGAS)return FString{};

	const int32 Damage = GetDamageByDamageType(FAuraGmaeplayTags::GetInstance().Damage_Lightning, Level);
	FText CostofMana = FText::FromString(FString::Printf(TEXT("%.1f"), GetManaCost(Level)));
	FText CoolDown = FText::FromString(FString::Printf(TEXT("%.1f"), GetCooldown(Level)));

	auto Description = FText::Format(AuraGAS->AbilityDescriptions->FindDescriptionsByTagAndLevel(GATag, Level), CostofMana, CoolDown, Level, Damage,FMath::Min(Level-1,MaxNumShockTargets));
	return Description.ToString();
}
