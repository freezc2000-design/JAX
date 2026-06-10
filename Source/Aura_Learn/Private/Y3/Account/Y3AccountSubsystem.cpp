#include "Y3/Account/Y3AccountSubsystem.h"

#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

void UY3AccountSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadLastAccount();
}

void UY3AccountSubsystem::Deinitialize()
{
	SaveCurrentAccount();
	Super::Deinitialize();
}

UY3AccountSaveGame* UY3AccountSubsystem::LoadLastAccount()
{
	UY3AccountIndexSaveGame* Index = LoadOrCreateIndex();
	if (!Index)
	{
		return nullptr;
	}

	if (!Index->LastAccountId.IsEmpty())
	{
		if (UY3AccountSaveGame* Loaded = LoginLocalAccount(Index->LastAccountId))
		{
			return Loaded;
		}
	}

	if (Index->KnownLocalAccountIds.Num() > 0)
	{
		if (UY3AccountSaveGame* Loaded = LoginLocalAccount(Index->KnownLocalAccountIds[0]))
		{
			return Loaded;
		}
	}

	return CreateNewLocalAccount(TEXT(""));
}

UY3AccountSaveGame* UY3AccountSubsystem::CreateNewLocalAccount(const FString& RequestedAccountId)
{
	LoadOrCreateIndex();

	const FString AccountId = RequestedAccountId.IsEmpty() ? MakeLocalAccountId() : RequestedAccountId;
	const FString SlotName = GetAccountSlotName(AccountId);
	const int64 Now = UtcNowTicks();

	UY3AccountSaveGame* NewAccount = Cast<UY3AccountSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UY3AccountSaveGame::StaticClass()));
	if (!NewAccount)
	{
		return nullptr;
	}

	NewAccount->Y3AccountId = AccountId;
	NewAccount->ProviderType = EY3AccountProvider::Local;
	NewAccount->ProviderUserId = AccountId;
	NewAccount->CreatedAtUtc = Now;
	NewAccount->LastLoginAtUtc = Now;
	NewAccount->LastSavedAtUtc = Now;
	NewAccount->AccountLevel = 1;
	NewAccount->AccountXP = 0;
	NewAccount->Gold = 0;

	CurrentAccount = NewAccount;
	RememberAccountId(AccountId);

	UGameplayStatics::SaveGameToSlot(CurrentAccount, SlotName, SaveUserIndex);
	SaveIndex();

	return CurrentAccount;
}

UY3AccountSaveGame* UY3AccountSubsystem::LoginLocalAccount(const FString& AccountId)
{
	if (AccountId.IsEmpty())
	{
		return nullptr;
	}

	LoadOrCreateIndex();

	const FString SlotName = GetAccountSlotName(AccountId);
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex))
	{
		return nullptr;
	}

	UY3AccountSaveGame* LoadedAccount = Cast<UY3AccountSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SlotName, SaveUserIndex));
	if (!LoadedAccount)
	{
		return nullptr;
	}

	LoadedAccount->LastLoginAtUtc = UtcNowTicks();
	CurrentAccount = LoadedAccount;
	RememberAccountId(AccountId);
	SaveCurrentAccount();
	SaveIndex();

	return CurrentAccount;
}

bool UY3AccountSubsystem::SaveCurrentAccount()
{
	if (!CurrentAccount)
	{
		return false;
	}

	CurrentAccount->LastSavedAtUtc = UtcNowTicks();
	CurrentAccount->bHasPendingCloudSync = true;

	return UGameplayStatics::SaveGameToSlot(
		CurrentAccount,
		GetAccountSlotName(CurrentAccount->Y3AccountId),
		SaveUserIndex);
}

void UY3AccountSubsystem::AddAccountXP(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	if (!CurrentAccount)
	{
		LoadLastAccount();
	}

	if (!CurrentAccount)
	{
		return;
	}

	CurrentAccount->AccountXP += Amount;

	while (CurrentAccount->AccountXP >= GetXPRequiredForLevel(CurrentAccount->AccountLevel))
	{
		CurrentAccount->AccountXP -= GetXPRequiredForLevel(CurrentAccount->AccountLevel);
		CurrentAccount->AccountLevel++;
	}

	SaveCurrentAccount();
}

void UY3AccountSubsystem::AddGold(int32 Amount)
{
	if (Amount == 0)
	{
		return;
	}

	if (!CurrentAccount)
	{
		LoadLastAccount();
	}

	if (!CurrentAccount)
	{
		return;
	}

	CurrentAccount->Gold = FMath::Max(0, CurrentAccount->Gold + Amount);
	SaveCurrentAccount();
}

void UY3AccountSubsystem::AddRunReward(int32 AccountXPReward, int32 GoldReward)
{
	if (AccountXPReward > 0)
	{
		AddAccountXP(AccountXPReward);
	}
	if (GoldReward != 0)
	{
		AddGold(GoldReward);
	}
}

int32 UY3AccountSubsystem::GetXPRequiredForLevel(int32 Level) const
{
	const int32 ClampedLevel = FMath::Max(1, Level);
	return 100 + (ClampedLevel - 1) * 50;
}

FString UY3AccountSubsystem::GetAccountDebugString() const
{
	if (!CurrentAccount)
	{
		return TEXT("Y3Account: <not loaded>");
	}

	return FString::Printf(
		TEXT("Y3Account %s Provider=%s Level=%d XP=%d/%d Gold=%d"),
		*GetCurrentAccountDisplayLabel(),
		*UEnum::GetValueAsString(CurrentAccount->ProviderType),
		CurrentAccount->AccountLevel,
		CurrentAccount->AccountXP,
		GetXPRequiredForLevel(CurrentAccount->AccountLevel),
		CurrentAccount->Gold);
}

FString UY3AccountSubsystem::GetCurrentAccountDisplayLabel() const
{
	if (!CurrentAccount)
	{
		return TEXT("<not loaded>");
	}

	return FString::Printf(
		TEXT("%s：%s"),
		*GetDisplayNameForAccountId(CurrentAccount->Y3AccountId),
		*CurrentAccount->Y3AccountId);
}

FString UY3AccountSubsystem::GetDisplayNameForAccountId(const FString& AccountId) const
{
	if (!AccountIndex)
	{
		return AccountId.IsEmpty() ? TEXT("账号") : AccountId;
	}

	if (const FString* Found = AccountIndex->LocalAccountDisplayNames.Find(AccountId))
	{
		if (!Found->IsEmpty())
		{
			return *Found;
		}
	}

	const int32 Index = AccountIndex->KnownLocalAccountIds.IndexOfByKey(AccountId);
	return Index == INDEX_NONE ? TEXT("账号") : FString::Printf(TEXT("账号%d"), Index + 1);
}

void UY3AccountSubsystem::PrintCurrentAccount() const
{
	const FString Message = GetAccountDebugString();
	UE_LOG(LogTemp, Log, TEXT("[Y3Account] %s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Cyan, Message);
	}
}

TArray<FString> UY3AccountSubsystem::GetKnownLocalAccountIds() const
{
	return AccountIndex ? AccountIndex->KnownLocalAccountIds : TArray<FString>();
}

UY3AccountIndexSaveGame* UY3AccountSubsystem::LoadOrCreateIndex()
{
	if (AccountIndex)
	{
		return AccountIndex;
	}

	const FString SlotName = GetIndexSlotName();
	if (UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex))
	{
		AccountIndex = Cast<UY3AccountIndexSaveGame>(
			UGameplayStatics::LoadGameFromSlot(SlotName, SaveUserIndex));
		EnsureDisplayNames();
	}

	if (!AccountIndex)
	{
		AccountIndex = Cast<UY3AccountIndexSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UY3AccountIndexSaveGame::StaticClass()));
		SaveIndex();
	}

	return AccountIndex;
}

bool UY3AccountSubsystem::SaveIndex() const
{
	if (!AccountIndex)
	{
		return false;
	}

	return UGameplayStatics::SaveGameToSlot(AccountIndex, GetIndexSlotName(), SaveUserIndex);
}

FString UY3AccountSubsystem::GetIndexSlotName()
{
	return TEXT("Y3Account_Index");
}

FString UY3AccountSubsystem::GetAccountSlotName(const FString& AccountId)
{
	return FString::Printf(TEXT("Y3Account_%s"), *AccountId);
}

FString UY3AccountSubsystem::MakeLocalAccountId()
{
	return FString::Printf(TEXT("Local_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
}

int64 UY3AccountSubsystem::UtcNowTicks()
{
	return FDateTime::UtcNow().GetTicks();
}

void UY3AccountSubsystem::RememberAccountId(const FString& AccountId)
{
	if (!AccountIndex || AccountId.IsEmpty())
	{
		return;
	}

	AccountIndex->LastAccountId = AccountId;
	AccountIndex->KnownLocalAccountIds.AddUnique(AccountId);
	if (!AccountIndex->LocalAccountDisplayNames.Contains(AccountId))
	{
		AccountIndex->LocalAccountDisplayNames.Add(AccountId, MakeNextDisplayName());
	}
}

FString UY3AccountSubsystem::MakeNextDisplayName() const
{
	if (!AccountIndex)
	{
		return TEXT("账号1");
	}

	int32 MaxNumber = 0;
	for (const TPair<FString, FString>& Pair : AccountIndex->LocalAccountDisplayNames)
	{
		const FString& Name = Pair.Value;
		if (Name.StartsWith(TEXT("账号")))
		{
			MaxNumber = FMath::Max(MaxNumber, FCString::Atoi(*Name.RightChop(2)));
		}
	}

	return FString::Printf(TEXT("账号%d"), MaxNumber + 1);
}

void UY3AccountSubsystem::EnsureDisplayNames()
{
	if (!AccountIndex)
	{
		return;
	}

	bool bChanged = false;
	for (int32 i = 0; i < AccountIndex->KnownLocalAccountIds.Num(); ++i)
	{
		const FString& AccountId = AccountIndex->KnownLocalAccountIds[i];
		if (AccountId.IsEmpty())
		{
			continue;
		}

		if (!AccountIndex->LocalAccountDisplayNames.Contains(AccountId) ||
			AccountIndex->LocalAccountDisplayNames[AccountId].IsEmpty())
		{
			AccountIndex->LocalAccountDisplayNames.Add(AccountId, FString::Printf(TEXT("账号%d"), i + 1));
			bChanged = true;
		}
	}

	if (bChanged)
	{
		SaveIndex();
	}
}
