#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "AbilitieDescriptions.generated.h"

/**
 * 技能不同等级的描述，应该放在服务器上
 */

USTRUCT(BlueprintType)
struct FAbilityDescription
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,DisplayName="技能标签")
	FGameplayTag AbilityTag{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, DisplayName = "技能描述")
	TArray<FText>AbilityDescriptions;
	
};
UCLASS()
class AURA_LEARN_API UAbilitieDescriptions : public UDataAsset
{
	GENERATED_BODY()
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "技能信息", DisplayName = "技能描述集")
	TArray<FAbilityDescription> AbilityDescriptionInfos;

public:
	UFUNCTION(BlueprintCallable)
	const FText& FindDescriptionsByTagAndLevel(const FGameplayTag& AbilityTag, const int32 Level);
};
