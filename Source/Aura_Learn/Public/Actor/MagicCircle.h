#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MagicCircle.generated.h"

/**
 * 魔法阵
 */
UCLASS()
class AURA_LEARN_API AMagicCircle : public AActor
{
	GENERATED_BODY()
	
public:	
	AMagicCircle();
	UDecalComponent& GetMagicCircleDecal();
protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere,DisplayName="阵法贴花")
	TObjectPtr<UDecalComponent> MagicCircleDecal;//魔法贴花
public:	

};
