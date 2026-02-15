#include "Commands/UMG/SetWidgetPlacementCommand.h"
#include "Services/UMG/IUMGService.h"
#include "MCPErrorHandler.h"
#include "MCPError.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogSetWidgetPlacementCommand, Log, All);

FSetWidgetPlacementCommand::FSetWidgetPlacementCommand(TSharedPtr<IUMGService> InUMGService)
    : UMGService(InUMGService)
{
}

FString FSetWidgetPlacementCommand::Execute(const FString& Parameters)
{
    UE_LOG(LogSetWidgetPlacementCommand, Log, TEXT("SetWidgetPlacementCommand::Execute - Command execution started"));
    UE_LOG(LogSetWidgetPlacementCommand, Verbose, TEXT("Parameters: %s"), *Parameters);
    
    // Clear vector storage from previous call and reserve for max params
    ExtractedVectorStorage.Empty();
    ExtractedVectorStorage.Reserve(8); // position, size, alignment, anchors, anchor_min, anchor_max + spare
    
    // Parse JSON parameters using centralized JSON utilities
    TSharedPtr<FJsonObject> JsonObject = ParseJsonParameters(Parameters);
    if (!JsonObject.IsValid())
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(TEXT("Invalid JSON parameters"));
        return SerializeErrorResponse(Error);
    }

    // Validate parameters using structured validation
    FString ValidationError;
    if (!ValidateParamsInternal(JsonObject, ValidationError))
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(ValidationError);
        return SerializeErrorResponse(Error);
    }

    // Execute command using service layer delegation
    TSharedPtr<FJsonObject> Response = ExecuteInternal(JsonObject);
    
    // Serialize response using centralized JSON utilities
    return SerializeJsonResponse(Response);
}

TSharedPtr<FJsonObject> FSetWidgetPlacementCommand::ExecuteInternal(const TSharedPtr<FJsonObject>& Params)
{
    // Validate service availability using dependency injection pattern
    if (!UMGService.IsValid())
    {
        UE_LOG(LogSetWidgetPlacementCommand, Error, TEXT("UMG service is not available - dependency injection failed"));
        FMCPError Error = FMCPErrorHandler::CreateInternalError(TEXT("UMG service is not available"));
        return CreateErrorResponse(Error);
    }

    // Extract and validate parameters using structured parameter extraction
    FWidgetPlacementParams PlacementParams;
    if (!ExtractPlacementParameters(Params, PlacementParams))
    {
        FMCPError Error = FMCPErrorHandler::CreateValidationFailedError(TEXT("Failed to extract placement parameters"));
        return CreateErrorResponse(Error);
    }

    UE_LOG(LogSetWidgetPlacementCommand, Log, TEXT("Setting placement for component '%s' in widget '%s'"), 
           *PlacementParams.ComponentName, *PlacementParams.WidgetName);

    // Delegate to service layer following single responsibility principle
    bool bSuccess = UMGService->SetWidgetPlacement(
        PlacementParams.WidgetName, 
        PlacementParams.ComponentName, 
        PlacementParams.Position, 
        PlacementParams.Size, 
        PlacementParams.Alignment,
        PlacementParams.AnchorMin,
        PlacementParams.AnchorMax,
        PlacementParams.bHasAutoSize ? &PlacementParams.bAutoSize : nullptr
    );
    
    if (!bSuccess)
    {
        UE_LOG(LogSetWidgetPlacementCommand, Warning, TEXT("Service layer failed to set widget placement"));
        FMCPError Error = FMCPErrorHandler::CreateExecutionFailedError(
            FString::Printf(TEXT("Failed to set placement for component '%s' in widget '%s'"), 
                          *PlacementParams.ComponentName, *PlacementParams.WidgetName));
        return CreateErrorResponse(Error);
    }
    
    UE_LOG(LogSetWidgetPlacementCommand, Log, TEXT("Widget placement set successfully"));
    return CreateSuccessResponse(PlacementParams);
}

FString FSetWidgetPlacementCommand::GetCommandName() const
{
    return TEXT("set_widget_component_placement");
}

bool FSetWidgetPlacementCommand::ValidateParams(const FString& Parameters) const
{
    // Parse JSON for validation
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString ValidationError;
    return ValidateParamsInternal(JsonObject, ValidationError);
}

bool FSetWidgetPlacementCommand::ValidateParamsInternal(const TSharedPtr<FJsonObject>& Params, FString& OutError) const
{
    if (!Params.IsValid())
    {
        OutError = TEXT("Invalid JSON parameters");
        return false;
    }

    // Check for widget_name or blueprint_name
    if (!Params->HasField(TEXT("widget_name")) && !Params->HasField(TEXT("blueprint_name")))
    {
        OutError = TEXT("Missing required parameter: widget_name or blueprint_name");
        return false;
    }

    FString WidgetName;
    if (Params->HasField(TEXT("widget_name")))
    {
        WidgetName = Params->GetStringField(TEXT("widget_name"));
    }
    else if (Params->HasField(TEXT("blueprint_name")))
    {
        WidgetName = Params->GetStringField(TEXT("blueprint_name"));
    }
    
    if (WidgetName.IsEmpty())
    {
        OutError = TEXT("widget_name/blueprint_name cannot be empty");
        return false;
    }

    // Check for component_name
    if (!Params->HasField(TEXT("component_name")))
    {
        OutError = TEXT("Missing required parameter: component_name");
        return false;
    }

    FString ComponentName = Params->GetStringField(TEXT("component_name"));
    if (ComponentName.IsEmpty())
    {
        OutError = TEXT("component_name cannot be empty");
        return false;
    }

    // At least one placement parameter must be provided
    if (!Params->HasField(TEXT("position")) && !Params->HasField(TEXT("size")) && !Params->HasField(TEXT("alignment"))
        && !Params->HasField(TEXT("anchors")) && !Params->HasField(TEXT("anchor_min")) && !Params->HasField(TEXT("anchor_max"))
        && !Params->HasField(TEXT("auto_size")))
    {
        OutError = TEXT("At least one placement parameter (position, size, alignment, anchors, anchor_min, anchor_max, or auto_size) must be provided");
        return false;
    }

    // Validate position array if provided
    if (Params->HasField(TEXT("position")))
    {
        const TArray<TSharedPtr<FJsonValue>>* PositionArray;
        if (!Params->TryGetArrayField(TEXT("position"), PositionArray) || PositionArray->Num() != 2)
        {
            OutError = TEXT("position must be an array with exactly 2 elements [X, Y]");
            return false;
        }
    }

    // Validate size array if provided
    if (Params->HasField(TEXT("size")))
    {
        const TArray<TSharedPtr<FJsonValue>>* SizeArray;
        if (!Params->TryGetArrayField(TEXT("size"), SizeArray) || SizeArray->Num() != 2)
        {
            OutError = TEXT("size must be an array with exactly 2 elements [Width, Height]");
            return false;
        }
    }

    // Validate alignment array if provided
    if (Params->HasField(TEXT("alignment")))
    {
        const TArray<TSharedPtr<FJsonValue>>* AlignmentArray;
        if (!Params->TryGetArrayField(TEXT("alignment"), AlignmentArray) || AlignmentArray->Num() != 2)
        {
            OutError = TEXT("alignment must be an array with exactly 2 elements [X, Y]");
            return false;
        }
    }

    // Validate anchors shorthand (sets both min and max to same value)
    if (Params->HasField(TEXT("anchors")))
    {
        const TArray<TSharedPtr<FJsonValue>>* AnchorsArray;
        if (!Params->TryGetArrayField(TEXT("anchors"), AnchorsArray) || AnchorsArray->Num() != 2)
        {
            OutError = TEXT("anchors must be an array with exactly 2 elements [X, Y] (sets both min and max)");
            return false;
        }
    }

    // Validate anchor_min array if provided
    if (Params->HasField(TEXT("anchor_min")))
    {
        const TArray<TSharedPtr<FJsonValue>>* AnchorMinArray;
        if (!Params->TryGetArrayField(TEXT("anchor_min"), AnchorMinArray) || AnchorMinArray->Num() != 2)
        {
            OutError = TEXT("anchor_min must be an array with exactly 2 elements [X, Y]");
            return false;
        }
    }

    // Validate anchor_max array if provided
    if (Params->HasField(TEXT("anchor_max")))
    {
        const TArray<TSharedPtr<FJsonValue>>* AnchorMaxArray;
        if (!Params->TryGetArrayField(TEXT("anchor_max"), AnchorMaxArray) || AnchorMaxArray->Num() != 2)
        {
            OutError = TEXT("anchor_max must be an array with exactly 2 elements [X, Y]");
            return false;
        }
    }

    return true;
}



TSharedPtr<FJsonObject> FSetWidgetPlacementCommand::CreateErrorResponse(const FMCPError& Error) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject);
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), Error.ErrorMessage);
    ResponseObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Failed to set widget placement: %s"), *Error.ErrorMessage));

    return ResponseObj;
}

bool FSetWidgetPlacementCommand::ParseVector2DFromJson(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FVector2D& OutVector) const
{
    if (JsonArray.Num() != 2)
    {
        return false;
    }
    
    double X, Y;
    if (!JsonArray[0]->TryGetNumber(X) || !JsonArray[1]->TryGetNumber(Y))
    {
        return false;
    }
    
    OutVector.X = static_cast<float>(X);
    OutVector.Y = static_cast<float>(Y);
    return true;
}

// JSON Utility Methods (following centralized JSON utilities pattern)
TSharedPtr<FJsonObject> FSetWidgetPlacementCommand::ParseJsonParameters(const FString& Parameters) const
{
    if (Parameters.IsEmpty())
    {
        UE_LOG(LogSetWidgetPlacementCommand, Warning, TEXT("Empty parameters provided"));
        return nullptr;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogSetWidgetPlacementCommand, Error, TEXT("Failed to parse JSON parameters: %s"), *Parameters);
        return nullptr;
    }

    return JsonObject;
}

FString FSetWidgetPlacementCommand::SerializeJsonResponse(const TSharedPtr<FJsonObject>& Response) const
{
    if (!Response.IsValid())
    {
        UE_LOG(LogSetWidgetPlacementCommand, Error, TEXT("Invalid response object for serialization"));
        return TEXT("{}");
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
    return OutputString;
}

FString FSetWidgetPlacementCommand::SerializeErrorResponse(const FMCPError& Error) const
{
    TSharedPtr<FJsonObject> ErrorResponse = CreateErrorResponse(Error);
    return SerializeJsonResponse(ErrorResponse);
}

// Parameter Extraction (following structured parameter extraction pattern)
bool FSetWidgetPlacementCommand::ExtractPlacementParameters(const TSharedPtr<FJsonObject>& Params, FWidgetPlacementParams& OutParams) const
{
    // Extract widget name with backward compatibility
    if (Params->HasField(TEXT("widget_name")))
    {
        OutParams.WidgetName = Params->GetStringField(TEXT("widget_name"));
    }
    else if (Params->HasField(TEXT("blueprint_name")))
    {
        OutParams.WidgetName = Params->GetStringField(TEXT("blueprint_name"));
    }
    else
    {
        UE_LOG(LogSetWidgetPlacementCommand, Error, TEXT("Missing widget_name or blueprint_name parameter"));
        return false;
    }

    // Extract component name
    if (!Params->HasField(TEXT("component_name")))
    {
        UE_LOG(LogSetWidgetPlacementCommand, Error, TEXT("Missing component_name parameter"));
        return false;
    }
    OutParams.ComponentName = Params->GetStringField(TEXT("component_name"));

    // Extract optional placement parameters
    OutParams.Position = ExtractVector2DParameter(Params, TEXT("position"));
    OutParams.Size = ExtractVector2DParameter(Params, TEXT("size"));
    OutParams.Alignment = ExtractVector2DParameter(Params, TEXT("alignment"));

    // Extract anchor parameters
    // "anchors" is shorthand that sets both min and max to the same value (e.g. [0,1] for bottom-left)
    FVector2D* AnchorsShorthand = ExtractVector2DParameter(Params, TEXT("anchors"));
    if (AnchorsShorthand)
    {
        OutParams.AnchorMin = AnchorsShorthand;
        OutParams.AnchorMax = ExtractVector2DParameter(Params, TEXT("anchors")); // re-extract for separate pointer
    }
    // Explicit anchor_min/anchor_max override shorthand
    FVector2D* ExplicitMin = ExtractVector2DParameter(Params, TEXT("anchor_min"));
    FVector2D* ExplicitMax = ExtractVector2DParameter(Params, TEXT("anchor_max"));
    if (ExplicitMin) OutParams.AnchorMin = ExplicitMin;
    if (ExplicitMax) OutParams.AnchorMax = ExplicitMax;

    // Extract auto_size parameter
    if (Params->HasField(TEXT("auto_size")))
    {
        OutParams.bAutoSize = Params->GetBoolField(TEXT("auto_size"));
        OutParams.bHasAutoSize = true;
    }

    // Validate that at least one placement parameter is provided
    if (!OutParams.Position && !OutParams.Size && !OutParams.Alignment && !OutParams.AnchorMin && !OutParams.AnchorMax && !OutParams.bHasAutoSize)
    {
        UE_LOG(LogSetWidgetPlacementCommand, Error, TEXT("At least one placement parameter must be provided"));
        return false;
    }

    return true;
}

FVector2D* FSetWidgetPlacementCommand::ExtractVector2DParameter(const TSharedPtr<FJsonObject>& Params, const FString& ParameterName) const
{
    if (!Params->HasField(ParameterName))
    {
        return nullptr;
    }

    const TArray<TSharedPtr<FJsonValue>>* Array;
    if (!Params->TryGetArrayField(ParameterName, Array) || Array->Num() != 2)
    {
        UE_LOG(LogSetWidgetPlacementCommand, Warning, TEXT("Invalid %s parameter format - expected array with 2 elements"), *ParameterName);
        return nullptr;
    }

    // Store each vector separately to avoid reuse/overwrite bugs
    FVector2D NewVector;
    if (ParseVector2DFromJson(*Array, NewVector))
    {
        int32 Index = ExtractedVectorStorage.Add(NewVector);
        return &ExtractedVectorStorage[Index];
    }

    UE_LOG(LogSetWidgetPlacementCommand, Warning, TEXT("Failed to parse %s parameter values"), *ParameterName);
    return nullptr;
}

// Response Creation (following structured error handling pattern)
TSharedPtr<FJsonObject> FSetWidgetPlacementCommand::CreateSuccessResponse(const FWidgetPlacementParams& Params) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject);
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("widget_name"), Params.WidgetName);
    ResponseObj->SetStringField(TEXT("component_name"), Params.ComponentName);
    
    // Add placement information that was set
    TSharedPtr<FJsonObject> PlacementObj = MakeShareable(new FJsonObject);
    
    if (Params.Position)
    {
        TArray<TSharedPtr<FJsonValue>> PositionArray;
        PositionArray.Add(MakeShareable(new FJsonValueNumber(Params.Position->X)));
        PositionArray.Add(MakeShareable(new FJsonValueNumber(Params.Position->Y)));
        PlacementObj->SetArrayField(TEXT("position"), PositionArray);
    }
    
    if (Params.Size)
    {
        TArray<TSharedPtr<FJsonValue>> SizeArray;
        SizeArray.Add(MakeShareable(new FJsonValueNumber(Params.Size->X)));
        SizeArray.Add(MakeShareable(new FJsonValueNumber(Params.Size->Y)));
        PlacementObj->SetArrayField(TEXT("size"), SizeArray);
    }
    
    if (Params.Alignment)
    {
        TArray<TSharedPtr<FJsonValue>> AlignmentArray;
        AlignmentArray.Add(MakeShareable(new FJsonValueNumber(Params.Alignment->X)));
        AlignmentArray.Add(MakeShareable(new FJsonValueNumber(Params.Alignment->Y)));
        PlacementObj->SetArrayField(TEXT("alignment"), AlignmentArray);
    }

    if (Params.AnchorMin)
    {
        TArray<TSharedPtr<FJsonValue>> AnchorMinArray;
        AnchorMinArray.Add(MakeShareable(new FJsonValueNumber(Params.AnchorMin->X)));
        AnchorMinArray.Add(MakeShareable(new FJsonValueNumber(Params.AnchorMin->Y)));
        PlacementObj->SetArrayField(TEXT("anchor_min"), AnchorMinArray);
    }

    if (Params.AnchorMax)
    {
        TArray<TSharedPtr<FJsonValue>> AnchorMaxArray;
        AnchorMaxArray.Add(MakeShareable(new FJsonValueNumber(Params.AnchorMax->X)));
        AnchorMaxArray.Add(MakeShareable(new FJsonValueNumber(Params.AnchorMax->Y)));
        PlacementObj->SetArrayField(TEXT("anchor_max"), AnchorMaxArray);
    }

    if (Params.bHasAutoSize)
    {
        PlacementObj->SetBoolField(TEXT("auto_size"), Params.bAutoSize);
    }
    
    ResponseObj->SetObjectField(TEXT("placement"), PlacementObj);
    ResponseObj->SetStringField(TEXT("message"), 
        FString::Printf(TEXT("Successfully set placement for component '%s' in widget '%s'"), 
                       *Params.ComponentName, *Params.WidgetName));

    return ResponseObj;
}

