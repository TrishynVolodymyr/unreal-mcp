#include "MCPServerRunnable.h"
#include "UnrealMCPBridge.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "JsonObjectConverter.h"
#include "Misc/ScopeLock.h"
#include "HAL/PlatformTime.h"

// Buffer size for receiving data - renamed to avoid UE 5.6 template conflicts
const int32 MCPBufferSize = 8192;

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
        // UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Waiting for client connection..."));
        
        bool bPending = false;
        if (ListenerSocket->HasPendingConnection(bPending) && bPending)
        {
            UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Client connection pending, accepting..."));
            
            ClientSocket = MakeShareable(ListenerSocket->Accept(TEXT("MCPClient")));
            if (ClientSocket.IsValid())
            {
                UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Client connection accepted"));
                
                // Log client connection details
                TSharedRef<FInternetAddr> ClientAddr = ISocketSubsystem::Get()->CreateInternetAddr();
                if (ClientSocket->GetPeerAddress(*ClientAddr))
                {
                    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Client connected from: %s"), *ClientAddr->ToString(true));
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Could not get client address"));
                }
                
                // Set socket options to improve connection stability
                bool bNoDelayResult = ClientSocket->SetNoDelay(true);
                UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: SetNoDelay result: %s"), bNoDelayResult ? TEXT("Success") : TEXT("Failed"));
                
                int32 SocketBufferSize = 65536;  // 64KB buffer
                int32 ActualSendBufferSize = 0;
                int32 ActualReceiveBufferSize = 0;
                
                bool bSendBufferResult = ClientSocket->SetSendBufferSize(SocketBufferSize, ActualSendBufferSize);
                bool bReceiveBufferResult = ClientSocket->SetReceiveBufferSize(SocketBufferSize, ActualReceiveBufferSize);
                
                UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Buffer setup - SendBuffer: %s (requested: %d, actual: %d), ReceiveBuffer: %s (requested: %d, actual: %d)"), 
                       bSendBufferResult ? TEXT("Success") : TEXT("Failed"), SocketBufferSize, ActualSendBufferSize,
                       bReceiveBufferResult ? TEXT("Success") : TEXT("Failed"), SocketBufferSize, ActualReceiveBufferSize);
                
                // Set socket to non-blocking mode for better control
                bool bNonBlockingResult = ClientSocket->SetNonBlocking(false);
                UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: SetNonBlocking(false) result: %s"), bNonBlockingResult ? TEXT("Success") : TEXT("Failed"));
                
                uint8 Buffer[8192];
                int32 ConnectionAttempts = 0;
                double ConnectionStartTime = FPlatformTime::Seconds();
                
                while (bRunning)
                {
                    ConnectionAttempts++;
                    int32 BytesRead = 0;
                    
                    // Log connection state before attempting to receive
                    ESocketConnectionState ConnectionState = ClientSocket->GetConnectionState();
                    FString ConnectionStateStr;
                    switch (ConnectionState)
                    {
                        case SCS_NotConnected: ConnectionStateStr = TEXT("NotConnected"); break;
                        case SCS_Connected: ConnectionStateStr = TEXT("Connected"); break;
                        case SCS_ConnectionError: ConnectionStateStr = TEXT("ConnectionError"); break;
                        default: ConnectionStateStr = TEXT("Unknown"); break;
                    }
                    
                    // Check for pending data before attempting to receive
                    uint32 PendingDataSize = 0;
                    bool bHasPendingData = ClientSocket->HasPendingData(PendingDataSize);
                    
                    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Attempt %d - ConnectionState: %s, HasPendingData: %s, PendingSize: %d"), 
                           ConnectionAttempts, *ConnectionStateStr, bHasPendingData ? TEXT("Yes") : TEXT("No"), PendingDataSize);
                    
                    bool bRecvResult = ClientSocket->Recv(Buffer, sizeof(Buffer), BytesRead);
                    
                    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Recv result - Success: %s, BytesRead: %d"), 
                           bRecvResult ? TEXT("Yes") : TEXT("No"), BytesRead);
                    
                    if (bRecvResult)
                    {
                        if (BytesRead == 0)
                        {
                            double ConnectionDuration = FPlatformTime::Seconds() - ConnectionStartTime;
                            UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Client disconnected (zero bytes) after %d attempts in %.3f seconds"), 
                                   ConnectionAttempts, ConnectionDuration);
                            
                            // Check if this is a graceful disconnect or an error
                            int32 LastError = (int32)ISocketSubsystem::Get()->GetLastErrorCode();
                            UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Last socket error code: %d"), LastError);
                            break;
                        }

                        // Convert received data to string
                        Buffer[BytesRead] = '\0';
                        FString ReceivedText = UTF8_TO_TCHAR(Buffer);
                        
                        // Log first 200 characters to avoid spam with large payloads
                        FString LogText = ReceivedText.Len() > 200 ? ReceivedText.Left(200) + TEXT("...") : ReceivedText;
                        UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Received %d bytes: %s"), BytesRead, *LogText);

                        // Parse JSON
                        TSharedPtr<FJsonObject> JsonObject;
                        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ReceivedText);
                        
                        double ParseStartTime = FPlatformTime::Seconds();
                        bool bParseSuccess = FJsonSerializer::Deserialize(Reader, JsonObject);
                        double ParseDuration = FPlatformTime::Seconds() - ParseStartTime;
                        
                        if (bParseSuccess && JsonObject.IsValid())
                        {
                            UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: JSON parsed successfully in %.3f seconds"), ParseDuration);
                            
                            // Get command type
                            FString CommandType;
                            if (JsonObject->TryGetStringField(TEXT("type"), CommandType))
                            {
                                UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Executing command: %s"), *CommandType);
                                
                                // Execute command with timing
                                double ExecuteStartTime = FPlatformTime::Seconds();
                                FString Response = Bridge->ExecuteCommand(CommandType, JsonObject->GetObjectField(TEXT("params")));
                                double ExecuteDuration = FPlatformTime::Seconds() - ExecuteStartTime;
                                
                                UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Command executed in %.3f seconds"), ExecuteDuration);
                                
                                // Log response for debugging
                                UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Response: %s"), *Response);
                                
                                // Convert to UTF-8 and get actual byte length
                                FTCHARToUTF8 UTF8Response(*Response);
                                int32 UTF8ByteLength = UTF8Response.Length();
                                
                                UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Sending response (%d characters, %d UTF-8 bytes)"), Response.Len(), UTF8ByteLength);
                                
                                // Send response with correct byte length
                                int32 BytesSent = 0;
                                double SendStartTime = FPlatformTime::Seconds();
                                bool bSendSuccess = ClientSocket->Send((const uint8*)UTF8Response.Get(), UTF8ByteLength, BytesSent);
                                double SendDuration = FPlatformTime::Seconds() - SendStartTime;
                                
                                if (!bSendSuccess)
                                {
                                    int32 SendError = (int32)ISocketSubsystem::Get()->GetLastErrorCode();
                                    UE_LOG(LogTemp, Error, TEXT("MCPServerRunnable: Failed to send response. Error: %d, Duration: %.3f seconds"), SendError, SendDuration);
                                }
                                else {
                                    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Response sent successfully - %d bytes in %.3f seconds"), BytesSent, SendDuration);
                                }
                            }
                            else
                            {
                                UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Missing 'type' field in command JSON"));
                                
                                // Log available fields for debugging
                                TArray<FString> FieldNames;
                                JsonObject->Values.GetKeys(FieldNames);
                                FString FieldList = FString::Join(FieldNames, TEXT(", "));
                                UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Available fields: %s"), *FieldList);
                            }
                        }
                        else
                        {
                            UE_LOG(LogTemp, Error, TEXT("MCPServerRunnable: Failed to parse JSON in %.3f seconds. Raw data: %s"), ParseDuration, *ReceivedText);
                            
                            // Try to identify the issue
                            if (ReceivedText.IsEmpty())
                            {
                                UE_LOG(LogTemp, Error, TEXT("MCPServerRunnable: Received empty string"));
                            }
                            else if (!ReceivedText.StartsWith(TEXT("{")))
                            {
                                UE_LOG(LogTemp, Error, TEXT("MCPServerRunnable: Data doesn't start with '{' - not valid JSON"));
                            }
                        }
                    }
                    else
                    {
                        int32 LastError = (int32)ISocketSubsystem::Get()->GetLastErrorCode();
                        ESocketConnectionState CurrentState = ClientSocket->GetConnectionState();
                        
                        // Log detailed error information
                        FString ErrorDescription;
                        bool bShouldBreak = true;
                        
                        // Check for "would block" error which isn't a real error for non-blocking sockets
                        if (LastError == SE_EWOULDBLOCK) 
                        {
                            ErrorDescription = TEXT("Socket would block (normal for non-blocking)");
                            UE_LOG(LogTemp, Verbose, TEXT("MCPServerRunnable: %s, continuing..."), *ErrorDescription);
                            bShouldBreak = false;
                            // Small sleep to prevent tight loop when no data
                            FPlatformProcess::Sleep(0.01f);
                        }
                        // Check for other transient errors we might want to tolerate
                        else if (LastError == SE_EINTR) // Interrupted system call
                        {
                            ErrorDescription = TEXT("Socket read interrupted");
                            UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: %s, continuing..."), *ErrorDescription);
                            bShouldBreak = false;
                        }
                        else 
                        {
                            // Map common error codes to descriptions
                            switch (LastError)
                            {
                                case 0: // No error - normal graceful disconnection
                                    ErrorDescription = TEXT("Graceful disconnection (no error)");
                                    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Client disconnected gracefully after %d attempts. Connection completed successfully."), 
                                           ConnectionAttempts);
                                    break;
                                case SE_ECONNRESET: ErrorDescription = TEXT("Connection reset by peer"); break;
                                case SE_ECONNABORTED: ErrorDescription = TEXT("Connection aborted"); break;
                                case SE_ENETDOWN: ErrorDescription = TEXT("Network is down"); break;
                                case SE_ENETUNREACH: ErrorDescription = TEXT("Network unreachable"); break;
                                case SE_ENOTCONN: ErrorDescription = TEXT("Socket not connected"); break;
                                case SE_ESHUTDOWN: ErrorDescription = TEXT("Socket shutdown"); break;
                                case SE_ETIMEDOUT: ErrorDescription = TEXT("Connection timed out"); break;
                                default: 
                                    ErrorDescription = FString::Printf(TEXT("Unknown error code %d"), LastError);
                                    UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Client disconnected or error after %d attempts. Error: %s, ConnectionState: %d"), 
                                           ConnectionAttempts, *ErrorDescription, (int32)CurrentState);
                                    break;
                            }
                        }
                        
                        if (bShouldBreak)
                        {
                            break;
                        }
                    }
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Failed to accept client connection"));
            }
        }
        
        // Small sleep to prevent tight loop
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
    
    // Set socket options for better connection stability
    InClientSocket->SetNonBlocking(false);
    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Set socket to blocking mode"));
    
    // Properly read full message with timeout
    const int32 MaxBufferSize = 4096;
    uint8 Buffer[MaxBufferSize];
    FString MessageBuffer;
    
    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Starting message receive loop"));
    
    while (bRunning && InClientSocket.IsValid())
    {
        // Log socket state
        bool bIsConnected = InClientSocket->GetConnectionState() == SCS_Connected;
        UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Socket state - Connected: %s"), 
               bIsConnected ? TEXT("true") : TEXT("false"));
        
        // Log pending data status before receive
        uint32 PendingDataSize = 0;
        bool HasPendingData = InClientSocket->HasPendingData(PendingDataSize);
        UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Before Recv - HasPendingData=%s, Size=%d"), 
               HasPendingData ? TEXT("true") : TEXT("false"), PendingDataSize);
        
        // Try to receive data with timeout
        int32 BytesRead = 0;
        bool bReadSuccess = false;
        
        UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Attempting to receive data..."));
        bReadSuccess = InClientSocket->Recv(Buffer, MaxBufferSize, BytesRead, ESocketReceiveFlags::None);
        
        UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Recv attempt complete - Success=%s, BytesRead=%d"), 
               bReadSuccess ? TEXT("true") : TEXT("false"), BytesRead);
        
        if (BytesRead > 0)
        {
            // Log raw data for debugging
            FString HexData;
            for (int32 i = 0; i < FMath::Min(BytesRead, 50); ++i)
            {
                HexData += FString::Printf(TEXT("%02X "), Buffer[i]);
            }
            UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Raw data (first 50 bytes hex): %s%s"), 
                   *HexData, BytesRead > 50 ? TEXT("...") : TEXT(""));
            
            // Convert and log received data
            Buffer[BytesRead] = 0; // Null terminate
            FString ReceivedData = UTF8_TO_TCHAR(Buffer);
            UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Received data as string: '%s'"), *ReceivedData);
            
            // Append to message buffer
            MessageBuffer.Append(ReceivedData);
            
            // Process complete messages (messages are terminated with newline)
            if (MessageBuffer.Contains(TEXT("\n")))
            {
                UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Newline detected in buffer, processing messages"));
                
                TArray<FString> Messages;
                MessageBuffer.ParseIntoArray(Messages, TEXT("\n"), true);
                
                UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Found %d message(s) in buffer"), Messages.Num());
                
                // Process all complete messages
                for (int32 i = 0; i < Messages.Num() - 1; ++i)
                {
                    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Processing message %d: '%s'"), 
                           i + 1, *Messages[i]);
                    ProcessMessage(InClientSocket, Messages[i]);
                }
                
                // Keep any incomplete message in the buffer
                MessageBuffer = Messages.Last();
                UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Remaining buffer after processing: %s"), 
                       *MessageBuffer);
            }
            else
            {
                UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: No complete message yet (no newline detected)"));
            }
        }
        else if (!bReadSuccess)
        {
            UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Connection closed or error occurred - Last error: %d"), 
                   (int32)ISocketSubsystem::Get()->GetLastErrorCode());
            break;
        }
        
        // Small sleep to prevent tight loop
        FPlatformProcess::Sleep(0.01f);
    }
    
    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Exited message receive loop"));
}

void FMCPServerRunnable::ProcessMessage(TSharedPtr<FSocket> Client, const FString& Message)
{
    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Processing message: %s"), *Message);
    
    // Parse message as JSON
    TSharedPtr<FJsonObject> JsonMessage;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonMessage) || !JsonMessage.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Failed to parse message as JSON"));
        return;
    }
    
    // Extract command type and parameters using MCP protocol format
    FString CommandType;
    TSharedPtr<FJsonObject> Params = MakeShareable(new FJsonObject());
    
    if (!JsonMessage->TryGetStringField(TEXT("command"), CommandType))
    {
        UE_LOG(LogTemp, Warning, TEXT("MCPServerRunnable: Message missing 'command' field"));
        return;
    }
    
    // Parameters are optional in MCP protocol
    if (JsonMessage->HasField(TEXT("params")))
    {
        TSharedPtr<FJsonValue> ParamsValue = JsonMessage->TryGetField(TEXT("params"));
        if (ParamsValue.IsValid() && ParamsValue->Type == EJson::Object)
        {
            Params = ParamsValue->AsObject();
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Executing command: %s"), *CommandType);
    
    // Execute command
    FString Response = Bridge->ExecuteCommand(CommandType, Params);
    
    // Send response with newline terminator
    Response += TEXT("\n");
    int32 BytesSent = 0;
    
    UE_LOG(LogTemp, Display, TEXT("MCPServerRunnable: Sending response: %s"), *Response);
    
    if (!Client->Send((uint8*)TCHAR_TO_UTF8(*Response), Response.Len(), BytesSent))
    {
        UE_LOG(LogTemp, Error, TEXT("MCPServerRunnable: Failed to send response"));
    }
} 