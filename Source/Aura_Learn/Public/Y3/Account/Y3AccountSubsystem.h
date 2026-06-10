#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Y3/Account/Y3AccountSaveGame.h"
#include "Y3AccountSubsystem.generated.h"

UCLASS()
class AURA_LEARN_API UY3AccountSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Y3|Account")
	UY3AccountSaveGame* LoadLastAccount();

	UFUNCTION(BlueprintCallable, Category = "Y3|Account")
	UY3AccountSaveGame* CreateNewLocalAccount(const FString& RequestedAccountId);

	UFUNCTION(BlueprintCallable, Category = "Y3|Account")
	UY3AccountSaveGame* LoginLocalAccount(const FString& AccountId);

	UFUNCTION(BlueprintCallable, Category = "Y3|Account")
	bool SaveCurrentAccount();

	UFUNCTION(BlueprintCallable, Category = "Y3|Account")
	void AddAccountXP(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Y3|Account")
	void AddGold(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Y3|Account")
	void AddRunReward(int32 AccountXPReward, int32 GoldReward);

	UFUNCTION(BlueprintPure, Category = "Y3|Account")
	int32 GetXPRequiredForLevel(int32 Level) const;

	UFUNCTION(BlueprintPure, Category = "Y3|Account")
	FString GetAccountDebugString() const;

	UFUNCTION(BlueprintPure, Category = "Y3|Account")
	FString GetCurrentAccountDisplayLabel() const;

	UFUNCTION(BlueprintPure, Category = "Y3|Account")
	FString GetDisplayNameForAccountId(const FString& AccountId) const;

	UFUNCTION(BlueprintCallable, Category = "Y3|Account")
	void PrintCurrentAccount() const;

	UFUNCTION(BlueprintPure, Category = "Y3|Account")
	UY3AccountSaveGame* GetCurrentAccount() const { return CurrentAccount; }

	UFUNCTION(BlueprintPure, Category = "Y3|Account")
	TArray<FString> GetKnownLocalAccountIds() const;

private:
	static constexpr int32 SaveUserIndex = 0;

	UPROPERTY(Transient)
	TObjectPtr<UY3AccountSaveGame> CurrentAccount;

	UPROPERTY(Transient)
	TObjectPtr<UY3AccountIndexSaveGame> AccountIndex;

	UY3AccountIndexSaveGame* LoadOrCreateIndex();
	bool SaveIndex() const;
	static FString GetIndexSlotName();
	static FString GetAccountSlotName(const FString& AccountId);
	static FString MakeLocalAccountId();
	static int64 UtcNowTicks();
	void RememberAccountId(const FString& AccountId);
	FString MakeNextDisplayName() const;
	void EnsureDisplayNames();
};
