#!/usr/bin/env python3
"""
Sound MCP Server

This server provides MCP tools for audio/sound operations in Unreal Engine,
including Sound Waves, Sound Cues, MetaSounds, and audio spatialization.
"""

import asyncio
import json
from typing import Any, Dict, List

from fastmcp import FastMCP

# Initialize FastMCP app
app = FastMCP("Sound MCP Server")

# TCP connection settings
TCP_HOST = "127.0.0.1"
TCP_PORT = 55557


async def send_tcp_command(command_type: str, params: Dict[str, Any]) -> Dict[str, Any]:
    """Send a command to the Unreal Engine TCP server."""
    try:
        # Create the command payload
        command_data = {
            "type": command_type,
            "params": params
        }

        # Convert to JSON string
        json_data = json.dumps(command_data)

        # Connect to TCP server
        reader, writer = await asyncio.open_connection(TCP_HOST, TCP_PORT)

        # Send command
        writer.write(json_data.encode('utf-8'))
        writer.write(b'\n')  # Add newline delimiter
        await writer.drain()

        # Read response
        response_data = await reader.read(49152)  # Read up to 48KB
        response_str = response_data.decode('utf-8').strip()

        # Close connection
        writer.close()
        await writer.wait_closed()

        # Parse JSON response
        if response_str:
            try:
                return json.loads(response_str)
            except json.JSONDecodeError as json_err:
                return {"success": False, "error": f"JSON decode error: {str(json_err)}, Response: {response_str[:200]}"}
        else:
            return {"success": False, "error": "Empty response from server"}

    except Exception as e:
        return {"success": False, "error": f"TCP communication error: {str(e)}"}


# ============================================================================
# Sound Wave Operations (Phase 1)
# ============================================================================

@app.tool()
async def import_sound_file(
    source_file_path: str,
    asset_name: str,
    folder_path: str = "/Game/Audio"
) -> Dict[str, Any]:
    """
    Import an audio file from disk into the Unreal Engine project.

    Supports common audio formats: WAV, MP3, OGG, FLAC, AIFF.

    Args:
        source_file_path: Absolute path to the audio file on disk
            (e.g., "C:/Users/User/Downloads/sound.mp3")
        asset_name: Name for the imported sound wave asset (e.g., "SW_RainLoop")
        folder_path: Content folder path for the imported asset (default: "/Game/Audio")

    Returns:
        Dictionary containing:
        - success: Whether the import was successful
        - path: Full asset path of the imported sound wave
        - name: Name of the imported asset
        - message: Success/error message

    Example:
        import_sound_file(
            source_file_path="C:/Users/User/Downloads/rain-loop.mp3",
            asset_name="SW_RainLoop",
            folder_path="/Game/Audio/Ambient"
        )
    """
    params = {
        "source_file_path": source_file_path,
        "asset_name": asset_name,
        "folder_path": folder_path
    }
    return await send_tcp_command("import_sound_file", params)


@app.tool()
async def get_sound_wave_metadata(sound_wave_path: str) -> Dict[str, Any]:
    """
    Get metadata about a sound wave asset.

    Returns duration, sample rate, channels, volume, pitch, and streaming info.

    Args:
        sound_wave_path: Full path to the sound wave asset (e.g., "/Game/Audio/Sounds/SW_Explosion")

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - name: Name of the sound wave
        - path: Asset path
        - duration: Duration in seconds
        - sample_rate: Sample rate in Hz
        - num_channels: Number of audio channels
        - is_looping: Whether the sound loops
        - volume: Base volume multiplier
        - pitch: Base pitch multiplier
        - is_streaming: Whether the sound uses streaming

    Example:
        get_sound_wave_metadata(sound_wave_path="/Game/Audio/Sounds/SW_Explosion")
    """
    params = {
        "sound_wave_path": sound_wave_path
    }
    return await send_tcp_command("get_sound_wave_metadata", params)


@app.tool()
async def set_sound_wave_properties(
    sound_wave_path: str,
    looping: bool = False,
    volume: float = 1.0,
    pitch: float = 1.0
) -> Dict[str, Any]:
    """
    Set properties on a sound wave asset.

    Args:
        sound_wave_path: Full path to the sound wave asset
        looping: Whether the sound should loop (default: False)
        volume: Base volume multiplier 0.0-4.0 (default: 1.0)
        pitch: Base pitch multiplier 0.1-4.0 (default: 1.0)

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - message: Success/error message

    Example:
        set_sound_wave_properties(
            sound_wave_path="/Game/Audio/Music/BGM_MainTheme",
            looping=True,
            volume=0.8
        )
    """
    params = {
        "sound_wave_path": sound_wave_path,
        "looping": looping,
        "volume": volume,
        "pitch": pitch
    }
    return await send_tcp_command("set_sound_wave_properties", params)


# ============================================================================
# Audio Component Operations (Phase 1)
# ============================================================================

@app.tool()
async def spawn_ambient_sound(
    sound_path: str,
    actor_name: str,
    x: float = 0.0,
    y: float = 0.0,
    z: float = 0.0,
    auto_activate: bool = True,
    attenuation_path: str = ""
) -> Dict[str, Any]:
    """
    Spawn an ambient sound actor in the level.

    Creates an AAmbientSound actor with the specified sound asset.

    Args:
        sound_path: Path to the sound asset (SoundWave, SoundCue, or MetaSound)
        actor_name: Name for the spawned actor
        x: X position in world space (default: 0.0)
        y: Y position in world space (default: 0.0)
        z: Z position in world space (default: 0.0)
        auto_activate: Whether sound plays automatically when spawned (default: True)
        attenuation_path: Optional path to USoundAttenuation asset for 3D spatialization

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - actor_name: Name of the spawned actor
        - location: World location {x, y, z}
        - message: Success/error message

    Example:
        spawn_ambient_sound(
            sound_path="/Game/Audio/Ambient/SW_Wind",
            actor_name="WindSound_Forest",
            x=1000, y=500, z=100,
            attenuation_path="/Game/Audio/Attenuation/AT_Ambient"
        )
    """
    params = {
        "sound_path": sound_path,
        "actor_name": actor_name,
        "location": {"x": x, "y": y, "z": z},
        "auto_activate": auto_activate
    }
    if attenuation_path:
        params["attenuation_path"] = attenuation_path

    return await send_tcp_command("spawn_ambient_sound", params)


@app.tool()
async def play_sound_at_location(
    sound_path: str,
    x: float,
    y: float,
    z: float,
    volume_multiplier: float = 1.0,
    pitch_multiplier: float = 1.0
) -> Dict[str, Any]:
    """
    Play a one-shot sound at a specific world location.

    The sound will play once and then be destroyed. This is ideal for
    sound effects like explosions, impacts, or UI feedback.

    Args:
        sound_path: Path to the sound asset
        x: X position in world space
        y: Y position in world space
        z: Z position in world space
        volume_multiplier: Volume multiplier (default: 1.0)
        pitch_multiplier: Pitch multiplier (default: 1.0)

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - message: Success/error message

    Example:
        play_sound_at_location(
            sound_path="/Game/Audio/SFX/SW_Explosion",
            x=500, y=200, z=50,
            volume_multiplier=1.5
        )
    """
    params = {
        "sound_path": sound_path,
        "location": {"x": x, "y": y, "z": z},
        "volume_multiplier": volume_multiplier,
        "pitch_multiplier": pitch_multiplier
    }
    return await send_tcp_command("play_sound_at_location", params)


# ============================================================================
# Sound Attenuation Operations (Phase 1)
# ============================================================================

@app.tool()
async def create_sound_attenuation(
    asset_name: str,
    folder_path: str = "/Game/Audio",
    inner_radius: float = 400.0,
    falloff_distance: float = 3600.0,
    attenuation_function: str = "Linear",
    spatialize: bool = True
) -> Dict[str, Any]:
    """
    Create a sound attenuation settings asset for 3D audio spatialization.

    Sound attenuation controls how audio volume decreases with distance from
    the listener, creating realistic 3D spatial audio.

    Args:
        asset_name: Name for the attenuation asset (e.g., "AT_Ambient")
        folder_path: Content folder path (default: "/Game/Audio")
        inner_radius: Distance at which sound is at full volume in cm (default: 400.0)
        falloff_distance: Distance over which sound fades to silence in cm (default: 3600.0)
        attenuation_function: Falloff curve type (default: "Linear")
            - "Linear": Linear falloff (simple, predictable)
            - "Logarithmic": Logarithmic falloff (more natural for large spaces)
            - "Inverse": Inverse distance falloff
            - "LogReverse": Reversed logarithmic
            - "NaturalSound": Physically accurate falloff
        spatialize: Whether to enable 3D spatialization (default: True)

    Returns:
        Dictionary containing:
        - success: Whether creation was successful
        - path: Full asset path of created attenuation
        - message: Success/error message

    Example:
        create_sound_attenuation(
            asset_name="AT_Footsteps",
            folder_path="/Game/Audio/Attenuation",
            inner_radius=100,
            falloff_distance=1500,
            attenuation_function="NaturalSound"
        )
    """
    params = {
        "asset_name": asset_name,
        "folder_path": folder_path,
        "inner_radius": inner_radius,
        "falloff_distance": falloff_distance,
        "attenuation_function": attenuation_function,
        "spatialize": spatialize
    }
    return await send_tcp_command("create_sound_attenuation", params)


@app.tool()
async def set_attenuation_property(
    attenuation_path: str,
    property_name: str,
    value: Any
) -> Dict[str, Any]:
    """
    Set a property on a sound attenuation settings asset.

    Args:
        attenuation_path: Path to the attenuation asset
        property_name: Name of the property to set
            - "inner_radius": Inner radius in cm
            - "falloff_distance": Falloff distance in cm
            - "spatialize": Whether to spatialize (bool)
            - "attenuate": Whether to attenuate (bool)
        value: Value to set (type depends on property)

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - message: Success/error message

    Example:
        set_attenuation_property(
            attenuation_path="/Game/Audio/Attenuation/AT_Ambient",
            property_name="falloff_distance",
            value=5000.0
        )
    """
    params = {
        "attenuation_path": attenuation_path,
        "property_name": property_name,
        "value": value
    }
    return await send_tcp_command("set_attenuation_property", params)


# ============================================================================
# Sound Cue Operations (Phase 2)
# ============================================================================

@app.tool()
async def create_sound_cue(
    asset_name: str,
    folder_path: str = "/Game/Audio",
    initial_sound_wave: str = ""
) -> Dict[str, Any]:
    """
    Create a new Sound Cue asset.

    Sound Cues are node-based audio assets that can layer, mix, randomize,
    and modulate sound waves for complex audio effects.

    Args:
        asset_name: Name for the Sound Cue (e.g., "SC_Footsteps")
        folder_path: Content folder path (default: "/Game/Audio")
        initial_sound_wave: Optional path to an initial sound wave to add

    Returns:
        Dictionary containing:
        - success: Whether creation was successful
        - path: Full asset path of created Sound Cue
        - message: Success/error message

    Example:
        create_sound_cue(
            asset_name="SC_Footsteps_Grass",
            folder_path="/Game/Audio/SoundCues",
            initial_sound_wave="/Game/Audio/Sounds/SW_Footstep_Grass_01"
        )
    """
    params = {
        "asset_name": asset_name,
        "folder_path": folder_path
    }
    if initial_sound_wave:
        params["initial_sound_wave"] = initial_sound_wave

    return await send_tcp_command("create_sound_cue", params)


@app.tool()
async def get_sound_cue_metadata(sound_cue_path: str) -> Dict[str, Any]:
    """
    Get metadata about a Sound Cue asset.

    Args:
        sound_cue_path: Full path to the Sound Cue asset

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - name: Name of the Sound Cue
        - path: Asset path
        - duration: Maximum duration in seconds
        - max_distance: Maximum audible distance
        - volume_multiplier: Base volume multiplier
        - pitch_multiplier: Base pitch multiplier
        - first_node: Name of the root node connected to output
        - nodes: List of nodes in the cue graph with type-specific properties
        - connections: List of node connections
        - node_count: Total number of nodes

    Example:
        get_sound_cue_metadata(sound_cue_path="/Game/Audio/SoundCues/SC_Explosion")
    """
    params = {
        "sound_cue_path": sound_cue_path
    }
    return await send_tcp_command("get_sound_cue_metadata", params)


@app.tool()
async def add_sound_cue_node(
    sound_cue_path: str,
    node_type: str,
    sound_wave_path: str = "",
    pos_x: int = 0,
    pos_y: int = 0
) -> Dict[str, Any]:
    """
    Add a node to a Sound Cue graph.

    Args:
        sound_cue_path: Path to the Sound Cue
        node_type: Type of node to add:
            - "WavePlayer": Plays a sound wave
            - "Mixer": Mixes multiple inputs together
            - "Random": Randomly selects from inputs
            - "Modulator": Modulates volume/pitch
            - "Looping": Loops the input sound
            - "Delay": Adds delay before playing
            - "Concatenator": Plays inputs in sequence
            - "Attenuation": Applies distance attenuation
        sound_wave_path: Path to sound wave (required for WavePlayer)
        pos_x: X position in the graph editor
        pos_y: Y position in the graph editor

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - node_id: ID of the created node
        - message: Success/error message

    Example:
        add_sound_cue_node(
            sound_cue_path="/Game/Audio/SoundCues/SC_Footsteps",
            node_type="WavePlayer",
            sound_wave_path="/Game/Audio/Sounds/SW_Footstep_01"
        )
    """
    params = {
        "sound_cue_path": sound_cue_path,
        "node_type": node_type,
        "pos_x": pos_x,
        "pos_y": pos_y
    }
    if sound_wave_path:
        params["sound_wave_path"] = sound_wave_path

    return await send_tcp_command("add_sound_cue_node", params)


@app.tool()
async def connect_sound_cue_nodes(
    sound_cue_path: str,
    source_node_id: str,
    target_node_id: str,
    source_pin_index: int = 0,
    target_pin_index: int = 0
) -> Dict[str, Any]:
    """
    Connect two nodes in a Sound Cue graph.

    Args:
        sound_cue_path: Path to the Sound Cue
        source_node_id: ID of the source node (output)
        target_node_id: ID of the target node (input), or "Output" for final output
        source_pin_index: Source pin index (default: 0)
        target_pin_index: Target pin index (default: 0)

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - message: Success/error message

    Example:
        connect_sound_cue_nodes(
            sound_cue_path="/Game/Audio/SoundCues/SC_Footsteps",
            source_node_id="SoundNodeWavePlayer_0",
            target_node_id="Output"
        )
    """
    params = {
        "sound_cue_path": sound_cue_path,
        "source_node_id": source_node_id,
        "target_node_id": target_node_id,
        "source_pin_index": source_pin_index,
        "target_pin_index": target_pin_index
    }
    return await send_tcp_command("connect_sound_cue_nodes", params)


@app.tool()
async def set_sound_cue_node_property(
    sound_cue_path: str,
    node_id: str,
    property_name: str,
    value: Any
) -> Dict[str, Any]:
    """
    Set a property on a Sound Cue node.

    Args:
        sound_cue_path: Path to the Sound Cue
        node_id: ID of the node to modify
        property_name: Name of the property to set
            WavePlayer: "looping", "sound_wave"
            Mixer: "input_volume" (array of floats)
            Random: "weights" (array of floats), "randomize_without_replacement"
            Modulator: "pitch_min", "pitch_max", "volume_min", "volume_max"
            Looping: "loop_count", "loop_indefinitely"
        value: Value to set (type depends on property)

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - message: Success/error message

    Example:
        set_sound_cue_node_property(
            sound_cue_path="/Game/Audio/SoundCues/SC_Footsteps",
            node_id="SoundNodeModulator_0",
            property_name="pitch_max",
            value=1.2
        )
    """
    params = {
        "sound_cue_path": sound_cue_path,
        "node_id": node_id,
        "property_name": property_name,
        "value": value
    }
    return await send_tcp_command("set_sound_cue_node_property", params)


@app.tool()
async def remove_sound_cue_node(
    sound_cue_path: str,
    node_id: str
) -> Dict[str, Any]:
    """
    Remove a node from a Sound Cue graph.

    Removes the specified node and disconnects it from all connected nodes.

    Args:
        sound_cue_path: Path to the Sound Cue
        node_id: ID of the node to remove

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - message: Success/error message

    Example:
        remove_sound_cue_node(
            sound_cue_path="/Game/Audio/SoundCues/SC_Footsteps",
            node_id="SoundNodeWavePlayer_2"
        )
    """
    params = {
        "sound_cue_path": sound_cue_path,
        "node_id": node_id
    }
    return await send_tcp_command("remove_sound_cue_node", params)


@app.tool()
async def compile_sound_cue(sound_cue_path: str) -> Dict[str, Any]:
    """
    Compile and validate a Sound Cue.

    Compiles the Sound Cue graph and caches aggregate values like duration
    and max distance. Call this after making changes to ensure the cue
    is properly configured.

    Args:
        sound_cue_path: Path to the Sound Cue

    Returns:
        Dictionary containing:
        - success: Whether compilation was successful
        - message: Success/error message

    Example:
        compile_sound_cue(sound_cue_path="/Game/Audio/SoundCues/SC_Footsteps")
    """
    params = {
        "sound_cue_path": sound_cue_path
    }
    return await send_tcp_command("compile_sound_cue", params)


# ============================================================================
# Sound Class/Mix Operations (Phase 4 - Music System)
# ============================================================================

@app.tool()
async def create_sound_class(
    asset_name: str,
    folder_path: str = "/Game/Audio",
    parent_class_path: str = "",
    default_volume: float = 1.0
) -> Dict[str, Any]:
    """
    Create a Sound Class asset for audio categorization.

    Sound Classes group sounds together for volume control, ducking,
    and other audio mixing purposes (e.g., Music, SFX, Voice, Ambient).

    Args:
        asset_name: Name for the Sound Class (e.g., "SC_Music")
        folder_path: Content folder path (default: "/Game/Audio")
        parent_class_path: Optional parent class path for inheritance
        default_volume: Default volume multiplier 0.0-1.0 (default: 1.0)

    Returns:
        Dictionary containing:
        - success: Whether creation was successful
        - path: Full asset path of created Sound Class
        - message: Success/error message

    Example:
        create_sound_class(
            asset_name="SC_Music",
            folder_path="/Game/Audio/Classes",
            default_volume=0.8
        )

    Note: This feature is planned for Phase 4 implementation.
    """
    params = {
        "asset_name": asset_name,
        "folder_path": folder_path,
        "default_volume": default_volume
    }
    if parent_class_path:
        params["parent_class_path"] = parent_class_path

    return await send_tcp_command("create_sound_class", params)


@app.tool()
async def create_sound_mix(
    asset_name: str,
    folder_path: str = "/Game/Audio"
) -> Dict[str, Any]:
    """
    Create a Sound Mix asset for audio mixing.

    Sound Mixes can modify the volume/pitch of Sound Classes, enabling
    dynamic audio mixing (e.g., lowering music during dialogue).

    Args:
        asset_name: Name for the Sound Mix (e.g., "SM_DialogueDuck")
        folder_path: Content folder path (default: "/Game/Audio")

    Returns:
        Dictionary containing:
        - success: Whether creation was successful
        - path: Full asset path of created Sound Mix
        - message: Success/error message

    Example:
        create_sound_mix(
            asset_name="SM_DialogueDuck",
            folder_path="/Game/Audio/Mixes"
        )

    Note: This feature is planned for Phase 4 implementation.
    """
    params = {
        "asset_name": asset_name,
        "folder_path": folder_path
    }
    return await send_tcp_command("create_sound_mix", params)


@app.tool()
async def add_sound_mix_modifier(
    sound_mix_path: str,
    sound_class_path: str,
    volume_adjust: float = 0.0,
    pitch_adjust: float = 0.0
) -> Dict[str, Any]:
    """
    Add a sound class modifier to a sound mix.

    This allows the Sound Mix to modify the volume/pitch of a specific
    Sound Class when the mix is active.

    Args:
        sound_mix_path: Path to the Sound Mix
        sound_class_path: Path to the Sound Class to modify
        volume_adjust: Volume adjustment -1.0 to 1.0 (default: 0.0)
        pitch_adjust: Pitch adjustment -1.0 to 1.0 (default: 0.0)

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - message: Success/error message

    Example:
        add_sound_mix_modifier(
            sound_mix_path="/Game/Audio/Mixes/SM_DialogueDuck",
            sound_class_path="/Game/Audio/Classes/SC_Music",
            volume_adjust=-0.5  # Lower music by 50%
        )

    Note: This feature is planned for Phase 4 implementation.
    """
    params = {
        "sound_mix_path": sound_mix_path,
        "sound_class_path": sound_class_path,
        "volume_adjust": volume_adjust,
        "pitch_adjust": pitch_adjust
    }
    return await send_tcp_command("add_sound_mix_modifier", params)


# ============================================================================
# MetaSound Operations (Phase 3)
# ============================================================================

@app.tool()
async def create_metasound_source(
    asset_name: str,
    folder_path: str = "/Game/Audio/MetaSounds",
    output_format: str = "Stereo",
    is_one_shot: bool = True
) -> Dict[str, Any]:
    """
    Create a new MetaSound Source asset.

    MetaSounds are Unreal's next-generation audio system providing
    node-based procedural audio creation with DSP nodes, triggers,
    and real-time parameter control.

    Args:
        asset_name: Name for the MetaSound (e.g., "MS_Laser")
        folder_path: Content folder path (default: "/Game/Audio/MetaSounds")
        output_format: Audio output format (default: "Stereo")
            - "Mono": Single channel
            - "Stereo": Two channels (L/R)
            - "Quad": Four channels
            - "FiveDotOne" or "5.1": 5.1 surround
            - "SevenDotOne" or "7.1": 7.1 surround
        is_one_shot: Whether this is a one-shot sound (auto-terminates) (default: True)

    Returns:
        Dictionary containing:
        - success: Whether creation was successful
        - path: Full asset path of created MetaSound
        - name: Asset name
        - output_format: The output format used
        - is_one_shot: Whether it's configured as one-shot
        - message: Success/error message

    Example:
        create_metasound_source(
            asset_name="MS_LaserShot",
            folder_path="/Game/Audio/MetaSounds/SFX",
            output_format="Stereo",
            is_one_shot=True
        )
    """
    params = {
        "asset_name": asset_name,
        "folder_path": folder_path,
        "output_format": output_format,
        "is_one_shot": is_one_shot
    }
    return await send_tcp_command("create_metasound_source", params)


@app.tool()
async def get_metasound_metadata(metasound_path: str) -> Dict[str, Any]:
    """
    Get metadata about a MetaSound asset.

    Args:
        metasound_path: Full path to the MetaSound asset

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - name: Name of the MetaSound
        - path: Asset path
        - class_name: The graph class name
        - inputs: List of graph inputs with name, type, node_id, vertex_id
        - outputs: List of graph outputs with name, type, node_id, vertex_id
        - nodes: List of nodes with id, class_name, namespace, inputs, outputs
        - edges: List of connections between nodes
        - node_count: Total number of nodes
        - edge_count: Total number of edges

    Example:
        get_metasound_metadata(metasound_path="/Game/Audio/MetaSounds/MS_Laser")
    """
    params = {
        "metasound_path": metasound_path
    }
    return await send_tcp_command("get_metasound_metadata", params)


@app.tool()
async def add_metasound_node(
    metasound_path: str,
    node_class_name: str,
    node_namespace: str = "UE",
    node_variant: str = "",
    pos_x: int = 0,
    pos_y: int = 0
) -> Dict[str, Any]:
    """
    Add a node to a MetaSound graph.

    Args:
        metasound_path: Path to the MetaSound
        node_class_name: Name of the node class to add. Common nodes include:
            - Oscillators: "Sine", "Saw", "Square", "Triangle", "Noise"
            - Envelopes: "AD Envelope", "ADSR Envelope"
            - Filters: "One Pole Low Pass Filter", "Biquad Filter"
            - Math: "Add", "Multiply", "Subtract", "Divide"
            - Triggers: "Trigger Repeat", "Trigger Delay", "Trigger Counter"
            - Audio: "Stereo Mixer", "Gain"
        node_namespace: Node namespace (default: "UE" for built-in nodes)
        node_variant: Node variant (e.g., "Audio" for oscillator/audio nodes, empty for trigger nodes)
        pos_x: X position in the graph editor
        pos_y: Y position in the graph editor

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - node_id: GUID of the created node (use this to connect nodes)
        - node_class_name: The class name used
        - node_namespace: The namespace used
        - node_variant: The variant used
        - message: Success/error message

    Example:
        add_metasound_node(
            metasound_path="/Game/Audio/MetaSounds/MS_Laser",
            node_class_name="Sine",
            node_namespace="UE",
            node_variant="Audio"  # Required for oscillator nodes
        )
    """
    params = {
        "metasound_path": metasound_path,
        "node_class_name": node_class_name,
        "node_namespace": node_namespace,
        "node_variant": node_variant,
        "pos_x": pos_x,
        "pos_y": pos_y
    }
    return await send_tcp_command("add_metasound_node", params)


@app.tool()
async def connect_metasound_nodes(
    metasound_path: str,
    source_node_id: str,
    source_pin_name: str,
    target_node_id: str,
    target_pin_name: str
) -> Dict[str, Any]:
    """
    Connect two nodes in a MetaSound graph.

    Args:
        metasound_path: Path to the MetaSound
        source_node_id: GUID of the source node (from add_metasound_node or get_metasound_metadata)
        source_pin_name: Name of the output pin on the source node (e.g., "Audio", "Out")
        target_node_id: GUID of the target node
        target_pin_name: Name of the input pin on the target node (e.g., "In", "Audio In")

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - message: Success/error message

    Example:
        connect_metasound_nodes(
            metasound_path="/Game/Audio/MetaSounds/MS_Laser",
            source_node_id="12345678-1234-1234-1234-123456789ABC",
            source_pin_name="Audio",
            target_node_id="ABCDEFGH-ABCD-ABCD-ABCD-ABCDEFGHIJKL",
            target_pin_name="In"
        )
    """
    params = {
        "metasound_path": metasound_path,
        "source_node_id": source_node_id,
        "source_pin_name": source_pin_name,
        "target_node_id": target_node_id,
        "target_pin_name": target_pin_name
    }
    return await send_tcp_command("connect_metasound_nodes", params)


@app.tool()
async def set_metasound_input(
    metasound_path: str,
    node_id: str,
    input_name: str,
    value: Any
) -> Dict[str, Any]:
    """
    Set an input value on a MetaSound node.

    Args:
        metasound_path: Path to the MetaSound
        node_id: GUID of the node to modify
        input_name: Name of the input pin to set
        value: Value to set (float, int, bool, or string depending on input type)

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - message: Success/error message

    Example:
        set_metasound_input(
            metasound_path="/Game/Audio/MetaSounds/MS_Laser",
            node_id="12345678-1234-1234-1234-123456789ABC",
            input_name="Frequency",
            value=440.0
        )
    """
    params = {
        "metasound_path": metasound_path,
        "node_id": node_id,
        "input_name": input_name,
        "value": value
    }
    return await send_tcp_command("set_metasound_input", params)


@app.tool()
async def add_metasound_input(
    metasound_path: str,
    input_name: str,
    data_type: str = "Float",
    default_value: str = ""
) -> Dict[str, Any]:
    """
    Add a graph input to a MetaSound.

    Graph inputs are exposed parameters that can be set from Blueprints
    or C++ at runtime.

    Args:
        metasound_path: Path to the MetaSound
        input_name: Name for the input (e.g., "Frequency", "Volume")
        data_type: Data type of the input (default: "Float")
            - "Float": Floating point number
            - "Int32" or "Int": Integer
            - "Bool" or "Boolean": True/false
            - "Trigger": Event trigger
            - "Audio": Audio signal
            - "String": Text string
        default_value: Optional default value as string

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - input_node_id: GUID of the created input node
        - input_name: Name of the input
        - data_type: Data type used
        - message: Success/error message

    Example:
        add_metasound_input(
            metasound_path="/Game/Audio/MetaSounds/MS_Laser",
            input_name="Pitch",
            data_type="Float"
        )
    """
    params = {
        "metasound_path": metasound_path,
        "input_name": input_name,
        "data_type": data_type
    }
    if default_value:
        params["default_value"] = default_value

    return await send_tcp_command("add_metasound_input", params)


@app.tool()
async def add_metasound_output(
    metasound_path: str,
    output_name: str,
    data_type: str = "Audio"
) -> Dict[str, Any]:
    """
    Add a graph output to a MetaSound.

    Graph outputs define what the MetaSound produces (usually audio).

    Args:
        metasound_path: Path to the MetaSound
        output_name: Name for the output (e.g., "Audio Out", "Trigger Out")
        data_type: Data type of the output (default: "Audio")
            - "Float": Floating point number
            - "Int32" or "Int": Integer
            - "Bool" or "Boolean": True/false
            - "Trigger": Event trigger
            - "Audio": Audio signal

    Returns:
        Dictionary containing:
        - success: Whether the operation was successful
        - output_node_id: GUID of the created output node
        - output_name: Name of the output
        - data_type: Data type used
        - message: Success/error message

    Example:
        add_metasound_output(
            metasound_path="/Game/Audio/MetaSounds/MS_Laser",
            output_name="Left",
            data_type="Audio"
        )
    """
    params = {
        "metasound_path": metasound_path,
        "output_name": output_name,
        "data_type": data_type
    }
    return await send_tcp_command("add_metasound_output", params)


@app.tool()
async def compile_metasound(metasound_path: str) -> Dict[str, Any]:
    """
    Compile and validate a MetaSound.

    Compiles the MetaSound graph and registers it for audio playback.
    Call this after making changes to ensure the MetaSound is properly
    configured and ready for use.

    Args:
        metasound_path: Path to the MetaSound

    Returns:
        Dictionary containing:
        - success: Whether compilation was successful
        - message: Success/error message

    Example:
        compile_metasound(metasound_path="/Game/Audio/MetaSounds/MS_Laser")
    """
    params = {
        "metasound_path": metasound_path
    }
    return await send_tcp_command("compile_metasound", params)


@app.tool()
async def search_metasound_palette(
    search_query: str = "",
    max_results: int = 50
) -> Dict[str, Any]:
    """
    Search the MetaSound node palette for available node classes.

    Use this tool to discover available MetaSound nodes before adding them.
    Returns the exact namespace, name, and variant needed for add_metasound_node.

    Args:
        search_query: Search term to filter nodes (searches name, namespace,
                     keywords, category). Empty string returns all nodes.
        max_results: Maximum number of results to return (default: 50)

    Returns:
        Dictionary containing:
        - success: Whether the search was successful
        - count: Number of results
        - results: Array of node class info with:
            - namespace: Node namespace (e.g., "UE")
            - name: Node name (e.g., "Sine")
            - variant: Node variant (e.g., "Audio" for oscillators)
            - display_name: Human-readable name
            - description: Node description
            - category: Category path
            - full_name: Full class name for reference
            - inputs: Array of input pins with name and type
            - outputs: Array of output pins with name and type

    Example:
        # Search for oscillator nodes
        search_metasound_palette(search_query="sine")

        # Search for envelope nodes
        search_metasound_palette(search_query="envelope")

        # List all trigger nodes
        search_metasound_palette(search_query="trigger")
    """
    params = {
        "search_query": search_query,
        "max_results": max_results
    }
    return await send_tcp_command("search_metasound_palette", params)


if __name__ == "__main__":
    app.run()
