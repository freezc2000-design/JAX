#pragma once

#include "CoreMinimal.h"
#include "Actor/AuraProjectile.h"
#include "AuraFireBall.generated.h"

/**
 * 继承自飞射物，火球回滚
 */
UCLASS()
class AURA_LEARN_API AAuraFireBall : public AAuraProjectile
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void LaunchFire();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BackToOnwer();

	UPROPERTY(BlueprintReadWrite)
	FDamageEffectParams ExplosionDamageParams;//最后爆炸将应用的伤害

	virtual void OnHit() override;
protected:
	virtual void BeginPlay() override;
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlapPrimitiveComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
};
