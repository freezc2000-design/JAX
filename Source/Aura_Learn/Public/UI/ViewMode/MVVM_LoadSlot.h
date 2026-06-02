#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "MVVM_LoadSlot.generated.h"

enum ESaveSlotStatus : uint8;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSetWgtSwitcherIndexDel, int32, WgtSwitcherIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnableSelectSlotBtnDel, bool, bEnable);

/**
 * 
 */
UCLASS()
class AURA_LEARN_API UMVVM_LoadSlot : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void InitSlostIndex();//通过枚举值设置界面显示的子项

	void SetLoadSlotName(const FString& TargetName);
	void SetPlayerName(FString InPlayerName);
	void SetMapName(FString InMapName);
	void SetPlayerLevel(int32 InLevel);

	FString GetPlayerName() const { return PlayerName; }
	const FString& GetLoadSlotName()const { return LoadSlotName; }
	const FString& GetMapName()const { return MapName; }
	int32 GetPlayerLevel() const { return PlayerLevel; };

	UPROPERTY(BlueprintAssignable)
	FSetWgtSwitcherIndexDel SetWgtSwitcherIndexDel;//切换堆叠界面的显示成员

	UPROPERTY(BlueprintAssignable)
	FEnableSelectSlotBtnDel EnableSelectSlotBtnDel;//加载存档按钮启用状态变化

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	FString PlayerName;//输入的存档名字

	UPROPERTY()
	uint8 SlotIndex;//

	UPROPERTY()
	FName PlayerStartTag;

	UPROPERTY()
	FString MapAssetName;

	UPROPERTY()
	TEnumAsByte<ESaveSlotStatus> SlotStatus;

private:
	/*
	 * FieldNotify 启用对属性的变更通知。UE5引入，更轻松地跟踪和响应属性值的变化，尤其是在数据绑定和MVVM模式下使用时。
	 * FieldNotify的变量 被Setter、Getter修饰，必须存在Set和Get函数
	 */
	UPROPERTY(EditAnywhere,BlueprintReadWrite,FieldNotify,Setter,Getter,meta=(AllowPrivateAccess="true"),DisplayName="插槽ID")
	FString LoadSlotName;//存档槽的名字

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess = "true"), DisplayName = "地图名")
	FString MapName;//地图名字

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess = "true"), DisplayName = "角色等级")
	int32 PlayerLevel;//地图名字
};
