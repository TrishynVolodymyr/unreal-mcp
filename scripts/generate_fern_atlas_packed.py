"""Pack 8 confirmed fern textures into a single 4096x4096 atlas (2x4 grid).
Each cell is 1024x2048 (tall). Ferns are vertical so this maximizes fill.
Original confirmed files are NOT modified."""
from PIL import Image

ATLAS_W, ATLAS_H = 4096, 4096
COLS, ROWS = 4, 4
CELL_W = ATLAS_W // COLS   # 1024
CELL_H = ATLAS_H // ROWS   # 1024

SRC = "E:/code/unreal-mcp/MCPGameProject/Content/Environment/GroundCover"
OUT = f"{SRC}/T_Fern_Atlas.png"

# 8 confirmed textures in order (each 1024x1024)
FERNS = [f"T_Fern_Confirmed_{i:02d}" for i in range(1, 9)]

atlas = Image.new("RGBA", (ATLAS_W, ATLAS_H), (0, 0, 0, 0))

for idx, name in enumerate(FERNS):
    col = idx % COLS
    row = idx // COLS
    src = Image.open(f"{SRC}/{name}.png").convert("RGBA")

    # Resize from 1024x1024 to fit cell (2048x1024)
    # Ferns are taller than wide, so fit to cell height and center horizontally
    src_w, src_h = src.size
    scale = min(CELL_W / src_w, CELL_H / src_h)
    new_w = int(src_w * scale)
    new_h = int(src_h * scale)
    resized = src.resize((new_w, new_h), Image.LANCZOS)

    # Center in cell
    x_off = col * CELL_W + (CELL_W - new_w) // 2
    y_off = row * CELL_H + (CELL_H - new_h) // 2

    atlas.paste(resized, (x_off, y_off))
    print(f"  [{row},{col}] {name} -> {new_w}x{new_h} at ({x_off},{y_off})")

atlas.save(OUT)
print(f"\nAtlas saved: {OUT}")
print(f"Size: {ATLAS_W}x{ATLAS_H}, Grid: {COLS}x{ROWS}, Cell: {CELL_W}x{CELL_H}")
