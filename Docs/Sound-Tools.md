# Sound Tools - Unreal MCP

This document provides comprehensive information about using Sound/Audio tools through the Unreal MCP (Model Context Protocol). These tools allow you to import, create, and manage audio assets — including Sound Waves, Sound Cues, MetaSounds, Sound Attenuation, Sound Classes, and Sound Mixes — using natural language commands.

## Overview

Sound tools enable you to:
- Import audio files (WAV, MP3, OGG, FLAC, AIFF) into Unreal Engine
- Inspect and configure Sound Wave properties (looping, volume, pitch)
- Create and build Sound Cue graphs with WavePlayers, Mixers, Randoms, Modulators, and more
- Create MetaSound Source assets with procedural audio (oscillators, envelopes, filters)
- Create Sound Attenuation settings for 3D spatial audio
- Set up Sound Classes and Sound Mixes for audio categorization and ducking
- Spawn ambient sound actors in the level
- Play one-shot sounds at world locations
- Search the MetaSound node palette for available DSP nodes

## Natural Language Usage Examples

### Importing and Playing Sounds

```
"Import the rain loop MP3 from my Downloads folder as SW_RainLoop"

"Play an explosion sound at position 500, 200, 50 with volume 1.5"

"Spawn an ambient wind sound actor at the forest location with spatial attenuation"

"Set SW_RainLoop to loop with volume 0.8"
```

### Sound Cues

```
"Create a Sound Cue called SC_Footsteps_Grass with a footstep sound wave"

"Add a Random node and three WavePlayers to SC_Footsteps for footstep variation"

"Add a Modulator node with pitch between 0.8 and 1.2 for natural variation"

"Connect the WavePlayer to the Output node"
```

### MetaSounds

```
"Create a new MetaSound called MS_LaserShot as a one-shot stereo source"

"Add a Sine oscillator node and an AD Envelope to the MetaSound"

"Connect the oscillator output to the stereo mixer input"

"Add a Frequency input parameter so I can control it from Blueprints"

"Search the MetaSound palette for filter nodes"
```

### 3D Audio and Mixing

```
"Create a sound attenuation asset with 100cm inner radius and natural sound falloff"

"Create a Sound Class for music with volume 0.8"

"Create a Sound Mix that ducks music by 50% during dialogue"
```

## Tool Reference

---

### `import_sound_file`

Import an audio file from disk into the Unreal Engine project.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `source_file_path` | string | ✅ | Absolute path to the audio file on disk |
| `asset_name` | string | ✅ | Name for the imported sound wave (e.g., `"SW_RainLoop"`) |
| `folder_path` | string | | Content folder path (default: `"/Game/Audio"`) |

---

### `get_sound_wave_metadata`

Get metadata about a sound wave — duration, sample rate, channels, volume, pitch, streaming.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `sound_wave_path` | string | ✅ | Full path to the sound wave asset |

---

### `set_sound_wave_properties`

Set properties on a sound wave (looping, volume, pitch).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `sound_wave_path` | string | ✅ | Full path to the sound wave asset |
| `looping` | boolean | | Whether the sound should loop |
| `volume` | number | | Base volume multiplier 0.0-4.0 (default: 1.0) |
| `pitch` | number | | Base pitch multiplier 0.1-4.0 (default: 1.0) |

---

### `spawn_ambient_sound`

Spawn an ambient sound actor in the level.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `sound_path` | string | ✅ | Path to the sound asset (SoundWave, SoundCue, or MetaSound) |
| `actor_name` | string | ✅ | Name for the spawned actor |
| `x` | number | | X position (default: 0.0) |
| `y` | number | | Y position (default: 0.0) |
| `z` | number | | Z position (default: 0.0) |
| `auto_activate` | boolean | | Whether sound plays automatically (default: True) |
| `attenuation_path` | string | | Path to USoundAttenuation asset for 3D spatialization |

---

### `play_sound_at_location`

Play a one-shot sound at a specific world location. The sound plays once and is destroyed.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `sound_path` | string | ✅ | Path to the sound asset |
| `x` | number | ✅ | X position |
| `y` | number | ✅ | Y position |
| `z` | number | ✅ | Z position |
| `volume_multiplier` | number | | Volume multiplier (default: 1.0) |
| `pitch_multiplier` | number | | Pitch multiplier (default: 1.0) |

---

### `create_sound_attenuation`

Create a sound attenuation settings asset for 3D audio spatialization.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `asset_name` | string | ✅ | Name for the attenuation asset |
| `folder_path` | string | | Content folder path (default: `"/Game/Audio"`) |
| `inner_radius` | number | | Distance at full volume in cm (default: 400.0) |
| `falloff_distance` | number | | Fade-to-silence distance in cm (default: 3600.0) |
| `attenuation_function` | string | | `"Linear"`, `"Logarithmic"`, `"Inverse"`, `"LogReverse"`, `"NaturalSound"` |
| `spatialize` | boolean | | Enable 3D spatialization (default: True) |

---

### `set_attenuation_property`

Set a property on a sound attenuation settings asset.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `attenuation_path` | string | ✅ | Path to the attenuation asset |
| `property_name` | string | ✅ | `"inner_radius"`, `"falloff_distance"`, `"spatialize"`, `"attenuate"` |
| `value` | any | ✅ | Value to set |

---

### `create_sound_cue`

Create a new Sound Cue asset for node-based audio mixing.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `asset_name` | string | ✅ | Name for the Sound Cue |
| `folder_path` | string | | Content folder path (default: `"/Game/Audio"`) |
| `initial_sound_wave` | string | | Path to an initial sound wave to add |

---

### `get_sound_cue_metadata`

Get metadata about a Sound Cue — nodes, connections, duration, and graph structure.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `sound_cue_path` | string | ✅ | Full path to the Sound Cue asset |

---

### `add_sound_cue_node`

Add a node to a Sound Cue graph.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `sound_cue_path` | string | ✅ | Path to the Sound Cue |
| `node_type` | string | ✅ | `"WavePlayer"`, `"Mixer"`, `"Random"`, `"Modulator"`, `"Looping"`, `"Delay"`, `"Concatenator"`, `"Attenuation"` |
| `sound_wave_path` | string | | Path to sound wave (required for WavePlayer) |
| `pos_x` | number | | X position in the graph editor |
| `pos_y` | number | | Y position in the graph editor |

---

### `connect_sound_cue_nodes`

Connect two nodes in a Sound Cue graph.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `sound_cue_path` | string | ✅ | Path to the Sound Cue |
| `source_node_id` | string | ✅ | ID of the source node (output) |
| `target_node_id` | string | ✅ | ID of the target node, or `"Output"` for final output |
| `source_pin_index` | number | | Source pin index (default: 0) |
| `target_pin_index` | number | | Target pin index (default: 0) |

---

### `set_sound_cue_node_property`

Set a property on a Sound Cue node.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `sound_cue_path` | string | ✅ | Path to the Sound Cue |
| `node_id` | string | ✅ | ID of the node to modify |
| `property_name` | string | ✅ | Property name (varies by node type — e.g., `"pitch_max"`, `"volume_min"`, `"weights"`) |
| `value` | any | ✅ | Value to set |

---

### `remove_sound_cue_node`

Remove a node from a Sound Cue graph.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `sound_cue_path` | string | ✅ | Path to the Sound Cue |
| `node_id` | string | ✅ | ID of the node to remove |

---

### `compile_sound_cue`

Compile and validate a Sound Cue to cache aggregate values.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `sound_cue_path` | string | ✅ | Path to the Sound Cue |

---

### `create_sound_class`

Create a Sound Class asset for audio categorization (Music, SFX, Voice, etc.).

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `asset_name` | string | ✅ | Name for the Sound Class |
| `folder_path` | string | | Content folder path |
| `parent_class_path` | string | | Parent class path for inheritance |
| `default_volume` | number | | Default volume 0.0-1.0 (default: 1.0) |

---

### `create_sound_mix`

Create a Sound Mix asset for dynamic audio mixing.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `asset_name` | string | ✅ | Name for the Sound Mix |
| `folder_path` | string | | Content folder path |

---

### `add_sound_mix_modifier`

Add a sound class modifier to a sound mix for volume/pitch ducking.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `sound_mix_path` | string | ✅ | Path to the Sound Mix |
| `sound_class_path` | string | ✅ | Path to the Sound Class to modify |
| `volume_adjust` | number | | Volume adjustment -1.0 to 1.0 (default: 0.0) |
| `pitch_adjust` | number | | Pitch adjustment -1.0 to 1.0 (default: 0.0) |

---

### `create_metasound_source`

Create a new MetaSound Source asset for procedural audio.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `asset_name` | string | ✅ | Name for the MetaSound |
| `folder_path` | string | | Content folder path (default: `"/Game/Audio/MetaSounds"`) |
| `output_format` | string | | `"Mono"`, `"Stereo"` (default), `"Quad"`, `"5.1"`, `"7.1"` |
| `is_one_shot` | boolean | | Whether this auto-terminates (default: True) |

---

### `get_metasound_metadata`

Get metadata about a MetaSound — nodes, edges, inputs, outputs.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `metasound_path` | string | ✅ | Full path to the MetaSound asset |

---

### `add_metasound_node`

Add a node to a MetaSound graph.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `metasound_path` | string | ✅ | Path to the MetaSound |
| `node_class_name` | string | ✅ | Node class: `"Sine"`, `"Saw"`, `"Square"`, `"Noise"`, `"AD Envelope"`, `"ADSR Envelope"`, `"One Pole Low Pass Filter"`, `"Biquad Filter"`, `"Add"`, `"Multiply"`, `"Trigger Repeat"`, `"Stereo Mixer"`, `"Gain"`, etc. |
| `node_namespace` | string | | Namespace (default: `"UE"`) |
| `node_variant` | string | | Variant (e.g., `"Audio"` for oscillators) |
| `pos_x` | number | | X position in the graph |
| `pos_y` | number | | Y position in the graph |

---

### `connect_metasound_nodes`

Connect two nodes in a MetaSound graph.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `metasound_path` | string | ✅ | Path to the MetaSound |
| `source_node_id` | string | ✅ | GUID of the source node |
| `source_pin_name` | string | ✅ | Name of the output pin |
| `target_node_id` | string | ✅ | GUID of the target node |
| `target_pin_name` | string | ✅ | Name of the input pin |

---

### `set_metasound_input`

Set an input value on a MetaSound node.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `metasound_path` | string | ✅ | Path to the MetaSound |
| `node_id` | string | ✅ | GUID of the node |
| `input_name` | string | ✅ | Input pin name |
| `value` | any | ✅ | Value to set |

---

### `add_metasound_input` / `add_metasound_output`

Add graph inputs/outputs to a MetaSound for external parameter control.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `metasound_path` | string | ✅ | Path to the MetaSound |
| `input_name` / `output_name` | string | ✅ | Name for the input/output |
| `data_type` | string | | `"Float"`, `"Int32"`, `"Bool"`, `"Trigger"`, `"Audio"`, `"String"` |
| `default_value` | string | | Default value as string (inputs only) |

---

### `compile_metasound`

Compile and validate a MetaSound for audio playback.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `metasound_path` | string | ✅ | Path to the MetaSound |

---

### `search_metasound_palette`

Search the MetaSound node palette for available node classes.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `search_query` | string | | Search term (searches name, namespace, keywords, category) |
| `max_results` | number | | Maximum results (default: 50) |

## Advanced Usage Patterns

### Building a Complete Footstep Sound System

```
"Create a full footstep audio system:
1. Import three grass footstep WAV files as SW_Footstep_Grass_01, _02, _03
2. Create a Sound Cue called 'SC_Footsteps_Grass'
3. Add a Random node and three WavePlayer nodes with each footstep sound
4. Add a Modulator node with pitch range 0.85-1.15 for natural variation
5. Connect all WavePlayers to the Random node
6. Connect the Random node to the Modulator, then Modulator to Output
7. Compile the Sound Cue"
```

### Creating a 3D Ambient Soundscape

```
"Set up spatial ambient audio for a forest scene:
1. Create a sound attenuation asset 'SA_Forest' with 500cm inner radius, 5000cm falloff, and NaturalSound function
2. Import wind loop, bird chirps, and creek sounds
3. Set SW_WindLoop to loop with volume 0.6
4. Spawn an ambient sound actor 'AmbientWind' with SW_WindLoop at the forest center using SA_Forest attenuation
5. Create a Sound Cue with random bird chirps and a delay node
6. Spawn a second ambient sound actor for birds at [200, 300, 150]
7. Spawn a creek ambient at the river location with lower inner radius"
```

### Building a Procedural Laser Sound with MetaSound

```
"Create a procedural laser sound effect:
1. Create a MetaSound Source called 'MS_LaserShot' as a one-shot stereo source
2. Add a Sine oscillator and set the frequency to 880 Hz
3. Add an AD Envelope with attack 0.01 and decay 0.3
4. Add a Multiply node to combine oscillator with envelope
5. Add a Biquad Filter set to low-pass for warmth
6. Connect oscillator to filter, filter through multiply with envelope, then to Stereo Mixer output
7. Add a Float input parameter 'Frequency' with default 880 for Blueprint control
8. Compile the MetaSound"
```

## Best Practices for Natural Language Commands

### Name Audio Assets with Type Prefixes
Use consistent prefixes: *"SW_"* for Sound Waves, *"SC_"* for Sound Cues, *"MS_"* for MetaSounds, *"SA_"* for Sound Attenuation. Example: *"Import the explosion WAV as SW_Explosion_Large"*

### Inspect Metadata Before Connecting Nodes
Before wiring Sound Cue or MetaSound graphs, ask: *"Get the metadata for SC_Footsteps"* to see existing nodes and their IDs for accurate connections.

### Specify Positions for Graph Readability
When adding multiple nodes, include position hints: *"Add a Modulator node at position [400, 200] in SC_Footsteps"* — this keeps the graph layout clean and readable.

### Use Sound Classes for Category-Level Control
Group sounds by purpose: *"Create Sound Classes for Music, SFX, Voice, and Ambient"* — then use Sound Mixes to duck categories relative to each other during gameplay.

### Set Looping at the Sound Wave Level
Instead of relying on Blueprint logic: *"Set SW_RainLoop to loop"* directly on the sound wave for simpler playback management.

## Common Use Cases

### Ambient Environments
Creating looping ambient sound actors with 3D attenuation for forests, caves, cities, and interiors — placing multiple spatialized sources to build an immersive soundscape.

### Weapon and Combat Audio
Building Sound Cues with random variation and pitch modulation for gunshots, sword swings, impacts, and explosions to avoid repetitive playback.

### Procedural Sound Effects
Using MetaSounds to create synthesized audio for lasers, UI feedback, alarms, and sci-fi elements with runtime-controllable parameters.

### Music and Dialogue Systems
Setting up Sound Classes and Sound Mixes to manage music volume ducking during dialogue, fade-outs between tracks, and priority-based audio mixing.

### UI Feedback Sounds
Importing short one-shot sounds for button clicks, menu transitions, notifications, and achievement popups — played at location or as 2D sounds.

### Dynamic Audio Mixing
Creating Sound Mixes with class modifiers to dynamically adjust audio levels — e.g., ducking all SFX during cutscenes or boosting ambient sounds in exploration mode.

## Error Handling and Troubleshooting

If you encounter issues:

1. **"Sound Wave not found"**: Verify the full asset path (e.g., `/Game/Audio/SW_RainLoop`). After importing, the asset path follows the `folder_path` + `asset_name` structure.
2. **Sound Cue nodes not connecting**: Use `get_sound_cue_metadata` to get the exact node IDs. The Output node is referenced as `"Output"` — ensure source and target pin indices match the node types.
3. **MetaSound compilation errors**: MetaSounds require all audio paths to terminate at an output. Use `get_metasound_metadata` to verify all nodes are connected. Disconnected nodes or type mismatches (e.g., Float to Audio pin) cause compilation failures.
4. **3D sound not spatializing**: Ensure a Sound Attenuation asset is assigned when spawning the ambient sound. Without attenuation, sounds play as 2D regardless of world position.
5. **Imported audio sounds wrong**: Check the source file format — WAV is recommended for best quality. Verify sample rate and channel count with `get_sound_wave_metadata` after import.

## Performance Considerations

- **Use streaming for long audio files**: Music tracks and long ambient loops should use streaming loading policy to avoid loading entire files into memory. Short SFX can stay as fully loaded sound waves.
- **Limit concurrent sound sources**: Each playing sound consumes CPU for mixing. Use Sound Concurrency settings and Sound Classes to cap simultaneous instances — especially for frequently triggered SFX like footsteps.
- **Choose mono for 3D sounds**: Spatialized sounds only need mono source files. Importing stereo files for 3D audio wastes memory and can cause unexpected panning behavior.
- **Compile MetaSounds before shipping**: Uncompiled MetaSounds trigger runtime compilation on first play, causing hitches. Always call `compile_metasound` after graph changes.
