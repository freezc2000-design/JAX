// 学习使用

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "AuraPlayerController.generated.h"

class IHighlightInterface;
class AMagicCircle;
class UNiagaraSystem;
class UDamageTextComponent;
class IEnemyInterface;
class USplineComponent;
class UAuraAbilitySystemComponent;
class UAuraInputConfig;
class UInputMappingContext;
class UInputAction;
struct  FInputActionValue;

enum class ETargetingStatus : uint8
{
	TargetingEnemy, //是可以战斗的对象
	TargetingNonEnemy, //仅为可交互对象
	NotTargeting
};

/**
 * 
 */
UCLASS()
class AURA_LEARN_API AAuraPlayerController : public APlayerController
{
	GENERATED_BODY()
	friend class AAuraCharacterBase;
public:
	AAuraPlayerController();

	/* *
	 *  Client 此函数仅在拥有在其上调用此函数的对象的客户端上执行。用于声明名称与主函数相同的附加函数，但是末尾添加了_Implementation。
	 *           - 必要时，此自动生成的代码将调用 _Implementation 方法。
	 *  Reliable 此函数将通过网络复制，并且一定会到达
	 */
	UFUNCTION(Client,Reliable)
	void ShowDamage(float DamageAmount,ACharacter* TargetCharacter, const bool bBlocked=false, const bool bCriticalHit=false);

	UFUNCTION(BlueprintCallable)
	void HideMagicCircle();
	UFUNCTION(BlueprintCallable)
	void ShowMagicCircle();

	void SetMagicCircleMaterial(UMaterialInterface* DecalMaterial = nullptr);
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;//PlayerTick只能用于本地Controller
private:
	void Move(const FInputActionValue& InputActionValue);
	void CursorTrace();

	void AbilityInputTagPressed(FGameplayTag InputTag); //按下的输入操作回调
	void AbilityInputTagReleased(FGameplayTag InputTag);//松开的输入操作回调
	void AbilityInputTagHeld(FGameplayTag InputTag);//按住的输入操作回调

	bool bShiftKeyDown{ false };//shift是否按住

	void ShiftPressed() { bShiftKeyDown = true; };
	void ShiftReleased() { bShiftKeyDown = false; };
	
	void AutoRun();
	UAuraAbilitySystemComponent* GetAuraASC();

	void UpdateMagicCircleLocation();

	static void HighlightActor(AActor* InActor);
	static void UnHighlightActor(AActor* InActor);

private:
	UPROPERTY(EditAnywhere,Category="Input")
	TObjectPtr<UInputMappingContext> AuraContext;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ShiftAction;

	UPROPERTY(EditDefaultsOnly,Category="Input")
	TObjectPtr<UAuraInputConfig> InputConfig;//输入操作的DA 配置文件

	UPROPERTY()
	TObjectPtr<UAuraAbilitySystemComponent> AuraAbilitySystemComponent{nullptr};

	TObjectPtr<AActor> MouseHoverCurrentActor{ nullptr };//鼠标悬浮到的Actor
	TObjectPtr<AActor>  MouseHoverLastActor{ nullptr };

	FVector CachedDestination{FVector::Zero()};//缓存点中的场景坐标
	float FllowTime{0.f};//跟随光标移动了的时间
	float ShortPressThreshold{ 0.5f };//判断是否属于长按的临界值
	bool bAutoRunning{ false };

	ETargetingStatus TargetingStatus{ ETargetingStatus::NotTargeting};

	UPROPERTY(EditDefaultsOnly)
	float AutoRunAcceptanceRadius{ 50.f };//距离目标多远则被视为到达目标

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USplineComponent> Spline;//根据点生成平滑的路径

	FHitResult CursorHit;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UDamageTextComponent> DamageTextComponentClass;

	UPROPERTY(EditDefaultsOnly,DisplayName="点击特效")
	TObjectPtr<UNiagaraSystem> ClickNiagara{nullptr};

	UPROPERTY(EditDefaultsOnly,DisplayName="法阵特效Actor类型")
	TSubclassOf<AMagicCircle> MagicCircleClass{ nullptr };

	UPROPERTY(DisplayName = "法阵特效Actor")
	TObjectPtr<AMagicCircle> MagicCircle{ nullptr };
};
