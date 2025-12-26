#pragma once

#include "CoreMinimal.h"

// Forward declarations
class UEdGraph;
class UEdGraphPin;
struct FEdGraphPinType;
struct FAutoInsertedNodeInfo;

/**
 * Service for creating type conversion/cast nodes between incompatible pins.
 * Handles automatic insertion of conversion nodes for primitive types (Int, Float, Bool, String)
 * and dynamic cast nodes for object types.
 *
 * Extracted from BlueprintNodeConnectionService for better code organization.
 */
class UNREALMCP_API FBlueprintCastNodeService
{
public:
    /**
     * Get the singleton instance
     */
    static FBlueprintCastNodeService& Get();

    /**
     * Check if two pin types are compatible or need a cast
     * @param SourcePinType - Type of the source pin
     * @param TargetPinType - Type of the target pin
     * @return true if types are directly compatible (no cast needed)
     */
    bool ArePinTypesCompatible(const FEdGraphPinType& SourcePinType, const FEdGraphPinType& TargetPinType);

    /**
     * Determine if a cast is needed between two pins
     * @param SourcePin - Source pin (output)
     * @param TargetPin - Target pin (input)
     * @return true if a cast node needs to be inserted
     */
    bool DoesCastNeed(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

    /**
     * Create the appropriate cast node between two incompatible pins
     * Automatically determines the correct conversion type based on pin categories.
     * @param Graph - The graph to create the node in
     * @param SourcePin - Source pin (output)
     * @param TargetPin - Target pin (input)
     * @return true if cast node was successfully created and connected
     */
    bool CreateCastNode(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

    /**
     * Create an Integer to String conversion node
     */
    bool CreateIntToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

    /**
     * Create a Float to String conversion node
     */
    bool CreateFloatToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

    /**
     * Create a Boolean to String conversion node
     */
    bool CreateBoolToStringCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

    /**
     * Create a String to Integer conversion node
     */
    bool CreateStringToIntCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

    /**
     * Create a String to Float conversion node
     */
    bool CreateStringToFloatCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

    /**
     * Create an Object to Object dynamic cast node
     * @param Graph - The graph to create the node in
     * @param SourcePin - Source pin (output)
     * @param TargetPin - Target pin (input)
     * @param OutNodeInfo - Optional pointer to receive info about the created cast node
     * @return true if cast node was successfully created and connected
     */
    bool CreateObjectCast(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin,
                         FAutoInsertedNodeInfo* OutNodeInfo = nullptr);

private:
    /** Private constructor for singleton pattern */
    FBlueprintCastNodeService() = default;

    /**
     * Helper to create a conversion function call node
     * @param Graph - The graph to create the node in
     * @param SourcePin - Source pin to connect from
     * @param TargetPin - Target pin to connect to
     * @param LibraryClassName - Name of the library class containing the function
     * @param FunctionName - Name of the conversion function
     * @param InputPinName - Name of the input pin on the conversion node
     * @return true if conversion node was successfully created and connected
     */
    bool CreateConversionNode(UEdGraph* Graph, UEdGraphPin* SourcePin, UEdGraphPin* TargetPin,
                             const TCHAR* LibraryClassName, const TCHAR* FunctionName, const TCHAR* InputPinName);
};
