#include "Services/IBlueprintService.h"

bool FBlueprintCreationParams::IsValid(FString& OutError) const
{
    if (Name.IsEmpty())
    {
        OutError = TEXT("Blueprint name cannot be empty");
        return false;
    }
    
    // Check for invalid characters in blueprint name
    if (Name.Contains(TEXT(" ")) || Name.Contains(TEXT(".")))
    {
        OutError = TEXT("Blueprint name cannot contain spaces or dots");
        return false;
    }
    
    // Validate folder path if provided
    if (!FolderPath.IsEmpty())
    {
        // Check for invalid path characters (allow forward slashes for /Game/ paths)
        if (FolderPath.Contains(TEXT("\\")))
        {
            OutError = TEXT("Invalid folder path format - backslashes not allowed");
            return false;
        }
    }
    
    return true;
}
