from mcp.server.fastmcp import FastMCP
from node_tools.node_tools import register_blueprint_node_tools

mcp = FastMCP(
    "nodeMCP",
    description="Blueprint Node tools for Unreal via MCP"
)

register_blueprint_node_tools(mcp)

if __name__ == "__main__":
    mcp.run(transport='stdio')


"""
## Blueprint Node Management

### Enhanced Node Creation

> **Important:**
> For advanced node creation including **event nodes** (BeginPlay, Tick, EndPlay), **custom events**, 
> **function calls**, **loops**, and **complex Blueprint logic**, use the **Blueprint Action Commands** 
> which provide universal dynamic node creation via the Blueprint Action Database.
> 
> The `create_node_by_action_name` function supports:
> - **Event Nodes**: BeginPlay, Tick, EndPlay, ActorBeginOverlap, etc.
> - **Custom Events**: Fully customizable events with parameters
> - **Control Flow**: Branch, Sequence, Cast, loops, etc.
> - **Function Calls**: All BlueprintCallable functions from any class
> - **Math Operations**: All Kismet Math Library functions
> - **Utility Functions**: GameplayStatics, KismetSystemLibrary, etc.
> - **Variable Operations**: Get/Set variable nodes
> - **Universal Dynamic Creation**: Any node discoverable via Blueprint Action Database

### Basic Node Tools

The tools in this module provide **basic node manipulation** for specific use cases:

- **Connection Management**: Connect and disconnect Blueprint nodes
- **Component References**: Get references to Blueprint components
- **Node Discovery**: Find existing nodes in Blueprint graphs
- **Variable Information**: Query Blueprint variable types and properties

### Migration Notes

The following tools have been **consolidated into Blueprint Action Commands**:
- ~~`add_blueprint_event_node`~~ → Use `create_node_by_action_name` with event names
- ~~`add_blueprint_function_node`~~ → Use `create_node_by_action_name` with function names
- ~~`add_blueprint_custom_event_node`~~ → Use `create_node_by_action_name` with "CustomEvent"

### Examples

```python
# Event nodes (NEW unified approach)
create_node_by_action_name(blueprint_name="BP_Player", function_name="BeginPlay")
create_node_by_action_name(blueprint_name="BP_Player", function_name="Tick")
create_node_by_action_name(blueprint_name="BP_Player", function_name="ReceiveEndPlay")

# Custom events
create_node_by_action_name(
    blueprint_name="BP_Player", 
    function_name="CustomEvent",
    kwargs='{"event_name": "OnPlayerDied"}'
)

# Function calls
create_node_by_action_name(
    blueprint_name="BP_Player", 
    function_name="GetActorLocation"
)

# Math operations
create_node_by_action_name(
    blueprint_name="BP_Calculator", 
    function_name="SelectFloat",
    class_name="KismetMathLibrary"
)
```

### Workflow

1. Use **Blueprint Action Commands** for creating nodes
2. Use **Node Tools** for connecting and managing existing nodes
3. Use **Component Tools** for Blueprint component operations
4. Use **Variable Tools** for Blueprint variable management
""" 