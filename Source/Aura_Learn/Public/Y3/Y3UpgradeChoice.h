// Y3 三选一兜底升级卡（技能池耗尽后提供：基础属性增加 / 货币奖励）
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Y3UpgradeChoice.generated.h"

/**
 * DT_UpgradeChoice 行。两种卡：
 * - 属性卡：AttributeTag 有效 + Magnitude != 0 → 给玩家基础属性永久加值（本局内）
 * - 货币卡：Gold > 0 → 直接进账号金币
 * 两者可叠加在同一张卡上。
 */
USTRUCT(BlueprintType)
struct FY3UpgradeChoiceRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "显示名(中文)")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "卡面图标")
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** 属性卡：要增加的属性标签（经 DA_AttributeInfo 解析成 FGameplayAttribute） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "属性标签", meta = (Categories = "Attributes"))
	FGameplayTag AttributeTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "属性加值")
	float Magnitude = 0.f;

	/** 货币卡：账号金币奖励 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "金币")
	int32 Gold = 0;

	/** 加权随机抽取权重；<=0 表示不进池 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "抽取权重")
	int32 Weight = 1;
};
