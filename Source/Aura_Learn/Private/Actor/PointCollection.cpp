#include "Actor/PointCollection.h"

#include "AbilitySystem/AuraAbilitySystemBPLibary.h"
#include "Kismet/KismetMathLibrary.h"

#define CREATE_PT(NUM)\
	Pt_##NUM = CreateDefaultSubobject<USceneComponent>(TEXT("Pt_") TEXT(#NUM));\
	ImmutablePts.Emplace(Pt_##NUM);\
	Pt_##NUM->SetupAttachment(GetRootComponent())

APointCollection::APointCollection()
{
	PrimaryActorTick.bCanEverTick = false;

	CREATE_PT(0);
	SetRootComponent(Pt_0);
	CREATE_PT(1);
	CREATE_PT(2);
	CREATE_PT(3);
	CREATE_PT(4);
	CREATE_PT(5);
	CREATE_PT(6);
	CREATE_PT(7);
	CREATE_PT(8);
	CREATE_PT(9);
	CREATE_PT(10);
	CREATE_PT(11);
}

TArray<USceneComponent*> APointCollection::GetGroundPoints(const FVector& GroundLocation, int32 NumPoints,float YawOverride)
{
	TArray<USceneComponent*> Reslut;

	for(auto Pt:ImmutablePts)
	{
		if (Reslut.Num() >= NumPoints)return Reslut;

		//根据参数 调整非原点
		if(Pt!=Pt_0)
		{
			FVector ToPoint0 = Pt->GetComponentLocation() - Pt_0->GetComponentLocation();
			ToPoint0 = ToPoint0.RotateAngleAxis(YawOverride, FVector::UpVector);//也许应该支持 相对斜面法线的旋转
			Pt->SetWorldLocation(Pt_0->GetComponentLocation() + ToPoint0);
		}

		/*再进行计算，获得地面投影点*/
		const FVector RaisedLocation = FVector(Pt->GetComponentLocation().X, Pt->GetComponentLocation().Y, Pt->GetComponentLocation().Z + 500.f);
		const FVector LoweredLocation = FVector(Pt->GetComponentLocation().X, Pt->GetComponentLocation().Y, Pt->GetComponentLocation().Z - 500.f);
		FHitResult HitRes;
		TArray<AActor*>ActorToignal;
		//忽略战斗对象
		UAuraAbilitySystemBPLibary::GetLivePlayersWithRadius(this, ActorToignal, TArray<AActor*>(), GetActorLocation(), 1500.f);
		FCollisionQueryParams QueryParam;
		QueryParam.AddIgnoredActors(ActorToignal);
		GetWorld()->LineTraceSingleByProfile(HitRes, RaisedLocation, LoweredLocation, FName("BlockAll"), QueryParam);//Profile 是碰撞预设，描述了对每个通道的响应方式
		//设置组件位置到地面投影
		FVector AdjustLocation = LoweredLocation;
		AdjustLocation.Z = HitRes.ImpactPoint.Z;
		Pt->SetWorldLocation(AdjustLocation);
		Pt->SetWorldRotation(UKismetMathLibrary::MakeRotFromZ(HitRes.ImpactNormal));

		Reslut.Emplace(Pt);
	}

	return Reslut;
}

void APointCollection::BeginPlay()
{
	Super::BeginPlay();

}

