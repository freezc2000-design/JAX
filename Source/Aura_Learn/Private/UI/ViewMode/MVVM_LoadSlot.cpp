 #include "UI/ViewMode/MVVM_LoadSlot.h"

 void UMVVM_LoadSlot::InitSlostIndex()
 {

	 SetWgtSwitcherIndexDel.Broadcast(SlotStatus.GetValue());
 }

 void UMVVM_LoadSlot::SetLoadSlotName(const FString& TargetName)
 {
	 UE_MVVM_SET_PROPERTY_VALUE(LoadSlotName,TargetName);
 }

 void UMVVM_LoadSlot::SetPlayerName(FString InPlayerName)
 {
	 UE_MVVM_SET_PROPERTY_VALUE(PlayerName, InPlayerName);
 }

 void UMVVM_LoadSlot::SetMapName(FString InMapName)
 {
	 UE_MVVM_SET_PROPERTY_VALUE(MapName, InMapName);
 }

 void UMVVM_LoadSlot::SetPlayerLevel(int32 InLevel)
 {
	 UE_MVVM_SET_PROPERTY_VALUE(PlayerLevel, InLevel);
 }
