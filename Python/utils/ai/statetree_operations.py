"""
StateTree Operations for Unreal MCP.

This module provides utility functions for generating StateTree evaluator and task
C++ source files. StateTree evaluators and tasks are USTRUCT types that cannot be
created as Blueprints - they require C++ source code generation.

After generating files, the project must be recompiled for changes to take effect.
"""

import os
import logging
from typing import Dict, List, Any
from datetime import datetime

# Get logger
logger = logging.getLogger("UnrealMCP")

# Import dynamic module name retrieval
try:
    from utils.unreal_connection_utils import get_project_module_name
except ImportError:
    # Fallback if import fails
    def get_project_module_name():
        return "MyGame"


def _sanitize_name(name: str) -> str:
    """Sanitize a name for use as a C++ identifier."""
    # Remove invalid characters, keep alphanumeric and underscores
    result = ''.join(c if c.isalnum() or c == '_' else '' for c in name)
    # Ensure doesn't start with a number
    if result and result[0].isdigit():
        result = '_' + result
    return result


def _get_cpp_type(type_str: str) -> str:
    """Convert a simplified type string to proper C++ type."""
    type_map = {
        "bool": "bool",
        "boolean": "bool",
        "int": "int32",
        "int32": "int32",
        "int64": "int64",
        "float": "float",
        "double": "double",
        "string": "FString",
        "fstring": "FString",
        "name": "FName",
        "fname": "FName",
        "text": "FText",
        "ftext": "FText",
        "vector": "FVector",
        "fvector": "FVector",
        "rotator": "FRotator",
        "frotator": "FRotator",
        "transform": "FTransform",
        "ftransform": "FTransform",
        "actor": "TObjectPtr<AActor>",
        "aactor": "TObjectPtr<AActor>",
        "pawn": "TObjectPtr<APawn>",
        "apawn": "TObjectPtr<APawn>",
        "gameplaytag": "FGameplayTag",
        "fgameplaytag": "FGameplayTag",
        "gameplaytagcontainer": "FGameplayTagContainer",
        "fgameplaytagcontainer": "FGameplayTagContainer",
    }

    lower_type = type_str.lower().strip()
    if lower_type in type_map:
        return type_map[lower_type]

    # If not in map, assume it's already a proper C++ type
    return type_str


def _get_default_value(cpp_type: str) -> str:
    """Get a sensible default value for a C++ type."""
    defaults = {
        "bool": "false",
        "int32": "0",
        "int64": "0",
        "float": "0.0f",
        "double": "0.0",
        "FString": "TEXT(\"\")",
        "FName": "NAME_None",
        "FText": "FText::GetEmpty()",
        "FVector": "FVector::ZeroVector",
        "FRotator": "FRotator::ZeroRotator",
        "FTransform": "FTransform::Identity",
        "FGameplayTag": "FGameplayTag()",
        "FGameplayTagContainer": "FGameplayTagContainer()",
    }

    if cpp_type in defaults:
        return defaults[cpp_type]

    # Object pointers default to nullptr
    if cpp_type.startswith("TObjectPtr<") or cpp_type.endswith("*"):
        return "nullptr"

    return ""  # No default


def create_statetree_evaluator(
    name: str,
    project_source_path: str,
    module_name: str = None,
    evaluation_type: str = "bool",
    input_properties: List[Dict[str, Any]] = None,
    output_name: str = None,
    output_default: str = None,
    category: str = "AI",
    display_name: str = None,
    tick_interval: str = "EveryTick",
    description: str = None
) -> Dict[str, Any]:
    """
    Generate C++ source files for a StateTree evaluator.

    StateTree evaluators are USTRUCT types that inherit from FStateTreeEvaluatorBase.
    They evaluate conditions each tick and provide output values for StateTree logic.

    Args:
        name: Evaluator name without prefix (e.g., "AlliesNearby" -> creates "FEval_AlliesNearby")
        project_source_path: Full path to project Source directory
        module_name: C++ module name (auto-detected from Unreal project if not provided)
        evaluation_type: Type of output value: "bool", "int", "float", "enum", "tag", "vector"
        input_properties: Input properties list:
            [{"name": "SearchRadius", "type": "float", "default": "1000.0f", "category": "Input"}]
        output_name: Custom output property name (default: based on evaluation_type)
        output_default: Custom default value for output
        category: Display category in StateTree editor
        display_name: Human-readable name in editor (default: derives from name)
        tick_interval: "EveryTick", "EachFrame", or "Manual"
        description: Optional description comment for the class

    Returns:
        Dictionary with success status, created file paths, and any warnings

    Example:
        create_statetree_evaluator(
            name="TargetInRange",
            project_source_path="C:/Project/Source",
            evaluation_type="bool",
            input_properties=[
                {"name": "Target", "type": "Actor", "category": "Input"},
                {"name": "Range", "type": "float", "default": "500.0f", "category": "Parameter"}
            ]
        )
    """
    result = {
        "success": False,
        "header_path": "",
        "source_path": "",
        "class_name": "",
        "warnings": [],
        "message": ""
    }

    try:
        # Auto-detect module name if not provided
        if module_name is None:
            module_name = get_project_module_name()
            logger.info(f"Auto-detected module name: {module_name}")

        # Sanitize and format name
        clean_name = _sanitize_name(name)
        if not clean_name:
            result["message"] = "Invalid evaluator name"
            return result

        # Add prefix if not present
        if not clean_name.startswith("Eval_"):
            clean_name = f"Eval_{clean_name}"

        struct_name = f"F{clean_name}"
        result["class_name"] = struct_name

        # Determine output type and name
        output_type_map = {
            "bool": ("bool", "bResult", "false"),
            "boolean": ("bool", "bResult", "false"),
            "int": ("int32", "ResultValue", "0"),
            "int32": ("int32", "ResultValue", "0"),
            "float": ("float", "ResultValue", "0.0f"),
            "enum": ("uint8", "ResultEnum", "0"),
            "tag": ("FGameplayTag", "ResultTag", "FGameplayTag()"),
            "vector": ("FVector", "ResultLocation", "FVector::ZeroVector"),
        }

        output_cpp_type, default_output_name, default_output_value = output_type_map.get(
            evaluation_type.lower(),
            ("bool", "bResult", "false")
        )

        if output_name:
            final_output_name = _sanitize_name(output_name)
        else:
            final_output_name = default_output_name

        if output_default:
            final_output_default = output_default
        else:
            final_output_default = default_output_value

        # Format display name
        if not display_name:
            # Convert "Eval_SomeName" to "Evaluate Some Name"
            display_name = clean_name.replace("Eval_", "Evaluate ").replace("_", " ")

        # Build include list
        includes = [
            "CoreMinimal.h",
            "StateTreeEvaluatorBase.h"
        ]

        # Check if we need GameplayTag includes
        input_properties = input_properties or []
        needs_gameplay_tags = False
        for prop in input_properties:
            prop_type = prop.get("type", "").lower()
            if "gameplaytag" in prop_type or "tag" in prop_type:
                needs_gameplay_tags = True
                break
        if "tag" in evaluation_type.lower():
            needs_gameplay_tags = True

        if needs_gameplay_tags:
            includes.append("GameplayTagContainer.h")

        # Generate header file content
        header_lines = []

        # Copyright and description
        header_lines.append("// Copyright Your Company")
        header_lines.append("//")
        header_lines.append(f"// {struct_name}")
        if description:
            header_lines.append("//")
            for line in description.split("\n"):
                header_lines.append(f"// {line}")
        header_lines.append("//")
        header_lines.append(f"// Generated by Claude Code MCP Tools - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        header_lines.append("")
        header_lines.append("#pragma once")
        header_lines.append("")

        # Includes
        for inc in includes:
            header_lines.append(f'#include "{inc}"')
        header_lines.append(f'#include "{clean_name}.generated.h"')
        header_lines.append("")

        # USTRUCT declaration
        header_lines.append(f'USTRUCT(BlueprintType, meta = (DisplayName = "{display_name}", Category = "{category}", StateTreeEvaluator))')
        api_macro = module_name.upper() + "_API"
        header_lines.append(f"struct {api_macro} {struct_name} : public FStateTreeEvaluatorBase")
        header_lines.append("{")
        header_lines.append("\tGENERATED_BODY()")
        header_lines.append("")

        # Input properties
        if input_properties:
            header_lines.append("\t// =========================================================================")
            header_lines.append("\t// INPUT/PARAMETER PROPERTIES")
            header_lines.append("\t// =========================================================================")
            header_lines.append("")

            for prop in input_properties:
                prop_name = _sanitize_name(prop.get("name", "Property"))
                prop_type = _get_cpp_type(prop.get("type", "float"))
                prop_default = prop.get("default", _get_default_value(prop_type))
                prop_category = prop.get("category", "Input")
                prop_tooltip = prop.get("tooltip", "")

                meta_parts = []
                if prop_tooltip:
                    meta_parts.append(f'ToolTip = "{prop_tooltip}"')

                meta_str = ""
                if meta_parts:
                    meta_str = f', meta = ({", ".join(meta_parts)})'

                header_lines.append(f'\tUPROPERTY(EditAnywhere, Category = "{prop_category}"{meta_str})')
                if prop_default:
                    header_lines.append(f"\t{prop_type} {prop_name} = {prop_default};")
                else:
                    header_lines.append(f"\t{prop_type} {prop_name};")
                header_lines.append("")

        # Output property
        header_lines.append("\t// =========================================================================")
        header_lines.append("\t// OUTPUT PROPERTY")
        header_lines.append("\t// =========================================================================")
        header_lines.append("")
        header_lines.append('\tUPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")')
        header_lines.append(f"\t{output_cpp_type} {final_output_name} = {final_output_default};")
        header_lines.append("")

        # Override method
        header_lines.append("\t// =========================================================================")
        header_lines.append("\t// OVERRIDES")
        header_lines.append("\t// =========================================================================")
        header_lines.append("")
        header_lines.append("\tvirtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;")
        header_lines.append("};")
        header_lines.append("")

        # Generate source file content
        source_lines = []
        source_lines.append("// Copyright Your Company")
        source_lines.append("")
        source_lines.append(f'#include "AI/Evaluators/{clean_name}.h"')
        source_lines.append('#include "StateTreeExecutionContext.h"')
        source_lines.append("")
        source_lines.append(f"void {struct_name}::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const")
        source_lines.append("{")
        source_lines.append("\t// Get the owner actor")
        source_lines.append("\tAActor* OwnerActor = Cast<AActor>(Context.GetOwner());")
        source_lines.append("\tif (!OwnerActor)")
        source_lines.append("\t{")
        source_lines.append("\t\treturn;")
        source_lines.append("\t}")
        source_lines.append("")
        source_lines.append("\t// TODO: Implement evaluation logic here")
        source_lines.append(f"\t// Set {final_output_name} based on your conditions")
        source_lines.append(f"\t// const_cast<{struct_name}*>(this)->{final_output_name} = /* your logic */;")
        source_lines.append("")
        source_lines.append(f"\t// Example (remove and replace with actual logic):")
        source_lines.append(f"\tconst_cast<{struct_name}*>(this)->{final_output_name} = {final_output_default};")
        source_lines.append("}")
        source_lines.append("")

        # Write files
        header_dir = os.path.join(project_source_path, module_name, "Public", "AI", "Evaluators")
        source_dir = os.path.join(project_source_path, module_name, "Private", "AI", "Evaluators")

        os.makedirs(header_dir, exist_ok=True)
        os.makedirs(source_dir, exist_ok=True)

        header_path = os.path.join(header_dir, f"{clean_name}.h")
        source_path = os.path.join(source_dir, f"{clean_name}.cpp")

        # Check if files already exist
        if os.path.exists(header_path):
            result["warnings"].append(f"Header file already exists: {header_path}")
            result["message"] = "File already exists. Use overwrite=True to replace."
            return result

        if os.path.exists(source_path):
            result["warnings"].append(f"Source file already exists: {source_path}")
            result["message"] = "File already exists. Use overwrite=True to replace."
            return result

        # Write files
        with open(header_path, 'w', encoding='utf-8-sig') as f:
            f.write('\n'.join(header_lines))

        with open(source_path, 'w', encoding='utf-8-sig') as f:
            f.write('\n'.join(source_lines))

        result["success"] = True
        result["header_path"] = header_path
        result["source_path"] = source_path
        result["message"] = f"Successfully created StateTree evaluator {struct_name}. Rebuild project to compile."

        logger.info(f"Created StateTree evaluator: {struct_name}")
        logger.info(f"  Header: {header_path}")
        logger.info(f"  Source: {source_path}")

    except Exception as e:
        result["message"] = f"Error creating evaluator: {str(e)}"
        logger.error(f"Failed to create StateTree evaluator: {e}")

    return result


def create_statetree_task(
    name: str,
    project_source_path: str,
    module_name: str = None,
    task_type: str = "instant",
    input_properties: List[Dict[str, Any]] = None,
    output_properties: List[Dict[str, Any]] = None,
    parameter_properties: List[Dict[str, Any]] = None,
    category: str = "AI",
    display_name: str = None,
    description: str = None
) -> Dict[str, Any]:
    """
    Generate C++ source files for a StateTree task.

    StateTree tasks are USTRUCT types that inherit from FStateTreeTaskBase.
    They perform actions and can have input/output/parameter properties.

    Args:
        name: Task name (e.g., "SelectTarget" -> creates "FSelectTargetTask")
        project_source_path: Full path to project Source directory
        module_name: C++ module name (auto-detected from Unreal project if not provided)
        task_type: Execution type:
            - "instant": Completes immediately in EnterState
            - "latent": Runs over time with Tick
        input_properties: Input bindings from StateTree:
            [{"name": "Target", "type": "Actor"}]
        output_properties: Output bindings to StateTree:
            [{"name": "SelectedPosition", "type": "Vector"}]
        parameter_properties: Configuration parameters:
            [{"name": "Range", "type": "float", "default": "500.0f"}]
        category: Display category
        display_name: Human-readable name (derives from name if not set)
        description: Class description comment

    Returns:
        Dictionary with success status, created file paths, and warnings

    Example:
        create_statetree_task(
            name="SelectTacticalPosition",
            project_source_path="C:/Project/Source",
            task_type="instant",
            input_properties=[
                {"name": "ThreatActor", "type": "Actor"}
            ],
            parameter_properties=[
                {"name": "SearchRadius", "type": "float", "default": "1000.0f"}
            ],
            output_properties=[
                {"name": "SelectedPosition", "type": "Vector"},
                {"name": "bFoundPosition", "type": "bool"}
            ]
        )
    """
    result = {
        "success": False,
        "header_path": "",
        "source_path": "",
        "class_name": "",
        "instance_data_class": "",
        "warnings": [],
        "message": ""
    }

    try:
        # Auto-detect module name if not provided
        if module_name is None:
            module_name = get_project_module_name()
            logger.info(f"Auto-detected module name: {module_name}")

        # Sanitize and format name
        clean_name = _sanitize_name(name)
        if not clean_name:
            result["message"] = "Invalid task name"
            return result

        # Add suffix if not present
        if not clean_name.endswith("Task"):
            clean_name = f"{clean_name}Task"

        struct_name = f"F{clean_name}"
        instance_data_name = f"F{clean_name}InstanceData"
        result["class_name"] = struct_name
        result["instance_data_class"] = instance_data_name

        # Format display name
        if not display_name:
            # Convert "SomeNameTask" to "Some Name"
            display_name = clean_name.replace("Task", "").replace("_", " ")
            # Add spaces before capitals
            spaced = ""
            for i, c in enumerate(display_name):
                if c.isupper() and i > 0 and display_name[i-1].islower():
                    spaced += " "
                spaced += c
            display_name = spaced.strip()

        # Normalize property lists
        input_properties = input_properties or []
        output_properties = output_properties or []
        parameter_properties = parameter_properties or []

        # Check if we need instance data (any inputs, outputs, or parameters)
        needs_instance_data = bool(input_properties or output_properties or parameter_properties)

        # Build include list
        includes = [
            "CoreMinimal.h",
            "StateTreeTaskBase.h",
            "StateTreeExecutionContext.h"
        ]

        # Check for gameplay tags
        all_props = input_properties + output_properties + parameter_properties
        needs_gameplay_tags = any(
            "gameplaytag" in prop.get("type", "").lower() or "tag" in prop.get("type", "").lower()
            for prop in all_props
        )
        if needs_gameplay_tags:
            includes.append("GameplayTagContainer.h")

        # Generate header file content
        header_lines = []

        # Copyright and description
        header_lines.append("// Copyright Your Company")
        header_lines.append("//")
        header_lines.append(f"// {struct_name}")
        if description:
            header_lines.append("//")
            for line in description.split("\n"):
                header_lines.append(f"// {line}")
        header_lines.append("//")
        header_lines.append(f"// Generated by Claude Code MCP Tools - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        header_lines.append("")
        header_lines.append("#pragma once")
        header_lines.append("")

        # Includes
        for inc in includes:
            header_lines.append(f'#include "{inc}"')
        header_lines.append(f'#include "{clean_name}.generated.h"')
        header_lines.append("")

        api_macro = module_name.upper() + "_API"

        # Instance data struct (if needed)
        if needs_instance_data:
            header_lines.append("/**")
            header_lines.append(f" * {instance_data_name}")
            header_lines.append(f" *")
            header_lines.append(f" * Instance data for {struct_name}.")
            header_lines.append(" */")
            header_lines.append("USTRUCT()")
            header_lines.append(f"struct {api_macro} {instance_data_name}")
            header_lines.append("{")
            header_lines.append("\tGENERATED_BODY()")
            header_lines.append("")

            # Input properties
            if input_properties:
                header_lines.append("\t// =========================================================================")
                header_lines.append("\t// INPUTS")
                header_lines.append("\t// =========================================================================")
                header_lines.append("")

                for prop in input_properties:
                    prop_name = _sanitize_name(prop.get("name", "Input"))
                    prop_type = _get_cpp_type(prop.get("type", "float"))
                    prop_default = prop.get("default", _get_default_value(prop_type))
                    prop_tooltip = prop.get("tooltip", "")

                    meta_parts = []
                    if prop_tooltip:
                        meta_parts.append(f'ToolTip = "{prop_tooltip}"')

                    meta_str = ""
                    if meta_parts:
                        meta_str = f', meta = ({", ".join(meta_parts)})'

                    header_lines.append(f'\tUPROPERTY(EditAnywhere, Category = "Input"{meta_str})')
                    if prop_default:
                        header_lines.append(f"\t{prop_type} {prop_name} = {prop_default};")
                    else:
                        header_lines.append(f"\t{prop_type} {prop_name};")
                    header_lines.append("")

            # Parameter properties
            if parameter_properties:
                header_lines.append("\t// =========================================================================")
                header_lines.append("\t// PARAMETERS")
                header_lines.append("\t// =========================================================================")
                header_lines.append("")

                for prop in parameter_properties:
                    prop_name = _sanitize_name(prop.get("name", "Param"))
                    prop_type = _get_cpp_type(prop.get("type", "float"))
                    prop_default = prop.get("default", _get_default_value(prop_type))
                    prop_tooltip = prop.get("tooltip", "")
                    prop_meta = prop.get("meta", "")

                    meta_parts = []
                    if prop_tooltip:
                        meta_parts.append(f'ToolTip = "{prop_tooltip}"')
                    if prop_meta:
                        meta_parts.append(prop_meta)

                    meta_str = ""
                    if meta_parts:
                        meta_str = f', meta = ({", ".join(meta_parts)})'

                    header_lines.append(f'\tUPROPERTY(EditAnywhere, Category = "Parameter"{meta_str})')
                    if prop_default:
                        header_lines.append(f"\t{prop_type} {prop_name} = {prop_default};")
                    else:
                        header_lines.append(f"\t{prop_type} {prop_name};")
                    header_lines.append("")

            # Output properties
            if output_properties:
                header_lines.append("\t// =========================================================================")
                header_lines.append("\t// OUTPUTS")
                header_lines.append("\t// =========================================================================")
                header_lines.append("")

                for prop in output_properties:
                    prop_name = _sanitize_name(prop.get("name", "Output"))
                    prop_type = _get_cpp_type(prop.get("type", "float"))
                    prop_default = prop.get("default", _get_default_value(prop_type))
                    prop_tooltip = prop.get("tooltip", "")

                    meta_parts = ["StateTreeInstanceOnly"]
                    if prop_tooltip:
                        meta_parts.append(f'ToolTip = "{prop_tooltip}"')

                    meta_str = f', meta = ({", ".join(meta_parts)})'

                    header_lines.append(f'\tUPROPERTY(EditAnywhere, Category = "Output"{meta_str})')
                    if prop_default:
                        header_lines.append(f"\t{prop_type} {prop_name} = {prop_default};")
                    else:
                        header_lines.append(f"\t{prop_type} {prop_name};")
                    header_lines.append("")

            header_lines.append("};")
            header_lines.append("")

        # Main task struct
        header_lines.append("/**")
        header_lines.append(f" * {struct_name}")
        header_lines.append(f" *")
        header_lines.append(f" * StateTree task: {display_name}")
        if description:
            header_lines.append(f" *")
            for line in description.split("\n"):
                header_lines.append(f" * {line}")
        header_lines.append(" */")
        header_lines.append(f'USTRUCT(BlueprintType, meta = (DisplayName = "{display_name}"))')
        header_lines.append(f"struct {api_macro} {struct_name} : public FStateTreeTaskBase")
        header_lines.append("{")
        header_lines.append("\tGENERATED_BODY()")
        header_lines.append("")

        if needs_instance_data:
            header_lines.append(f"\tusing FInstanceDataType = {instance_data_name};")
            header_lines.append("")

        header_lines.append(f"\t{struct_name}() = default;")
        header_lines.append("")

        # Override methods
        header_lines.append("\t// =========================================================================")
        header_lines.append("\t// OVERRIDES")
        header_lines.append("\t// =========================================================================")
        header_lines.append("")

        if needs_instance_data:
            header_lines.append(f"\tvirtual const UStruct* GetInstanceDataType() const override {{ return {instance_data_name}::StaticStruct(); }}")

        header_lines.append("\tvirtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;")

        if task_type.lower() == "latent":
            header_lines.append("\tvirtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;")
            header_lines.append("\tvirtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;")

        header_lines.append("};")
        header_lines.append("")

        # Generate source file content
        source_lines = []
        source_lines.append("// Copyright Your Company")
        source_lines.append("")
        source_lines.append(f'#include "AI/Tasks/{clean_name}.h"')
        source_lines.append('#include "StateTreeExecutionContext.h"')
        source_lines.append('#include "AIController.h"')
        source_lines.append("")

        # EnterState implementation
        source_lines.append(f"EStateTreeRunStatus {struct_name}::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const")
        source_lines.append("{")

        if needs_instance_data:
            source_lines.append("\tFInstanceDataType& Data = Context.GetInstanceData(*this);")
            source_lines.append("")

        source_lines.append("\t// Get owner (AIController or Actor)")
        source_lines.append("\tAActor* OwnerActor = nullptr;")
        source_lines.append("\tif (AAIController* Controller = Cast<AAIController>(Context.GetOwner()))")
        source_lines.append("\t{")
        source_lines.append("\t\tOwnerActor = Controller->GetPawn();")
        source_lines.append("\t}")
        source_lines.append("\telse")
        source_lines.append("\t{")
        source_lines.append("\t\tOwnerActor = Cast<AActor>(Context.GetOwner());")
        source_lines.append("\t}")
        source_lines.append("")
        source_lines.append("\tif (!OwnerActor)")
        source_lines.append("\t{")
        source_lines.append("\t\treturn EStateTreeRunStatus::Failed;")
        source_lines.append("\t}")
        source_lines.append("")
        source_lines.append("\t// TODO: Implement task logic here")

        if task_type.lower() == "latent":
            source_lines.append("\t// Return Running for latent tasks that need Tick")
            source_lines.append("\treturn EStateTreeRunStatus::Running;")
        else:
            source_lines.append("\t// Return Succeeded or Failed based on task completion")
            source_lines.append("\treturn EStateTreeRunStatus::Succeeded;")

        source_lines.append("}")
        source_lines.append("")

        # Tick implementation for latent tasks
        if task_type.lower() == "latent":
            source_lines.append(f"EStateTreeRunStatus {struct_name}::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const")
            source_lines.append("{")
            if needs_instance_data:
                source_lines.append("\tFInstanceDataType& Data = Context.GetInstanceData(*this);")
                source_lines.append("")
            source_lines.append("\t// TODO: Implement tick logic for latent task")
            source_lines.append("\t// Return Running to continue, Succeeded/Failed to complete")
            source_lines.append("")
            source_lines.append("\treturn EStateTreeRunStatus::Running;")
            source_lines.append("}")
            source_lines.append("")

            source_lines.append(f"void {struct_name}::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const")
            source_lines.append("{")
            if needs_instance_data:
                source_lines.append("\tFInstanceDataType& Data = Context.GetInstanceData(*this);")
                source_lines.append("")
            source_lines.append("\t// TODO: Cleanup when task exits")
            source_lines.append("}")
            source_lines.append("")

        # Write files
        header_dir = os.path.join(project_source_path, module_name, "Public", "AI", "Tasks")
        source_dir = os.path.join(project_source_path, module_name, "Private", "AI", "Tasks")

        os.makedirs(header_dir, exist_ok=True)
        os.makedirs(source_dir, exist_ok=True)

        header_path = os.path.join(header_dir, f"{clean_name}.h")
        source_path = os.path.join(source_dir, f"{clean_name}.cpp")

        # Check if files already exist
        if os.path.exists(header_path):
            result["warnings"].append(f"Header file already exists: {header_path}")
            result["message"] = "File already exists. Cannot overwrite."
            return result

        if os.path.exists(source_path):
            result["warnings"].append(f"Source file already exists: {source_path}")
            result["message"] = "File already exists. Cannot overwrite."
            return result

        # Write files
        with open(header_path, 'w', encoding='utf-8-sig') as f:
            f.write('\n'.join(header_lines))

        with open(source_path, 'w', encoding='utf-8-sig') as f:
            f.write('\n'.join(source_lines))

        result["success"] = True
        result["header_path"] = header_path
        result["source_path"] = source_path
        result["message"] = f"Successfully created StateTree task {struct_name}. Rebuild project to compile."

        logger.info(f"Created StateTree task: {struct_name}")
        logger.info(f"  Header: {header_path}")
        logger.info(f"  Source: {source_path}")

    except Exception as e:
        result["message"] = f"Error creating task: {str(e)}"
        logger.error(f"Failed to create StateTree task: {e}")

    return result
