// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Helper functions for node creation in Blueprint graphs.
 * Contains utility functions for text conversion and formatting.
 */
namespace NodeCreationHelpers
{
    /**
     * Convert property names into friendly display strings.
     * Example: "bIsActive" -> "Is Active"
     */
    FString ConvertPropertyNameToDisplay(const FString& InPropName);

    /**
     * Convert CamelCase function names to Title Case.
     * Example: "GetActorLocation" -> "Get Actor Location"
     */
    FString ConvertCamelCaseToTitleCase(const FString& InFunctionName);
}
