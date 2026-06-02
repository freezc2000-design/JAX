#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LoadScreenHUD.generated.h"

class UMVVM_LoadScreen;
class ULoadScreenWgt;
/**
 * 
 */
UCLASS()
class AURA_LEARN_API ALoadScreenHUD : public AHUD
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly,DisplayName="加载存档界面类")
	TSubclassOf<ULoadScreenWgt> LoadScreenWidgetClass;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ULoadScreenWgt> LoadScreenWidget;

	UPROPERTY(EditDefaultsOnly,DisplayName="视图模型类")
	TSubclassOf<UMVVM_LoadScreen> LoadScreenViewModelClass;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UMVVM_LoadScreen> LoadScreenViewModel;
protected:
	virtual void BeginPlay() override;
};
