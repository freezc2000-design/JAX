#include "Y3/Y3EnemyBase.h"

AY3EnemyBase::AY3EnemyBase()
{
    PrimaryActorTick.bCanEverTick = false;
    // 默认就让胶囊体显示作为占位（避免空场景）
}
