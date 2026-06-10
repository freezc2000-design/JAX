// Y3 技能图鉴 — 注册表行结构 + TileView 条目数据对象。
// 每行 = 一个技能图标 + 元数据 +(可选)对应的 GA 技能类。
// Ability 为空 = 未开发(只有占位图标);非空 = 已开发(可测试)。
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Y3SkillRegistry.generated.h"

class UGameplayAbility;
class UTexture2D;

USTRUCT(BlueprintType)
struct FY3SkillRegistryRow : public FTableRowBase
{
	GENERATED_BODY()

	// 图标贴图（软引用，按需加载，避免 833 张全常驻内存）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Y3|Skill")
	TSoftObjectPtr<UTexture2D> Icon;

	// 显示名（技能名）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Y3|Skill")
	FString DisplayName;

	// 来源英雄（占位元数据，来自 Dota 图鉴）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Y3|Skill")
	FString Hero;

	// 属性分类：str / agi / int / all（英雄属性）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Y3|Skill")
	FString Attr;

	// 大类：hero / monster / system（一级 tab 用 monster/system 走这个）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Y3|Skill")
	FString Cat;

	// 标签（主动/被动/DOT/指向地面 …）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Y3|Skill")
	TArray<FString> Tags;

	// 对应的 GA 技能类；为空表示"未开发"（只是占位图标）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Y3|Skill", meta = (DisplayName = "技能类(空=未开发)"))
	TSoftClassPtr<UGameplayAbility> Ability;
};

/** TileView 的条目数据对象（UMG TileView 的 item 必须是 UObject）。 */
UCLASS(BlueprintType)
class AURA_LEARN_API UY3SkillEntryData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Y3|Skill")
	FName RowName;

	UPROPERTY(BlueprintReadOnly, Category = "Y3|Skill")
	FY3SkillRegistryRow Row;

	// 已开发 = 绑定了 GA 技能类
	UFUNCTION(BlueprintPure, Category = "Y3|Skill")
	bool IsDeveloped() const { return !Row.Ability.IsNull(); }
};
