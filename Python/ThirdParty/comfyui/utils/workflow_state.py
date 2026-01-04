"""
In-memory workflow state management for ComfyUI.

Manages the current workflow being built, including nodes and connections.
"""

from typing import Any, Dict, List, Optional, Set, Tuple


class WorkflowNode:
    """Represents a single node in the workflow."""

    def __init__(
        self,
        node_id: str,
        class_type: str,
        inputs: Dict[str, Any] = None,
        position: List[float] = None
    ):
        self.node_id = node_id
        self.class_type = class_type
        self.inputs = inputs or {}
        self.position = position or [0.0, 0.0]

    def to_dict(self) -> Dict[str, Any]:
        """Convert to ComfyUI workflow format."""
        return {
            "class_type": self.class_type,
            "inputs": self.inputs
        }

    def to_display_dict(self) -> Dict[str, Any]:
        """Convert to display format with all info."""
        return {
            "node_id": self.node_id,
            "class_type": self.class_type,
            "inputs": self.inputs,
            "position": self.position
        }


class WorkflowState:
    """Manages the in-memory workflow state."""

    def __init__(self, name: str = "Untitled"):
        self.name = name
        self._nodes: Dict[str, WorkflowNode] = {}
        self._next_id = 1

    def _generate_id(self) -> str:
        """Generate the next node ID."""
        node_id = str(self._next_id)
        self._next_id += 1
        return node_id

    def clear(self, name: str = "Untitled"):
        """Clear the workflow and start fresh."""
        self._nodes.clear()
        self._next_id = 1
        self.name = name

    def add_node(
        self,
        class_type: str,
        inputs: Dict[str, Any] = None,
        position: List[float] = None
    ) -> WorkflowNode:
        """
        Add a new node to the workflow.

        Args:
            class_type: The node class name (e.g., "KSampler")
            inputs: Initial input values
            position: [x, y] position in graph

        Returns:
            The created WorkflowNode
        """
        node_id = self._generate_id()
        node = WorkflowNode(node_id, class_type, inputs, position)
        self._nodes[node_id] = node
        return node

    def remove_node(self, node_id: str) -> Tuple[bool, List[Dict[str, Any]]]:
        """
        Remove a node and track broken connections.

        Args:
            node_id: The node ID to remove

        Returns:
            Tuple of (success, list of removed connections)
        """
        if node_id not in self._nodes:
            return False, []

        # Find and track connections that reference this node
        removed_connections = []

        for other_id, other_node in self._nodes.items():
            if other_id == node_id:
                continue

            for input_name, value in list(other_node.inputs.items()):
                # Check if this input is a connection to the removed node
                if isinstance(value, list) and len(value) == 2:
                    if str(value[0]) == str(node_id):
                        removed_connections.append({
                            "target_node": other_id,
                            "target_input": input_name,
                            "source_node": node_id,
                            "source_output": value[1]
                        })
                        # Remove the broken connection
                        del other_node.inputs[input_name]

        # Remove the node
        del self._nodes[node_id]
        return True, removed_connections

    def get_node(self, node_id: str) -> Optional[WorkflowNode]:
        """Get a node by ID."""
        return self._nodes.get(node_id)

    def set_input(self, node_id: str, input_name: str, value: Any) -> bool:
        """
        Set an input value on a node.

        Args:
            node_id: The node ID
            input_name: The input name
            value: The value (can be a literal or connection [node_id, output_index])

        Returns:
            True if successful
        """
        node = self._nodes.get(node_id)
        if not node:
            return False

        node.inputs[input_name] = value
        return True

    def connect_nodes(
        self,
        source_node_id: str,
        source_output: int,
        target_node_id: str,
        target_input: str
    ) -> bool:
        """
        Connect an output from one node to an input on another.

        Args:
            source_node_id: The source node ID
            source_output: The output index on the source node
            target_node_id: The target node ID
            target_input: The input name on the target node

        Returns:
            True if successful
        """
        if source_node_id not in self._nodes:
            return False
        if target_node_id not in self._nodes:
            return False

        target_node = self._nodes[target_node_id]
        target_node.inputs[target_input] = [source_node_id, source_output]
        return True

    def get_workflow_json(self) -> Dict[str, Any]:
        """
        Export the workflow in ComfyUI's prompt format.

        Returns:
            Dictionary suitable for the /prompt API
        """
        return {
            node_id: node.to_dict()
            for node_id, node in self._nodes.items()
        }

    def get_all_nodes(self) -> List[Dict[str, Any]]:
        """Get all nodes with display info."""
        return [node.to_display_dict() for node in self._nodes.values()]

    def get_connections(self) -> List[Dict[str, Any]]:
        """
        Get all connections in the workflow.

        Returns:
            List of connection objects with source/target info
        """
        connections = []

        for node_id, node in self._nodes.items():
            for input_name, value in node.inputs.items():
                # Check if this is a connection (list format)
                if isinstance(value, list) and len(value) == 2:
                    source_id, source_output = value
                    connections.append({
                        "source_node": str(source_id),
                        "source_output": source_output,
                        "target_node": node_id,
                        "target_input": input_name
                    })

        return connections

    def get_orphan_nodes(self) -> List[Dict[str, Any]]:
        """
        Find nodes that have no connections (neither input nor output).

        Returns:
            List of orphan node info
        """
        # Track which nodes have connections
        connected_nodes: Set[str] = set()

        for node_id, node in self._nodes.items():
            for value in node.inputs.values():
                if isinstance(value, list) and len(value) == 2:
                    # This node has an input connection
                    connected_nodes.add(node_id)
                    # Source node has an output connection
                    connected_nodes.add(str(value[0]))

        # Find orphans
        orphans = []
        for node_id, node in self._nodes.items():
            if node_id not in connected_nodes:
                orphans.append({
                    "node_id": node_id,
                    "class_type": node.class_type,
                    "position": node.position
                })

        return orphans

    def get_flow_analysis(self) -> Dict[str, Any]:
        """
        Analyze the workflow flow/graph structure.

        Returns:
            Dictionary with flow analysis including:
            - sources: Nodes with no input connections (data sources)
            - sinks: Nodes with no output connections (output nodes)
            - connections: All connections with types
        """
        # Track node connectivity
        has_input: Set[str] = set()
        has_output: Set[str] = set()

        connections = []

        for node_id, node in self._nodes.items():
            for input_name, value in node.inputs.items():
                if isinstance(value, list) and len(value) == 2:
                    source_id = str(value[0])
                    has_input.add(node_id)
                    has_output.add(source_id)
                    connections.append({
                        "source_node": source_id,
                        "source_output": value[1],
                        "target_node": node_id,
                        "target_input": input_name,
                        "source_type": self._nodes.get(source_id, {}).class_type if source_id in self._nodes else "unknown",
                        "target_type": node.class_type
                    })

        # Find sources and sinks
        sources = []
        sinks = []

        for node_id, node in self._nodes.items():
            if node_id not in has_input:
                sources.append({
                    "node_id": node_id,
                    "class_type": node.class_type
                })
            if node_id not in has_output:
                sinks.append({
                    "node_id": node_id,
                    "class_type": node.class_type
                })

        return {
            "sources": sources,
            "sinks": sinks,
            "connections": connections,
            "node_count": len(self._nodes),
            "connection_count": len(connections)
        }

    def get_summary(self) -> Dict[str, Any]:
        """
        Get a summary of the workflow.

        Returns:
            Summary statistics
        """
        # Count nodes by category
        categories: Dict[str, int] = {}
        for node in self._nodes.values():
            # Extract category from class_type (simplified)
            categories[node.class_type] = categories.get(node.class_type, 0) + 1

        connections = self.get_connections()

        return {
            "name": self.name,
            "node_count": len(self._nodes),
            "connection_count": len(connections),
            "node_types": categories,
            "orphan_count": len(self.get_orphan_nodes())
        }

    # Connection types that become input slots (not widgets)
    CONNECTION_TYPES = {
        "MODEL", "CLIP", "VAE", "CONDITIONING", "LATENT", "IMAGE",
        "MASK", "CONTROL_NET", "STYLE_MODEL", "CLIP_VISION", "CLIP_VISION_OUTPUT",
        "GLIGEN", "UPSCALE_MODEL", "SAMPLER", "SIGMAS", "NOISE", "GUIDER",
        "AUDIO", "TAESD", "*"
    }

    # Hidden widgets added by ComfyUI frontend (not in API object_info)
    # Format: {node_type: [(after_widget, hidden_widget_name, default_value), ...]}
    HIDDEN_WIDGETS = {
        "KSampler": [("seed", "control_after_generate", "randomize")],
        "KSamplerAdvanced": [("noise_seed", "control_after_generate", "randomize")],
    }

    def _is_connection_type(self, type_info: Any) -> bool:
        """Check if a type is a connection type (not a widget)."""
        if isinstance(type_info, str):
            return type_info.upper() in self.CONNECTION_TYPES
        # Lists are COMBO widgets (dropdown selections)
        if isinstance(type_info, list):
            return False
        return False

    def _get_widget_order(self, node_type: str, object_info: Dict[str, Any]) -> List[Tuple[str, bool]]:
        """
        Get the correct widget input order for a node type from object_info.

        Returns list of tuples: (input_name, is_hidden)
        - is_hidden=False means we have a value for it
        - is_hidden=True means it's a frontend-only widget, use default
        """
        if node_type not in object_info:
            return []

        node_def = object_info[node_type]
        input_def = node_def.get("input", {})
        required = input_def.get("required", {})
        optional = input_def.get("optional", {})

        widget_names: List[Tuple[str, bool]] = []

        # Get hidden widgets for this node type
        hidden_widgets = self.HIDDEN_WIDGETS.get(node_type, [])
        hidden_map = {after: (name, default) for after, name, default in hidden_widgets}

        # Process required inputs first, then optional
        for inputs_dict in [required, optional]:
            for input_name, config in inputs_dict.items():
                if not config:
                    continue

                # Config is typically [type, options_dict] or just [type]
                type_info = config[0] if isinstance(config, (list, tuple)) else config

                # Skip connection types - they become input slots, not widgets
                if self._is_connection_type(type_info):
                    continue

                widget_names.append((input_name, False))

                # Check if there's a hidden widget after this one
                if input_name in hidden_map:
                    hidden_name, _ = hidden_map[input_name]
                    widget_names.append((hidden_name, True))

        return widget_names

    def _get_hidden_widget_default(self, node_type: str, widget_name: str) -> Any:
        """Get default value for a hidden widget."""
        hidden_widgets = self.HIDDEN_WIDGETS.get(node_type, [])
        for _, name, default in hidden_widgets:
            if name == widget_name:
                return default
        return None

    def get_ui_workflow_json(self, object_info: Dict[str, Any] = None) -> Dict[str, Any]:
        """
        Export the workflow in ComfyUI's UI-loadable format.

        This format includes node positions and link metadata needed
        for the visual graph editor.

        Args:
            object_info: Optional node definitions from ComfyUI's /object_info endpoint.
                        If provided, widget values will be correctly ordered.

        Returns:
            Dictionary suitable for loading via ComfyUI's Load button
        """
        # First pass: collect all connections and assign link IDs
        links = []
        link_id = 1

        # Map: (target_node_id, input_name) -> link_id
        input_link_map: Dict[Tuple[str, str], int] = {}
        # Map: (source_node_id, output_index) -> list of link_ids
        output_link_map: Dict[Tuple[str, int], List[int]] = {}

        for node_id, node in self._nodes.items():
            for input_name, value in node.inputs.items():
                if isinstance(value, list) and len(value) == 2:
                    source_id, source_output = value
                    if isinstance(source_id, str) and isinstance(source_output, int):
                        # Record the link
                        input_link_map[(node_id, input_name)] = link_id

                        output_key = (source_id, source_output)
                        if output_key not in output_link_map:
                            output_link_map[output_key] = []
                        output_link_map[output_key].append(link_id)

                        link_id += 1

        # Second pass: build nodes with proper inputs/outputs arrays
        nodes = []

        for node_id, node in self._nodes.items():
            # Collect connection inputs (these become input slots)
            input_slots = []
            widget_values = []

            # Get widget order from object_info if available
            if object_info:
                widget_order = self._get_widget_order(node.class_type, object_info)
                # Build widget_values in correct order
                for widget_name, is_hidden in widget_order:
                    if is_hidden:
                        # Use default value for hidden frontend widgets
                        default = self._get_hidden_widget_default(node.class_type, widget_name)
                        widget_values.append(default)
                    elif widget_name in node.inputs:
                        value = node.inputs[widget_name]
                        # Skip if it's a connection
                        if isinstance(value, list) and len(value) == 2:
                            if isinstance(value[0], str) and isinstance(value[1], int):
                                continue
                        widget_values.append(value)

            # Build input slots for connections
            for input_name, value in node.inputs.items():
                if isinstance(value, list) and len(value) == 2:
                    source_id, source_output = value
                    if isinstance(source_id, str) and isinstance(source_output, int):
                        link_for_input = input_link_map.get((node_id, input_name))
                        input_slots.append({
                            "name": input_name,
                            "type": "*",
                            "link": link_for_input
                        })

            # Build output slots
            output_slots = []
            max_output_idx = -1
            for (src_id, src_out), _ in output_link_map.items():
                if src_id == node_id:
                    max_output_idx = max(max_output_idx, src_out)

            for out_idx in range(max_output_idx + 1):
                out_key = (node_id, out_idx)
                out_links = output_link_map.get(out_key, [])
                output_slots.append({
                    "name": f"output_{out_idx}",
                    "type": "*",
                    "links": out_links if out_links else None
                })

            node_data = {
                "id": int(node_id),
                "type": node.class_type,
                "pos": node.position,
                "size": {"0": 315, "1": 150},
                "flags": {},
                "order": int(node_id) - 1,
                "mode": 0,
                "inputs": input_slots,
                "outputs": output_slots,
                "properties": {},
                "widgets_values": widget_values
            }
            nodes.append(node_data)

        # Third pass: build links array
        # Format: [link_id, source_node_id, source_slot, target_node_id, target_slot, type]
        for node_id, node in self._nodes.items():
            input_slot_idx = 0
            for input_name, value in node.inputs.items():
                if isinstance(value, list) and len(value) == 2:
                    source_id, source_output = value
                    if isinstance(source_id, str) and isinstance(source_output, int):
                        lid = input_link_map[(node_id, input_name)]
                        links.append([
                            lid,
                            int(source_id),
                            source_output,
                            int(node_id),
                            input_slot_idx,
                            "*"
                        ])
                        input_slot_idx += 1

        return {
            "last_node_id": self._next_id - 1,
            "last_link_id": link_id - 1,
            "nodes": nodes,
            "links": links,
            "groups": [],
            "config": {},
            "extra": {},
            "version": 0.4
        }


# Global workflow state
_workflow_state: Optional[WorkflowState] = None


def get_workflow_state() -> WorkflowState:
    """Get or create the global workflow state."""
    global _workflow_state
    if _workflow_state is None:
        _workflow_state = WorkflowState()
    return _workflow_state


def new_workflow(name: str = "Untitled") -> WorkflowState:
    """Create a new empty workflow."""
    global _workflow_state
    _workflow_state = WorkflowState(name)
    return _workflow_state
