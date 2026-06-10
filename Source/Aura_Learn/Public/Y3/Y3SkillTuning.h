#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Y3SkillTuning.generated.h"

/**
 * 技能数值调优表行 —— 商业级"数值层"的起点。
 * 策划可调的"数字"集中到这张表,各技能 C++ 模板从这里读,而不是散落在每个 GA 的 CDO。
 * 第一版只含库存(可升级份数);后续逐列加入 弹数/范围/目标数/CD 倍率 等,无需改这套读取框架。
 */
USTRUCT(BlueprintType)
struct FY3SkillTuningRow : public FTableRowBase
{
	GENERATED_BODY()

	// 技能标识(与 DA_AbilityInfo.AbilityTag 对应;建议行名也直接用 tag 名,便于查)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Y3|Tuning", meta = (Categories = "Abilities"))
	FGameplayTag AbilityTag;

	// 库存份数 = 可升级次数。封顶等级 = 1 + StockCount。
	// 例:火球 StockCount=5 → 首次抽到 1 级,再抽 5 份升到 6 级,满级后移出卡池。
	// 0 = 不参与抽卡升级(抽到即定级,不再出现在卡池)。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Y3|Tuning", meta = (DisplayName = "库存(可升级份数)"))
	int32 StockCount = 0;
};
