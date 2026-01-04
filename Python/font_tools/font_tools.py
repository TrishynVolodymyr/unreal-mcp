"""Font Tools for Unreal MCP."""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_font_tools(mcp: FastMCP):
    """Register font tools with the MCP server."""

    @mcp.tool()
    def create_font_face(
        ctx: Context,
        font_name: str,
        source_texture: str,
        path: str = "/Game/Fonts",
        use_sdf: bool = True,
        distance_field_spread: int = 4,
        font_metrics: Dict[str, Any] = None
    ) -> Dict[str, Any]:
        """
        Create a new FontFace asset from an SDF texture.

        FontFace assets are required to use custom fonts in UMG widgets. This tool creates
        a FontFace that references an SDF (Signed Distance Field) texture for high-quality
        text rendering at any scale.

        Args:
            font_name: Name of the FontFace asset to create (e.g., "FF_DarkFantasy_Regular")
            source_texture: Path to the source SDF texture (e.g., "/Game/Fonts/DarkFantasy_Regular_sdf")
            path: Path where to create the FontFace asset (default: "/Game/Fonts")
            use_sdf: Whether this font uses SDF rendering (default: True)
            distance_field_spread: SDF spread value in pixels (default: 4)
            font_metrics: Optional font metrics dict with keys:
                - ascender: Font ascender value
                - descender: Font descender value
                - line_height: Line height value

        Example:
            create_font_face(
                font_name="FF_DarkFantasy_Regular",
                source_texture="/Game/Fonts/DarkFantasy_Regular_sdf"
            )
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "font_name": font_name,
                "source_texture": source_texture,
                "path": path,
                "use_sdf": use_sdf,
                "distance_field_spread": distance_field_spread
            }

            if font_metrics:
                params["font_metrics"] = font_metrics

            logger.info(f"Creating FontFace '{font_name}' from texture '{source_texture}'")
            response = unreal.send_command("create_font_face", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Create FontFace response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating FontFace: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_font_face_properties(
        ctx: Context,
        font_path: str,
        properties: Dict[str, Any]
    ) -> Dict[str, Any]:
        """
        Set properties on an existing FontFace asset.

        Args:
            font_path: Path to the FontFace asset (e.g., "/Game/Fonts/FF_DarkFantasy_Regular")
            properties: Dictionary of properties to set. Supported properties:
                - Hinting: Font hinting mode ("Default", "Auto", "AutoLight", "Monochrome", "None")
                - LoadingPolicy: Font loading policy ("LazyLoad", "Stream", "Inline")
                - Ascender: Font ascender value (float)
                - Descender: Font descender value (float)
                - SubFaceIndex: Sub-face index for multi-face fonts (int)

        Example:
            set_font_face_properties(
                font_path="/Game/Fonts/FF_DarkFantasy_Regular",
                properties={"Hinting": "None"}
            )
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "font_path": font_path,
                "properties": properties
            }

            logger.info(f"Setting properties on FontFace '{font_path}'")
            response = unreal.send_command("set_font_face_properties", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set FontFace properties response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting FontFace properties: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def get_font_face_metadata(
        ctx: Context,
        font_path: str
    ) -> Dict[str, Any]:
        """
        Get metadata about an existing FontFace asset.

        Args:
            font_path: Path to the FontFace asset (e.g., "/Game/Fonts/FF_DarkFantasy_Regular")

        Returns:
            Dict with: font_path, font_name, source_filename, hinting, loading_policy,
            ascender, descender, sub_face_index

        Example:
            get_font_face_metadata(font_path="/Game/Fonts/FF_DarkFantasy_Regular")
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "font_path": font_path
            }

            logger.info(f"Getting metadata for FontFace '{font_path}'")
            response = unreal.send_command("get_font_face_metadata", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Get FontFace metadata response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error getting FontFace metadata: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def create_offline_font(
        ctx: Context,
        font_name: str,
        texture_path: str,
        metrics_file_path: str,
        path: str = "/Game/Fonts"
    ) -> Dict[str, Any]:
        """
        Create an offline (bitmap/SDF atlas) font from a texture and metrics JSON file.

        This tool creates a UFont asset with offline caching that uses a pre-rendered
        texture atlas (like SDF fonts) instead of TTF font data. The metrics JSON file
        (on disk, NOT an Unreal asset) provides character positions and font metrics.

        Args:
            font_name: Name of the font asset to create (e.g., "Font_DarkFantasy_Regular")
            texture_path: Path to the SDF texture atlas (e.g., "/Game/Fonts/DarkFantasy_Regular_sdf")
            metrics_file_path: Absolute file path to the metrics JSON file on disk (NOT an Unreal asset path).
                The JSON file should have the following structure:
                - atlasWidth: Width of the texture atlas in pixels
                - atlasHeight: Height of the texture atlas in pixels
                - lineHeight: Line height in pixels
                - baseline: Baseline position from top in pixels
                - characters: Object mapping character codes to glyph data:
                    - u, v: Normalized UV coordinates (0-1)
                    - width, height: Glyph dimensions in pixels
                    - xOffset, yOffset: Positioning offsets
                    - xAdvance: Horizontal advance for cursor
            path: Path where to create the font asset (default: "/Game/Fonts")

        Example:
            create_offline_font(
                font_name="Font_DarkFantasy_Regular",
                texture_path="/Game/Fonts/DarkFantasy_Regular_sdf",
                metrics_file_path="E:/code/unreal-mcp/Docs/fonts/DarkFantasy_Regular_metrics.json"
            )
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "font_name": font_name,
                "texture_path": texture_path,
                "metrics_file_path": metrics_file_path,
                "path": path
            }

            logger.info(f"Creating offline font '{font_name}' from texture '{texture_path}' with metrics from '{metrics_file_path}'")
            response = unreal.send_command("create_offline_font", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Create offline font response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating offline font: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def get_font_metadata(
        ctx: Context,
        font_path: str
    ) -> Dict[str, Any]:
        """
        Get metadata about an existing UFont asset.

        Args:
            font_path: Path to the font asset (e.g., "/Game/Fonts/Font_DarkFantasy_Regular")

        Returns:
            Dict with: font_path, font_name, cache_type, em_scale, ascent, descent,
            leading, kerning, scaling_factor, legacy_font_size, character_count,
            texture_count, is_remapped

        Example:
            get_font_metadata(font_path="/Game/Fonts/Font_DarkFantasy_Regular")
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "font_path": font_path
            }

            logger.info(f"Getting metadata for font '{font_path}'")
            response = unreal.send_command("get_font_metadata", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Get font metadata response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error getting font metadata: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def create_font(
        ctx: Context,
        font_name: str,
        source_type: str,
        ttf_file_path: str = None,
        sdf_texture: str = None,
        atlas_texture: str = None,
        metrics_file: str = None,
        path: str = "/Game/Fonts",
        use_sdf: bool = True,
        distance_field_spread: int = 32,
        font_metrics: Dict[str, Any] = None
    ) -> Dict[str, Any]:
        """
        Unified font creation command supporting multiple source types.

        This is the recommended tool for creating font assets in Unreal Engine.
        It consolidates TTF import, SDF texture, and offline bitmap font workflows.

        Args:
            font_name: Name of the font asset to create (e.g., "Font_CinzelTorn")
            source_type: Type of font source. One of:
                - "ttf": Import an external TTF file as a FontFace asset
                - "sdf_texture": Create a FontFace from an SDF texture
                - "offline": Create a UFont from a texture atlas and metrics JSON
            ttf_file_path: (Required for "ttf") Absolute path to the TTF file on disk
                Example: "E:/code/unreal-mcp/Python/font_generation/Cinzel-Torn.ttf"
            sdf_texture: (Optional for "sdf_texture") Path to the SDF texture in UE
                Example: "/Game/Fonts/DarkFantasy_sdf"
            atlas_texture: (Required for "offline") Path to the texture atlas in UE
                Example: "/Game/Fonts/DarkFantasy_atlas"
            metrics_file: (Required for "offline") Absolute path to metrics JSON on disk
                Example: "E:/fonts/DarkFantasy_metrics.json"
            path: Path where to create the font asset (default: "/Game/Fonts")
            use_sdf: (For "sdf_texture") Whether to use SDF rendering (default: True)
            distance_field_spread: (For "sdf_texture") SDF spread value (default: 32)
            font_metrics: Optional font metrics dict with keys:
                - ascender: Font ascender value
                - descender: Font descender value
                - line_height: Line height value

        Example:
            create_font(
                font_name="Font_CinzelTorn",
                source_type="ttf",
                ttf_file_path="E:/code/unreal-mcp/Python/font_generation/Cinzel-Torn.ttf"
            )
        """
        from utils.unreal_connection_utils import get_unreal_engine_connection as get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            # Validate source_type
            valid_source_types = ["ttf", "sdf_texture", "offline"]
            if source_type not in valid_source_types:
                return {
                    "success": False,
                    "message": f"Invalid source_type '{source_type}'. Must be one of: {valid_source_types}"
                }

            # Build params based on source_type
            params = {
                "font_name": font_name,
                "source_type": source_type,
                "path": path
            }

            if source_type == "ttf":
                if not ttf_file_path:
                    return {"success": False, "message": "ttf_file_path is required for source_type='ttf'"}
                params["ttf_file_path"] = ttf_file_path
                if font_metrics:
                    params["font_metrics"] = font_metrics

            elif source_type == "sdf_texture":
                if sdf_texture:
                    params["sdf_texture"] = sdf_texture
                params["use_sdf"] = use_sdf
                params["distance_field_spread"] = distance_field_spread
                if font_metrics:
                    params["font_metrics"] = font_metrics

            elif source_type == "offline":
                if not atlas_texture:
                    return {"success": False, "message": "atlas_texture is required for source_type='offline'"}
                if not metrics_file:
                    return {"success": False, "message": "metrics_file is required for source_type='offline'"}
                params["atlas_texture"] = atlas_texture
                params["metrics_file"] = metrics_file

            logger.info(f"Creating font '{font_name}' with source_type='{source_type}'")
            response = unreal.send_command("create_font", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Create font response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating font: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    logger.info("Font tools registered successfully")
