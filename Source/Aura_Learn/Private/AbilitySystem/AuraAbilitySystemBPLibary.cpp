#include "AbilitySystem/AuraAbilitySystemBPLibary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AuraAbilityTypes.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/Data/AbilitieDescriptions.h"
#include "AbilitySystem/Data/CharacterClassInfo.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "Game/AuraGameModeBase.h"
#include "Game/LoadScreenSaveGame.h"
#include "Interaction/CombatInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AuraPlayerState.h"
#include "UI/HUD/AuraHUD.h"

UOverlayWidgetController* UAuraAbilitySystemBPLibary::GetOverlayWgtController(const UObject* WorldContenxt)
{
	FWidgetControllerParams WgtControllerParams{};
	AAuraHUD* AuraHUD{ nullptr };
	bool Sucesses= SetWgtCtrlParamsByWorldContenxt(WorldContenxt, WgtControllerParams, AuraHUD);
	if (!Sucesses)return nullptr; 

	return AuraHUD->GetOverlaiController(WgtControllerParams);
}

UAttributeMenuWgtController* UAuraAbilitySystemBPLibary::GetAttributeMenuWgtController(const UObject* WorldContenxt)
{
	FWidgetControllerParams WgtControllerParams{};
	AAuraHUD* AuraHUD{ nullptr };
	bool Sucesses = SetWgtCtrlParamsByWorldContenxt(WorldContenxt, WgtControllerParams, AuraHUD);
	if (!Sucesses)return nullptr;

	return AuraHUD->GetAtributeMenuController(WgtControllerParams);
}

USpellMenuWgtController* UAuraAbilitySystemBPLibary::GetSpellMenuWgtController(const UObject* WorldContenxt)
{
	FWidgetControllerParams WgtControllerParams{};
	AAuraHUD* AuraHUD{ nullptr };
	bool Sucesses = SetWgtCtrlParamsByWorldContenxt(WorldContenxt, WgtControllerParams, AuraHUD);
	if (!Sucesses)return nullptr;

	return AuraHUD->GetSpellMenuController(WgtControllerParams);
}

bool UAuraAbilitySystemBPLibary::SetWgtCtrlParamsByWorldContenxt(const UObject* WorldContenxt,
                                                                 OUT FWidgetControllerParams& WgtCtrlParam,
                                                                 AAuraHUD*& OutAuraHud)
{
	auto Controller = UGameplayStatics::GetPlayerController(WorldContenxt, 0); //该函数在本地调用，0一定是玩家自己的控制器
	if (!IsValid(Controller))return false;

	auto PS = Controller->GetPlayerState<AAuraPlayerState>();
	if (!IsValid(PS))return false;

	auto ASC = PS->GetAbilitySystemComponent();
	auto AttributeSet = PS->GetAttributeSet();
	WgtCtrlParam.PlayerController = Controller;
	WgtCtrlParam.PlayerState = PS;
	WgtCtrlParam.AbilitySystemComponent = ASC;
	WgtCtrlParam.AttributeSet = AttributeSet;
	OutAuraHud = Cast<AAuraHUD>(WgtCtrlParam.PlayerController->GetHUD());

	if (!IsValid(OutAuraHud))return false;
	return true;
}

void UAuraAbilitySystemBPLibary::InitializeDefaultAttributes(const UObject* WorldContenxt, ECharacterClass CharacterClass, float Level, UAbilitySystemComponent* ASC)
{
	auto ClassDefaultInfo = GetCharacterClassInfo(WorldContenxt);
	if (!IsValid(ClassDefaultInfo))return;

	auto GEContext = ASC->MakeEffectContext();
	GEContext.AddSourceObject(ASC->GetAvatarActor());
	
	auto PrimaryAttributeSpecHandl = ASC->MakeOutgoingSpec(ClassDefaultInfo->GetClassDefaultInfo(CharacterClass).PrimaryAttributes, Level, GEContext);
	ASC->ApplyGameplayEffectSpecToSelf(*PrimaryAttributeSpecHandl.Data.Get());

	auto SecondaryAttributeSpecHandl = ASC->MakeOutgoingSpec(ClassDefaultInfo->SecondaryAttributes, Level, GEContext);
	ASC->ApplyGameplayEffectSpecToSelf(*SecondaryAttributeSpecHandl.Data.Get());

	auto ViatalAttributeSpecHandl = ASC->MakeOutgoingSpec(ClassDefaultInfo->ViatalAttributes, Level, GEContext);
	ASC->ApplyGameplayEffectSpecToSelf(*ViatalAttributeSpecHandl.Data.Get());
}

void UAuraAbilitySystemBPLibary::InitAttributesFormLoadData(const UObject* WorldContext, UAbilitySystemComponent* ASC, ULoadScreenSaveGame* SaveObj)
{
	auto ClassDefaultInfo = GetCharacterClassInfo(WorldContext);
	if (!ClassDefaultInfo)return;


	const auto& GameplayTags = FAuraGmaeplayTags::GetInstance();

	const AActor* SourceAvatarActor = ASC->GetAvatarActor();

	FGameplayEffectContextHandle EffectContexthandle = ASC->MakeEffectContext();
	EffectContexthandle.AddSourceObject(SourceAvatarActor);

	//主要属性设置
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ClassDefaultInfo->PrimaryAttributes_SetByCaller, 1.f, EffectContexthandle);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Strength, SaveObj->Strength);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Intelligence, SaveObj->Intelligence);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Resilience, SaveObj->Resilience);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Vigor, SaveObj->Vigor);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);

	//次要属性并非存档 靠角色的 DefaultVitalAtributes 和 DefaultVitalAtributes
}

void UAuraAbilitySystemBPLibary::GiveStartupAbilities(const UObject* WorldContenxt, UAbilitySystemComponent* ASC, const ECharacterClass CharacterClass)
{

	auto ClassDefaultInfo = GetCharacterClassInfo(WorldContenxt);
	if (!IsValid(ClassDefaultInfo))return;

	//获取通用能力
	for(const auto& it:ClassDefaultInfo->CommonAbilities)
	{
		FGameplayAbilitySpec GASpec = FGameplayAbilitySpec(it, 1.f);
		ASC->GiveAbility(GASpec);
	}

	//获取该类型特有的能力
	const auto& TargetDefaultInfo=ClassDefaultInfo->GetClassDefaultInfo(CharacterClass);
	for(const auto& Abilities:TargetDefaultInfo.StartupAbilities)
	{
		if(auto CombatInterface=Cast<ICombatInterface>(ASC->GetAvatarActor()))
		{
			
			FGameplayAbilitySpec GASpec = FGameplayAbilitySpec(Abilities, ICombatInterface::Execute_GetPlayerLevel(ASC->GetAvatarActor()));
			ASC->GiveAbility(GASpec);

		}
	}
}

UCharacterClassInfo* UAuraAbilitySystemBPLibary::GetCharacterClassInfo(const UObject* WorldContenxt)
{
	auto GameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(WorldContenxt));
	if (!IsValid(GameMode))return nullptr;

	return GameMode->CharacterClassInfo;
}

UAbilityInfo* UAuraAbilitySystemBPLibary::GetAbilityInfo(const UObject* WorldContext)
{
	auto GameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(WorldContext));
	if (!IsValid(GameMode))return nullptr;

	return GameMode->AbilityInfo;
}

ULootTiers* UAuraAbilitySystemBPLibary::GetLootTiersInfo(const UObject* WorldContext)
{
	auto GameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(WorldContext));
	if (!IsValid(GameMode))return nullptr;

	return GameMode->LootTiersInfo;
}

const FVector UAuraAbilitySystemBPLibary::GetKnockbackForce(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const auto AuraEffectContext = static_cast<const FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetKnockbackForce();
	}
	return FVector::ZeroVector;
}

void UAuraAbilitySystemBPLibary::SetKnockbackForce(FGameplayEffectContextHandle& EffectContextHandle,
	const FVector& InForce)
{
	if (auto AuraEffectContext = static_cast<FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetKnockbackForce(InForce);
	}
}

bool UAuraAbilitySystemBPLibary::IsBlockedHit(const FGameplayEffectContextHandle& EffectContextHandle)
{
	
	if (auto GEContext = static_cast<const FAuraGameEffectContext*>(EffectContextHandle.Get()))
		return GEContext->IsBlockedHit();
	return false;
}

bool UAuraAbilitySystemBPLibary::IsCriticalHit(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (auto GEContext = static_cast<const FAuraGameEffectContext*>(EffectContextHandle.Get()))
		return GEContext->IsCriticalHit();
	return false;
}

bool UAuraAbilitySystemBPLibary::IsRadialDamage(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (auto AuraEffectContext = static_cast<const FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->IsRadialDamage();
	}
	return false;
}

float UAuraAbilitySystemBPLibary::GetRadialDamageInnerRadius(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (auto AuraEffectContext = static_cast<const FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetRadialDamageInnerRadius();
	}
	return 0.f;
}

float UAuraAbilitySystemBPLibary::GetRadialDamageOuterRadius(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (auto AuraEffectContext = static_cast<const FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetRadialDamageOuterRadius();
	}
	return 0.f;
}

FVector UAuraAbilitySystemBPLibary::GetRadialDamageOrigin(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (auto AuraEffectContext = static_cast<const FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetRadialDamageOrigin();
	}
	return FVector::ZeroVector;
}

void UAuraAbilitySystemBPLibary::GetLivePlayersWithRadius(const UObject* WorldContext, TArray<AActor*>& OutOverlappingActors,
                                                          const TArray<AActor*>& IgnoreActors, const FVector& Center, float Radius)
{
	/*无非就是生成个检测圆，进行范围内检测,筛选后装载*/

	//碰撞查询参数
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActors(IgnoreActors);
	if (auto World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull))
	{
		TArray<FOverlapResult> Results;
		World->OverlapMultiByObjectType(Results, Center, FQuat::Identity,
		                                FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllDynamicObjects), /*所有动态物体*/
		                                FCollisionShape::MakeSphere(Radius), QueryParams);

		for (const auto& HitRes : Results)
		{
			if (!HitRes.GetActor()->Implements<UCombatInterface>())continue; //Actor是否实现了接口？
			if (ICombatInterface::Execute_IsDead(HitRes.GetActor()))continue; //挂了就挂了
			if (IgnoreActors.Contains(HitRes.GetActor()))continue;//不是忽略对象

			OutOverlappingActors.AddUnique(ICombatInterface::Execute_GetAvatar(HitRes.GetActor()));
		}
	}
}

bool UAuraAbilitySystemBPLibary::IsNotFriend(const AActor* FirstActor, const AActor* OtherActor, const FName Tag)
{
	if(IsValid(FirstActor)&&IsValid(OtherActor))
	{
		return !(FirstActor->ActorHasTag(Tag) && OtherActor->ActorHasTag(Tag));
	}
	return false;
}

int32 UAuraAbilitySystemBPLibary::GetXpRewardForClassAndLevel(const UObject* WorldContext, ECharacterClass ClassType, const int16 Level)
{
	auto ClassDefaultInfo = GetCharacterClassInfo(WorldContext);
	if (!ClassDefaultInfo)return 0;

	const auto& DefaultInfo = ClassDefaultInfo->GetClassDefaultInfo(ClassType);

	return (int32)DefaultInfo.XPReward.GetValueAtLevel(Level);
}

TMap<FGameplayTag, FGameplayEffectContextHandle> UAuraAbilitySystemBPLibary::ApplyDamageEffect(const FDamageEffectParams& DamageEffectParams)
{
	TMap<FGameplayTag, FGameplayEffectContextHandle> DamageTypeToGeContextHandle;

	if (!IsValid(DamageEffectParams.TargetAbilitySystemComponent)|| !IsValid(DamageEffectParams.SourceAbilitySystemComponent))return DamageTypeToGeContextHandle;

	const auto SourceAvatarActor = DamageEffectParams.SourceAbilitySystemComponent->GetAvatarActor();
	const auto& Tags = FAuraGmaeplayTags::GetInstance();

	for(const auto& it:DamageEffectParams.DebuffMapGEParams)
	{
		auto GEContext = DamageEffectParams.SourceAbilitySystemComponent->MakeEffectContext();
		GEContext.AddSourceObject(SourceAvatarActor);
		SetDeathImpulse(GEContext, DamageEffectParams.DeathImpulseVectro);
		SetKnockbackForce(GEContext, DamageEffectParams.KnockbackForce);
		SetActiveHitReact(GEContext, DamageEffectParams.bApplyHitReact);

		//径向伤害
		SetIsRadialDamage(GEContext, DamageEffectParams.bIsRadialDamage);
		SetRadialDamageInnerRadius(GEContext, DamageEffectParams.RadialDamageInnerRadius);
		SetRadialDamageOuterRadius(GEContext, DamageEffectParams.RadialDamageOuterRadius);
		SetRadialDamageOrigin(GEContext, DamageEffectParams.RadialDamageOrigin);

		auto GESpecHandle = DamageEffectParams.SourceAbilitySystemComponent->MakeOutgoingSpec(DamageEffectParams.DamageGameplayEffectClass,
		                                                                                      DamageEffectParams.AbilityLevel,
		                                                                                      GEContext);
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(GESpecHandle, it.Value.DamageType, it.Value.BaseDamage);//伤害

		//Debuff
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(GESpecHandle, Tags.Debuff_Damage, it.Value.DebuffDamage);
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(GESpecHandle, Tags.Debuff_Duration, it.Value.DebuffDuration);
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(GESpecHandle, Tags.Debuff_Frequency, it.Value.DebuffFrequency);
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(GESpecHandle, Tags.Debuff_Chance, it.Value.DebuffChance);

		DamageEffectParams.TargetAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*GESpecHandle.Data);
	
		DamageTypeToGeContextHandle.Emplace(it.Value.DamageType, GEContext);
		
	}
	return DamageTypeToGeContextHandle;
}

void UAuraAbilitySystemBPLibary::SetDamageParamsRadialDamageOrigin(FDamageEffectParams& DamageParams, const FVector& Origin, const float OutRadius, const float InnerRadius)
{
	DamageParams.bIsRadialDamage = true;
	DamageParams.RadialDamageOrigin = Origin;
	DamageParams.RadialDamageOuterRadius = OutRadius;
	DamageParams.RadialDamageInnerRadius = InnerRadius;
}

void UAuraAbilitySystemBPLibary::SetDamageParamsKnockback(FDamageEffectParams& DamageParams, const FVector& Vector, const float Strength, const float Chance)
{
	
	if(Strength<0.01f)
	{
		DamageParams.KnockbackChance = 0.f;
		return;
	}else
	{
		auto Rotation = Vector.Rotation();
		Rotation.Pitch = 45.f;
		const FVector ToTarget = Rotation.Vector();

		DamageParams.DeathImpulseVectro = ToTarget * Strength;
		DamageParams.KnockbackForce = ToTarget * Strength;
		DamageParams.KnockbackChance = Chance;
	}
}

void UAuraAbilitySystemBPLibary::SetDamageParamsTargetASC(FDamageEffectParams& DamageParams,UAbilitySystemComponent* TargetASC)
{
	DamageParams.TargetAbilitySystemComponent = TargetASC;
}

TArray<FRotator> UAuraAbilitySystemBPLibary::EvenlySpeacedRotators(const FVector& Forward, const FVector& Axis, const float Spread, const int32 Count)
{
	const FVector Left = Forward.RotateAngleAxis(-Spread / 2.f, Axis);
	const FVector Right = Forward.RotateAngleAxis(Spread / 2.f, Axis);

	TArray<FRotator> Rotatores;
	if(Count>1)
	{
		const float DeltaSpread = Spread / (Count - 1);
		for (int i{ 0 }; i < Count; ++i)
		{
			const FVector Direction = Left.RotateAngleAxis(i * DeltaSpread, FVector::UpVector);
			Rotatores.Add(Direction.Rotation());
		}
	}else
	{
		Rotatores.Add(Forward.Rotation());
	}

	return Rotatores;

}

TArray<FVector> UAuraAbilitySystemBPLibary::EvenlySpeacedFVector(const FVector& Forward, const FVector& Axis, const float Spread, const int32 Count)
{
	const FVector Left = Forward.RotateAngleAxis(-Spread / 2.f, Axis);
	const FVector Right = Forward.RotateAngleAxis(Spread / 2.f, Axis);

	TArray<FVector> Vectors;
	if (Count > 1)
	{
		const float DeltaSpread = Spread / (Count - 1);
		for (int i{ 0 }; i < Count; ++i)
		{
			const FVector Direction = Left.RotateAngleAxis(i * DeltaSpread, FVector::UpVector);
			Vectors.Add(Direction);
		}
	}
	else
	{
		Vectors.Add(Forward);
	}

	return Vectors;
}

void UAuraAbilitySystemBPLibary::GetClosesTarget(const FVector& Origin, int32 MaxTarget, const TArray<AActor*>& Actors, TArray<AActor*>& OutClosesTargets)
{

	//如果搜索容器都不足目标数量，那么直接赋值
	if(Actors.Num()<=MaxTarget)
	{
		OutClosesTargets = Actors;
		return;
	}

	TMap<float, AActor*> DistanceToActor;
	for(auto Actor:Actors)
	{
		float Distance = (Actor->GetActorLocation() - Origin).Length();
		DistanceToActor.Add(Distance,Actor);
	}

	//从小到大排序
	DistanceToActor.KeySort([](float a, float b)
	{
		return a < b;
	});

	int count = 0;
	for(auto& Pair:DistanceToActor)
	{
		if (count >= MaxTarget)break;
		OutClosesTargets.Add(Pair.Value);
		count++;
	}
}

void UAuraAbilitySystemBPLibary::SetIsBlockedHit(FGameplayEffectContextHandle& EffectContextHandle, const bool Value)
{
	if (auto GEContext = static_cast<FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		GEContext->SetBlockedHit(Value);
	}
}

void UAuraAbilitySystemBPLibary::SetIsCriticalHit(FGameplayEffectContextHandle& EffectContextHandle, const bool Value)
{
	if (auto GEContext = static_cast<FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		GEContext->SetCriticalHit(Value);
	}
}

void UAuraAbilitySystemBPLibary::SetIsRadialDamage(FGameplayEffectContextHandle& EffectContextHandle, bool bInIsRadialDamage)
{
	if (auto AuraEffectContext = static_cast<FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->SetIsRadialDamage(bInIsRadialDamage);
	}
	
}

void UAuraAbilitySystemBPLibary::SetRadialDamageInnerRadius(FGameplayEffectContextHandle& EffectContextHandle, float InInnerRadius)
{
	if (auto AuraEffectContext = static_cast<FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->SetRadialDamageInnerRadius(InInnerRadius);
	}

}

void UAuraAbilitySystemBPLibary::SetRadialDamageOuterRadius(FGameplayEffectContextHandle& EffectContextHandle, float InOuterRadius)
{
	if (auto AuraEffectContext = static_cast<FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->SetRadialDamageOuterRadius(InOuterRadius);
	}

}

void UAuraAbilitySystemBPLibary::SetRadialDamageOrigin(FGameplayEffectContextHandle& EffectContextHandle, const FVector& InOrigin)
{
	if (auto AuraEffectContext = static_cast<FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->SetRadialDamageOrigin(InOrigin);
	}
}

void UAuraAbilitySystemBPLibary::SetIsSuccessfulDebuff(FGameplayEffectContextHandle& EffectContextHandle,
                                                       bool bInSuccessfulDebuff)
{
	if (FAuraGameEffectContext* AuraEffectContext = static_cast<FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetIsSuccessfulDebuff(bInSuccessfulDebuff);
	}
}
void UAuraAbilitySystemBPLibary::SetDebuffDamage(FGameplayEffectContextHandle& EffectContextHandle, float InDamage)
{
	if (FAuraGameEffectContext* AuraEffectContext = static_cast<FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDebuffDamage(InDamage);
	}
}
void UAuraAbilitySystemBPLibary::SetDebuffDuration(FGameplayEffectContextHandle& EffectContextHandle, float InDuration)
{
	if (FAuraGameEffectContext* AuraEffectContext = static_cast<FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDebuffDuration(InDuration);
	}
}
void UAuraAbilitySystemBPLibary::SetDebuffFrequency(FGameplayEffectContextHandle& EffectContextHandle, float InFrequency)
{
	if (FAuraGameEffectContext* AuraEffectContext = static_cast<FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDebuffFrequency(InFrequency);
	}
}
void UAuraAbilitySystemBPLibary::SetDamageType(FGameplayEffectContextHandle& EffectContextHandle,
	const FGameplayTag& InDamageType)
{
	if (FAuraGameEffectContext* AuraEffectContext = static_cast<FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		const TSharedPtr<FGameplayTag> DamageType = MakeShared<FGameplayTag>(InDamageType);
		AuraEffectContext->SetDamageType(DamageType);
	}
}

void UAuraAbilitySystemBPLibary::SetDeathImpulse(FGameplayEffectContextHandle& EffectContextHandle,
                                                 const FVector& InImpulse)
{
	if (auto AuraEffectContext = static_cast<FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDeathImpulse(InImpulse);
	}
}

FVector UAuraAbilitySystemBPLibary::GetDeathImpulse(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const auto* AuraEffectContext = static_cast<const FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetDeathImpulse();
	}
	return FVector::ZeroVector;
}

bool UAuraAbilitySystemBPLibary::IsActiveHitReact(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const auto* AuraEffectContext = static_cast<const FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetHitReact();
	}
	return true;
}

void UAuraAbilitySystemBPLibary::SetActiveHitReact(FGameplayEffectContextHandle& EffectContextHandle,const bool bIsActive)
{
	if ( auto* AuraEffectContext = static_cast<FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetHitReact(bIsActive);
	}
}

bool UAuraAbilitySystemBPLibary::IsSuccessfulDebuff(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameEffectContext* AuraEffectContext = static_cast<const FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->IsSuccessfulDebuff();
	}
	return false;
}
float UAuraAbilitySystemBPLibary::GetDebuffDamage(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameEffectContext* AuraEffectContext = static_cast<const FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetDebuffDamage();
	}
	return 0.f;
}
float UAuraAbilitySystemBPLibary::GetDebuffDuration(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameEffectContext* AuraEffectContext = static_cast<const FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetDebuffDuration();
	}
	return 0.f;
}
float UAuraAbilitySystemBPLibary::GetDebuffFrequency(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameEffectContext* AuraEffectContext = static_cast<const FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetDebuffFrequency();
	}
	return 0.f;
}
FGameplayTag UAuraAbilitySystemBPLibary::GetDamageType(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameEffectContext* AuraEffectContext = static_cast<const FAuraGameEffectContext*>(EffectContextHandle.Get()))
	{
		if (AuraEffectContext->GetDamageType().IsValid())
		{
			return *AuraEffectContext->GetDamageType();
		}
	}
	return FGameplayTag();
}

float UAuraAbilitySystemBPLibary::GetRadialDamageWithFalloff(const AActor* TargetActor, float BaseDamage,
                                                             float MinimumDamage, const FVector& Origin, float DamageInnerRadius, float DamageOuterRadius,
                                                             float DamageFalloff)
{
	if (!TargetActor) return 0.f;

	FRadialDamageParams RadialDamageParams;
	RadialDamageParams.BaseDamage = BaseDamage;
	RadialDamageParams.DamageFalloff = DamageFalloff;
	RadialDamageParams.InnerRadius = DamageInnerRadius;
	RadialDamageParams.OuterRadius = DamageOuterRadius;
	RadialDamageParams.MinimumDamage = MinimumDamage;
	float DamageScale = RadialDamageParams.GetDamageScale((Origin - TargetActor->GetActorLocation()).Length());
	return BaseDamage * DamageScale;
}
