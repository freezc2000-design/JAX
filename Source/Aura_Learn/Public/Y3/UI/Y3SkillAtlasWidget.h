// Y3 技能图鉴主控件 — 读 DT_SkillRegistry，TileView 虚拟化展示 833+ 图标，
// 支持搜索过滤、已开发/未开发统计，点击已开发技能直接授予+激活测试。
// 逻辑全在 C++；薄壳 WBP 只摆 TileView/搜索框/统计文字。
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Y3SkillAtlasWidget.generated.h"

class UTileView;
class UEditableTextBox;
class UTextBlock;
class UButton;
class UWrapBox;
class UVerticalBox;
class UPanelWidget;
class UDataTable;
class UAbilityInfo;
class UY3SkillEntryData;
class UY3FilterChip;

UCLASS(Abstract)
class AURA_LEARN_API UY3SkillAtlasWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// 已开发技能的唯一真源：DA_AbilityInfo（图鉴"已开发"区从这里构建，自动同步新技能）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Y3|Skill")
	TObjectPtr<UAbilityInfo> AbilityInfoSource;

	// 未开发占位图标库：DT_SkillRegistry（行类型 FY3SkillRegistryRow，833 张 Dota 占位）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Y3|Skill")
	TObjectPtr<UDataTable> RegistryTable;

	// 仅显示已开发的技能（开关）
	UPROPERTY(BlueprintReadWrite, Category = "Y3|Skill")
	bool bDevelopedOnly = false;

	// 筛选标签控件类（WBP_FilterChip）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Y3|Skill")
	TSubclassOf<UY3FilterChip> ChipClass;

protected:
	// 一级 tab（属性/分类）容器（正上方）
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> AttrFilterBox;

	// 二级分组（标签）侧栏（左侧，含分组标题）
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> TagSidebar;

	// AND/OR 模式容器
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> ModeBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTileView> IconTileView;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> SearchBox;

	// 统计/状态文字："已开发 9 / 共 842"
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	// 关闭按钮（可选）
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	// 只看已开发 的切换按钮（可选）
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DevelopedFilterButton;

private:
	// 全部条目（建一次，过滤时只重排 TileView 的 item 列表）
	UPROPERTY()
	TArray<TObjectPtr<UY3SkillEntryData>> AllEntries;

	int32 DevelopedCount = 0;

	// 一级 tab：all/str/agi/int/allhero/monster/system
	FString PrimFilter = TEXT("all");
	// 二级标签（多选）
	TArray<FString> ActiveTags;
	// 标签模式：true=交集AND / false=并集OR
	bool bTagModeAnd = true;

	UPROPERTY()
	TArray<TObjectPtr<UY3FilterChip>> PrimChips;
	UPROPERTY()
	TArray<TObjectPtr<UY3FilterChip>> TagChips;
	UPROPERTY()
	TArray<TObjectPtr<UY3FilterChip>> ModeChips;

	void BuildEntries();
	void BuildFilterChips();
	void ApplyFilter();

	// 一级 tab 命中判断（按 PrimFilter 匹配 Attr/Cat）
	bool PassesPrim(const UY3SkillEntryData* E) const;

	UFUNCTION()
	void OnFilterChipClicked(UY3FilterChip* Chip);

	UFUNCTION()
	void OnSearchChanged(const FText& Text);

	UFUNCTION()
	void OnItemClicked(UObject* Item);

	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnDevelopedFilterClicked();
};
