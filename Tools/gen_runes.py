"""Procedurally draw the Elder Futhark rune glyphs used by Runik as white
glowing sprites (tinted yellow/blue at runtime), plus simple UI icons.
Outputs to Content/Raw/runes and Content/Raw/icons."""
import os
from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_RUNES = os.path.join(ROOT, "Content", "Raw", "runes")
OUT_ICONS = os.path.join(ROOT, "Content", "Raw", "icons")

SIZE = 256

# Polylines in unit space (0..1, y down). Multiple strokes per glyph.
# Chosen from Elder Futhark by meaning:
#   Orbit      = Raidho   (r, "ride/journey")
#   Push       = Thurisaz (th, "giant/directed force")
#   Pull       = Nauthiz  (n, "need/constraint")
#   Fuse       = Gebo     (g, "gift/union")
#   Detach     = Kenaz    (k, "torch/opening")
#   RealmShift = Dagaz    (d, "day/transformation between states")
GLYPHS = {
    "rune_orbit": [  # Raidho
        [(0.32, 0.10), (0.32, 0.90)],
        [(0.32, 0.10), (0.68, 0.28), (0.32, 0.48)],
        [(0.32, 0.48), (0.70, 0.90)],
    ],
    "rune_push": [  # Thurisaz
        [(0.35, 0.08), (0.35, 0.92)],
        [(0.35, 0.30), (0.72, 0.50), (0.35, 0.70)],
    ],
    "rune_pull": [  # Nauthiz
        [(0.50, 0.08), (0.50, 0.92)],
        [(0.28, 0.34), (0.72, 0.58)],
    ],
    "rune_fuse": [  # Gebo
        [(0.22, 0.18), (0.78, 0.82)],
        [(0.78, 0.18), (0.22, 0.82)],
    ],
    "rune_detach": [  # Kenaz
        [(0.66, 0.12), (0.32, 0.50), (0.66, 0.88)],
    ],
    "rune_shift": [  # Dagaz
        [(0.20, 0.15), (0.20, 0.85), (0.80, 0.15), (0.80, 0.85), (0.20, 0.15)],
    ],
}

ICONS = {
    "icon_play":  [[(0.30, 0.18), (0.30, 0.82), (0.80, 0.50), (0.30, 0.18)]],
    "icon_pause": [[(0.34, 0.18), (0.34, 0.82)], [(0.66, 0.18), (0.66, 0.82)]],
    "icon_step":  [[(0.25, 0.20), (0.25, 0.80), (0.60, 0.50), (0.25, 0.20)],
                   [(0.72, 0.20), (0.72, 0.80)]],
    "icon_stop":  [[(0.28, 0.28), (0.72, 0.28), (0.72, 0.72), (0.28, 0.72), (0.28, 0.28)]],
    "icon_reset": [[(0.72, 0.30), (0.50, 0.20), (0.28, 0.32), (0.22, 0.55),
                    (0.34, 0.76), (0.58, 0.82), (0.76, 0.68)],
                   [(0.76, 0.68), (0.78, 0.48)], [(0.76, 0.68), (0.58, 0.66)]],
    "icon_gear":  [[(0.50, 0.22), (0.62, 0.28), (0.72, 0.40), (0.74, 0.55),
                    (0.66, 0.70), (0.50, 0.76), (0.34, 0.70), (0.26, 0.55),
                    (0.28, 0.40), (0.38, 0.28), (0.50, 0.22)],
                   [(0.50, 0.10), (0.50, 0.24)], [(0.50, 0.76), (0.50, 0.90)],
                   [(0.12, 0.50), (0.26, 0.50)], [(0.74, 0.50), (0.88, 0.50)]],
    "icon_book":  [[(0.15, 0.22), (0.50, 0.30), (0.85, 0.22), (0.85, 0.78),
                    (0.50, 0.86), (0.15, 0.78), (0.15, 0.22)],
                   [(0.50, 0.30), (0.50, 0.86)]],
    "icon_gif":   [[(0.14, 0.30), (0.86, 0.30), (0.86, 0.70), (0.14, 0.70), (0.14, 0.30)],
                   [(0.30, 0.42), (0.24, 0.42), (0.24, 0.58), (0.30, 0.58)],
                   [(0.44, 0.42), (0.44, 0.58)],
                   [(0.58, 0.58), (0.58, 0.42), (0.68, 0.42)], [(0.58, 0.50), (0.65, 0.50)]],
    "icon_chart": [[(0.18, 0.20), (0.18, 0.80), (0.84, 0.80)],
                   [(0.32, 0.80), (0.32, 0.58)], [(0.46, 0.80), (0.46, 0.40)],
                   [(0.60, 0.80), (0.60, 0.50)], [(0.74, 0.80), (0.74, 0.30)]],
    "icon_undo":  [[(0.70, 0.72), (0.44, 0.72), (0.28, 0.55), (0.34, 0.36), (0.52, 0.28)],
                   [(0.52, 0.28), (0.40, 0.20)], [(0.52, 0.28), (0.44, 0.40)]],
    "icon_plane": [[(0.50, 0.14), (0.82, 0.38), (0.50, 0.62), (0.18, 0.38), (0.50, 0.14)],
                   [(0.50, 0.38), (0.82, 0.62), (0.50, 0.86), (0.18, 0.62), (0.50, 0.38)]],
    "icon_trash": [[(0.30, 0.30), (0.70, 0.30), (0.66, 0.84), (0.34, 0.84), (0.30, 0.30)],
                   [(0.22, 0.30), (0.78, 0.30)], [(0.42, 0.30), (0.42, 0.20), (0.58, 0.20), (0.58, 0.30)]],
}


def draw_glyph(strokes, size, width, glow):
    scale = 4  # supersample
    s = size * scale
    core = Image.new("L", (s, s), 0)
    d = ImageDraw.Draw(core)
    for stroke in strokes:
        pts = [(x * s, y * s) for x, y in stroke]
        d.line(pts, fill=255, width=width * scale, joint="curve")
        for p in pts:  # round caps
            r = width * scale / 2
            d.ellipse([p[0] - r, p[1] - r, p[0] + r, p[1] + r], fill=255)
    core = core.resize((size, size), Image.LANCZOS)
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    if glow:
        halo = core.filter(ImageFilter.GaussianBlur(size / 18))
        halo = halo.point(lambda v: min(255, int(v * 1.6)))
        img.paste((255, 255, 255), (0, 0), halo.point(lambda v: v // 2))
    img.paste((255, 255, 255), (0, 0), core)
    return img


if __name__ == "__main__":
    os.makedirs(OUT_RUNES, exist_ok=True)
    os.makedirs(OUT_ICONS, exist_ok=True)
    for name, strokes in GLYPHS.items():
        draw_glyph(strokes, SIZE, 13, glow=True).save(
            os.path.join(OUT_RUNES, name + ".png"))
        print("wrote", name)
    for name, strokes in ICONS.items():
        draw_glyph(strokes, 128, 7, glow=False).save(
            os.path.join(OUT_ICONS, name + ".png"))
        print("wrote", name)
