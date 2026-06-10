// 学习使用


#include "Player/AuraPlayerController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Input/AuraInputComponent.h"
#include "Interaction/EnemyInterface.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Actor/MagicCircle.h"
#include "Aura_Learn/Aura_Learn.h"
#include "Components/DecalComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Character.h"
#include "Interaction/CombatInterface.h"
#include "Interaction/HighlightInterface.h"
#include "UI/Widget/DamageTextComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/AuraAbilitySystemBPLibary.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Y3/Account/Y3AccountSubsystem.h"

AAuraPlayerController::AAuraPlayerController():
	Spline(CreateDefaultSubobject<USplineComponent>("Spline"))
{
	bReplicates = true;//允许复制 复制基本指的是当一个实体在服务器上发生变化时，该变化会被发送至每个客户端
}

void AAuraPlayerController::HideMagicCircle()
{
	if(IsValid(MagicCircle))
	{
		MagicCircle->Destroy();
		bShowMouseCursor = true;
	}
}

void AAuraPlayerController::ShowMagicCircle()
{
	if (IsValid(MagicCircleClass)&&!IsValid(MagicCircle))
	{
		MagicCircle = GetWorld()->SpawnActor<AMagicCircle>(MagicCircleClass);
		bShowMouseCursor = false;
	}

}

void AAuraPlayerController::SetMagicCircleMaterial(UMaterialInterface* DecalMaterial)
{
	if (IsValid(MagicCircle))
	{
		MagicCircle->GetMagicCircleDecal().SetMaterial(0,DecalMaterial);
	}
}

void AAuraPlayerController::ShowDamage_Implementation(float DamageAmount, ACharacter* TargetCharacter, const bool bBlocked, const bool bCriticalHit)
{
	if(IsValid(DamageTextComponentClass)&&IsValid(TargetCharacter)&&IsLocalController())//仅在客户端显示
	{
		//TODO::找到并非每次都创建新的 方法 (对象池?)
		auto DamageText = NewObject<UDamageTextComponent>(TargetCharacter, DamageTextComponentClass);
		DamageText->RegisterComponent();//注册此组件，创建任何渲染/物理状态  动态创建可以用此
		DamageText->AttachToComponent(TargetCharacter->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

		DamageText->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);//

		DamageText->SetDamageText(DamageAmount, bBlocked, bCriticalHit);
	}

}

void AAuraPlayerController::BeginPlay()
{
	Super::BeginPlay();
	check(AuraContext)//断言 如果为false 直接崩溃
	auto SubSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());

	if (SubSystem)
	{
		SubSystem->AddMappingContext(AuraContext, 0); //优先级最高
	}
	

	bShowMouseCursor = true;//显示鼠标光标
	DefaultMouseCursor = EMouseCursor::Default;

	//设置输入模式内容
	FInputModeGameAndUI InputModeData;
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);//设置鼠标锁定到窗口的模式
	InputModeData.SetHideCursorDuringCapture(false);//当窗口捕捉到光标，不隐藏光标
	SetInputMode(InputModeData);

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			Y3AccountUiRefreshTimer,
			this,
			&AAuraPlayerController::Y3RefreshAccountWidgets,
			0.5f,
			true,
			0.5f);
	}
}

void AAuraPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	//使用的增强输入，将输入组件获取
	auto AuraInputCmpt = CastChecked<UAuraInputComponent>(InputComponent);//该转换如果失败，触发断言

	//绑定输入动作到函数
	AuraInputCmpt->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAuraPlayerController::Move);

	AuraInputCmpt->BindAction(ShiftAction, ETriggerEvent::Started, this, &AAuraPlayerController::ShiftPressed);
	AuraInputCmpt->BindAction(ShiftAction, ETriggerEvent::Completed, this, &AAuraPlayerController::ShiftReleased);

	//绑定回调函数到InputAction
	AuraInputCmpt->BindAbilityActions(InputConfig, this, &ThisClass::AbilityInputTagPressed, &ThisClass::AbilityInputTagReleased, &ThisClass::AbilityInputTagHeld);
}

void AAuraPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	CursorTrace();
	AutoRun();

	UpdateMagicCircleLocation();//依赖 CursorTrace
}

void AAuraPlayerController::Y3RefreshAccountWidgets()
{
	TArray<UUserWidget*> Widgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, UUserWidget::StaticClass(), false);

	for (UUserWidget* Widget : Widgets)
	{
		if (!Widget)
		{
			continue;
		}

		const FString ClassName = Widget->GetClass()->GetName();
		if (ClassName.Contains(TEXT("WBP_MainMenu")))
		{
			Y3InjectMainMenuAccountPanel(Widget);
		}
		else if (ClassName.Contains(TEXT("WBP_HeroSelection")))
		{
			Y3UpdateSelectionAccountInfo(Widget);
		}
	}
}

void AAuraPlayerController::Y3InjectMainMenuAccountPanel(UUserWidget* Widget)
{
	if (!Widget || !Widget->WidgetTree)
	{
		return;
	}

	if (UTextBlock* ExistingText = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("Y3_TxtAccountInfo"))))
	{
		ExistingText->SetText(FText::FromString(Y3BuildAccountSummaryText(true)));
		return;
	}

	UOverlay* RootOverlay = Cast<UOverlay>(Widget->WidgetTree->FindWidget(TEXT("RootOverlay")));
	if (!RootOverlay)
	{
		RootOverlay = Cast<UOverlay>(Widget->WidgetTree->RootWidget);
	}
	if (!RootOverlay)
	{
		return;
	}

	UVerticalBox* Panel = Widget->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Y3_AccountPanel"));
	UTextBlock* AccountText = Widget->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Y3_TxtAccountInfo"));
	UButton* NewButton = Widget->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Y3_BtnNewAccount"));
	UTextBlock* NewButtonText = Widget->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Y3_TxtNewAccount"));
	UButton* SwitchButton = Widget->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Y3_BtnSwitchAccount"));
	UTextBlock* SwitchButtonText = Widget->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Y3_TxtSwitchAccount"));

	if (!Panel || !AccountText || !NewButton || !NewButtonText || !SwitchButton || !SwitchButtonText)
	{
		return;
	}

	AccountText->SetText(FText::FromString(Y3BuildAccountSummaryText(true)));
	AccountText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.95f, 1.f, 1.f)));

	NewButtonText->SetText(FText::FromString(TEXT("新账号")));
	SwitchButtonText->SetText(FText::FromString(TEXT("切换账号")));
	NewButton->AddChild(NewButtonText);
	SwitchButton->AddChild(SwitchButtonText);
	NewButton->OnClicked.AddDynamic(this, &AAuraPlayerController::Y3CreateNewAccountFromMenu);
	SwitchButton->OnClicked.AddDynamic(this, &AAuraPlayerController::Y3CycleLocalAccountFromMenu);

	if (UVerticalBoxSlot* TextSlot = Panel->AddChildToVerticalBox(AccountText))
	{
		TextSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}
	if (UVerticalBoxSlot* NewSlot = Panel->AddChildToVerticalBox(NewButton))
	{
		NewSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}
	Panel->AddChildToVerticalBox(SwitchButton);

	if (UOverlaySlot* OverlaySlot = RootOverlay->AddChildToOverlay(Panel))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Right);
		OverlaySlot->SetVerticalAlignment(VAlign_Top);
		OverlaySlot->SetPadding(FMargin(0.f, 32.f, 40.f, 0.f));
	}
}

void AAuraPlayerController::Y3UpdateSelectionAccountInfo(UUserWidget* Widget)
{
	if (!Widget || !Widget->WidgetTree)
	{
		return;
	}

	if (UTextBlock* StatusText = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("StatusText"))))
	{
		StatusText->SetText(FText::FromString(Y3BuildAccountSummaryText(false)));
	}
}

FString AAuraPlayerController::Y3BuildAccountSummaryText(bool bIncludeAccountId) const
{
	UY3AccountSubsystem* Account = GetGameInstance() ? GetGameInstance()->GetSubsystem<UY3AccountSubsystem>() : nullptr;
	if (!Account)
	{
		return TEXT("账号未加载");
	}

	UY3AccountSaveGame* Save = Account->GetCurrentAccount();
	if (!Save)
	{
		Account->LoadLastAccount();
		Save = Account->GetCurrentAccount();
	}
	if (!Save)
	{
		return TEXT("账号未加载");
	}

	const FString Progress = FString::Printf(
		TEXT("账号等级 Lv.%d  经验 %d/%d  金币 %d"),
		Save->AccountLevel,
		Save->AccountXP,
		Account->GetXPRequiredForLevel(Save->AccountLevel),
		Save->Gold);

	return bIncludeAccountId
		? FString::Printf(TEXT("当前账号\n%s\n%s"), *Account->GetCurrentAccountDisplayLabel(), *Progress)
		: FString::Printf(TEXT("%s\n%s"), *Account->GetCurrentAccountDisplayLabel(), *Progress);
}

void AAuraPlayerController::Y3CreateNewAccountFromMenu()
{
	if (UY3AccountSubsystem* Account = GetGameInstance()->GetSubsystem<UY3AccountSubsystem>())
	{
		Account->CreateNewLocalAccount(TEXT(""));
		Account->PrintCurrentAccount();
		Y3RefreshAccountWidgets();
	}
}

void AAuraPlayerController::Y3CycleLocalAccountFromMenu()
{
	UY3AccountSubsystem* Account = GetGameInstance() ? GetGameInstance()->GetSubsystem<UY3AccountSubsystem>() : nullptr;
	if (!Account)
	{
		return;
	}

	const TArray<FString> Ids = Account->GetKnownLocalAccountIds();
	if (Ids.Num() <= 0)
	{
		Account->CreateNewLocalAccount(TEXT(""));
	}
	else if (Ids.Num() == 1)
	{
		Account->LoginLocalAccount(Ids[0]);
	}
	else
	{
		const UY3AccountSaveGame* Current = Account->GetCurrentAccount();
		const FString CurrentId = Current ? Current->Y3AccountId : FString();
		int32 CurrentIndex = Ids.IndexOfByKey(CurrentId);
		const int32 NextIndex = CurrentIndex == INDEX_NONE ? 0 : (CurrentIndex + 1) % Ids.Num();
		Account->LoginLocalAccount(Ids[NextIndex]);
	}

	Account->PrintCurrentAccount();
	Y3RefreshAccountWidgets();
}

void AAuraPlayerController::Move(const FInputActionValue& InputActionValue)
{
	if (GetAuraASC() && GetAuraASC()->HasMatchingGameplayTag(FAuraGmaeplayTags::GetInstance().Player_Block_InputPressed))
	{
		return;
	}
	const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();

	//控制器的旋转跟随玩家的视角，因此需要设置旋转和朝向
	const FRotator YawRotation {0,GetControlRotation().Yaw ,0};//根据控制器Yaw创建新的旋转
	//FRotationMatrix类是旋转构造的旋转矩阵。通过矩阵，可以将旋转应用于向量，以便在 3D 空间中将其旋转
	const FVector ForwardDirction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);//GetUnitAxis用于获取旋转矩阵对应轴的向量
	const FVector RightDirction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	//在控制的pawn上应用输入移动
	if(auto ControlledPawn = GetPawn())
	{
		ControlledPawn->AddMovementInput(ForwardDirction, InputAxisVector.Y);
		ControlledPawn->AddMovementInput(RightDirction, InputAxisVector.X);//输入中X是水平轴 AD
	}
}

void AAuraPlayerController::CursorTrace()
{

	if(GetAuraASC()&& GetAuraASC()->HasMatchingGameplayTag(FAuraGmaeplayTags::GetInstance().Player_Block_CursorTrace))
	{
		UnHighlightActor(MouseHoverLastActor);
		UnHighlightActor(MouseHoverCurrentActor);
		MouseHoverCurrentActor = nullptr;
		MouseHoverLastActor = nullptr;
		return;
	}

	//捕获鼠标下的物体
	GetHitResultUnderCursor(ECC_Target, false, CursorHit); //检测返回鼠标指针下的对象
	if (!CursorHit.bBlockingHit)return;

	if (IsValid(CursorHit.GetActor()) && CursorHit.GetActor()->Implements<UHighlightInterface>())
	{
		MouseHoverCurrentActor = CursorHit.GetActor();
	}
	else
	{
		MouseHoverCurrentActor = nullptr;
	}

	UnHighlightActor(MouseHoverLastActor);
	HighlightActor(MouseHoverCurrentActor);

	MouseHoverLastActor = MouseHoverCurrentActor;
}

void AAuraPlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{

	if (GetAuraASC() && GetAuraASC()->HasMatchingGameplayTag(FAuraGmaeplayTags::GetInstance().Player_Block_InputPressed))
	{
		return;
	}

	//输入的是左键？
	if(InputTag.MatchesTagExact(FAuraGmaeplayTags::GetInstance().InputTag_LMB))
	{
		// [Y3] 左键永远视为移动,不锁敌/不左键攻击(避免误操作)
		TargetingStatus = ETargetingStatus::NotTargeting;
		bAutoRunning = false;
	}

	if(GetAuraASC())
	{
		GetAuraASC()->AbilityInputTagPressed(InputTag);
	}
}

void AAuraPlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{

	if (!IsValid(GetAuraASC()))return;
	if (GetAuraASC()->HasMatchingGameplayTag(FAuraGmaeplayTags::GetInstance().Player_Block_InputReleased))
	{
		return;
	}

	//放掉的是左键 或者存在交互物？
	if (!InputTag.MatchesTagExact(FAuraGmaeplayTags::GetInstance().InputTag_LMB) || TargetingStatus != ETargetingStatus::NotTargeting ||bShiftKeyDown)
	{
		GetAuraASC()->AbilityInputTagReleased(InputTag);
	}
	else //生成导航路径
	{
		auto ControlledPawn = GetPawn();
		if (FllowTime <= ShortPressThreshold && IsValid(ControlledPawn)) //点击移动 生成路径
		{
			//如果存在可交互物，那么移动的目标位置为交互对象的目标位置
			if(IsValid(MouseHoverLastActor)&& MouseHoverLastActor->Implements<UHighlightInterface>())
			{
				IHighlightInterface::Execute_SetMoveToLocation(MouseHoverLastActor, CachedDestination);
			}else
			{
				//生成点击的特效
				if (bShowMouseCursor)
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ClickNiagara, CachedDestination);
			}

			if (UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(
				this, ControlledPawn->GetActorLocation(), CachedDestination))
			{
				//将生成的导航点添加至样条线
				Spline->ClearSplinePoints();
				for (const auto& CurPoint : NavPath->PathPoints)
				{
					Spline->AddSplinePoint(CurPoint, ESplineCoordinateSpace::World);
				}

				bAutoRunning = true;

				if (NavPath->PathPoints.Num() > 0)
				{
					CachedDestination = NavPath->PathPoints.Last(); //点击的点可能永远达不到，但是导航生成的点一定是有能达到的，因此缓存导航生成的最后一个点
					bAutoRunning = true;
				}
			}

		}
		FllowTime = 0.f;
		TargetingStatus=ETargetingStatus::NotTargeting;
	}

}

void AAuraPlayerController::AbilityInputTagHeld(FGameplayTag InputTag)
{

	if (!IsValid(GetAuraASC()))return;
	if (GetAuraASC()->HasMatchingGameplayTag(FAuraGmaeplayTags::GetInstance().Player_Block_InputHeld))
	{
		return;
	}

	//如果并非左键 或者存在交互对象 或者按住shift则执行技能
	if (!InputTag.MatchesTagExact(FAuraGmaeplayTags::GetInstance().InputTag_LMB)|| TargetingStatus == ETargetingStatus::TargetingEnemy ||bShiftKeyDown)
	{
		GetAuraASC()->AbilityInputTagHeld(InputTag);

		return;
	}

	
	FllowTime += GetWorld()->GetDeltaSeconds();//累加每帧间的时间差值 相当于每秒+1

	if(CursorHit.bBlockingHit)
	{
		CachedDestination = CursorHit.ImpactPoint;
	}

	if(auto ControledPawn = GetPawn())
	{
		const auto WorldDirection = (CachedDestination - ControledPawn->GetActorLocation()).GetSafeNormal();
		ControledPawn->AddMovementInput(WorldDirection);
	}

}

void AAuraPlayerController::AutoRun()
{
	auto ControledPawn = GetPawn();
	
	if (IsValid(ControledPawn)&&bAutoRunning)
	{
		//从线上找到距离角色最近的点
		const FVector LocationOnSpline = Spline->FindLocationClosestToWorldLocation(ControledPawn->GetActorLocation(), ESplineCoordinateSpace::World);
		//样条线中距离该点最近点的切线向量
		const auto Direction = Spline->FindDirectionClosestToWorldLocation(LocationOnSpline, ESplineCoordinateSpace::World);
		ControledPawn->AddMovementInput(Direction);

		const auto DistanceToTargetLocation = (LocationOnSpline - CachedDestination).Length();

		if (DistanceToTargetLocation <= AutoRunAcceptanceRadius)
		{
			bAutoRunning = false;
		}
	}
}

UAuraAbilitySystemComponent* AAuraPlayerController::GetAuraASC()
{
	if(!IsValid(AuraAbilitySystemComponent))
	{
		AuraAbilitySystemComponent=Cast<UAuraAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn<APawn>()));
	}
	return AuraAbilitySystemComponent;
}

void AAuraPlayerController::UpdateMagicCircleLocation()
{

	if (!IsValid(MagicCircle))return;
	
	MagicCircle->SetActorLocation(CursorHit.ImpactPoint);
	
}

void AAuraPlayerController::HighlightActor(AActor* InActor)
{
	if (IsValid(InActor) && InActor->Implements<UHighlightInterface>())
	{
		IHighlightInterface::Execute_HighlightActor(InActor);
	}
}

void AAuraPlayerController::UnHighlightActor(AActor* InActor)
{
	if (IsValid(InActor) && InActor->Implements<UHighlightInterface>())
	{
		IHighlightInterface::Execute_UnHightlightActor(InActor);
	}
}

// ===================== Y3 测试模式：控制台快速测试技能 =====================

namespace
{
FString Y3_NormalizeAbilityClassPath(const FString& AbilityPath)
{
	FString Path = AbilityPath.TrimStartAndEnd();
	if (Path.IsEmpty() || Path.Contains(TEXT(".")))
	{
		return Path;
	}

	FString Left;
	FString Name;
	if (Path.Split(TEXT("/"), &Left, &Name, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		Path = Path + TEXT(".") + Name + TEXT("_C");
	}
	return Path;
}

UClass* Y3_LoadAbilityClass(const FString& AbilityPath)
{
	const FString Path = Y3_NormalizeAbilityClassPath(AbilityPath);
	UClass* Cls = LoadObject<UClass>(nullptr, *Path);
	if (!Cls || !Cls->IsChildOf(UGameplayAbility::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Y3Test] 加载技能失败: %s"), *Path);
		return nullptr;
	}
	return Cls;
}

FGameplayTag Y3_GetAbilityTagFromClass(TSubclassOf<UGameplayAbility> AbilityClass)
{
	const UGameplayAbility* AbilityCDO = AbilityClass ? AbilityClass->GetDefaultObject<UGameplayAbility>() : nullptr;
	if (!AbilityCDO)
	{
		return FGameplayTag();
	}

	for (const FGameplayTag& Tag : AbilityCDO->AbilityTags)
	{
		if (Tag.MatchesTag(FAuraGmaeplayTags::GetInstance().Abilities))
		{
			return Tag;
		}
	}
	return FGameplayTag();
}

FGameplayTag Y3_RequestGameplayTag(const FString& TagName)
{
	FString CleanName = TagName.TrimStartAndEnd();
	if (CleanName.StartsWith(TEXT("(TagName=\"")) && CleanName.EndsWith(TEXT("\")")))
	{
		CleanName = CleanName.Mid(10, CleanName.Len() - 12);
	}
	return FGameplayTag::RequestGameplayTag(FName(*CleanName), false);
}
}

void AAuraPlayerController::Y3TestGive(const FString& AbilityPath)
{
	UAuraAbilitySystemComponent* ASC = GetAuraASC();
	if (!ASC) { UE_LOG(LogTemp, Warning, TEXT("[Y3Test] 无 ASC")); return; }

	if (UClass* Cls = Y3_LoadAbilityClass(AbilityPath))
	{
		GiveTestAbility(Cls, 1);
	}
}

void AAuraPlayerController::Y3TestEquip(const FString& AbilityPath, const FString& SlotTagName)
{
	UAuraAbilitySystemComponent* ASC = GetAuraASC();
	if (!ASC) { UE_LOG(LogTemp, Warning, TEXT("[Y3Test] 无 ASC")); return; }

	UClass* AbilityClass = Y3_LoadAbilityClass(AbilityPath);
	if (!AbilityClass)
	{
		return;
	}

	const FGameplayTag AbilityTag = Y3_GetAbilityTagFromClass(AbilityClass);
	if (!AbilityTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Y3Test] 技能没有 Abilities.* 标签: %s"), *AbilityClass->GetName());
		return;
	}

	const FGameplayTag SlotTag = Y3_RequestGameplayTag(SlotTagName);
	if (!SlotTag.IsValid() || !SlotTag.MatchesTag(FAuraGmaeplayTags::GetInstance().InputTag))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Y3Test] 无效槽位标签: %s"), *SlotTagName);
		return;
	}

	const FAuraGmaeplayTags& T = FAuraGmaeplayTags::GetInstance();
	if (FGameplayAbilitySpec* ExistingSpec = ASC->GetSpecFromAbilityTag(AbilityTag))
	{
		const FGameplayTag CurrentStatus = ASC->GetStatusFromSpec(*ExistingSpec);
		if (!CurrentStatus.MatchesTagExact(T.Abilities_Status_Unlocked) &&
			!CurrentStatus.MatchesTagExact(T.Abilities_Status_Equipped))
		{
			if (CurrentStatus.IsValid())
			{
				ExistingSpec->DynamicAbilityTags.RemoveTag(CurrentStatus);
			}
			ExistingSpec->DynamicAbilityTags.AddTag(T.Abilities_Status_Unlocked);
			ASC->MarkAbilitySpecDirty(*ExistingSpec);
		}
	}
	else
	{
		FGameplayAbilitySpec Spec(AbilityClass, 1);
		Spec.DynamicAbilityTags.AddTag(T.Abilities_Status_Unlocked);
		ASC->GiveAbility(Spec);
	}

	ASC->Y3_EquipAbilityToSlot(AbilityTag, SlotTag);
	UE_LOG(LogTemp, Log, TEXT("[Y3Test] 装备 %s -> %s"), *AbilityTag.ToString(), *SlotTag.ToString());
}

void AAuraPlayerController::Y3TestActivateSlot(const FString& SlotTagName)
{
	UAuraAbilitySystemComponent* ASC = GetAuraASC();
	if (!ASC) { UE_LOG(LogTemp, Warning, TEXT("[Y3Test] 无 ASC")); return; }

	const FGameplayTag SlotTag = Y3_RequestGameplayTag(SlotTagName);
	if (!SlotTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Y3Test] 无效槽位标签: %s"), *SlotTagName);
		return;
	}

	FGameplayAbilitySpec* Spec = ASC->GetAbilitySpecWithSlot(SlotTag);
	if (!Spec)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Y3Test] 槽位无技能: %s"), *SlotTag.ToString());
		return;
	}

	const FGameplayTag AbilityTag = UAuraAbilitySystemComponent::GetAbilityTagFromSpec(*Spec);
	const bool bOk = ASC->TryActivateAbility(Spec->Handle);
	UE_LOG(LogTemp, Log, TEXT("[Y3Test] 激活槽 %s -> %s (%s)"),
		*SlotTag.ToString(),
		bOk ? TEXT("成功") : TEXT("失败"),
		*AbilityTag.ToString());
}

void AAuraPlayerController::Y3DebugAbilities()
{
	UAuraAbilitySystemComponent* ASC = GetAuraASC();
	if (!ASC) { UE_LOG(LogTemp, Warning, TEXT("[Y3Debug] 无 ASC")); return; }

	int32 Index = 0;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		const FGameplayTag AbilityTag = UAuraAbilitySystemComponent::GetAbilityTagFromSpec(Spec);
		const FGameplayTag InputTag = UAuraAbilitySystemComponent::GetInputTagFromSpec(Spec);
		const FGameplayTag StatusTag = UAuraAbilitySystemComponent::GetStatusFromSpec(Spec);
		UE_LOG(LogTemp, Log, TEXT("[Y3Debug] #%d Ability=%s Class=%s Level=%d Slot=%s Status=%s Active=%s DynamicTags=%s"),
			Index++,
			*AbilityTag.ToString(),
			Spec.Ability ? *Spec.Ability->GetName() : TEXT("None"),
			Spec.Level,
			*InputTag.ToString(),
			*StatusTag.ToString(),
			Spec.IsActive() ? TEXT("true") : TEXT("false"),
			*Spec.DynamicAbilityTags.ToString());
	}
}

void AAuraPlayerController::GiveTestAbility(TSubclassOf<UGameplayAbility> AbilityClass, int32 Level)
{
	UAuraAbilitySystemComponent* ASC = GetAuraASC();
	if (!ASC || !AbilityClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Y3Test] GiveTestAbility: 无 ASC 或空技能类"));
		return;
	}

	const int32 Lvl = FMath::Max(1, Level);
	const FAuraGmaeplayTags& T = FAuraGmaeplayTags::GetInstance();
	FGameplayAbilitySpec Spec(AbilityClass, Lvl);
	Spec.DynamicAbilityTags.AddTag(T.Abilities_Status_Unlocked);
	const FGameplayAbilitySpecHandle H = ASC->GiveAbility(Spec);
	const bool bOk = ASC->TryActivateAbility(H);
	UE_LOG(LogTemp, Log, TEXT("[Y3Test] 授予并激活 %s (Lv%d) -> %s"),
		*AbilityClass->GetName(), Lvl, bOk ? TEXT("成功") : TEXT("失败"));
}

void AAuraPlayerController::Y3Atlas()
{
	UClass* WidgetClass = LoadObject<UClass>(nullptr, TEXT("/Game/UI/Menus/WBP_SkillAtlas.WBP_SkillAtlas_C"));
	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Y3Atlas] 找不到 /Game/UI/Menus/WBP_SkillAtlas"));
		return;
	}
	if (UUserWidget* W = CreateWidget<UUserWidget>(this, WidgetClass))
	{
		W->AddToViewport(500);
		bShowMouseCursor = true;
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		SetInputMode(Mode);
		UE_LOG(LogTemp, Log, TEXT("[Y3Atlas] 打开技能图鉴"));
	}
}

void AAuraPlayerController::Y3TestGiveAll()
{
	UAuraAbilitySystemComponent* ASC = GetAuraASC();
	if (!ASC) { UE_LOG(LogTemp, Warning, TEXT("[Y3Test] 无 ASC")); return; }

	UAbilityInfo* Info = UAuraAbilitySystemBPLibary::GetAbilityInfo(this);
	if (!Info) { UE_LOG(LogTemp, Warning, TEXT("[Y3Test] 无 AbilityInfo")); return; }

	const FAuraGmaeplayTags& T = FAuraGmaeplayTags::GetInstance();
	int32 Count = 0;
	for (const FAuraAbilityInfo& I : Info->AbilityInfomation)
	{
		if (!I.Ability) continue;
		FGameplayAbilitySpec Spec(I.Ability, 1);
		Spec.DynamicAbilityTags.AddTag(T.Abilities_Status_Unlocked);
		ASC->GiveAbility(Spec);
		++Count;
	}
	UE_LOG(LogTemp, Log, TEXT("[Y3Test] 授予技能库全部 %d 个"), Count);
}

void AAuraPlayerController::Y3TestClear()
{
	UAuraAbilitySystemComponent* ASC = GetAuraASC();
	if (!ASC) return;

	TArray<FGameplayAbilitySpecHandle> Handles;
	for (const FGameplayAbilitySpec& S : ASC->GetActivatableAbilities())
	{
		Handles.Add(S.Handle);
	}
	for (const FGameplayAbilitySpecHandle& H : Handles)
	{
		ASC->ClearAbility(H);
	}
	UE_LOG(LogTemp, Log, TEXT("[Y3Test] 清空 %d 个技能"), Handles.Num());
}

void AAuraPlayerController::Y3Account()
{
	if (UY3AccountSubsystem* Account = GetGameInstance()->GetSubsystem<UY3AccountSubsystem>())
	{
		Account->LoadLastAccount();
		Account->PrintCurrentAccount();
	}
}

void AAuraPlayerController::Y3AccountNew(const FString& AccountId)
{
	if (UY3AccountSubsystem* Account = GetGameInstance()->GetSubsystem<UY3AccountSubsystem>())
	{
		Account->CreateNewLocalAccount(AccountId);
		Account->PrintCurrentAccount();
	}
}

void AAuraPlayerController::Y3AccountLogin(const FString& AccountId)
{
	if (UY3AccountSubsystem* Account = GetGameInstance()->GetSubsystem<UY3AccountSubsystem>())
	{
		if (!Account->LoginLocalAccount(AccountId))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Y3Account] 登录失败,未找到账号: %s"), *AccountId);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
					FString::Printf(TEXT("登录失败: %s"), *AccountId));
			}
			return;
		}
		Account->PrintCurrentAccount();
	}
}

void AAuraPlayerController::Y3AccountList()
{
	if (UY3AccountSubsystem* Account = GetGameInstance()->GetSubsystem<UY3AccountSubsystem>())
	{
		Account->LoadLastAccount();
		const TArray<FString> Ids = Account->GetKnownLocalAccountIds();
		UE_LOG(LogTemp, Log, TEXT("[Y3Account] Known local accounts: %d"), Ids.Num());
		for (const FString& Id : Ids)
		{
			const FString Label = Account->GetDisplayNameForAccountId(Id) + TEXT("：") + Id;
			UE_LOG(LogTemp, Log, TEXT("[Y3Account] - %s"), *Label);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Cyan, Label);
			}
		}
	}
}

void AAuraPlayerController::Y3AccountReward(int32 AccountXPReward, int32 GoldReward)
{
	if (UY3AccountSubsystem* Account = GetGameInstance()->GetSubsystem<UY3AccountSubsystem>())
	{
		Account->AddRunReward(AccountXPReward, GoldReward);
		Account->PrintCurrentAccount();
	}
}
