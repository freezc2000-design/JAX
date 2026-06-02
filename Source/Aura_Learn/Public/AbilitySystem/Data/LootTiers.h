#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LootTiers.generated.h"

/**
 *  战利品 敌人掉落
 */

USTRUCT(BlueprintType)
struct FLootItem
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "奖励|生成",DisplayName="战利品类")
	TSubclassOf<AActor> LootClass;

	UPROPERTY(EditDefaultsOnly, Category = "奖励|生成",DisplayName="奖励几率")
	float ChanceToSpawn = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "奖励|生成", DisplayName = "最大生成数量")
	int32 MaxNumberToSpawn = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "奖励|生成",DisplayName="是否影响战利品等级")
	bool bLootLevelOverride = true;
};

UCLASS()
class AURA_LEARN_API ULootTiers : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	TArray<FLootItem> GetLootItems();

	UPROPERTY(EditDefaultsOnly,DisplayName="战利品集",Category="战利品配置")
	TArray<FLootItem> LootItems;
};
