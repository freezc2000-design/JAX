#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "AuraGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class AURA_LEARN_API UAuraGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName PlayerStartTag{""};//采用哪个玩家起点实例

	UPROPERTY()
	FString  LoadSlotName{ "" };

	UPROPERTY()
	uint8 LoadSlotIdx{0};
};
