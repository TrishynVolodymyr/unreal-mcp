#pragma once

#include "CoreMinimal.h"
#include "Commands/IUnrealMCPCommand.h"
#include "Services/IBlueprintActionService.h"

/**
 * Search types for find_in_blueprints command
 */
enum class EBlueprintSearchType : uint8
{
	All,        // Search all node types
	Function,   // Search function calls only
	Variable,   // Search variable get/set nodes only
	Event,      // Search event nodes only
	Comment,    // Search comment nodes only
	Custom      // Search custom events only
};

/**
 * Result of a blueprint search match
 */
struct FBlueprintSearchMatch
{
	FString BlueprintPath;    // Full path to the blueprint
	FString BlueprintName;    // Name of the blueprint
	FString GraphName;        // Name of the graph containing the match
	FString NodeId;           // Unique node ID
	FString NodeTitle;        // Node's display title
	FString NodeClass;        // Node class name
	FString MatchContext;     // Additional context about the match
};

/**
 * Command to search for function/variable/event usages across all blueprints
 * Similar to Unreal's "Find in Blueprints" feature
 */
class UNREALMCP_API FFindInBlueprintsCommand : public IUnrealMCPCommand
{
public:
	/**
	 * Constructor
	 * @param InBlueprintActionService - Service for blueprint action operations
	 */
	explicit FFindInBlueprintsCommand(TSharedPtr<IBlueprintActionService> InBlueprintActionService);

	// IUnrealMCPCommand interface
	virtual FString Execute(const FString& Parameters) override;
	virtual FString GetCommandName() const override;
	virtual bool ValidateParams(const FString& Parameters) const override;

private:
	/** Service for blueprint action operations */
	TSharedPtr<IBlueprintActionService> BlueprintActionService;

	/**
	 * Parse parameters from JSON string
	 * @param Parameters - JSON string containing parameters
	 * @param OutSearchQuery - Parsed search query
	 * @param OutSearchType - Parsed search type
	 * @param OutPath - Parsed search path
	 * @param OutMaxResults - Parsed max results
	 * @param OutCaseSensitive - Whether search is case sensitive
	 * @param OutError - Error message if parsing fails
	 * @return true if parsing succeeded
	 */
	bool ParseParameters(
		const FString& Parameters,
		FString& OutSearchQuery,
		EBlueprintSearchType& OutSearchType,
		FString& OutPath,
		int32& OutMaxResults,
		bool& OutCaseSensitive,
		FString& OutError
	) const;

	/**
	 * Parse search type string to enum
	 */
	EBlueprintSearchType ParseSearchType(const FString& TypeString) const;

	/**
	 * Search a single blueprint for matches
	 */
	void SearchBlueprint(
		class UBlueprint* Blueprint,
		const FString& SearchQuery,
		EBlueprintSearchType SearchType,
		bool bCaseSensitive,
		TArray<FBlueprintSearchMatch>& OutMatches,
		int32 MaxResults
	) const;

	/**
	 * Check if a node matches the search criteria
	 */
	bool MatchesSearchCriteria(
		class UEdGraphNode* Node,
		const FString& SearchQuery,
		EBlueprintSearchType SearchType,
		bool bCaseSensitive,
		FString& OutMatchContext
	) const;

	/**
	 * Check if node type matches the filter
	 */
	bool MatchesNodeTypeFilter(class UEdGraphNode* Node, EBlueprintSearchType SearchType) const;

	/**
	 * Create an error response JSON string
	 * @param ErrorMessage - Error message
	 * @return JSON string representing error response
	 */
	FString CreateErrorResponse(const FString& ErrorMessage) const;

	/**
	 * Create a success response with search results
	 */
	FString CreateSuccessResponse(
		const TArray<FBlueprintSearchMatch>& Matches,
		const FString& SearchQuery,
		const FString& SearchType,
		int32 BlueprintsSearched
	) const;
};
