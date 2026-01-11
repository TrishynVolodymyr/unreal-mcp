#include "Commands/Niagara/GetNiagaraDiagnosticsCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

// Niagara includes
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraValidationRule.h"
#include "NiagaraScript.h"
#include "NiagaraScriptSourceBase.h"

// NiagaraEditor includes for validation system
#include "ViewModels/NiagaraSystemViewModel.h"
#include "ViewModels/NiagaraEmitterHandleViewModel.h"
#include "ViewModels/Stack/NiagaraStackViewModel.h"
#include "ViewModels/Stack/NiagaraStackEntry.h"
#include "ViewModels/Stack/NiagaraStackErrorItem.h"
#include "NiagaraValidationRules.h"

// Asset loading
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

FGetNiagaraDiagnosticsCommand::FGetNiagaraDiagnosticsCommand(INiagaraService& InNiagaraService)
    : NiagaraService(InNiagaraService)
{
}

FString FGetNiagaraDiagnosticsCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return CreateErrorResponse(TEXT("Invalid JSON parameters"));
    }

    FString SystemPath;
    if (!JsonObject->TryGetStringField(TEXT("system"), SystemPath))
    {
        return CreateErrorResponse(TEXT("Missing 'system' parameter"));
    }

    // Resolve system path - handle both full paths and short names
    FString FullPath = SystemPath;
    if (!FullPath.StartsWith(TEXT("/Game/")))
    {
        // Try to find the asset
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        TArray<FAssetData> AssetList;
        AssetRegistryModule.Get().GetAssetsByClass(UNiagaraSystem::StaticClass()->GetClassPathName(), AssetList);

        for (const FAssetData& Asset : AssetList)
        {
            if (Asset.AssetName.ToString() == SystemPath)
            {
                FullPath = Asset.GetObjectPathString();
                break;
            }
        }
    }

    // Load the Niagara System
    UNiagaraSystem* NiagaraSystem = LoadObject<UNiagaraSystem>(nullptr, *FullPath);
    if (!NiagaraSystem)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Failed to load Niagara System: %s"), *SystemPath));
    }

    // Wait for compilation to complete
    NiagaraSystem->WaitForCompilationComplete();

    // Create view model - NOT data processing only, we need full stack for issues
    TSharedPtr<FNiagaraSystemViewModel> SystemViewModel = MakeShared<FNiagaraSystemViewModel>();
    {
        FNiagaraSystemViewModelOptions SystemOptions;
        SystemOptions.bCanModifyEmittersFromTimeline = false;
        SystemOptions.bCanSimulate = false;
        SystemOptions.bCanAutoCompile = false;
        SystemOptions.bIsForDataProcessingOnly = false;  // Need full stack for issues
        SystemOptions.MessageLogGuid = NiagaraSystem->GetAssetGuid();
        SystemViewModel->Initialize(*NiagaraSystem, SystemOptions);
    }

    // Collect validation results
    TArray<TSharedPtr<FJsonValue>> DiagnosticsArray;
    int32 InfoCount = 0;
    int32 WarningCount = 0;
    int32 ErrorCount = 0;

    // Helper lambda to convert EStackIssueSeverity to string
    auto StackSeverityToString = [](EStackIssueSeverity Severity) -> FString
    {
        switch (Severity)
        {
        case EStackIssueSeverity::Error: return TEXT("Error");
        case EStackIssueSeverity::Warning: return TEXT("Warning");
        case EStackIssueSeverity::Info: return TEXT("Info");
        default: return TEXT("None");
        }
    };

    // Helper lambda to add a stack issue to the diagnostics array
    auto AddStackIssue = [&](const UNiagaraStackEntry::FStackIssue& Issue, const FString& SourceName)
    {
        if (Issue.GetSeverity() == EStackIssueSeverity::None)
        {
            return;
        }

        // Count by severity
        switch (Issue.GetSeverity())
        {
        case EStackIssueSeverity::Info:
            InfoCount++;
            break;
        case EStackIssueSeverity::Warning:
            WarningCount++;
            break;
        case EStackIssueSeverity::Error:
            ErrorCount++;
            break;
        default:
            break;
        }

        TSharedPtr<FJsonObject> DiagnosticObj = MakeShared<FJsonObject>();
        DiagnosticObj->SetStringField(TEXT("severity"), StackSeverityToString(Issue.GetSeverity()));
        DiagnosticObj->SetStringField(TEXT("summary"), Issue.GetShortDescription().ToString());
        DiagnosticObj->SetStringField(TEXT("description"), Issue.GetLongDescription().ToString());
        DiagnosticObj->SetStringField(TEXT("source"), SourceName);
        DiagnosticObj->SetStringField(TEXT("type"), TEXT("StackIssue"));

        // Add fixes if available
        const TArray<UNiagaraStackEntry::FStackIssueFix>& Fixes = Issue.GetFixes();
        if (Fixes.Num() > 0)
        {
            TArray<TSharedPtr<FJsonValue>> FixesArray;
            for (const UNiagaraStackEntry::FStackIssueFix& Fix : Fixes)
            {
                FixesArray.Add(MakeShared<FJsonValueString>(Fix.GetDescription().ToString()));
            }
            DiagnosticObj->SetArrayField(TEXT("fixes"), FixesArray);
        }

        DiagnosticsArray.Add(MakeShared<FJsonValueObject>(DiagnosticObj));
    };

    // Helper to process all entries in a stack view model
    auto ProcessStackViewModel = [&](UNiagaraStackViewModel* StackViewModel, const FString& StackName)
    {
        if (!StackViewModel)
        {
            return;
        }

        UNiagaraStackEntry* RootEntry = StackViewModel->GetRootEntry();
        if (!RootEntry)
        {
            return;
        }

        // Force refresh the stack to populate children and issues
        RootEntry->RefreshChildren();

        // Manually traverse ALL entries and collect their issues
        TArray<UNiagaraStackEntry*> EntriesToProcess;
        EntriesToProcess.Add(RootEntry);

        while (EntriesToProcess.Num() > 0)
        {
            UNiagaraStackEntry* Entry = EntriesToProcess[0];
            EntriesToProcess.RemoveAt(0);

            if (!Entry)
            {
                continue;
            }

            // Refresh this entry to ensure issues are populated
            Entry->RefreshChildren();

            // Collect issues from this entry
            const TArray<UNiagaraStackEntry::FStackIssue>& Issues = Entry->GetIssues();
            for (const UNiagaraStackEntry::FStackIssue& Issue : Issues)
            {
                FString SourceName = Entry->GetDisplayName().ToString();
                if (!StackName.IsEmpty())
                {
                    SourceName = FString::Printf(TEXT("%s - %s"), *StackName, *SourceName);
                }
                AddStackIssue(Issue, SourceName);
            }

            // Add children to process
            TArray<UNiagaraStackEntry*> Children;
            Entry->GetUnfilteredChildren(Children);
            EntriesToProcess.Append(Children);
        }
    };

    // Process system stack view model
    ProcessStackViewModel(SystemViewModel->GetSystemStackViewModel(), TEXT("System"));

    // Process emitter stack view models
    const TArray<TSharedRef<FNiagaraEmitterHandleViewModel>>& EmitterHandleViewModels = SystemViewModel->GetEmitterHandleViewModels();
    for (const TSharedRef<FNiagaraEmitterHandleViewModel>& EmitterHandleViewModel : EmitterHandleViewModels)
    {
        FString EmitterName = EmitterHandleViewModel->GetName().ToString();
        UNiagaraStackViewModel* EmitterStackViewModel = EmitterHandleViewModel->GetEmitterStackViewModel();
        ProcessStackViewModel(EmitterStackViewModel, EmitterName);
    }

    // Also run system-level validation rules
    NiagaraValidation::ValidateAllRulesInSystem(
        SystemViewModel,
        [&](const FNiagaraValidationResult& Result)
        {
            // Count by severity
            switch (Result.Severity)
            {
            case ENiagaraValidationSeverity::Info:
                InfoCount++;
                break;
            case ENiagaraValidationSeverity::Warning:
                WarningCount++;
                break;
            case ENiagaraValidationSeverity::Error:
                ErrorCount++;
                break;
            }

            // Create diagnostic entry
            TSharedPtr<FJsonObject> DiagnosticObj = MakeShared<FJsonObject>();

            // Severity
            FString SeverityStr;
            switch (Result.Severity)
            {
            case ENiagaraValidationSeverity::Info:
                SeverityStr = TEXT("Info");
                break;
            case ENiagaraValidationSeverity::Warning:
                SeverityStr = TEXT("Warning");
                break;
            case ENiagaraValidationSeverity::Error:
                SeverityStr = TEXT("Error");
                break;
            }
            DiagnosticObj->SetStringField(TEXT("severity"), SeverityStr);

            // Summary and description
            DiagnosticObj->SetStringField(TEXT("summary"), Result.SummaryText.ToString());
            DiagnosticObj->SetStringField(TEXT("description"), Result.Description.ToString());
            DiagnosticObj->SetStringField(TEXT("type"), TEXT("ValidationRule"));

            // Source object name (if available)
            if (Result.SourceObject.IsValid())
            {
                DiagnosticObj->SetStringField(TEXT("source"), Result.SourceObject->GetName());
            }

            // Available fixes
            if (Result.Fixes.Num() > 0)
            {
                TArray<TSharedPtr<FJsonValue>> FixesArray;
                for (const FNiagaraValidationFix& Fix : Result.Fixes)
                {
                    FixesArray.Add(MakeShared<FJsonValueString>(Fix.Description.ToString()));
                }
                DiagnosticObj->SetArrayField(TEXT("fixes"), FixesArray);
            }

            DiagnosticsArray.Add(MakeShared<FJsonValueObject>(DiagnosticObj));
        }
    );

    // Build response
    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetBoolField(TEXT("success"), true);
    ResponseObj->SetStringField(TEXT("system"), NiagaraSystem->GetName());
    ResponseObj->SetStringField(TEXT("path"), FullPath);
    ResponseObj->SetArrayField(TEXT("diagnostics"), DiagnosticsArray);
    ResponseObj->SetNumberField(TEXT("info_count"), InfoCount);
    ResponseObj->SetNumberField(TEXT("warning_count"), WarningCount);
    ResponseObj->SetNumberField(TEXT("error_count"), ErrorCount);
    ResponseObj->SetNumberField(TEXT("total_count"), DiagnosticsArray.Num());

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ResponseObj.ToSharedRef(), Writer);
    return OutputString;
}

bool FGetNiagaraDiagnosticsCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    FString SystemPath;
    return JsonObject->TryGetStringField(TEXT("system"), SystemPath);
}

FString FGetNiagaraDiagnosticsCommand::CreateErrorResponse(const FString& ErrorMessage) const
{
    TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
    ErrorObj->SetBoolField(TEXT("success"), false);
    ErrorObj->SetStringField(TEXT("error"), ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(ErrorObj.ToSharedRef(), Writer);
    return OutputString;
}
