"""
Unreal Engine MCP Server

A simple MCP server for interacting with Unreal Engine.
"""

import logging
import socket
import sys
import json
from contextlib import asynccontextmanager
from typing import AsyncIterator, Dict, Any, Optional
from mcp.server.fastmcp import FastMCP

# Configure logging with more detailed format
logging.basicConfig(
    level=logging.DEBUG,  # Change to DEBUG level for more details
    format='%(asctime)s - %(name)s - %(levelname)s - [%(filename)s:%(lineno)d] - %(message)s',
    handlers=[
        logging.FileHandler('unreal_mcp.log'),
        # logging.StreamHandler(sys.stdout) # Remove this handler to unexpected non-whitespace characters in JSON
    ]
)
logger = logging.getLogger("UnrealMCP")

# Configuration
UNREAL_HOST = "127.0.0.1"
UNREAL_PORT = 55557

class UnrealConnection:
    """Connection to an Unreal Engine instance."""
    
    def __init__(self):
        """Initialize the connection."""
        self.socket = None
        self.connected = False
    
    def connect(self) -> bool:
        """Connect to the Unreal Engine instance."""
        try:
            # Close any existing socket
            if self.socket:
                try:
                    self.socket.close()
                except:
                    pass
                self.socket = None

            logger.info(f"Connecting to Unreal at {UNREAL_HOST}:{UNREAL_PORT}...")
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(10)  # Increased connection timeout

            # Set socket options for better stability
            self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)

            # Set larger buffer sizes
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 131072)  # 128KB
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 131072)  # 128KB

            # Enable socket reuse
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

            self.socket.connect((UNREAL_HOST, UNREAL_PORT))
            self.connected = True
            logger.info("Connected to Unreal Engine")
            return True

        except Exception as e:
            logger.error(f"Failed to connect to Unreal: {e}")
            self.connected = False
            return False
    
    def disconnect(self):
        """Disconnect from the Unreal Engine instance."""
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
        self.socket = None
        self.connected = False

    def receive_full_response(self, sock, buffer_size=4096, timeout=15) -> bytes:
        """Receive a complete response from Unreal, handling chunked data."""
        chunks = []
        sock.settimeout(timeout)  # Increased timeout for complex operations
        try:
            while True:
                chunk = sock.recv(buffer_size)
                if not chunk:
                    if not chunks:
                        raise Exception("Connection closed before receiving data")
                    break
                chunks.append(chunk)

                # Process data received so far
                data = b''.join(chunks)
                try:
                    decoded_data = data.decode('utf-8', errors='replace')  # Handle encoding errors gracefully
                except UnicodeDecodeError as e:
                    logger.warning(f"Unicode decode error: {e}, continuing to receive more data...")
                    continue

                # Try to parse as JSON to check if complete
                try:
                    json.loads(decoded_data)
                    logger.info(f"Received complete response ({len(data)} bytes)")
                    return data
                except json.JSONDecodeError:
                    # Not complete JSON yet, continue reading
                    logger.debug(f"Received partial response, waiting for more data...")
                    continue
                except Exception as e:
                    logger.warning(f"Error processing response chunk: {str(e)}")
                    continue
        except socket.timeout:
            logger.warning(f"Socket timeout during receive (timeout: {timeout}s)")
            if chunks:
                # If we have some data already, try to use it
                data = b''.join(chunks)
                try:
                    decoded_data = data.decode('utf-8', errors='replace')
                    json.loads(decoded_data)
                    logger.info(f"Using partial response after timeout ({len(data)} bytes)")
                    return data
                except:
                    pass
            raise Exception(f"Timeout receiving Unreal response after {timeout} seconds")
        except Exception as e:
            logger.error(f"Error during receive: {str(e)}")
            raise
    
    def send_command(self, command: str, params: Dict[str, Any] = None, timeout: int = 15) -> Optional[Dict[str, Any]]:
        """Send a command to Unreal Engine and get the response."""
        # Always reconnect for each command, since Unreal closes the connection after each command
        # This is different from Unity which keeps connections alive
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
            self.connected = False
        
        if not self.connect():
            logger.error("Failed to connect to Unreal Engine for command")
            return None
        
        try:
            # Determine timeout based on command type
            complex_operations = [
                'add_blueprint_event_node', 'add_blueprint_function_node', 'create_blueprint',
                'create_struct', 'add_blueprint_variable', 'show_struct_variables'
            ]
            medium_operations = [
                'compile_blueprint', 'spawn_actor', 'spawn_blueprint_actor',
                'list_input_actions', 'list_input_mapping_contexts'
            ]

            if command in complex_operations:
                timeout = 45  # Complex operations and Chinese character processing need longer timeout
            elif command in medium_operations:
                timeout = 35  # Medium timeout for compilation/generation and list operations

            # Match Unity's command format exactly
            command_obj = {
                "type": command,  # Use "type" instead of "command"
                "params": params or {}  # Use Unity's params or {} pattern
            }

            # Send without newline, exactly like Unity
            command_json = json.dumps(command_obj, ensure_ascii=False)
            logger.info(f"Sending command: {command_json} (timeout: {timeout}s)")
            self.socket.sendall(command_json.encode('utf-8'))

            # Read response using improved handler with dynamic timeout
            response_data = self.receive_full_response(self.socket, timeout=timeout)
            response = json.loads(response_data.decode('utf-8', errors='replace'))
            
            # Log complete response for debugging
            logger.info(f"Complete response from Unreal: {response}")
            
            # Check for both error formats: {"status": "error", ...} and {"success": false, ...}
            if response.get("status") == "error":
                error_message = response.get("error") or response.get("message", "Unknown Unreal error")
                logger.error(f"Unreal error (status=error): {error_message}")
                # We want to preserve the original error structure but ensure error is accessible
                if "error" not in response:
                    response["error"] = error_message
            elif response.get("success") is False:
                # This format uses {"success": false, "error": "message"} or {"success": false, "message": "message"}
                error_message = response.get("error") or response.get("message", "Unknown Unreal error")
                logger.error(f"Unreal error (success=false): {error_message}")
                # Convert to the standard format expected by higher layers
                response = {
                    "status": "error",
                    "error": error_message
                }
            
            # Always close the connection after command is complete
            # since Unreal will close it on its side anyway
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
            self.connected = False
            
            return response
            
        except Exception as e:
            logger.error(f"Error sending command: {e}")
            # Always reset connection state on any error
            self.connected = False
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
            return {
                "status": "error",
                "error": str(e)
            }

# Global connection state
_unreal_connection: UnrealConnection = None

def get_unreal_connection() -> Optional[UnrealConnection]:
    """Get the connection to Unreal Engine."""
    global _unreal_connection
    try:
        if _unreal_connection is None:
            _unreal_connection = UnrealConnection()
            if not _unreal_connection.connect():
                logger.warning("Could not connect to Unreal Engine")
                _unreal_connection = None
        else:
            # Verify connection is still valid with a ping-like test
            try:
                # Simple test by sending an empty buffer to check if socket is still connected
                _unreal_connection.socket.sendall(b'\x00')
                logger.debug("Connection verified with ping test")
            except Exception as e:
                logger.warning(f"Existing connection failed: {e}")
                _unreal_connection.disconnect()
                _unreal_connection = None
                # Try to reconnect
                _unreal_connection = UnrealConnection()
                if not _unreal_connection.connect():
                    logger.warning("Could not reconnect to Unreal Engine")
                    _unreal_connection = None
                else:
                    logger.info("Successfully reconnected to Unreal Engine")
        
        return _unreal_connection
    except Exception as e:
        logger.error(f"Error getting Unreal connection: {e}")
        return None

@asynccontextmanager
async def server_lifespan(server: FastMCP) -> AsyncIterator[Dict[str, Any]]:
    """Handle server startup and shutdown."""
    global _unreal_connection
    logger.info("UnrealMCP server starting up")
    try:
        _unreal_connection = get_unreal_connection()
        if _unreal_connection:
            logger.info("Connected to Unreal Engine on startup")
        else:
            logger.warning("Could not connect to Unreal Engine on startup")
    except Exception as e:
        logger.error(f"Error connecting to Unreal Engine on startup: {e}")
        _unreal_connection = None
    
    try:
        yield {}
    finally:
        if _unreal_connection:
            _unreal_connection.disconnect()
            _unreal_connection = None
        logger.info("Unreal MCP server shut down")

# Initialize server
mcp = FastMCP(
    "UnrealMCP",
    description="Unreal Engine integration via Model Context Protocol",
    lifespan=server_lifespan
)

# Import and register tools
from editor_tools.editor_tools import register_editor_tools
from blueprint_tools.blueprint_tools import register_blueprint_tools
from node_tools.node_tools import register_blueprint_node_tools
from project_tools.project_tools import register_project_tools
from umg_tools.umg_tools import register_umg_tools
from datatable_tools.datatable_tools import register_datatable_tools

# Register tools
register_editor_tools(mcp)
register_blueprint_tools(mcp)
register_blueprint_node_tools(mcp)
register_project_tools(mcp)
register_umg_tools(mcp)
register_datatable_tools(mcp)

@mcp.prompt()
def info():
    """Information about available Unreal MCP tools and best practices."""
    return """
    # Unreal MCP Server Tools and Best Practices
    
    ## UMG (Widget Blueprint) Tools
    
    - **create_umg_widget_blueprint(widget_name, parent_class="UserWidget", path="/Game/Widgets")**
      
      Create a new UMG Widget Blueprint.
      
      Args:
        - widget_name: Name of the widget blueprint to create
        - parent_class: Parent class for the widget (default: UserWidget)
        - path: Content browser path where the widget should be created
      
      Returns: Dict containing success status and widget path
      
      Example:
        create_umg_widget_blueprint(widget_name="MyWidget")
    
    - **bind_widget_component_event(widget_name, widget_component_name, event_name, function_name="")**
      
      Bind an event on a widget component to a function.
      
      Args:
        - widget_name: Name of the target Widget Blueprint
        - widget_component_name: Name of the widget component (button, etc.)
        - event_name: Name of the event to bind (OnClicked, etc.)
        - function_name: Name of the function to create/bind to (optional)
      
      Returns: Dict containing success status and binding information
      
      Example:
        bind_widget_component_event(widget_name="LoginScreen", widget_component_name="LoginButton", event_name="OnClicked")
    
    - **add_widget_to_viewport(widget_name, z_order=0)**
      
      Add a Widget Blueprint instance to the viewport.
      
      Args:
        - widget_name: Name of the Widget Blueprint to add
        - z_order: Z-order for the widget (higher numbers appear on top)
      
      Returns: Dict containing success status and widget instance information
      
      Example:
        add_widget_to_viewport(widget_name="MainMenu")
    
    - **set_text_block_widget_component_binding(widget_name, text_block_name, binding_property, binding_type="Text")**
      
      Set up a property binding for a Text Block widget.
      
      Args:
        - widget_name: Name of the target Widget Blueprint
        - text_block_name: Name of the Text Block to bind
        - binding_property: Name of the property to bind to
        - binding_type: Type of binding (Text, Visibility, etc.)
      
      Returns: Dict containing success status and binding information
      
      Example:
        set_text_block_widget_component_binding(widget_name="PlayerHUD", text_block_name="ScoreText", binding_property="CurrentScore")
    
    - **add_child_widget_component_to_parent(widget_name, parent_component_name, child_component_name, create_parent_if_missing=False, parent_component_type="Border", parent_position=[0.0, 0.0], parent_size=[300.0, 200.0])**
      
      Add a widget component as a child to another component. Can create the parent if missing.
      
      Args:
        - widget_name: Name of the target Widget Blueprint
        - parent_component_name: Name of the parent component
        - child_component_name: Name of the child component to add to the parent
        - create_parent_if_missing: Whether to create the parent component if it doesn't exist
        - parent_component_type: Type of parent component to create if needed (e.g., "Border", "VerticalBox")
        - parent_position: [X, Y] position of the parent component if created
        - parent_size: [Width, Height] of the parent component if created
      
      Returns: Dict containing success status and component relationship information
      
      Example:
        add_child_widget_component_to_parent(widget_name="MyWidget", parent_component_name="ContentBorder", child_component_name="HeaderText")
    
    - **create_parent_and_child_widget_components(widget_name, parent_component_name, child_component_name, parent_component_type="Border", child_component_type="TextBlock", parent_position=[0.0, 0.0], parent_size=[300.0, 200.0], child_attributes=None)**
      
      Create a new parent widget component with a new child component (one parent, one child).
      
      Args:
        - widget_name: Name of the target Widget Blueprint
        - parent_component_name: Name for the new parent component
        - child_component_name: Name for the new child component
        - parent_component_type: Type of parent component to create (e.g., "Border", "VerticalBox")
        - child_component_type: Type of child component to create (e.g., "TextBlock", "Button")
        - parent_position: [X, Y] position of the parent component
        - parent_size: [Width, Height] of the parent component
        - child_attributes: Additional attributes for the child component (content, colors, etc.)
      
      Returns: Dict containing success status and component creation information
      
      Example:
        create_parent_and_child_widget_components(widget_name="MyWidget", parent_component_name="HeaderBorder", child_component_name="TitleText", parent_component_type="Border", child_component_type="TextBlock", parent_position=[50.0, 50.0], parent_size=[400.0, 100.0], child_attributes={"text": "Welcome to My Game", "font_size": 24})
    
    - **check_widget_component_exists(widget_name, component_name)**
      
      Check if a component exists in the specified widget blueprint.
      
      Args:
        - widget_name: Name of the target Widget Blueprint
        - component_name: Name of the component to check
      
      Returns: Dict containing existence status of the component
      
      Example:
        check_widget_component_exists(widget_name="MyWidget", component_name="HeaderText")
    
    - **set_widget_component_placement(widget_name, component_name, position=None, size=None, alignment=None)**
      
      Change the placement (position/size) of a widget component.
      
      Args:
        - widget_name: Name of the target Widget Blueprint
        - component_name: Name of the component to modify
        - position: Optional [X, Y] new position in the canvas panel
        - size: Optional [Width, Height] new size for the component
        - alignment: Optional [X, Y] alignment values (0.0 to 1.0)
      
      Returns: Dict containing success status and updated placement information
      
      Example:
        set_widget_component_placement(widget_name="MainMenu", component_name="TitleText", position=[350.0, 75.0])
    
    - **get_widget_container_component_dimensions(widget_name, container_name="CanvasPanel_0")**
      
      Get the dimensions of a container widget in a UMG Widget Blueprint.
      
      Args:
        - widget_name: Name of the target Widget Blueprint
        - container_name: Name of the container widget (defaults to "CanvasPanel_0" for the root canvas panel)
      
      Returns: Dict containing the container dimensions and position
      
      Example:
        get_widget_container_component_dimensions(widget_name="MainMenu")
    
    - **add_widget_component_to_widget(widget_name, component_name, component_type, position=None, size=None, **kwargs)**
      
      Unified function to add any type of widget component to a UMG Widget Blueprint.
      
      Args:
        - widget_name: Name of the target Widget Blueprint
        - component_name: Name to give the new component
        - component_type: Type of component to add (e.g., "TextBlock", "Button", etc.)
        - position: Optional [X, Y] position in the canvas panel
        - size: Optional [Width, Height] of the component
        - **kwargs: Additional parameters specific to the component type
      
      Returns: Dict containing success status and component properties
      
      Example:
        add_widget_component_to_widget(widget_name="MyWidget", component_name="HeaderText", component_type="TextBlock", position=[100, 50], size=[200, 50], text="Hello World", font_size=24)
    
    - **set_widget_component_property(widget_name, component_name, **kwargs)**
      
      Set one or more properties on a specific component within a UMG Widget Blueprint.
      
      Args:
        - widget_name: Name of the target Widget Blueprint
        - component_name: Name of the component to modify
        - **kwargs: Properties to set (as keyword arguments or a dict)
      
      Returns: Dict containing success status and property update info
      
      Example:
        set_widget_component_property("MyWidget", "MyTextBlock", Text="Red Text", ColorAndOpacity={"SpecifiedColor": {"R": 1.0, "G": 0.0, "B": 0.0, "A": 1.0}})
    
    - **get_widget_component_layout(widget_name)**
      
      Get hierarchical layout information for all components within a UMG Widget Blueprint.
      
      Args:
        - widget_name: Name of the target Widget Blueprint (e.g., "WBP_MainMenu", "/Game/UI/MyWidget")
      
      Returns: Dict containing:
        - success (bool): True if the operation succeeded.
        - message (str): Status message.
        - hierarchy (dict): Root component with the following structure:
            - name (str): Component name.
            - type (str): Component class name (e.g., "TextBlock", "Button").
            - slot_properties (dict): Layout properties based on the slot type.
            - children (list[dict]): List of child components with the same structure.
      
      Example:
        layout = get_widget_component_layout(widget_name="WBP_PricingPage")
        if layout.get("success"):
            hierarchy = layout.get("hierarchy", {})
            print(f"Root component: {hierarchy.get('name')} ({hierarchy.get('type')})")
    
    ## Editor Tools
    ### Viewport and Screenshots
    - `focus_viewport(target, location, distance, orientation)` - Focus viewport
    - `take_screenshot(filename, show_ui, resolution)` - Capture screenshots

    ### Actor Management
    - **get_actors_in_level()**
      
      Get a list of all actors in the current level.
      
      Returns: List of actors in the current level with their properties.
      
      Example:
        actors = get_actors_in_level()
        for actor in actors:
            print(actor["name"])
    
    - **find_actors_by_name(pattern)**
      
      Find actors by name pattern (supports wildcards using *).
      
      Args:
        - pattern (str): Name pattern to search for
      
      Returns: List of actor names matching the pattern.
      
      Example:
        lights = find_actors_by_name("*PointLight*")
        player = find_actors_by_name("Player*")
    
    - **spawn_actor(name, type, location=None, rotation=None)**
      
      Create a new basic Unreal Engine actor in the current level (built-in types only).
      
      Args:
        - name (str): The name to give the new actor (must be unique)
        - type (str): The type of built-in actor to create (StaticMeshActor, PointLight, SpotLight, DirectionalLight, CameraActor)
        - location (list): The [x, y, z] world location to spawn at
        - rotation (list): The [pitch, yaw, roll] rotation in degrees
      
      Returns: Dict containing the created actor's properties.
      
      Example:
        spawn_actor(name="MyLight", type="PointLight")
        spawn_actor(name="MyCube", type="StaticMeshActor", location=[100, 200, 50], rotation=[0, 45, 0])
    
    - **delete_actor(name)**
      
      Delete an actor by name.
      
      Args:
        - name (str): Name of the actor to delete
      
      Returns: Dict containing response information.
      
      Example:
        delete_actor(name="MyCube")
    
    - **set_actor_transform(name, location=None, rotation=None, scale=None)**
      
      Set the transform of an actor.
      
      Args:
        - name (str): Name of the actor
        - location (list): Optional [X, Y, Z] position
        - rotation (list): Optional [Pitch, Yaw, Roll] rotation in degrees
        - scale (list): Optional [X, Y, Z] scale
      
      Returns: Dict containing response information.
      
      Example:
        set_actor_transform(name="MyCube", location=[100, 200, 50])
        set_actor_transform(name="MyCube", rotation=[0, 0, 45])
        set_actor_transform(name="MyCube", scale=[2.0, 2.0, 2.0])
        set_actor_transform(name="MyCube", location=[100, 200, 50], rotation=[0, 0, 45], scale=[2.0, 2.0, 2.0])
    
    - **get_actor_properties(name)**
      
      Get all properties of an actor.
      
      Args:
        - name (str): Name of the actor
      
      Returns: Dict containing actor properties.
      
      Example:
        props = get_actor_properties(name="MyCube")
        print(props["transform"]["location"])
    
    - **set_actor_property(name, property_name, property_value)**
      
      Set a property on an actor.
      
      Args:
        - name (str): Name of the actor
        - property_name (str): Name of the property to set
        - property_value: Value to set the property to (various types)
      
      Returns: Dict containing response information.
      
      Example:
        set_actor_property(name="MyPointLight", property_name="LightColor", property_value=[255, 0, 0, 255])
        set_actor_property(name="MyCube", property_name="Mobility", property_value="Movable")
        set_actor_property(name="MyCube", property_name="bHidden", property_value=True)
        set_actor_property(name="PointLightTest", property_name="Intensity", property_value=5000.0)

    ## Blueprint Management
    
    - **create_blueprint(name, parent_class, folder_path="")**
      
      Create a new Blueprint class.
      
      Args:
        - name (str): Name of the new Blueprint class (can include path with "/")
        - parent_class (str): Parent class for the Blueprint (e.g., "Actor", "Pawn")
        - folder_path (str): Optional folder path where the blueprint should be created
      
      Returns: Dictionary containing information about the created Blueprint including path and success status.
      
      Example:
        create_blueprint(name="MyBlueprint", parent_class="Actor")
        create_blueprint(name="System/Blueprints/my_bp", parent_class="Actor")
        create_blueprint(name="MyBlueprint", parent_class="Actor", folder_path="Success")
    
    - **add_blueprint_variable(blueprint_name, variable_name, variable_type, is_exposed=False)**
      
      Add a variable to a Blueprint. Supports built-in, user-defined struct, and delegate types.
      
      Args:
        - blueprint_name (str): Name of the target Blueprint
        - variable_name (str): Name of the variable
        - variable_type (str): Type of the variable (Boolean, Integer, Float, Vector, StructName, StructName[], Delegate, etc.)
        - is_exposed (bool): Whether to expose the variable to the editor
      
      Returns: Response indicating success or failure.
      
      Example:
        add_blueprint_variable(blueprint_name="PlayerBlueprint", variable_name="Score", variable_type="Integer", is_exposed=True)
        add_blueprint_variable(blueprint_name="PlayerBlueprint", variable_name="Inventory", variable_type="String[]", is_exposed=True)
        add_blueprint_variable(blueprint_name="PlayerBlueprint", variable_name="WBP_Widget", variable_type="Game/Widget/WBP_Widget", is_exposed=True)
        add_blueprint_variable(blueprint_name="PlayerBlueprint", variable_name="MyStruct", variable_type="Game/DataStructures/MyStruct", is_exposed=True)
        add_blueprint_variable(blueprint_name="PlayerBlueprint", variable_name="MyStructs", variable_type="Game/DataStructures/MyStruct[]", is_exposed=True)
        add_blueprint_variable(blueprint_name="PlayerBlueprint", variable_name="Datatable", variable_type="Game/DataTables/MyDatatable", is_exposed=True)
        add_blueprint_variable(blueprint_name="PlayerBlueprint", variable_name="BP_Manager", variable_type="Game/Blueprints/BP_Manager", is_exposed=True)
    
    - **add_component_to_blueprint(blueprint_name, component_type, component_name, location=None, rotation=None, scale=None, component_properties=None)**
      
      Add a component to a Blueprint.
      
      Args:
        - blueprint_name (str): Name of the target Blueprint
        - component_type (str): Type of component to add (use component class name without U prefix)
        - component_name (str): Name for the new component
        - location (list): [X, Y, Z] coordinates for component's position
        - rotation (list): [Pitch, Yaw, Roll] values for component's rotation
        - scale (list): [X, Y, Z] values for component's scale
        - component_properties (dict): Additional properties to set on the component
      
      Returns: Information about the added component.
    
    - **set_static_mesh_properties(blueprint_name, component_name, static_mesh="/Engine/BasicShapes/Cube.Cube")**
      
      Set static mesh properties on a StaticMeshComponent.
      
      Args:
        - blueprint_name (str): Name of the target Blueprint
        - component_name (str): Name of the StaticMeshComponent
        - static_mesh (str): Path to the static mesh asset
      
      Returns: Response indicating success or failure.
    
    - **set_component_property(blueprint_name, component_name, **kwargs)**
      
      Set one or more properties on a specific component within a Blueprint.
      
      Args:
        - blueprint_name (str): Name of the target Blueprint
        - component_name (str): Name of the component to modify
        - kwargs: Properties to set as a JSON string. Must be formatted as a valid JSON string.
      
      Returns: Dict containing:
        - success: Boolean indicating overall success
        - success_properties: List of property names successfully set
        - failed_properties: List of property names that failed to set, with error messages
      
      Example:
        set_component_property(blueprint_name="MyBlueprint", component_name="PointLight1", kwargs='{"Intensity": 5000.0, "AttenuationRadius": 1000.0}')
        set_component_property(blueprint_name="MyBlueprint", component_name="Mesh1", kwargs='{"RelativeLocation": [100.0, 200.0, 50.0]}')
    
    - **set_physics_properties(blueprint_name, component_name, simulate_physics=True, gravity_enabled=True, mass=1.0, linear_damping=0.01, angular_damping=0.0)**
      
      Set physics properties on a component.
      
      Args:
        - blueprint_name (str): Name of the target Blueprint
        - component_name (str): Name of the component
        - simulate_physics (bool): Whether to simulate physics on the component
        - gravity_enabled (bool): Whether gravity affects the component
        - mass (float): Mass of the component in kg
        - linear_damping (float): Linear damping factor
        - angular_damping (float): Angular damping factor
      
      Returns: Response indicating success or failure.
    
    - **compile_blueprint(blueprint_name)**
      
      Compile a Blueprint.
      
      Args:
        - blueprint_name (str): Name of the target Blueprint
      
      Returns: Response indicating success or failure.
    
    - **set_blueprint_property(blueprint_name, property_name, property_value)**
      
      Set a property on a Blueprint class default object.
      
      Args:
        - blueprint_name (str): Name of the target Blueprint
        - property_name (str): Name of the property to set
        - property_value: Value to set the property to
      
      Returns: Response indicating success or failure.
    
    - **set_pawn_properties(blueprint_name, auto_possess_player="", use_controller_rotation_yaw=None, use_controller_rotation_pitch=None, use_controller_rotation_roll=None, can_be_damaged=None)**
      
      Set common Pawn properties on a Blueprint. This is a utility function that sets multiple pawn-related properties at once.
      
      Args:
        - blueprint_name (str): Name of the target Blueprint (must be a Pawn or Character)
        - auto_possess_player (str): Auto possess player setting (None, "Disabled", "Player0", "Player1", etc.)
        - use_controller_rotation_yaw (bool): Whether the pawn should use the controller's yaw rotation
        - use_controller_rotation_pitch (bool): Whether the pawn should use the controller's pitch rotation
        - use_controller_rotation_roll (bool): Whether the pawn should use the controller's roll rotation
        - can_be_damaged (bool): Whether the pawn can be damaged
      
      Returns: Response indicating success or failure with detailed results for each property.

    ## Blueprint Node Management
    
    ### Important Limitation: Loop Nodes (ForEach, ForLoop, etc.)
    
    > **Note:**
    > The standard Blueprint loop nodes such as **ForEach** and **ForLoop** are implemented as Blueprint Macros, not as native K2 nodes. Due to Unreal Engine's public API limitations, these macro nodes **cannot be added programmatically** via the MCP tool or any C++ plugin. Only native K2 nodes (such as Branch, Sequence, etc.) can be added programmatically.
    > If you need loop logic in your Blueprint, please provide a comment for user in place of the loop node. This is a known limitation of Unreal Engine and is not specific to this plugin.
    
    - **add_blueprint_event_node(blueprint_name, event_name, node_position=None)**
      
      Add an event node to a Blueprint's event graph.
      
      Args:
        - blueprint_name (str): Name of the target Blueprint
        - event_name (str): Name of the event. Use 'Receive' prefix for standard events (e.g., 'ReceiveBeginPlay', 'ReceiveTick')
        - node_position (list): Optional [X, Y] position in the graph
      
      Returns: Response containing the node ID and success status.
    
    - **add_blueprint_input_action_node(blueprint_name, action_name, node_position=None)**
      
      Add an input action event node to a Blueprint's event graph.
      
      Args:
        - blueprint_name (str): Name of the target Blueprint
        - action_name (str): Name of the input action to respond to
        - node_position (list): Optional [X, Y] position in the graph
      
      Returns: Response containing the node ID and success status.
    
    - **add_blueprint_function_node(blueprint_name, target, function_name, params=None, node_position=None)**
      
      Add a function call node to a Blueprint's event graph.
      
      Args:
        - blueprint_name (str): Name of the target Blueprint
        - target (str): Target object for the function (component name or self)
        - function_name (str): Name of the function to call
        - params (dict): Optional parameters to set on the function node
        - node_position (list): Optional [X, Y] position in the graph
      
      Returns: Response containing the node ID and success status.
    
    - **connect_blueprint_nodes(blueprint_name, source_node_id, source_pin, target_node_id, target_pin)**
      
      Connect two nodes in a Blueprint's event graph. You must determine the correct node IDs and pins to connect.
      
      Args:
        - blueprint_name (str): Name of the target Blueprint
        - source_node_id (str): ID of the source node
        - source_pin (str): Name of the output pin on the source node
        - target_node_id (str): ID of the target node
        - target_pin (str): Name of the input pin on the target node
      
      Returns: Response indicating success or failure.
    
    - **add_blueprint_get_self_component_reference(blueprint_name, component_name, node_position=None)**
      
      Add a node that gets a reference to a component owned by the current Blueprint (like dragging a component from the Components panel).
      
      Args:
        - blueprint_name (str): Name of the target Blueprint
        - component_name (str): Name of the component to get a reference to
        - node_position (list): Optional [X, Y] position in the graph
      
      Returns: Response containing the node ID and success status.
    
    - **add_blueprint_self_reference(blueprint_name, node_position=None)**
      
      Add a 'Get Self' node to a Blueprint's event graph that returns a reference to this actor.
      
      Args:
        - blueprint_name (str): Name of the target Blueprint
        - node_position (list): Optional [X, Y] position in the graph
      
      Returns: Response containing the node ID and success status.
    
    - **find_blueprint_nodes(blueprint_name, node_type=None, event_type=None)**
      
      Find nodes in a Blueprint's event graph.
      
      Args:
        - blueprint_name (str): Name of the target Blueprint
        - node_type (str): Optional type of node to find (Event, Function, Variable, etc.)
        - event_type (str): Optional specific event type to find (BeginPlay, Tick, etc.)
      
      Returns: Response containing array of found node IDs and success status.
    
    ## Project Tools
    
    - **create_input_mapping(action_name, key, input_type="Action")**
      
      Create an input mapping for the project.
      
      Args:
        - action_name (str): Name of the input action
        - key (str): Key to bind (SpaceBar, LeftMouseButton, etc.)
        - input_type (str): Type of input mapping (Action or Axis)
      
      Returns: Response indicating success or failure.
      
      Example:
        create_input_mapping(action_name="Jump", key="SpaceBar")
    
    - **create_folder(folder_path)**
      
      Create a new folder in the Unreal project. Can create both content folders (for the content browser) and regular project folders.
      
      Args:
        - folder_path (str): Path to create, relative to project root. Use "Content/..." prefix for content browser folders.
      
      Returns: Dictionary with the creation status and folder path.
      
      Example:
        create_folder(folder_path="Content/MyGameContent")
        create_folder(folder_path="Intermediate/MyTools")
    
    - **create_struct(struct_name, properties, path="/Game/Blueprints", description="")**
      
      Create a new Unreal struct.
      
      Args:
        - struct_name (str): Name of the struct to create
        - properties (list): List of property dictionaries, each containing:
          - name: Property name
          - type: Property type (e.g., "Boolean", "Integer", "Float", "String", "Vector", etc.)
          - description: (Optional) Property description
        - path (str): Path where to create the struct
        - description (str): Optional description for the struct
      
      Returns: Dictionary with the creation status and struct path.
      
      Example:
        create_struct(
            struct_name="Item",
            properties=[
                {"name": "Name", "type": "String"},
                {"name": "Value", "type": "Integer"},
                {"name": "IsRare", "type": "Boolean"}
            ],
            path="/Game/DataStructures"
        )
    
    - **update_struct(struct_name, properties, path="/Game/Blueprints", description="")**
      
      Update an existing Unreal struct.
      
      Args:
        - struct_name (str): Name of the struct to update
        - properties (list): List of property dictionaries, each containing:
          - name: Property name
          - type: Property type (e.g., "Boolean", "Integer", "Float", "String", "Vector", etc.)
          - description: (Optional) Property description
        - path (str): Path where the struct exists
        - description (str): Optional description for the struct
      
      Returns: Dictionary with the update status and struct path.
    
    - **show_struct_variables(struct_name, path="/Game/Blueprints")**
      
      Show variables and types of a struct in Unreal Engine.
      
      Args:
        - struct_name (str): Name of the struct to inspect
        - path (str): Path where the struct exists (default: /Game/Blueprints)
      
      Returns: Dictionary with struct variable info.
    
    - **list_folder_contents(folder_path)**
      
      List the contents of a folder in the Unreal project (content or regular folder).
      
      Args:
        - folder_path (str): Path to the folder (e.g., "/Game/Blueprints" or "Content/MyFolder" or "Intermediate/MyTools")
      
      Returns: Dictionary with arrays of subfolders and files/assets.
    
    ## DataTable Tools
    
    - **create_datatable(datatable_name, row_struct_name, path="/Game/Data", description="")**
      
      Create a new DataTable asset.
      
      Args:
        - datatable_name (str): Name of the DataTable to create
        - row_struct_name (str): Name or path of the struct to use for rows. Can be a full path (recommended) or a simple name.
        - path (str): Path where to create the DataTable (defaults to "/Game/Data")
        - description (str): Optional description for the DataTable
      
      Returns: Dict containing success status and created asset info.
      
      Example:
        create_datatable(
            datatable_name="ItemTable",
            row_struct_name="/Game/Data/ItemStruct",
            path="/Game/Data"
        )
        create_datatable(
            datatable_name="ItemTable",
            row_struct_name="ItemStruct"
        )
        create_datatable(
            datatable_name="ItemTable",
            row_struct_name="/Game/Data/ItemStruct",
            description="Table containing all game items"
        )
    
    - **get_datatable_rows(datatable_name, row_names=None)**
      
      Get rows from a DataTable.
      
      Args:
        - datatable_name (str): Name of the target DataTable
        - row_names (list, optional): List of specific row names to retrieve
      
      Returns: Dict containing the requested rows.
    
    - **delete_datatable_row(datatable_name, row_name)**
      
      Delete a row from a DataTable.
      
      Args:
        - datatable_name (str): Name of the target DataTable
        - row_name (str): Name of the row to delete
      
      Returns: Dict containing success status and updated DataTable info.
    
    - **get_datatable_row_names(datatable_name)**
      
      Get all row names and struct field names from a DataTable.
      
      Args:
        - datatable_name (str): Name of the target DataTable
      
      Returns: Dict containing:
        - 'row_names': list of row names (str)
        - 'field_names': list of struct field names (str)
    
    - **add_rows_to_datatable(datatable_name, rows)**
      
      Add multiple rows to an existing DataTable.
      
      Args:
        - datatable_name (str): Name of the target DataTable (full path or simple name)
        - rows (list): List of dicts, each with:
          - 'row_name': Unique name for the row
          - 'row_data': Dict of property values using the internal GUID-based property names (get these from get_datatable_row_names)
      
      Returns: Dict containing success status and list of added row names.
      
      Example:
        property_info = get_datatable_row_names("MyItemTable")
        field_names = property_info["result"]["field_names"]
        add_rows_to_datatable(
            datatable_name="/Game/Data/MyItemTable",
            rows=[{
                "row_name": "Item1",
                "row_data": {
                    field_names[0]: "Sword",
                    field_names[1]: 99.99,
                    field_names[2]: 5,
                    field_names[3]: True,
                    field_names[4]: ["weapon"]
                }
            }]
        )
    
    - **update_rows_in_datatable(datatable_name, rows)**
      
      Update multiple rows in an existing DataTable.
      
      Args:
        - datatable_name (str): Name of the target DataTable (full path or simple name)
        - rows (list): List of dicts, each with:
          - 'row_name': Name of the row to update
          - 'row_data': Dict of property values using the internal GUID-based property names (get these from get_datatable_row_names)
      
      Returns: Dict containing success status and list of updated/failed row names.
      
      Example:
        property_info = get_datatable_row_names("MyItemTable")
        field_names = property_info["result"]["field_names"]
        update_rows_in_datatable(
            datatable_name="/Game/Data/MyItemTable",
            rows=[{
                "row_name": "Item1",
                "row_data": {
                    field_names[0]: "Enhanced Sword",
                    field_names[1]: 149.99,
                    field_names[2]: 3,
                    field_names[3]: True,
                    field_names[4]: ["weapon", "rare"]
                }
            }]
        )
    
    - **delete_datatable_rows(datatable_name, row_names)**
      
      Delete multiple rows from a DataTable.
      
      Args:
        - datatable_name (str): Name of the target DataTable
        - row_names (list): List of row names to delete
      
      Returns: Dict containing success status and updated DataTable info.
    
    ## Best Practices
    
    ### General
    - Read the documentation: Always refer to the up-to-date info() docstring for detailed arguments, return values, and usage examples for every tool.
    - Check command responses: Always check the returned dictionary for 'success' or 'status' fields to confirm operation results and handle errors gracefully.
    - Log important operations: Use logging for all major actions, especially those that modify assets, actors, or project settings.
    - Validate parameters: Double-check all arguments (names, paths, types) before invoking a tool to avoid runtime errors.
    - Clean up resources: Remove temporary actors, folders, or data after tests or experiments to keep the project clean.
    
    ### UMG Widget Development
    - Use descriptive names: Name widgets and components clearly to reflect their purpose and avoid confusion.
    - Consistent naming: Stick to a naming convention for all widget components and Blueprints.
    - Organize hierarchies: Structure widget hierarchies logically for easier maintenance and extensibility.
    - Responsive layouts: Set anchors, alignment, and size rules for widgets to ensure they look good at all resolutions.
    - Property bindings: Prefer property bindings for dynamic UI updates instead of direct property setting.
    - Event handling: Bind widget events to clearly named functions and keep event logic modular.
    - Test layouts: Preview widgets at multiple resolutions and aspect ratios.
    
    ### Actor and Editor Management
    - Unique actor names: Always use unique names for actors to prevent conflicts and accidental overwrites.
    - Transform validation: Validate locations, rotations, and scales before applying transforms.
    - Existence checks: Use find_actors_by_name or get_actors_in_level to confirm an actor exists before modifying or deleting it.
    - Regular screenshots: Use the screenshot tool to document progress and catch visual regressions.
    - Viewport focus: Use viewport focus tools to keep the editor centered on relevant actors during development.
    
    ### Blueprint Management
    - Compile after changes: Always compile Blueprints after making changes to ensure they are up to date and error-free.
    - Meaningful variable names: Use clear, descriptive names for Blueprint variables and functions.
    - Logical node layout: Organize Blueprint nodes for readability and maintainability.
    - Test in isolation: Test new Blueprint logic in isolation before integrating into larger systems.
    - Document complex setups: Add comments or documentation for complex Blueprint graphs or custom logic.
    
    ### DataTable Management
    - Use structs for rows: Always define a struct for your DataTable rows and keep it updated as your data model evolves.
    - Field name consistency: Use get_datatable_row_names to retrieve the correct GUID-based field names before adding or updating rows.
    - Batch operations: Use batch add/update/delete functions for efficiency when working with multiple rows.
    - Backup before bulk edits: Consider exporting or backing up DataTables before performing large-scale changes.
    
    ### Project Structure
    - Organize content: Use the folder creation tools to keep your Content Browser and project directories organized.
    - Consistent structs: Keep struct definitions consistent and up to date with your gameplay and UI needs.
    - Input mapping: Use the input mapping tool to centralize and standardize input actions across your project.
    
    ### Error Handling
    - Graceful degradation: If a tool fails, log the error and provide a user-friendly message or fallback.
    - Resource cleanup: Always clean up temporary or test resources, especially after failed operations.
    - Avoid duplicates: Check for existing assets, actors, or data before creating new ones to prevent duplication.
    """

if __name__ == "__main__":
    logger.info("Starting MCP server with stdio transport")
    mcp.run(transport='stdio')