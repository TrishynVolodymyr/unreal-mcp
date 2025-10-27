#include "Commands/BlueprintAction/GetActionsForClassHierarchyCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

FGetActionsForClassHierarchyCommand::FGetActionsForClassHierarchyCommand(TSharedPtr<IBlueprintActionService> InBlueprintActionService)
    : BlueprintActionService(InBlueprintActionService)
{
}

FString FGetActionsForClassHierarchyCommand::Execute(const FString& Parameters)
{
    UE_LOG(LogTemp, Warning, TEXT("FGetActionsForClassHierarchyCommand::Execute: NEW ARCHITECTURE COMMAND CALLED!"));
    
    if (!BlueprintActionService.IsValid())
    {
        return CreateErrorResponse(TEXT("Blueprint action service is not available"));
    }

    // Parse parameters
    FString ClassName;
    FString SearchFilter;
    int32 MaxResults;
    FString Error;

    if (!ParseParameters(Parameters, ClassName, SearchFilter, MaxResults, Error))
    {
        return CreateErrorResponse(Error);
    }

    UE_LOG(LogTemp, Warning, TEXT("FGetActionsForClassHierarchyCommand::Execute: Calling service with ClassName=%s, SearchFilter=%s, MaxResults=%d"), *ClassName, *SearchFilter, MaxResults);

    // Execute the service call
    return BlueprintActionService->GetActionsForClassHierarchy(ClassName, SearchFilter, MaxResults);
}

FString FGetActionsForClassHierarchyCommand::GetCommandName() const
{
    return TEXT("get_actions_for_class_hierarchy");
}

bool FGetActionsForClassHierarchyCommand::ValidateParams(const FString& Parameters) const
{
    FString ClassName;
    FString SearchFilter;
    int32 MaxResults;
    FString Error;

    return ParseParameters(Parameters, ClassName, SearchFilter, MaxResults, Error);
}

bool FGetActionsForClassHierarchyCommand::ParseParameters(
    const FString& Parameters,
    FString& OutClassName,
    FString& OutSearchFilter,
    int32& OutMaxResults,
    FString& OutError
) const
{
    if (Parameters.IsEmpty())
    {
        OutError = TEXT("Empty parameters provided");
        return false;
    }

    // Parse JSON
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Failed to parse JSON parameters");
        return false;
    }

    // Validate required fields
    if (!JsonObject->HasField(TEXT("class_name")))
    {
        OutError = TEXT("Missing required field: class_name");
        return false;
    }

    OutClassName = JsonObject->GetStringField(TEXT("class_name"));
    if (OutClassName.IsEmpty())
    {
        OutError = TEXT("class_name cannot be empty");
        return false;
    }

    // Optional fields with defaults
    OutSearchFilter = JsonObject->GetStringField(TEXT("search_filter"));
    OutMaxResults = JsonObject->GetIntegerField(TEXT("max_results"));
    
    if (OutMaxResults <= 0)
    {
        OutMaxResults = 50; // Default value
    }

    return true;
}

FString FGetActionsForClassHierarchyCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorResponse = MakeShareable(new FJsonObject);
    ErrorResponse->SetBoolField(TEXT("success"), false);
    ErrorResponse->SetStringField(TEXT("error"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorResponse.ToSharedRef(), Writer);
    
    return OutputString;
}


