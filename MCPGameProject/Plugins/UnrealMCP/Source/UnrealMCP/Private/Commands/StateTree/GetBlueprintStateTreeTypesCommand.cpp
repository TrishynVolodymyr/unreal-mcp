#include "Commands/StateTree/GetBlueprintStateTreeTypesCommand.h"
#include "Services/IStateTreeService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FGetBlueprintStateTreeTypesCommand::FGetBlueprintStateTreeTypesCommand(IStateTreeService& InService)
    : Service(InService)
{
}

FString FGetBlueprintStateTreeTypesCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> Types;
    if (!Service.GetBlueprintStateTreeTypes(Types))
    {
        return CreateErrorResponse(TEXT("Failed to get Blueprint StateTree types"));
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetObjectField(TEXT("data"), Types);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

FString FGetBlueprintStateTreeTypesCommand::GetCommandName() const
{
    return TEXT("get_blueprint_state_tree_types");
}

bool FGetBlueprintStateTreeTypesCommand::ValidateParams(const FString& Parameters) const
{
    // No required parameters
    return true;
}

FString FGetBlueprintStateTreeTypesCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), false);
    ResponseObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}
