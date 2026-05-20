#include "Commands/Blueprint/SetBlueprintParentClassCommand.h"

#include "Services/IBlueprintService.h"
#include "Services/AssetDiscoveryService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Blueprint.h"
#include "Kismet2/KismetEditorUtilities.h"

namespace
{
    FString MakeErr(const FString& Msg)
    {
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetBoolField(TEXT("success"), false);
        Obj->SetStringField(TEXT("error"), Msg);
        FString Out;
        TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
        FJsonSerializer::Serialize(Obj.ToSharedRef(), W);
        return Out;
    }

    UClass* ResolveParentClass(const FString& Path)
    {
        if (Path.IsEmpty()) return nullptr;

        // Blueprint asset path (e.g. /Game/Blueprints/BP_Base): append _C and load as class
        if (Path.StartsWith(TEXT("/Game")))
        {
            FString ClassPath = Path;
            int32 DotIdx;
            FString PackagePart = Path;
            FString ObjectPart;
            if (Path.FindChar('.', DotIdx))
            {
                PackagePart = Path.Left(DotIdx);
                ObjectPart = Path.RightChop(DotIdx + 1);
            }
            else
            {
                int32 SlashIdx;
                if (Path.FindLastChar('/', SlashIdx)) ObjectPart = Path.RightChop(SlashIdx + 1);
            }
            if (!ClassPath.EndsWith(TEXT("_C")))
            {
                ClassPath = FString::Printf(TEXT("%s.%s_C"), *PackagePart, *ObjectPart);
            }
            if (UClass* C = LoadClass<UObject>(nullptr, *ClassPath)) return C;
        }

        // Try AssetDiscoveryService — handles short names, /Script/, project module, engine, UMG, etc.
        if (UClass* C = FAssetDiscoveryService::Get().ResolveObjectClass(Path)) return C;

        return nullptr;
    }
}

FSetBlueprintParentClassCommand::FSetBlueprintParentClassCommand(IBlueprintService& InBlueprintService)
    : BlueprintService(InBlueprintService)
{
}

FString FSetBlueprintParentClassCommand::Execute(const FString& Parameters)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return MakeErr(TEXT("Invalid JSON parameters"));
    }

    FString BlueprintName;
    FString NewParentClass;
    if (!JsonObject->TryGetStringField(TEXT("blueprint_name"), BlueprintName) || BlueprintName.IsEmpty())
    {
        return MakeErr(TEXT("Missing 'blueprint_name' parameter"));
    }
    if (!JsonObject->TryGetStringField(TEXT("new_parent_class"), NewParentClass) || NewParentClass.IsEmpty())
    {
        return MakeErr(TEXT("Missing 'new_parent_class' parameter"));
    }

    UBlueprint* Blueprint = BlueprintService.FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return MakeErr(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    UClass* OldParent = Blueprint->ParentClass;
    const FString OldParentName = OldParent ? OldParent->GetName() : TEXT("None");

    UClass* NewParent = ResolveParentClass(NewParentClass);
    if (!NewParent)
    {
        return MakeErr(FString::Printf(TEXT("Could not resolve parent class: %s"), *NewParentClass));
    }

    if (Blueprint->ParentClass == NewParent)
    {
        TSharedPtr<FJsonObject> NoOp = MakeShared<FJsonObject>();
        NoOp->SetBoolField(TEXT("success"), true);
        NoOp->SetBoolField(TEXT("changed"), false);
        NoOp->SetStringField(TEXT("blueprint_name"), BlueprintName);
        NoOp->SetStringField(TEXT("parent_class"), NewParent->GetPathName());
        NoOp->SetStringField(TEXT("message"), TEXT("Already has requested parent class"));
        FString Out;
        TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
        FJsonSerializer::Serialize(NoOp.ToSharedRef(), W);
        return Out;
    }

    Blueprint->Modify();
    Blueprint->ParentClass = NewParent;

    // Recompile so the generated class hierarchy updates correctly.
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    Blueprint->MarkPackageDirty();

    UE_LOG(LogTemp, Log, TEXT("SetBlueprintParentClass: '%s' reparented from '%s' to '%s'"),
        *BlueprintName, *OldParentName, *NewParent->GetName());

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetBoolField(TEXT("changed"), true);
    Result->SetStringField(TEXT("blueprint_name"), BlueprintName);
    Result->SetStringField(TEXT("old_parent_class"), OldParentName);
    Result->SetStringField(TEXT("new_parent_class"), NewParent->GetPathName());

    FString Out;
    TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
    FJsonSerializer::Serialize(Result.ToSharedRef(), W);
    return Out;
}

FString FSetBlueprintParentClassCommand::GetCommandName() const
{
    return TEXT("set_blueprint_parent_class");
}

bool FSetBlueprintParentClassCommand::ValidateParams(const FString& Parameters) const
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid()) return false;
    FString BlueprintName, NewParentClass;
    return JsonObject->TryGetStringField(TEXT("blueprint_name"), BlueprintName) && !BlueprintName.IsEmpty()
        && JsonObject->TryGetStringField(TEXT("new_parent_class"), NewParentClass) && !NewParentClass.IsEmpty();
}
