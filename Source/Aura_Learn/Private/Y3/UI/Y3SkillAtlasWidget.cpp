#include "Y3/UI/Y3SkillAtlasWidget.h"
#include "Y3/UI/Y3SkillRegistry.h"
#include "Y3/UI/Y3FilterChip.h"
#include "Player/AuraPlayerController.h"
#include "Components/TileView.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/WrapBox.h"
#include "Components/VerticalBox.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/DataTable.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "Engine/Texture2D.h"

namespace
{
	// 一级 tab(属性/分类)：key, 中文标签。来自 Tools/skill_filter_struct.json。
	// str/agi/int/allhero 走 Row.Attr(数据值: str/agi/int/all)；monster/system 走 Row.Cat。
	struct FPrimDef { const TCHAR* Key; const TCHAR* Label; };
	static const FPrimDef GPrims[] = {
		{ TEXT("all"),     TEXT("全部") },
		{ TEXT("str"),     TEXT("力量英雄") },
		{ TEXT("agi"),     TEXT("敏捷英雄") },
		{ TEXT("int"),     TEXT("智力英雄") },
		{ TEXT("allhero"), TEXT("全才英雄") },
		{ TEXT("monster"), TEXT("怪物技能") },
		{ TEXT("system"),  TEXT("系统/道具") },
	};

	// 二级分组(组名 -> 标签)，多选 + AND/OR。来自 Tools/skill_filter_struct.json。
	struct FTagGroup { const TCHAR* Group; TArray<FString> Tags; };
	static TArray<FTagGroup> MakeTagGroups()
	{
		return {
			{ TEXT("施法方式"), { TEXT("主动"), TEXT("被动"), TEXT("引导"), TEXT("无目标"), TEXT("指向地面"), TEXT("指向单位"), TEXT("自动施法") } },
			{ TEXT("伤害类型"), { TEXT("魔法伤害"), TEXT("物理伤害"), TEXT("纯粹伤害") } },
			{ TEXT("控制效果"), { TEXT("眩晕"), TEXT("减速"), TEXT("沉默"), TEXT("缠绕/定身"), TEXT("击飞/位移"), TEXT("嘲讽/恐惧") } },
			{ TEXT("玩法功能"), { TEXT("范围伤害(AOE)"), TEXT("持续伤害(DOT)"), TEXT("治疗"), TEXT("护盾/减伤"), TEXT("增益/加速"), TEXT("召唤"), TEXT("驱散/净化"), TEXT("隐身"), TEXT("视野/侦查") } },
			{ TEXT("魔免交互"), { TEXT("穿透魔免"), TEXT("被魔免阻挡") } },
			{ TEXT("驱散性"),   { TEXT("可驱散"), TEXT("仅强驱散"), TEXT("不可驱散") } },
			{ TEXT("目标阵营"), { TEXT("敌方目标"), TEXT("友方目标") } },
		};
	}

	// 标签 -> 所属分组下标（找不到返回 -1）。用于"组内 OR、组间 AND"的分面筛选。
	static int32 GroupIndexOfTag(const FString& Tag)
	{
		const TArray<FTagGroup> Groups = MakeTagGroups();
		for (int32 i = 0; i < Groups.Num(); ++i)
		{
			if (Groups[i].Tags.Contains(Tag)) return i;
		}
		return -1;
	}
}

void UY3SkillAtlasWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildEntries();
	BuildFilterChips();

	if (SearchBox)
	{
		SearchBox->OnTextChanged.AddDynamic(this, &UY3SkillAtlasWidget::OnSearchChanged);
	}
	if (IconTileView)
	{
		IconTileView->OnItemClicked().AddUObject(this, &UY3SkillAtlasWidget::OnItemClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UY3SkillAtlasWidget::OnCloseClicked);
	}
	if (DevelopedFilterButton)
	{
		DevelopedFilterButton->OnClicked.AddDynamic(this, &UY3SkillAtlasWidget::OnDevelopedFilterClicked);
	}

	ApplyFilter();
}

void UY3SkillAtlasWidget::BuildEntries()
{
	AllEntries.Reset();
	DevelopedCount = 0;

	// 已被 DA 覆盖的图标（按贴图短名），未开发区据此去重，避免同一张图既"已开发"又"未开发"。
	TSet<FName> DevelopedIcons;

	// ---- 1) 已开发技能：唯一真源 = DA_AbilityInfo（加技能只动这里，图鉴自动同步）----
	if (AbilityInfoSource)
	{
		for (const FAuraAbilityInfo& I : AbilityInfoSource->AbilityInfomation)
		{
			// 无图标 / 无 GA 的系统级条目（如 ListenForEvent）不进图鉴
			if (!I.AbilityIcon || !I.Ability) continue;

			UY3SkillEntryData* Entry = NewObject<UY3SkillEntryData>(this);
			Entry->RowName = I.AbilityTag.GetTagName();
			Entry->Row.Icon = TSoftObjectPtr<UTexture2D>(I.AbilityIcon.Get());
			Entry->Row.DisplayName = I.DisplayName.IsEmpty() ? I.AbilityTag.ToString() : I.DisplayName;
			Entry->Row.Ability = TSoftClassPtr<UGameplayAbility>(I.Ability.Get()); // 非空 => IsDeveloped()
			Entry->Row.Attr = TEXT("dev");                       // 不参与英雄/怪物一级分类
			Entry->Row.Tags.Add(Y3CastTypeToTag(I.CastType));    // 施法方式归类（主动/被动/引导）
			AllEntries.Add(Entry);
			++DevelopedCount;
			DevelopedIcons.Add(I.AbilityIcon->GetFName());
		}
	}

	// ---- 2) 未开发占位：DT_SkillRegistry 里 Ability 为空、且图标未被 DA 覆盖的行 ----
	if (RegistryTable)
	{
		TArray<FY3SkillRegistryRow*> Rows;
		RegistryTable->GetAllRows<FY3SkillRegistryRow>(TEXT("Y3 Atlas"), Rows);
		const TArray<FName> RowNames = RegistryTable->GetRowNames();
		for (int32 i = 0; i < Rows.Num(); ++i)
		{
			if (!Rows[i]) continue;
			if (!Rows[i]->Ability.IsNull()) continue;                 // 旧 DEV 行交给 DA，跳过
			if (RowNames.IsValidIndex(i) && DevelopedIcons.Contains(RowNames[i])) continue; // 同图去重
			UY3SkillEntryData* Entry = NewObject<UY3SkillEntryData>(this);
			Entry->RowName = RowNames.IsValidIndex(i) ? RowNames[i] : NAME_None;
			Entry->Row = *Rows[i];
			AllEntries.Add(Entry);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Y3Atlas] 载入 %d 个图标，已开发 %d"), AllEntries.Num(), DevelopedCount);
}

void UY3SkillAtlasWidget::BuildFilterChips()
{
	if (!ChipClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Y3Atlas] ChipClass 未设置，跳过筛选标签"));
		return;
	}

	auto MakeChip = [this](UPanelWidget* Box, const FString& Cat, const FString& Val, const FString& Label,
		TArray<TObjectPtr<UY3FilterChip>>& Store) -> UY3FilterChip*
	{
		if (!Box) return nullptr;
		UY3FilterChip* Chip = CreateWidget<UY3FilterChip>(this, ChipClass);
		if (!Chip) return nullptr;
		Chip->Setup(Cat, Val, Label);
		Chip->OnChipClicked.AddDynamic(this, &UY3SkillAtlasWidget::OnFilterChipClicked);
		Box->AddChild(Chip);
		Store.Add(Chip);
		return Chip;
	};

	// ---- 一级 tab：属性/分类（单选，默认"全部"）----
	if (AttrFilterBox)
	{
		AttrFilterBox->ClearChildren();
		PrimChips.Reset();
		for (const FPrimDef& P : GPrims)
		{
			MakeChip(AttrFilterBox, TEXT("prim"), P.Key, P.Label, PrimChips);
		}
		if (PrimChips.Num() > 0) PrimChips[0]->SetSelected(true);
		PrimFilter = TEXT("all");
	}

	// ---- 二级标签：按分组堆到侧栏（多选）----
	if (TagSidebar)
	{
		TagSidebar->ClearChildren();
		TagChips.Reset();
		ActiveTags.Reset();

		// 顶部"清除标签"chip（value 空 = 清空多选）
		MakeChip(TagSidebar, TEXT("tag"), FString(), TEXT("清除标签"), TagChips);

		for (const FTagGroup& G : MakeTagGroups())
		{
			// 分组标题
			if (UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>())
			{
				Title->SetText(FText::FromString(G.Group));
				TagSidebar->AddChild(Title);
			}
			// 该组标签横向铺开
			UWrapBox* GroupBox = WidgetTree->ConstructWidget<UWrapBox>();
			if (!GroupBox) continue;
			TagSidebar->AddChild(GroupBox);
			for (const FString& T : G.Tags)
			{
				MakeChip(GroupBox, TEXT("tag"), T, T, TagChips);
			}
		}
	}

	// ---- AND/OR 模式（单选，默认 AND）----
	if (ModeBox)
	{
		ModeBox->ClearChildren();
		ModeChips.Reset();
		MakeChip(ModeBox, TEXT("mode"), TEXT("and"), TEXT("全部满足"), ModeChips);
		MakeChip(ModeBox, TEXT("mode"), TEXT("or"),  TEXT("任一满足"), ModeChips);
		bTagModeAnd = true;
		if (ModeChips.Num() > 0) ModeChips[0]->SetSelected(true);
	}
}

void UY3SkillAtlasWidget::OnFilterChipClicked(UY3FilterChip* Chip)
{
	if (!Chip) return;

	if (Chip->Category == TEXT("prim"))
	{
		PrimFilter = Chip->Value;
		for (UY3FilterChip* C : PrimChips) { if (C) C->SetSelected(C == Chip); }
	}
	else if (Chip->Category == TEXT("tag"))
	{
		if (Chip->Value.IsEmpty())
		{
			// 清除标签
			ActiveTags.Reset();
			for (UY3FilterChip* C : TagChips) { if (C) C->SetSelected(false); }
		}
		else if (ActiveTags.Contains(Chip->Value))
		{
			ActiveTags.Remove(Chip->Value);
			Chip->SetSelected(false);
		}
		else
		{
			ActiveTags.Add(Chip->Value);
			Chip->SetSelected(true);
		}
	}
	else if (Chip->Category == TEXT("mode"))
	{
		bTagModeAnd = (Chip->Value == TEXT("and"));
		for (UY3FilterChip* C : ModeChips) { if (C) C->SetSelected(C == Chip); }
	}

	ApplyFilter();
}

bool UY3SkillAtlasWidget::PassesPrim(const UY3SkillEntryData* E) const
{
	if (!E) return false;
	if (PrimFilter.IsEmpty() || PrimFilter == TEXT("all")) return true;

	const FString& Attr = E->Row.Attr;
	const FString& Cat  = E->Row.Cat;

	if (PrimFilter == TEXT("str") || PrimFilter == TEXT("agi") || PrimFilter == TEXT("int"))
	{
		return Attr == PrimFilter;
	}
	if (PrimFilter == TEXT("allhero"))
	{
		return Attr == TEXT("all");
	}
	if (PrimFilter == TEXT("monster"))
	{
		// Cat 尚未填充时，把"非英雄"(Attr 空且非 dev)的行归到怪物技能
		return Cat == TEXT("monster") || (Cat.IsEmpty() && Attr.IsEmpty());
	}
	if (PrimFilter == TEXT("system"))
	{
		return Cat == TEXT("system");
	}
	return true;
}

void UY3SkillAtlasWidget::ApplyFilter()
{
	if (!IconTileView) return;

	const FString Query = SearchBox ? SearchBox->GetText().ToString().TrimStartAndEnd() : FString();

	TArray<UObject*> Shown;
	for (UY3SkillEntryData* E : AllEntries)
	{
		if (!E) continue;
		if (bDevelopedOnly && !E->IsDeveloped()) continue;

		// 一级：属性/分类
		if (!PassesPrim(E)) continue;

		// 二级：分面筛选 —— 组内 OR（同组标签互斥，取并集），组间 AND/OR。
		// 例：勾选「主动」+「被动」(同属"施法方式") => 主动 或 被动 都通过，而不是要求同时满足。
		if (ActiveTags.Num() > 0)
		{
			// 先按所属分组归并：每个有选中标签的组，记录本行是否命中该组任一标签
			TMap<int32, bool> GroupHit;
			for (const FString& T : ActiveTags)
			{
				const int32 G = GroupIndexOfTag(T);
				bool& Hit = GroupHit.FindOrAdd(G);   // 默认 false
				if (E->Row.Tags.Contains(T)) Hit = true;
			}

			// 组间组合：AND=每个被选组都要命中 / OR=任一被选组命中即可
			bool bPass = bTagModeAnd;
			for (const TPair<int32, bool>& KV : GroupHit)
			{
				if (bTagModeAnd) { if (!KV.Value) { bPass = false; break; } }
				else             { if (KV.Value)  { bPass = true;  break; } }
			}
			if (!bPass) continue;
		}

		// 搜索：名称 / 英雄 / 行名 / 属性 / 标签
		if (!Query.IsEmpty())
		{
			bool bMatch =
				E->Row.DisplayName.Contains(Query) ||
				E->Row.Hero.Contains(Query) ||
				E->RowName.ToString().Contains(Query) ||
				E->Row.Attr.Contains(Query);
			if (!bMatch)
			{
				for (const FString& T : E->Row.Tags)
				{
					if (T.Contains(Query)) { bMatch = true; break; }
				}
			}
			if (!bMatch) continue;
		}

		Shown.Add(E);
	}

	IconTileView->SetListItems(Shown);

	if (StatusText)
	{
		StatusText->SetText(FText::FromString(FString::Printf(
			TEXT("显示 %d · 已开发 %d / 共 %d"), Shown.Num(), DevelopedCount, AllEntries.Num())));
	}
}

void UY3SkillAtlasWidget::OnSearchChanged(const FText& /*Text*/)
{
	ApplyFilter();
}

void UY3SkillAtlasWidget::OnDevelopedFilterClicked()
{
	bDevelopedOnly = !bDevelopedOnly;
	ApplyFilter();
}

void UY3SkillAtlasWidget::OnItemClicked(UObject* Item)
{
	UY3SkillEntryData* Entry = Cast<UY3SkillEntryData>(Item);
	if (!Entry) return;

	if (!Entry->IsDeveloped())
	{
		UE_LOG(LogTemp, Log, TEXT("[Y3Atlas] %s 未开发，暂无可测试技能"), *Entry->Row.DisplayName);
		return;
	}

	// 已开发 → 加载 GA 类，交给玩家控制器授予+激活（复用测试逻辑）
	UClass* AbilityClass = Entry->Row.Ability.LoadSynchronous();
	if (!AbilityClass) return;

	if (AAuraPlayerController* PC = Cast<AAuraPlayerController>(GetOwningPlayer()))
	{
		PC->GiveTestAbility(AbilityClass, 1);
		UE_LOG(LogTemp, Log, TEXT("[Y3Atlas] 测试技能 %s"), *Entry->Row.DisplayName);
	}
}

void UY3SkillAtlasWidget::OnCloseClicked()
{
	RemoveFromParent();
	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
	}
}
