// 学习使用

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AuraGameModeBase.generated.h"

class ULootTiers;
class ULoadScreenSaveGame;
class USaveGame;
class UMVVM_LoadSlot;
class UAbilitieDescriptions;
class UAbilityInfo;
class UCharacterClassInfo;
/**
 * 
 */
UCLASS()
class AURA_LEARN_API AAuraGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:

	void SaveSlotData(UMVVM_LoadSlot* LoadSlot, uint8 SlotIdx) const;
	static void DeleteSlotData(UMVVM_LoadSlot* LoadSlot, uint8 SlotIdx);
	ULoadScreenSaveGame* GetSaveSlotData(const FString& SlotName, int32 SlotIdx) const;
	ULoadScreenSaveGame* RetrieveInGameSaveData();//获取当前游戏的存档
	FString GetMapNameFromMapAssetName(const FString& AssetName);//从资源名称获取地图名称
	void SaveWorldState(UWorld* World,const FString& AssetNameOfTargetMap=FString(""), const FName& StartTagOfTargetMap = FName(""));
	void LoadWorldState(UWorld* World);
	void SaveInGameProgressData(ULoadScreenSaveGame* SaveObj);

	void TravelToMap(UMVVM_LoadSlot* Slot);

	void PlayerDied(ACharacter* Deader);//玩家死亡 加载上次状态

	UPROPERTY(EditDefaultsOnly,Category="Character Class Defaults",DisplayName="职业类型信息")
	TObjectPtr<UCharacterClassInfo> CharacterClassInfo;

	UPROPERTY(EditDefaultsOnly, Category = "GAS", DisplayName = "技能信息")
	TObjectPtr<UAbilityInfo> AbilityInfo;

	UPROPERTY(EditDefaultsOnly, Category = "奖励", DisplayName = "战利品配置文件")
	TObjectPtr<ULootTiers> LootTiersInfo;

	UPROPERTY(EditDefaultsOnly,Category="存档",DisplayName="存档类")
	TSubclassOf<USaveGame> LoadScreenSaveGameClass;

	UPROPERTY(EditDefaultsOnly, Category = "存档", DisplayName = "默认起点实例Tag")
	FName DefaultPlayerStartTag;

	UPROPERTY(EditDefaultsOnly, Category = "地图", DisplayName = "初始地图名称")
	FString DefaultMapName{};

	UPROPERTY(EditDefaultsOnly, Category = "地图", DisplayName = "初始地图地图")
	TSoftObjectPtr<UWorld> DefaultMap;
	
	UPROPERTY(EditDefaultsOnly, Category = "地图", DisplayName = "名称映射地图")
	TMap<FString, TSoftObjectPtr<UWorld>> NameToMap;

protected:
	virtual void BeginPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;//获取所有PlayerStart 类型，按Tag匹配返回所需
};
