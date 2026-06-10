#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "AbilityInfo.generated.h"

class UGameplayAbility;

// 施法方式（图鉴一级归类 + 技能栏装备路径判断）。
UENUM(BlueprintType)
enum class EY3CastType : uint8
{
	Active   UMETA(DisplayName = "主动"),
	Passive  UMETA(DisplayName = "被动"),
	Channel  UMETA(DisplayName = "引导"),
};

// 施法方式 -> 图鉴标签字符串（与 Y3SkillAtlasWidget 的"施法方式"分组标签对齐）。
inline FString Y3CastTypeToTag(EY3CastType InType)
{
	switch (InType)
	{
	case EY3CastType::Passive: return TEXT("被动");
	case EY3CastType::Channel: return TEXT("引导");
	default:                   return TEXT("主动");
	}
}

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

	// 当前技能等级(运行时由 BroadcastAbilityInfo 从 AbilitySpec.Level 填;技能栏角标显示用)
	UPROPERTY(BlueprintReadOnly, DisplayName = "当前等级")
	int32 Level{ 1 };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,DisplayName="技能类型",meta = (Categories = "Abilities.Type"))
	FGameplayTag AbilityType{ FGameplayTag() };

	// 中文显示名（三选一卡片 + 图鉴；单一真源，不再从 DT_SkillRegistry 反查）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, DisplayName = "显示名(中文)")
	FString DisplayName;

	// 施法方式（图鉴归类：主动/被动/引导）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, DisplayName = "施法方式")
	EY3CastType CastType{ EY3CastType::Active };

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
