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

    // Allow Blueprint paths, Class paths, and direct asset paths
    if (Type.StartsWith(TEXT("Blueprint:")) ||
        Type.StartsWith(TEXT("Class:")) ||
        Type.StartsWith(TEXT("/Game/")) ||
        Type.StartsWith(TEXT("/Script/")))
    {
        return true;
    }

    // Validate supported friendly name actor types
    static const TSet<FString> SupportedTypes = {
        // Basic Actors
        TEXT("StaticMeshActor"),
        TEXT("PointLight"),
        TEXT("SpotLight"),
        TEXT("DirectionalLight"),
        TEXT("CameraActor"),
        // Volumes/BSP
        TEXT("TriggerBox"),
        TEXT("TriggerSphere"),
        TEXT("TriggerCapsule"),
        TEXT("BlockingVolume"),
        TEXT("NavMeshBoundsVolume"),
        TEXT("PhysicsVolume"),
        TEXT("AudioVolume"),
        TEXT("PostProcessVolume"),
        TEXT("LightmassImportanceVolume"),
        TEXT("KillZVolume"),
        TEXT("PainCausingVolume"),
        // Utility Actors
        TEXT("TextRenderActor"),
        TEXT("PlayerStart"),
        TEXT("TargetPoint"),
        TEXT("DecalActor"),
        TEXT("Note"),
        TEXT("ExponentialHeightFog"),
        TEXT("SkyLight"),
        TEXT("SphereReflectionCapture"),
        TEXT("BoxReflectionCapture"),
        // Special Types
        TEXT("InvisibleWall")
    };

    if (!SupportedTypes.Contains(Type))
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
