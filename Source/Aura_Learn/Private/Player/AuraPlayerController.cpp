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
