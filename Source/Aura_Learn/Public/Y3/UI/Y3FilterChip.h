// Y3 技能图鉴 — 一个可点击的筛选标签（属性/标签 分类用）。
// 数据驱动：图鉴在 C++ 里按注册表的属性/标签自动生成一排 chip，点击回传(类别,值)。
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Y3FilterChip.generated.h"

class UButton;
class UTextBlock;
class UY3FilterChip;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnY3FilterChipClicked, UY3FilterChip*, Chip);

UCLASS(Abstract)
class AURA_LEARN_API UY3FilterChip : public UUserWidget
{
	GENERATED_BODY()

public:
	// 类别："attr"(一级·属性) 或 "tag"(二级·标签)
	UPROPERTY(BlueprintReadOnly, Category = "Y3|Filter")
	FString Category;

	// 该 chip 代表的筛选值（空=全部；"__dev__"=已开发）
	UPROPERTY(BlueprintReadOnly, Category = "Y3|Filter")
	FString Value;

	// 点击回调（图鉴绑定它）
	UPROPERTY(BlueprintAssignable, Category = "Y3|Filter")
	FOnY3FilterChipClicked OnChipClicked;

	// 配置 chip（类别/值/显示文字）
	void Setup(const FString& InCategory, const FString& InValue, const FString& InLabel);

	// 选中态高亮（图鉴切换筛选时调用）
	void SetSelected(bool bSelected);

	virtual void NativeConstruct() override;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ChipButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ChipText;

	UPROPERTY(EditAnywhere, Category = "Y3|Filter")
	FLinearColor SelectedColor = FLinearColor(0.10f, 0.55f, 0.95f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Y3|Filter")
	FLinearColor NormalColor = FLinearColor(0.12f, 0.12f, 0.14f, 1.f);

private:
	FString PendingLabel;

	UFUNCTION()
	void HandleClicked();
};
