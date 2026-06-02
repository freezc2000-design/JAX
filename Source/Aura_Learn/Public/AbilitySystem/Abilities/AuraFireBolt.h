#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraProjectileSpell.h"
#include "AuraFireBolt.generated.h"

/**
 * 
 */
UCLASS()
class AURA_LEARN_API UAuraFireBolt : public UAuraProjectileSpell
{
	GENERATED_BODY()
public:

	//随着技能等级的上升 技能可能发射多个火球
	UFUNCTION(BlueprintCallable)
	void SpawnProjectiles(const FVector& ProjectileTargetLocation, bool bOverridePitch, float PitchOverride, AActor* HomingTarget);

protected:

	UPROPERTY(EditDefaultsOnly, Category = "FireBolt",DisplayName="发射扇形角度")
	float ProjectileSpread = 90.f;//火球发射的扇形区域

	UPROPERTY(EditDefaultsOnly, Category = "FireBolt", DisplayName = "追踪目标最小速度")
	float HomingMinSpeed{ 900.f};

	UPROPERTY(EditDefaultsOnly, Category = "FireBolt", DisplayName = "追踪目标最大速度")
	float HomingMaxSpeed{1800.f};

	UPROPERTY(EditDefaultsOnly, Category = "FireBolt", DisplayName = "启用追踪")
	bool bLaunchHoming{ false };
};
