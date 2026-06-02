#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SaveInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USaveInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class AURA_LEARN_API ISaveInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintCallable,BlueprintNativeEvent)
	bool ShouldLoadTransform();//是否应该保存ActorTransform

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void LoadActor();//根据数据加载一些状态
};
