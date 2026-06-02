// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoadScreenWgt.generated.h"

/**
 * 
 */
UCLASS()
class AURA_LEARN_API ULoadScreenWgt : public UUserWidget
{
	GENERATED_BODY()

public:

	/*
	 * 该事件调用时，界面以及设置好自己的ViewModel
	 * 因此需要依赖ViewModel的操作可在此事件上执行
	 */
	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void BlueprintInitWgtEvent();
};
