#include "Commands/Blueprint/ComponentCreationParams.h"

bool FComponentCreationParams::IsValid(FString& OutError) const
{
    if (ComponentName.IsEmpty())
    {
        OutError = TEXT("Component name cannot be empty");
        return false;
    }
    
    if (ComponentType.IsEmpty())
    {
        OutError = TEXT("Component type cannot be empty");
        return false;
    }
    
    // Validate location array
    if (Location.Num() != 3)
    {
        OutError = TEXT("Location must have exactly 3 values (X, Y, Z)");
        return false;
    }
    
    // Validate rotation array
    if (Rotation.Num() != 3)
    {
        OutError = TEXT("Rotation must have exactly 3 values (Pitch, Yaw, Roll)");
        return false;
    }
    
    // Validate scale array
    if (Scale.Num() != 3)
    {
        OutError = TEXT("Scale must have exactly 3 values (X, Y, Z)");
        return false;
    }
    
    // Check for valid scale values (should not be zero)
    for (float ScaleValue : Scale)
    {
        if (FMath::IsNearlyZero(ScaleValue))
        {
            OutError = TEXT("Scale values cannot be zero");
            return false;
        }
    }
    
    return true;
}



