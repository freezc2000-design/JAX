#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "AuraEffectActor.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;
class USphereComponent;

UENUM(BlueprintType) //GE应用策略
enum class EEffectApplicationPolicy :uint8
{
	ApplyOnOverlap,
	ApplyOnEndoverlap,
	DoNotApply
};

UENUM(BlueprintType)//GE移除策略
enum class EEffectRemovalPolicy :uint8
{
	RemoveOnOverlap,
	RemoveOnEndoverlap,
	DoNotRemove
};

USTRUCT(BlueprintType)
struct FEffectWithPolicy//应用GE的策略
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Applied Effects")
	EEffectApplicationPolicy EffectApplicationPolicy{ EEffectApplicationPolicy::DoNotApply };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Applied Effects")
	TSubclassOf<UGameplayEffect> GameplayEffectClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Applied Effects")
	EEffectRemovalPolicy  InfiniteRemovalPolicy{ EEffectRemovalPolicy::DoNotRemove };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy",DisplayName="敌人能否拾取")
	bool bApplyToEnemy{ false };//敌人也能拾取吗
};

UCLASS()
class AURA_LEARN_API AAuraEffectActor : public AActor
{
	GENERATED_BODY()

public:
	AAuraEffectActor();
protected:
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable)
	void ApplyEffecToTarget(AActor* TargetActor,TSubclassOf<UGameplayEffect> GEClass,const FEffectWithPolicy& CurentEffectPolicy);

	UFUNCTION(BlueprintCallable)
	void OnOverlap(AActor* TargetActor);
	UFUNCTION(BlueprintCallable)
	void OnEndOverlap(AActor* TargetActor);

	UFUNCTION(BlueprintImplementableEvent)
	void FloatctorEvent();

	UPROPERTY(EditAnywhere, DisplayName = "是否浮动")
	bool bFloattActor{ true };

protected:
	UPROPERTY(EditAnywhere)
	TArray<FEffectWithPolicy> EffectWithPolicys;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Applied Effects",DisplayName="应用GE之后自动销毁")
	bool bDestroyOnEffectApplication{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Applied Effects")
	float ActorLevel{ 1.f };//GE应用的等级


private:

};
