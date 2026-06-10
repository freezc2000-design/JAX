#pragma once

#include "CoreMinimal.h"
#include "Character/AuraCharacterBase.h"
#include "Interaction/PlayerInterface.h"
#include "AuraCharacter.generated.h"

class UNiagaraComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class AURA_LEARN_API AAuraCharacter : public AAuraCharacterBase,public IPlayerInterface
{
	GENERATED_BODY()
public:
	AAuraCharacter();

	/*CombatInterface 实现区域*/
	virtual int32 GetPlayerLevel_Implementation() override;
	virtual void Die(const FVector& DeathImpulse) override;
	/*CombatInterface 结束区域*/

	/*PlayerInterface*/
	virtual void AddToXP_Implementation(int32 InXP) override;
	virtual void LevelUp_Implementation() override;
	virtual int32 GetXP_Implementation() const override;
	virtual int32 FindLevelFromXP_Implementation(int32 XP) const override;
	virtual void AddToPlayerLevel_Implementation(int32 InPlayerLevel) const override;
	virtual void HideMagicCricle_Implementation() override;
	virtual void ShowMagicCricle_Implementation(UMaterialInterface* DecalMaterial = nullptr) override;
	virtual void SaveProgress_Implementation(const FName& PlayerStartName) override;
	/*PlayerInterface*/

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;//当 PlayerState 属性在服务器上复制到客户端时，这个函数会被调用
	virtual void OnRep_Stunned() override;
	virtual void OnRep_Burned() override;

	void LoadProgress();//加载角色属性信息
private:
	virtual void InitAbilityActorInfo() override;

	UFUNCTION(NetMulticast,Reliable)
	void MulticastLevelUpParticles()const;//向其他客户端广播升级特效
protected:
	UPROPERTY(EditAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent>SpringArm;
	UPROPERTY(EditAnywhere,Category="Camera")
	TObjectPtr<UCameraComponent>Camera;

	UPROPERTY(EditDefaultsOnly,DisplayName="亡后重载存档倒计时")
	float DeathTime{5.f};

	FTimerHandle DeatTimer;//死亡计时器

public:
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	TObjectPtr<UNiagaraComponent> LevelUpNiagaraComponent;
};

