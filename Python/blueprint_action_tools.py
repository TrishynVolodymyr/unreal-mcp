from utils.blueprint_actions import blueprint_action_operations

def get_real_actions_for_pin(pin_type: str, pin_subcategory: str = "") -> dict:
    """
    Get actual available Blueprint actions from FBlueprintActionDatabase for a specific pin type.
    Uses Unreal's actual FBlueprintActionDatabase to discover real actions that are 
    available for a specific pin type, providing authentic data directly from the engine's 
    action database instead of hardcoded examples.
    """
    return blueprint_action_operations.get_real_actions_for_pin(None, pin_type, pin_subcategory)

def get_complete_actions_for_pin(pin_type: str, pin_subcategory: str = "") -> dict:
    """
    Get complete available Blueprint actions using FBlueprintActionMenuBuilder for a specific pin type.
    Uses the same action menu builder system that Unreal Engine uses natively, including math 
    operators and all other actions that appear in Unreal's native search/action menu.
    This should include actions like "Add" that don't appear in the standard FBlueprintActionDatabase.
    """
    return blueprint_action_operations.get_complete_actions_for_pin(None, pin_type, pin_subcategory)

def create_node_from_action(blueprint_name: str, action_title: str, pin_type: str, pin_subcategory: str = "", node_position: str = "") -> dict:
    """
    Create a Blueprint node from an action discovered in the FBlueprintActionDatabase.
    This takes an action that was discovered and creates the corresponding node.
    """
    return blueprint_action_operations.create_node_from_action(None, blueprint_name, action_title, pin_type, pin_subcategory, node_position) 