#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AuraGameplayAbility.h"
#include "AuraSummonAbility.generated.h"

/**
 *  用以召唤类技能 生成召唤物
 */
UCLASS()
class AURA_LEARN_API UAuraSummonAbility : public UAuraGameplayAbility
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	TArray<FVector> GetSpwanLocation();//获取召唤物生成的位置

	UFUNCTION(BlueprintPure, BlueprintCallable)//随机获取一个种子
	TSubclassOf<AActor> GetRandomMinionClass();

	UPROPERTY(EditDefaultsOnly, DisplayName = "生成数量", Category = "Summoning")
	int16 NumMinions{ 2 };//准备生成多少个召唤物

	UPROPERTY(EditDefaultsOnly, DisplayName = "召唤物库", Category = "Summoning")
	TArray<TSubclassOf<AActor>> MinionClasses;//生成种类库

	UPROPERTY(EditDefaultsOnly, DisplayName = "召唤最小距离", Category = "Summoning")
	float MinSpawnDistance{ 50.f };//最小生成距离

	UPROPERTY(EditDefaultsOnly, DisplayName = "召唤最大距离", Category = "Summoning")
	float MaxSpawnDistance{ 250.f };//最小生成距离

	UPROPERTY(EditDefaultsOnly, DisplayName = "召唤展开角度", Category = "Summoning")
	float SpawnSpread{ 90.f };//生成展开角度
};
