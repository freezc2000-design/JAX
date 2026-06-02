// 学习使用


#include "AbilitySystem/Data/AttributeInfo.h"

FAuraAttributeInfo UAttributeInfo::FindAttributeInfoForTag(const FGameplayTag& Tag, bool bLogNotFound) const
{
	for (const auto& Info : AttributeInformation)
	{
		if (Info.AttributeTag.MatchesTagExact(Tag))
		{
			return Info;
		}
	}
	//未找到对应Tag
	if (!bLogNotFound)return FAuraAttributeInfo{};

	UE_LOG(LogTemp,Warning,TEXT("在 FAuraAttributeInfo [%s] 未找到tag [%s] "),*GetNameSafe(this),*Tag.ToString())
	return FAuraAttributeInfo{};
}
