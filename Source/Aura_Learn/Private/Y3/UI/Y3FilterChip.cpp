#include "Y3/UI/Y3FilterChip.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UY3FilterChip::Setup(const FString& InCategory, const FString& InValue, const FString& InLabel)
{
	Category = InCategory;
	Value = InValue;
	PendingLabel = InLabel;
	if (ChipText)
	{
		ChipText->SetText(FText::FromString(InLabel));
	}
}

void UY3FilterChip::SetSelected(bool bSelected)
{
	if (ChipButton)
	{
		ChipButton->SetBackgroundColor(bSelected ? SelectedColor : NormalColor);
	}
}

void UY3FilterChip::NativeConstruct()
{
	Super::NativeConstruct();
	if (ChipText && !PendingLabel.IsEmpty())
	{
		ChipText->SetText(FText::FromString(PendingLabel));
	}
	if (ChipButton)
	{
		ChipButton->OnClicked.AddDynamic(this, &UY3FilterChip::HandleClicked);
		ChipButton->SetBackgroundColor(NormalColor);
	}
}

void UY3FilterChip::HandleClicked()
{
	OnChipClicked.Broadcast(this);
}
