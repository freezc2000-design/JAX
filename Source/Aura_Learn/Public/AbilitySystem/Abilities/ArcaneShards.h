// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraDamageGameplayAbility.h"
#include "ArcaneShards.generated.h"

/**
 * 
 */
UCLASS()
class AURA_LEARN_API UArcaneShards : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()
public:
	virtual FString GetDescription(const UAuraAbilitySystemComponent* AuraGAS, const FGameplayTag& GATag, const int32 Level) override;
protected:
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,DisplayName="最大水晶数量",Category="奥术")
	int32 MaxNumArcane{ 11 };
};
