#include "Utils/GeometryUtils.h"

bool FGeometryUtils::ParseVector(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FVector& OutVector)
{
    if (JsonArray.Num() == 3)
    {
        double X, Y, Z;
        if (JsonArray[0]->TryGetNumber(X) && JsonArray[1]->TryGetNumber(Y) && JsonArray[2]->TryGetNumber(Z))
        {
            OutVector.X = X;
            OutVector.Y = Y;
            OutVector.Z = Z;
            return true;
        }
    }
    return false;
}

// Example implementation for ParseLinearColor (adjust as needed)
bool FGeometryUtils::ParseLinearColor(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FLinearColor& OutColor)
{
    UE_LOG(LogTemp, Log, TEXT("ParseLinearColor - Array has %d elements"), JsonArray.Num());

    if (JsonArray.Num() < 3)
    {
        UE_LOG(LogTemp, Warning, TEXT("ParseLinearColor - Array has insufficient elements: %d (need at least 3)"), JsonArray.Num());
        return false;
    }

    // Check each element to ensure they are numbers
    for (int32 i = 0; i < JsonArray.Num() && i < 4; ++i)
    {
        if (JsonArray[i]->Type != EJson::Number)
        {
            UE_LOG(LogTemp, Warning, TEXT("ParseLinearColor - Element %d is not a number (type: %d)"), i, (int)JsonArray[i]->Type);
            return false;
        }
    }

    // Extract RGB values
    float R = JsonArray[0]->AsNumber();
    float G = JsonArray[1]->AsNumber();
    float B = JsonArray[2]->AsNumber();

    // Extract Alpha if available, otherwise default to 1.0
    float A = 1.0f;
    if (JsonArray.Num() >= 4)
    {
        A = JsonArray[3]->AsNumber();
    }

    UE_LOG(LogTemp, Log, TEXT("ParseLinearColor - Parsed color: R=%f, G=%f, B=%f, A=%f"), R, G, B, A);

    // Set the output color
    OutColor = FLinearColor(R, G, B, A);
    return true;
}

// Placeholder for ParseRotator if needed
bool FGeometryUtils::ParseRotator(const TArray<TSharedPtr<FJsonValue>>& JsonArray, FRotator& OutRotator)
{
    if (JsonArray.Num() == 3)
    {
        double P, Y, R;
        if (JsonArray[0]->TryGetNumber(P) && JsonArray[1]->TryGetNumber(Y) && JsonArray[2]->TryGetNumber(R))
        {
            OutRotator.Pitch = P;
            OutRotator.Yaw = Y;
            OutRotator.Roll = R;
            return true;
        }
    }
    return false;
}
