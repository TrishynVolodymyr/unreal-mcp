#include "MCPServerRunnable.h"
#include "UnrealMCPBridge.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "JsonObjectConverter.h"
#include "HAL/PlatformTime.h"

namespace
{
constexpr int32 MCPRecvChunkSize = 8192;
constexpr int32 MCPMaxMessageBytes = 256 * 1024;

enum class EReadJsonResult
{
	Success,
	Disconnected,
	TooLarge,
	ParseError
};

FString MakeErrorResponse(const FString& ErrorMessage)
{
	TSharedPtr<FJsonObject> ErrorJson = MakeShared<FJsonObject>();
	ErrorJson->SetStringField(TEXT("status"), TEXT("error"));
	ErrorJson->SetStringField(TEXT("error"), ErrorMessage);

	FString ResultString;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&ResultString);
	FJsonSerializer::Serialize(ErrorJson.ToSharedRef(), Writer.Get());
	return ResultString;
}

bool TryParseJsonObject(const FString& JsonText, TSharedPtr<FJsonObject>& OutObject)
{
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	return FJsonSerializer::Deserialize(Reader, OutObject) && OutObject.IsValid();
}

FString BytesToUtf8String(const TArray<uint8>& Bytes)
{
	if (Bytes.Num() == 0)
	{
		return FString();
	}

	FUTF8ToTCHAR Converter(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
	return FString(Converter.Length(), Converter.Get());
}

EReadJsonResult ReadCompleteJsonMessage(TSharedPtr<FSocket> Socket, FString& OutMessage, FString& OutError)
{
	TArray<uint8> AccumulatedBytes;
	AccumulatedBytes.Reserve(MCPRecvChunkSize);

	uint8 Buffer[MCPRecvChunkSize];

	while (AccumulatedBytes.Num() < MCPMaxMessageBytes)
	{
		int32 BytesRead = 0;
		if (!Socket->Recv(Buffer, sizeof(Buffer), BytesRead))
		{
			const int32 LastError = static_cast<int32>(ISocketSubsystem::Get()->GetLastErrorCode());
			if (LastError == SE_EWOULDBLOCK || LastError == SE_EINTR)
			{
				FPlatformProcess::Sleep(0.01f);
				continue;
			}

			if (AccumulatedBytes.Num() == 0)
			{
				OutError = TEXT("Client disconnected before sending data");
				return EReadJsonResult::Disconnected;
			}

			break;
		}

		if (BytesRead == 0)
		{
			if (AccumulatedBytes.Num() == 0)
			{
				OutError = TEXT("Client disconnected before sending data");
				return EReadJsonResult::Disconnected;
			}
			break;
		}

		AccumulatedBytes.Append(Buffer, BytesRead);

		FString ReceivedText = BytesToUtf8String(AccumulatedBytes);
		ReceivedText.TrimStartAndEndInline();

		TSharedPtr<FJsonObject> JsonObject;
		if (TryParseJsonObject(ReceivedText, JsonObject))
		{
			OutMessage = ReceivedText;
			return EReadJsonResult::Success;
		}
	}

	if (AccumulatedBytes.Num() >= MCPMaxMessageBytes)
	{
		OutError = FString::Printf(TEXT("Message exceeds maximum size of %d bytes"), MCPMaxMessageBytes);
		return EReadJsonResult::TooLarge;
	}

	FString ReceivedText = BytesToUtf8String(AccumulatedBytes);
	ReceivedText.TrimStartAndEndInline();
	if (ReceivedText.IsEmpty())
	{
		OutError = TEXT("Received empty message");
	}
	else
	{
		OutError = TEXT("Invalid JSON payload");
	}
	return EReadJsonResult::ParseError;
}

bool SendJsonResponse(TSharedPtr<FSocket> Socket, const FString& ResponseJson)
{
	FTCHARToUTF8 UTF8Response(*ResponseJson);
	const int32 UTF8ByteLength = UTF8Response.Length();
	int32 BytesSent = 0;

	if (!Socket->Send(reinterpret_cast<const uint8*>(UTF8Response.Get()), UTF8ByteLength, BytesSent))
	{
		const int32 SendError = static_cast<int32>(ISocketSubsystem::Get()->GetLastErrorCode());
		UE_LOG(LogTemp, Error, TEXT("MCPServerRunnable: Failed to send response. Error: %d"), SendError);
		return false;
	}

	if (BytesSent != UTF8ByteLength)
	{
		UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Partial response sent (%d/%d bytes)"), BytesSent, UTF8ByteLength);
		return false;
	}

	UE_LOG(LogTemp, Verbose, TEXT("MCPServerRunnable: Response sent successfully (%d bytes)"), BytesSent);
	return true;
}

void ConfigureClientSocket(TSharedPtr<FSocket> ClientSocket)
{
	ClientSocket->SetNoDelay(true);

	int32 SocketBufferSize = 65536;
	int32 ActualSendBufferSize = 0;
	int32 ActualReceiveBufferSize = 0;
	ClientSocket->SetSendBufferSize(SocketBufferSize, ActualSendBufferSize);
	ClientSocket->SetReceiveBufferSize(SocketBufferSize, ActualReceiveBufferSize);
	ClientSocket->SetNonBlocking(false);
}

FString ProcessClientRequest(UUnrealMCPBridge* Bridge, const FString& ReceivedText)
{
	TSharedPtr<FJsonObject> JsonObject;
	if (!TryParseJsonObject(ReceivedText, JsonObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Failed to parse JSON command"));
		return MakeErrorResponse(TEXT("Invalid JSON payload"));
	}

	FString CommandType;
	if (!JsonObject->TryGetStringField(TEXT("type"), CommandType))
	{
		TArray<FString> FieldNames;
		for (const auto& Pair : JsonObject->Values)
		{
			FieldNames.Add(FString(Pair.Key.ToView()));
		}
		const FString FieldList = FString::Join(FieldNames, TEXT(", "));
		UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Missing 'type' field. Available fields: %s"), *FieldList);
		return MakeErrorResponse(TEXT("Missing type field"));
	}

	TSharedPtr<FJsonObject> Params = MakeShared<FJsonObject>();
	const TSharedPtr<FJsonObject>* ParamsPtr = nullptr;
	if (JsonObject->TryGetObjectField(TEXT("params"), ParamsPtr) && ParamsPtr && ParamsPtr->IsValid())
	{
		Params = *ParamsPtr;
	}

	UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Executing command: %s"), *CommandType);
	const double ExecuteStartTime = FPlatformTime::Seconds();
	const FString Response = Bridge->ExecuteCommand(CommandType, Params);
	UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Command '%s' completed in %.3f seconds"), *CommandType, FPlatformTime::Seconds() - ExecuteStartTime);
	return Response;
}
} // namespace

FMCPServerRunnable::FMCPServerRunnable(UUnrealMCPBridge* InBridge, TSharedPtr<FSocket> InListenerSocket)
	: Bridge(InBridge)
	, ListenerSocket(InListenerSocket)
	, bRunning(true)
{
	UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Created server runnable"));
}

FMCPServerRunnable::~FMCPServerRunnable()
{
	// Note: We don't delete the sockets here as they're owned by the bridge
}

bool FMCPServerRunnable::Init()
{
	return true;
}

uint32 FMCPServerRunnable::Run()
{
	UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Server thread starting..."));

	while (bRunning)
	{
		bool bPending = false;
		if (ListenerSocket->HasPendingConnection(bPending) && bPending)
		{
			ClientSocket = MakeShareable(ListenerSocket->Accept(TEXT("MCPClient")));
			if (ClientSocket.IsValid())
			{
				UE_LOG(LogTemp, Verbose, TEXT("MCPServerRunnable: Client connection accepted"));
				ConfigureClientSocket(ClientSocket);

				FString ReceivedText;
				FString ReadError;
				const EReadJsonResult ReadResult = ReadCompleteJsonMessage(ClientSocket, ReceivedText, ReadError);

				FString ResponseJson;
				switch (ReadResult)
				{
				case EReadJsonResult::Success:
					ResponseJson = ProcessClientRequest(Bridge, ReceivedText);
					break;
				case EReadJsonResult::Disconnected:
					UE_LOG(LogTemp, Verbose, TEXT("MCPServerRunnable: %s"), *ReadError);
					break;
				case EReadJsonResult::TooLarge:
				case EReadJsonResult::ParseError:
				default:
					UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: %s"), *ReadError);
					ResponseJson = MakeErrorResponse(ReadError);
					break;
				}

				if (!ResponseJson.IsEmpty())
				{
					SendJsonResponse(ClientSocket, ResponseJson);
				}

				ClientSocket.Reset();
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Failed to accept client connection"));
			}
		}

		FPlatformProcess::Sleep(0.1f);
	}

	UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Server thread stopping"));
	return 0;
}

void FMCPServerRunnable::Stop()
{
	bRunning = false;
}

void FMCPServerRunnable::Exit()
{
}

void FMCPServerRunnable::HandleClientConnection(TSharedPtr<FSocket> InClientSocket)
{
	if (!InClientSocket.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("MCPServerRunnable: Invalid client socket passed to HandleClientConnection"));
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Starting to handle client connection"));

	InClientSocket->SetNonBlocking(false);

	const int32 MaxBufferSize = 4096;
	uint8 Buffer[MaxBufferSize];
	FString MessageBuffer;

	while (bRunning && InClientSocket.IsValid())
	{
		int32 BytesRead = 0;
		const bool bReadSuccess = InClientSocket->Recv(Buffer, MaxBufferSize, BytesRead, ESocketReceiveFlags::None);

		if (BytesRead > 0)
		{
			Buffer[BytesRead] = 0;
			MessageBuffer.Append(UTF8_TO_TCHAR(Buffer));

			if (MessageBuffer.Contains(TEXT("\n")))
			{
				TArray<FString> Messages;
				MessageBuffer.ParseIntoArray(Messages, TEXT("\n"), true);

				for (int32 i = 0; i < Messages.Num() - 1; ++i)
				{
					ProcessMessage(InClientSocket, Messages[i]);
				}

				MessageBuffer = Messages.Last();
			}
		}
		else if (!bReadSuccess)
		{
			UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Connection closed or error occurred - Last error: %d"),
				static_cast<int32>(ISocketSubsystem::Get()->GetLastErrorCode()));
			break;
		}

		FPlatformProcess::Sleep(0.01f);
	}
}

void FMCPServerRunnable::ProcessMessage(TSharedPtr<FSocket> Client, const FString& Message)
{
	TSharedPtr<FJsonObject> JsonMessage;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

	if (!FJsonSerializer::Deserialize(Reader, JsonMessage) || !JsonMessage.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Failed to parse message as JSON"));
		return;
	}

	FString CommandType;
	TSharedPtr<FJsonObject> Params = MakeShareable(new FJsonObject());

	if (!JsonMessage->TryGetStringField(TEXT("command"), CommandType))
	{
		UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Message missing 'command' field"));
		return;
	}

	if (JsonMessage->HasField(TEXT("params")))
	{
		const TSharedPtr<FJsonValue> ParamsValue = JsonMessage->TryGetField(TEXT("params"));
		if (ParamsValue.IsValid() && ParamsValue->Type == EJson::Object)
		{
			Params = ParamsValue->AsObject();
		}
	}

	FString Response = Bridge->ExecuteCommand(CommandType, Params);
	Response += TEXT("\n");
	int32 BytesSent = 0;

	if (!Client->Send(reinterpret_cast<uint8*>(TCHAR_TO_UTF8(*Response)), Response.Len(), BytesSent))
	{
		UE_LOG(LogTemp, Error, TEXT("MCPServerRunnable: Failed to send response"));
	}
}
