// Copyright Epic Games, Inc. All Rights Reserved.

#include "NodeCreationHelpers.h"
#include "CoreMinimal.h"

namespace NodeCreationHelpers
{
    FString ConvertPropertyNameToDisplay(const FString& InPropName)
    {
        FString Name = InPropName;
        // Strip leading 'b' for bool properties
        if (Name.StartsWith(TEXT("b")) && Name.Len() > 1 && FChar::IsUpper(Name[1]))
        {
            Name = Name.RightChop(1);
        }

        FString Out;
        Out.Reserve(Name.Len()*2);
        for (int32 Index = 0; Index < Name.Len(); ++Index)
        {
            const TCHAR Ch = Name[Index];
            if (Index > 0 && FChar::IsUpper(Ch) && !FChar::IsUpper(Name[Index-1]))
            {
                Out += TEXT(" ");
            }
            Out.AppendChar(Ch);
        }
        return Out;
    }

    FString ConvertCamelCaseToTitleCase(const FString& InFunctionName)
    {
        if (InFunctionName.IsEmpty())
        {
            return InFunctionName;
        }

        FString Out;
        Out.Reserve(InFunctionName.Len() * 2);
        
        for (int32 Index = 0; Index < InFunctionName.Len(); ++Index)
        {
            const TCHAR Ch = InFunctionName[Index];
            
            // Add space before uppercase letters (except the first character)
            if (Index > 0 && FChar::IsUpper(Ch) && !FChar::IsUpper(InFunctionName[Index-1]))
            {
                // Don't add space if the previous character was already a space
                if (Out.Len() > 0 && Out[Out.Len()-1] != TEXT(' '))
                {
                    Out += TEXT(" ");
                }
            }
            
            Out.AppendChar(Ch);
        }
        
        return Out;
    }
}  // namespace NodeCreationHelpers