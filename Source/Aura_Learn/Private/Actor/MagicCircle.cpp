#include "Actor/MagicCircle.h"

#include "Components/DecalComponent.h"

AMagicCircle::AMagicCircle()
{
	PrimaryActorTick.bCanEverTick = false;

	MagicCircleDecal = CreateDefaultSubobject<UDecalComponent>("MagicCircleDecal");
}

UDecalComponent& AMagicCircle::GetMagicCircleDecal()
{
	return  *MagicCircleDecal;
}

void AMagicCircle::BeginPlay()
{
	Super::BeginPlay();
	
}

