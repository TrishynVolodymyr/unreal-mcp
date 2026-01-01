#include "Commands/BlueprintAction/GetNodePinInfoCommand.h"
#include "Services/IBlueprintActionService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

FGetNodePinInfoCommand::FGetNodePinInfoCommand(TSharedPtr<IBlueprintActionService> InBlueprintActionService)
    : BlueprintActionService(InBlueprintActionService)
{
}

FString FGetNodePinInfoCommand::Execute(const FString& Parameters)
{
    if (!BlueprintActionService.IsValid())
    {
        return CreateErrorResponse(TEXT("Blueprint action service is not available"));
    }

    // Validate parameters first
    if (!ValidateParams(Parameters))
    {
        return CreateErrorResponse(TEXT("Invalid parameters provided"));
    }

    // Parse parameters
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        return CreateErrorResponse(TEXT("Failed to parse JSON parameters"));
    }

    FString NodeName = ParamsObj->GetStringField(TEXT("node_name"));
    FString PinName = ParamsObj->GetStringField(TEXT("pin_name"));
    FString ClassName = ParamsObj->HasField(TEXT("class_name")) ? ParamsObj->GetStringField(TEXT("class_name")) : TEXT("");

    // Execute the service method
    return BlueprintActionService->GetNodePinInfo(NodeName, PinName, ClassName);
}

FString FGetNodePinInfoCommand::GetCommandName() const
{
    return TEXT("get_node_pin_info");
}

bool FGetNodePinInfoCommand::ValidateParams(const FString& Parameters) const
{
    if (Parameters.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("GetNodePinInfoCommand: Parameters are empty"));
        return false;
    }

    // Parse JSON to validate structure
    TSharedPtr<FJsonObject> ParamsObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, ParamsObj) || !ParamsObj.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("GetNodePinInfoCommand: Failed to parse JSON parameters"));
        return false;
    }

    // Validate required fields
    if (!ParamsObj->HasField(TEXT("node_name")) || ParamsObj->GetStringField(TEXT("node_name")).IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("GetNodePinInfoCommand: Missing or empty 'node_name' parameter"));
        return false;
    }

    if (!ParamsObj->HasField(TEXT("pin_name")) || ParamsObj->GetStringField(TEXT("pin_name")).IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("GetNodePinInfoCommand: Missing or empty 'pin_name' parameter"));
        return false;
    }

    return true;
}

FString FGetNodePinInfoCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    
    return OutputString;
}


