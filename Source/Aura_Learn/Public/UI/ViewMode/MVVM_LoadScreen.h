#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "MVVM_LoadScreen.generated.h"

class UMVVM_LoadSlot;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHasSlotSelectedDel);

/**
 * 
 */
UCLASS()
class AURA_LEARN_API UMVVM_LoadScreen : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:

	void InitLoadSlot();

	UPROPERTY(EditDefaultsOnly,DisplayName="存档槽界面的ViewModel类")
	TSubclassOf<UMVVM_LoadSlot> LoadSlotViewModelClass;

	UFUNCTION(BlueprintCallable)
	UMVVM_LoadSlot* GetSlotViewModelByIndex(const uint8 Index) const;

	UFUNCTION(BlueprintCallable)
	void NewSlotBtnPressed(int32 idx,const FString& EnterName);//新存档按钮 按下

	UFUNCTION(BlueprintCallable)
	void NewGameBtnPressed(int32 idx);//新游戏按钮 按下

	UFUNCTION(BlueprintCallable)
	void SelectSlotBtnPressed(int32 idx);

	UFUNCTION(BlueprintCallable)
	void DeleteBtnPressed();

	UFUNCTION(BlueprintCallable)
	void OnPlayBtnPressed();

	void LoadData();//加载存档

	UPROPERTY(BlueprintAssignable)
	FHasSlotSelectedDel HasSlotSelectedDel;//有存档被选中时，得通知开始和删除按钮启用

private:
	UPROPERTY()
	TMap<int32, UMVVM_LoadSlot*> LoadSlots;

	UPROPERTY()
	TObjectPtr<UMVVM_LoadSlot> LoadSlot_0;
	UPROPERTY()
	TObjectPtr<UMVVM_LoadSlot> LoadSlot_1;
	UPROPERTY()
	TObjectPtr<UMVVM_LoadSlot> LoadSlot_2;

	UPROPERTY()
	TObjectPtr<UMVVM_LoadSlot> ActiveSlot{nullptr};//当前激活的存档槽
};
