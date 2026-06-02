#pragma once
#include "CoreMinimal.h"
#include "AuraBeamSpell.h"

#include "Electrocute.generated.h"

/**
 * 
 */
UCLASS()
class AURA_LEARN_API UElectrocute : public UAuraBeamSpell
{
	GENERATED_BODY()

public:
	virtual FString GetDescription(const UAuraAbilitySystemComponent* AuraGAS, const FGameplayTag& GATag, const int32 Level) override;
};
