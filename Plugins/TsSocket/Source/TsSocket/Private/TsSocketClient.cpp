// Fill out your copyright notice in the Description page of Project Settings.


#include "TsSocketClient.h"

// Sets default values
ATsSocketClient::ATsSocketClient()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATsSocketClient::BeginPlay()
{
	Super::BeginPlay();
	
}

void ATsSocketClient::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CloseSocket();
}

// Called every frame
void ATsSocketClient::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATsSocketClient::ConnectToSocketAsClient(const FString& InIP, const int32 InPort)
{
	bool bIsValid;

	// set up Remote Address Values
	RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	RemoteAddress->SetIp(*InIP, bIsValid);
	RemoteAddress->SetPort(InPort);

	if (!bIsValid)
	{
		UE_LOG(LogTemp, Error, TEXT("TCP address is invalid <%s:%d>"), *InIP, InPort);
		return;
	}

	// Create a client socket
	ClientSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, ClientSocketName, false);

	//client socket buffer size settings
	ClientSocket->SetSendBufferSize(BufferMaxSize, BufferMaxSize);
	ClientSocket->SetReceiveBufferSize(BufferMaxSize, BufferMaxSize);

	// connect to server with InIP and InPort
	bIsConnected = ClientSocket->Connect(*RemoteAddress);

	if (bIsConnected)
		OnConnected.Broadcast();

	bShouldReceiveData = true;

	//Start an Aynsc thread to Listen for data on our end 
	ClientConnectionFinishedFuture = ATsSocketClient::RunLambdaOnBackGroundThread([&]()
	{
			uint32 BufferSize = 0;

			TArray<uint8> ReceiveBuffer;
			
			while (bShouldReceiveData)
			{
				// we got some data from server...
				if (ClientSocket->HasPendingData(BufferSize))
				{
					ReceiveBuffer.SetNumUninitialized(BufferSize);
					
					int32 Read = 0;
					ClientSocket->Recv(ReceiveBuffer.GetData(), ReceiveBuffer.Num(), Read);


					if (bReceiveDataOnGameThread)
					{
						//Copy buffer so it's still valid on game thread
						TArray<uint8> ReceiveBufferGT;
						ReceiveBufferGT.Append(ReceiveBuffer);

						//Pass the reference to be used on game thread
						AsyncTask(ENamedThreads::GameThread, [&, ReceiveBufferGT]()
						{
							OnReceivedBytes.Broadcast(ReceiveBufferGT);
						});
					}
					else
					{
						OnReceivedBytes.Broadcast(ReceiveBuffer);
					}
				}
				ClientSocket->Wait(ESocketWaitConditions::WaitForReadOrWrite, FTimespan(1));
			}// end while
	});
}

void ATsSocketClient::CloseSocket()
{
	if (ClientSocket)
	{
		bShouldReceiveData = false;
		ClientConnectionFinishedFuture.Get();

		ClientSocket->Close();

		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
		ClientSocket = nullptr;
	}
}

bool ATsSocketClient::Send(const TArray<uint8>& Bytes)
{
	if (ClientSocket && ClientSocket->GetConnectionState() == SCS_Connected)
	{
		int32 BytesSent = 0;

		return ClientSocket->Send(Bytes.GetData(), Bytes.Num(), BytesSent);
	}

	return false;
}

bool ATsSocketClient::SendUTFString(const FString& message)
{
	if (!ClientSocket)
		return false;
	
	//int32 BytesSent;

	FTimespan waitTime = FTimespan(10);

	const TCHAR* StrPtr = *message;
	FTCHARToUTF8 UTF8String(StrPtr);

	int32 CTXSize = UTF8String.Length();

	TArray<uint8> Data;
	Data.SetNum(CTXSize);

	memcpy(Data.GetData(), UTF8String.Get(), CTXSize);


	return Send(Data);
}

