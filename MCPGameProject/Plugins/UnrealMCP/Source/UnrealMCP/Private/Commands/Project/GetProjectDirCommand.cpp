#include "Commands/Project/GetProjectDirCommand.h"
#include "Utils/UnrealMCPCommonUtils.h"

FGetProjectDirCommand::FGetProjectDirCommand(TSharedPtr<IProjectService> InProjectService)
    : ProjectService(InProjectService)
{
}

bool FGetProjectDirCommand::ValidateParams(const FString& Parameters) const
{
    // No parameters required for this command
    return true;
}

FString FGetProjectDirCommand::Execute(const FString& Parameters)
{
    // Get project directory
    FString ProjectDir = ProjectService->GetProjectDirectory();

    // Get project name (used as module name for /Script/{ProjectName} paths)
    FString ProjectName = FApp::GetProjectName();

    // Create success response
    TSharedPtr<FJsonObject> ResponseData = MakeShared<FJsonObject>();
    ResponseData->SetBoolField(TEXT("success"), true);
    ResponseData->SetStringField(TEXT("project_dir"), ProjectDir);
    ResponseData->SetStringField(TEXT("project_name"), ProjectName);
    ResponseData->SetStringField(TEXT("module_path"), FString::Printf(TEXT("/Script/%s"), *ProjectName));

    // Convert response to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseData.ToSharedRef(), Writer);
    return OutputString;
}
