#include "Y3/UI/Y3SkillAtlasEntry.h"
#include "Y3/UI/Y3SkillRegistry.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UY3SkillAtlasEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	Data = Cast<UY3SkillEntryData>(ListItemObject);
	if (!Data)
	{
		return;
	}

	const bool bDeveloped = Data->IsDeveloped();

	// 图标：软引用同步加载（TileView 只渲染可见项，一次也就十几张）
	if (IconImage)
	{
		if (UTexture2D* Tex = Data->Row.Icon.LoadSynchronous())
		{
			IconImage->SetBrushFromTexture(Tex);
			// 未开发的图标压暗
			IconImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, bDeveloped ? 1.0f : 0.5f));
		}
	}

	if (StatusBorder)
	{
		StatusBorder->SetBrushColor(bDeveloped ? DevelopedColor : UndevelopedColor);
	}

	if (NameText)
	{
		NameText->SetText(FText::FromString(Data->Row.DisplayName));
	}

	// 鼠标悬浮提示：名称 +(已/未开发)
	SetToolTipText(FText::FromString(FString::Printf(TEXT("%s [%s]"),
		*Data->Row.DisplayName, bDeveloped ? TEXT("已开发·点击测试") : TEXT("未开发"))));
}
