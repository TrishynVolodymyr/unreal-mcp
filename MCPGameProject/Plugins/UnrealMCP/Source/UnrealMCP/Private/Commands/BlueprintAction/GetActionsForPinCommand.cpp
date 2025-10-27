#include "Commands/BlueprintAction/GetActionsForPinCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

FGetActionsForPinCommand::FGetActionsForPinCommand(TSharedPtr<IBlueprintActionService> InBlueprintActionService)
    : BlueprintActionService(InBlueprintActionService)
{
}

FString FGetActionsForPinCommand::Execute(const FString& Parameters)
{
    if (!BlueprintActionService.IsValid())
    {
        return CreateErrorResponse(TEXT("Blueprint action service not available"));
    }
    
    FString PinType, PinSubcategory, SearchFilter;
    int32 MaxResults;
    FString ParseError;
    
    if (!ParseParameters(Parameters, PinType, PinSubcategory, SearchFilter, MaxResults, ParseError))
    {
        return CreateErrorResponse(ParseError);
    }
    
    return BlueprintActionService->GetActionsForPin(PinType, PinSubcategory, SearchFilter, MaxResults);
}

FString FGetActionsForPinCommand::GetCommandName() const
{
    return TEXT("get_actions_for_pin");
}

bool FGetActionsForPinCommand::ValidateParams(const FString& Parameters) const
{
    FString PinType, PinSubcategory, SearchFilter;
    int32 MaxResults;
    FString ParseError;
    return ParseParameters(Parameters, PinType, PinSubcategory, SearchFilter, MaxResults, ParseError);
}

bool FGetActionsForPinCommand::ParseParameters(
    const FString& Parameters,
    FString& OutPinType,
    FString& OutPinSubcategory,
    FString& OutSearchFilter,
    int32& OutMaxResults,
    FString& OutError) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }
    
    // Parse required and optional parameters
    OutPinType = JsonObject->GetStringField(TEXT("pin_type"));
    OutPinSubcategory = JsonObject->GetStringField(TEXT("pin_subcategory"));
    OutSearchFilter = JsonObject->GetStringField(TEXT("search_filter"));
    
    // Parse max_results with default value
    if (JsonObject->HasField(TEXT("max_results")))
    {
        OutMaxResults = JsonObject->GetIntegerField(TEXT("max_results"));
    }
    else
    {
        OutMaxResults = 50; // Default value
    }
    
    // Validate max_results
    if (OutMaxResults <= 0 || OutMaxResults > 1000)
    {
        OutError = TEXT("max_results must be between 1 and 1000");
        return false;
    }
    
    return true;
}

FString FGetActionsForPinCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    
    return OutputString;
}


