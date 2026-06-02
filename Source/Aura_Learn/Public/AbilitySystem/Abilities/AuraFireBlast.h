#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraDamageGameplayAbility.h"
#include "AuraFireBlast.generated.h"

class AAuraFireBall;
/**
 * 
 */
UCLASS()
class AURA_LEARN_API UAuraFireBlast : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()
public:
	virtual FString GetDescription(const UAuraAbilitySystemComponent* AuraGAS, const FGameplayTag& GATag, const int32 Level) override;
	UFUNCTION(BlueprintCallable)
	TArray<AAuraFireBall*>SpawnFireBall();

protected:
	UPROPERTY(EditDefaultsOnly,Category="火元子",DisplayName="火元数量")
	int32 NumFireBalls{ 12 };

	UPROPERTY(EditDefaultsOnly, Category = "火元子", DisplayName = "火元子类")
	TSubclassOf<AAuraFireBall>  FireballClass;
};
