// Y3 技能图鉴 — 单个图标条目（TileView 的 EntryWidget）。
// 只负责显示：图标 + 已开发/未开发的视觉区分。点击交互由 TileView 在主控件里统一处理。
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Y3SkillAtlasEntry.generated.h"

class UImage;
class UBorder;
class UTextBlock;
class UY3SkillEntryData;

UCLASS(Abstract)
class AURA_LEARN_API UY3SkillAtlasEntry : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	// IUserObjectListEntry：TileView 把 item 数据塞进来时调用
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

protected:
	// 薄壳 WBP 里需要有这些同名子控件
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;

	// 外框：已开发=绿，未开发=暗灰（可选）
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> StatusBorder;

	// 名称文字（可选）
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	// 已开发外框颜色
	UPROPERTY(EditAnywhere, Category = "Y3|Skill")
	FLinearColor DevelopedColor = FLinearColor(0.15f, 0.85f, 0.25f, 1.f);

	// 未开发外框颜色
	UPROPERTY(EditAnywhere, Category = "Y3|Skill")
	FLinearColor UndevelopedColor = FLinearColor(0.18f, 0.18f, 0.2f, 1.f);

	UPROPERTY()
	TObjectPtr<UY3SkillEntryData> Data;
};
