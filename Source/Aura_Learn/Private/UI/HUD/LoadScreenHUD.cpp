#include "UI/HUD/LoadScreenHUD.h"
#include "Blueprint/UserWidget.h"
#include "UI/ViewMode/MVVM_LoadScreen.h"
#include "UI/Widget/LoadScreenWgt.h"

void ALoadScreenHUD::BeginPlay()
{
	Super::BeginPlay();

	check(LoadScreenWidgetClass);

	LoadScreenViewModel = NewObject<UMVVM_LoadScreen>(this,LoadScreenViewModelClass);
	LoadScreenViewModel->InitLoadSlot();

	LoadScreenWidget = CreateWidget<ULoadScreenWgt>(GetWorld(), LoadScreenWidgetClass);
	LoadScreenWidget->AddToViewport();
	
	/*
	 * 此时 加载菜单界面已经完成 初始化存档槽界面
	 */
	LoadScreenWidget->BlueprintInitWgtEvent();

	LoadScreenViewModel->LoadData();
}
