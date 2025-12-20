// Copyright Epic Games, Inc. All Rights Reserved.

#include "NodeResultBuilder.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "EdGraphSchema_K2.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphNode.h"

FString FNodeResultBuilder::BuildNodeResult(
    bool bSuccess,
    const FString& Message,
    const FString& BlueprintName,
    const FString& FunctionName,
    UEdGraphNode* NewNode,
    const FString& NodeTitle,
    const FString& NodeType,
    UClass* TargetClass,
    int32 PositionX,
    int32 PositionY,
    const FString& Warning
)
{
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), bSuccess);
    
    // Use the correct field name based on success/failure
    if (bSuccess)
    {
        ResultObj->SetStringField(TEXT("message"), Message);
    }
    else
    {
        ResultObj->SetStringField(TEXT("error"), Message);
    }
    
    if (bSuccess && NewNode)
    {
        ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
        ResultObj->SetStringField(TEXT("function_name"), FunctionName);
        ResultObj->SetStringField(TEXT("node_type"), NodeType);
        ResultObj->SetStringField(TEXT("class_name"), NodeType.Equals(TEXT("UK2Node_CallFunction")) ? (TargetClass ? TargetClass->GetName() : TEXT("")) : TEXT(""));
        ResultObj->SetStringField(TEXT("node_id"), NewNode->NodeGuid.ToString());
        ResultObj->SetStringField(TEXT("node_title"), NodeTitle);
        
        // Check if this is a pure function (no execution pins)
        bool bHasExecutionPins = false;
        for (UEdGraphPin* Pin : NewNode->Pins)
        {
            if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
            {
                bHasExecutionPins = true;
                break;
            }
        }
        bool bIsPure = !bHasExecutionPins;
        
        // CRITICAL: Add execution pin information for AI assistants
        ResultObj->SetBoolField(TEXT("is_pure_function"), bIsPure);
        ResultObj->SetBoolField(TEXT("requires_execution_flow"), !bIsPure);
        
        // Add position info
        TSharedPtr<FJsonObject> PositionObj = MakeShared<FJsonObject>();
        PositionObj->SetNumberField(TEXT("x"), PositionX);
        PositionObj->SetNumberField(TEXT("y"), PositionY);
        ResultObj->SetObjectField(TEXT("position"), PositionObj);
        
        // Add pin information
        TArray<TSharedPtr<FJsonValue>> PinsArray;
        for (UEdGraphPin* Pin : NewNode->Pins)
        {
            TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
            PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
            PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
            PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
            PinObj->SetBoolField(TEXT("is_execution"), Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec);
            PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
        }
        ResultObj->SetArrayField(TEXT("pins"), PinsArray);
    }

    // Add warning if present
    if (!Warning.IsEmpty())
    {
        ResultObj->SetStringField(TEXT("warning"), Warning);
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
    return OutputString;
}
