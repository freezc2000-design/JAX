#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "AbilityInfo.generated.h"

class UGameplayAbility;

USTRUCT(BlueprintType)
struct FAuraAbilityInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, DisplayName = "技能标识标签", meta = (Categories = "Abilities"))
	FGameplayTag AbilityTag{FGameplayTag()};

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag InputTag{ FGameplayTag() };

	UPROPERTY(BlueprintReadOnly,DisplayName="技能状态")
	FGameplayTag StatusTag{ FGameplayTag() };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,DisplayName="技能类型",meta = (Categories = "Abilities.Type"))
	FGameplayTag AbilityType{ FGameplayTag() };

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, DisplayName = "冷却标签", meta = (Categories = "Cooldown"))
	FGameplayTag CooldownTag{ FGameplayTag() };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTexture2D> AbilityIcon{ nullptr };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UMaterialInstance> BackgroundMaterial{ nullptr };

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,DisplayName="等级要求")
	int32 LevelRequirement{ 1 };//该技能的等级要求

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, DisplayName = "技能类源")
	TSubclassOf<UGameplayAbility> Ability;//该技能本身的类型
};

UCLASS()
class AURA_LEARN_API UAbilityInfo : public UDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,Category="技能信息")
	TArray<FAuraAbilityInfo> AbilityInfomation;

	FAuraAbilityInfo FindAbilityInfoForTag(const FGameplayTag& AbilityTag, bool bLogNotFound = false) const;
};
