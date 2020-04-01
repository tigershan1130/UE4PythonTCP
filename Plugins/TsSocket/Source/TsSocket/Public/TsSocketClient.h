// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Networking/Public/Networking.h"
#include "TsSocketClient.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTCPEventSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTCPMessageSignature, const TArray<uint8>&, Bytes);

UCLASS()
class TSSOCKET_API ATsSocketClient : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATsSocketClient();


	static TFuture<void> RunLambdaOnBackGroundThread(TFunction< void()> InFunction)
	{
		return Async(EAsyncExecution::Thread, InFunction);
	}

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;



	// On message received on the receiving socket
	UPROPERTY(BlueprintAssignable, Category = "TCP Events")
		FTCPMessageSignature OnReceivedBytes;

	// Callback when we start listening on the TCP receive socket
	UPROPERTY(BlueprintAssignable, Category = "TCP Events")
		FTCPEventSignature OnConnected;

	// Default sending socket IP string in form e.g. 127.0.0.1.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP Connection Properties")
		FString ConnectionIP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP Connection Properties")
		int32 ConnectionPort;

	// name of the client socket
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP Connection Properties")
		FString ClientSocketName;

	// in bytes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP Connection Properties")
		int32 BufferMaxSize;

	// If true will auto-connect on begin play to IP/port specified as a client. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP Connection Properties")
		bool bShouldAutoConnectOnBeginPlay;

	// Whether we should process our data on the game thread or the TCP thread. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCP Connection Properties")
		bool bReceiveDataOnGameThread;

	UPROPERTY(BlueprintReadOnly, Category = "TCP Connection Properties")
		bool bIsConnected;

	UFUNCTION(BlueprintCallable, Category = "TsTCP Function")
		void ConnectToSocketAsClient(const FString& InIP = TEXT("127.0.0.1"), const int32 InPort = 12345);

	UFUNCTION(BlueprintCallable, Category = "TsTCP Function")
		void CloseSocket();

	UFUNCTION(BlueprintCallable, Category = "TsTCP Function")
		bool Send(const TArray<uint8>& Bytes);

	UFUNCTION(BlueprintCallable, Category = "TsTCP Function")
		bool SendUTFString(const FString& message);

protected:
	FSocket* ClientSocket;
	FThreadSafeBool bShouldReceiveData;
	TFuture<void>	ClientConnectionFinishedFuture;

	FString SocketDescription;
	TSharedPtr<FInternetAddr> RemoteAddress;
};
