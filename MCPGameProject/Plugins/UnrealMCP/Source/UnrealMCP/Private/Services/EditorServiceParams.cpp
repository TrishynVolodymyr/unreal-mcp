#include "Services/IEditorService.h"

bool FActorSpawnParams::IsValid(FString& OutError) const
{
    if (Name.IsEmpty())
    {
        OutError = TEXT("Actor name cannot be empty");
        return false;
    }
    
    if (Type.IsEmpty())
    {
        OutError = TEXT("Actor type cannot be empty");
        return false;
    }
    
    // Validate supported actor types
    if (Type != TEXT("StaticMeshActor") && 
        Type != TEXT("PointLight") && 
        Type != TEXT("SpotLight") && 
        Type != TEXT("DirectionalLight") && 
        Type != TEXT("CameraActor"))
    {
        OutError = FString::Printf(TEXT("Unsupported actor type: %s"), *Type);
        return false;
    }
    
    return true;
}

bool FBlueprintActorSpawnParams::IsValid(FString& OutError) const
{
    if (BlueprintName.IsEmpty())
    {
        OutError = TEXT("Blueprint name cannot be empty");
        return false;
    }
    
    if (ActorName.IsEmpty())
    {
        OutError = TEXT("Actor name cannot be empty");
        return false;
    }
    
    return true;
}
