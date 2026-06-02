// Y3 怪物轻量基类：不继承 AAuraEnemy 避免 GAS 链路崩溃；占位用，后续可改父类
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Y3EnemyBase.generated.h"

UCLASS()
class AURA_LEARN_API AY3EnemyBase : public ACharacter
{
    GENERATED_BODY()

public:
    AY3EnemyBase();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Y3")
    int32 Y3_UnitID = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Y3")
    FString Y3_Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Y3")
    FString Y3_AtkType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Y3")
    FString Y3_UnitTag;

    /** 关联的属性表行ID（attrID）*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Y3")
    int32 Y3_AttrID = 0;

    /** 当前属性快照（spawn 时由 GameMode 注入）*/
    UPROPERTY(BlueprintReadWrite, Category="Y3|Runtime")
    float CurrentHP = 100.f;

    UPROPERTY(BlueprintReadWrite, Category="Y3|Runtime")
    float MaxHP = 100.f;

    UPROPERTY(BlueprintReadWrite, Category="Y3|Runtime")
    float ATK = 10.f;

    UPROPERTY(BlueprintReadWrite, Category="Y3|Runtime")
    float DEF = 0.f;

    UPROPERTY(BlueprintReadWrite, Category="Y3|Runtime")
    float MoveSpeed = 300.f;
};
