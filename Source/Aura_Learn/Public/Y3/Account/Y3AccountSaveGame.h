#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Y3AccountSaveGame.generated.h"

UENUM(BlueprintType)
enum class EY3AccountProvider : uint8
{
	Local UMETA(DisplayName = "Local"),
	Steam UMETA(DisplayName = "Steam"),
	Epic UMETA(DisplayName = "Epic"),
	Console UMETA(DisplayName = "Console"),
	Y3Backend UMETA(DisplayName = "Y3Backend")
};

UCLASS(BlueprintType)
class AURA_LEARN_API UY3AccountSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Y3|Account")
	int32 SaveVersion = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Y3|Account")
	FString Y3AccountId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Y3|Account")
	EY3AccountProvider ProviderType = EY3AccountProvider::Local;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Y3|Account")
	FString ProviderUserId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Y3|Account")
	int64 CreatedAtUtc = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Y3|Account")
	int64 LastLoginAtUtc = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Y3|Account")
	int64 LastSavedAtUtc = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Y3|Progress")
	int32 AccountLevel = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Y3|Progress")
	int32 AccountXP = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Y3|Progress")
	int32 Gold = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Y3|Sync")
	bool bHasPendingCloudSync = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Y3|Sync")
	int64 LastCloudSyncAtUtc = 0;
};

UCLASS(BlueprintType)
class AURA_LEARN_API UY3AccountIndexSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Y3|Account")
	int32 SaveVersion = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Y3|Account")
	FString LastAccountId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Y3|Account")
	TArray<FString> KnownLocalAccountIds;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Y3|Account")
	TMap<FString, FString> LocalAccountDisplayNames;
};
