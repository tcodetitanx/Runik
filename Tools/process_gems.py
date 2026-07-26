"""Slice the 4x4 gem sheet, remove the neon pink chroma background, and emit
individual RGBA sprites into Content/Raw/gems/. Also keys the hex base tile
and copies board art for use as backgrounds."""
import os
from PIL import Image, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "Content", "SourceArt")
OUT = os.path.join(ROOT, "Content", "Raw")

GEM_NAMES = [
    "ruby", "emerald", "sapphire", "amethyst",
    "rosetear", "citrine", "aquamarine", "diamond",
    "onyx", "smokequartz", "starstone", "glacier",
    "heartstone", "cluster", "moonstone", "peridot",
]

CELL_OUT = 256


def chroma_key_pink(img):
    """Alpha out neon pink/magenta, with feathered edges and despill."""
    img = img.convert("RGBA")
    px = img.load()
    w, h = img.size
    key = px[2, 2][:3]  # sample background from corner
    kr, kg, kb = key
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            # distance to key color
            d = ((r - kr) ** 2 + (g - kg) ** 2 + (b - kb) ** 2) ** 0.5
            # generic magenta test too (background varies slightly)
            magenta = r > 150 and b > 150 and g < 0.55 * min(r, b)
            if d < 60 or (magenta and d < 140):
                px[x, y] = (0, 0, 0, 0)
            elif d < 120:
                t = (d - 60) / 60.0
                # despill: pull the magenta fringe toward neutral
                nr = int(r * t + g * (1 - t))
                nb = int(b * t + g * (1 - t))
                px[x, y] = (nr, g, nb, int(a * t))
    return img


def slice_gems():
    sheet = Image.open(os.path.join(SRC, "gems.png"))
    sheet = chroma_key_pink(sheet)
    w, h = sheet.size
    cw, ch = w // 4, h // 4
    os.makedirs(os.path.join(OUT, "gems"), exist_ok=True)
    for i, name in enumerate(GEM_NAMES):
        cx, cy = (i % 4) * cw, (i // 4) * ch
        cell = sheet.crop((cx, cy, cx + cw, cy + ch))
        bbox = cell.getchannel("A").getbbox()
        if bbox:
            cell = cell.crop(bbox)
        # fit into square canvas with small margin
        scale = (CELL_OUT - 8) / max(cell.size)
        cell = cell.resize((max(1, int(cell.width * scale)),
                            max(1, int(cell.height * scale))), Image.LANCZOS)
        canvas = Image.new("RGBA", (CELL_OUT, CELL_OUT), (0, 0, 0, 0))
        canvas.paste(cell, ((CELL_OUT - cell.width) // 2,
                            (CELL_OUT - cell.height) // 2), cell)
        canvas.save(os.path.join(OUT, "gems", f"gem_{name}.png"))
        print("wrote", name)


def key_hex_base():
    img = Image.open(os.path.join(SRC, "hexBase.png")).convert("RGBA")
    px = img.load()
    w, h = img.size
    key = px[1, 1][:3]  # corner is background (white-ish)
    kr, kg, kb = key
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            d = ((r - kr) ** 2 + (g - kg) ** 2 + (b - kb) ** 2) ** 0.5
            if d < 30:
                px[x, y] = (r, g, b, 0)
            elif d < 70:
                px[x, y] = (r, g, b, int(a * (d - 30) / 40.0))
    img = img.filter(ImageFilter.SMOOTH)
    img.save(os.path.join(OUT, "hex_base.png"))
    print("wrote hex_base")


def copy_backgrounds():
    board = Image.open(os.path.join(SRC, "board.png")).convert("RGB")
    board.save(os.path.join(OUT, "bg_board.png"))
    # darker variant for menus
    dark = board.point(lambda v: int(v * 0.45))
    dark.save(os.path.join(OUT, "bg_menu.png"))
    print("wrote backgrounds")


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    slice_gems()
    key_hex_base()
    copy_backgrounds()
