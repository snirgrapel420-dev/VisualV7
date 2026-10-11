#!/usr/bin/env python3
"""DALI VISUAL app icon: the DALI AUDIO monogram "DA" as a neon sign (pink → purple → cyan) with the full
name "DALI AUDIO" underneath, on a dark rounded tile. Small Windows sizes (16-48 px) show the monogram only.
Needs the Inter font (Inter-Black / Inter-Bold); the generated PNG / ICO / BMP files are committed.
Writes Resources/icon/icon_1024.png, icon_512.png, icon_256.png and Installer/Windows/DaliVisual.ico."""
import math, os, sys
from PIL import Image, ImageDraw, ImageFilter, ImageChops, ImageFont

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SS = 2                      # supersampling
N = 1024 * SS
PURPLE, CYAN, PINK = (168, 85, 247), (76, 201, 255), (236, 72, 153)

def lerp(a, b, t): return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))

def gradient(w, h):
    """diagonal purple → cyan with a touch of pink at the top-left"""
    g = Image.new("RGB", (w, h))
    px = g.load()
    for y in range(h):
        for x in range(w):
            t = min(max(((x / w) - 0.32) / 0.42 + (0.5 - y / h) * 0.15, 0.0), 1.0)
            c = lerp(PINK, PURPLE, t / 0.25) if t < 0.25 else lerp(PURPLE, CYAN, (t - 0.25) / 0.75)
            px[x, y] = c
    return g

def s(v): return int(v * SS)

FONT_DIRS = ["/usr/share/fonts/opentype/inter/", os.path.expanduser("~/Library/Fonts/"), "/Library/Fonts/", "C:/Windows/Fonts/"]
def font(name, size):
    for d in FONT_DIRS:
        if os.path.exists(d + name): return ImageFont.truetype(d + name, size)
    sys.exit("font not found: " + name)

def text_mask(txt, fnt, cx, cy, stroke=0, spacing=0):
    """white text centred at (cx, cy); stroke > 0 grows the letters outwards"""
    m = Image.new("L", (N, N), 0)
    d = ImageDraw.Draw(m)
    widths = [d.textlength(ch, font=fnt) for ch in txt]
    total = sum(widths) + spacing * (len(txt) - 1)
    x = cx - total / 2
    asc, desc = fnt.getmetrics()
    for ch, w in zip(txt, widths):
        d.text((x, cy), ch, font=fnt, fill=255, anchor="lm", stroke_width=stroke, stroke_fill=255)
        x += w + spacing
    return m

def neon(light, mask_ring, grad, layers):
    for blur, gain in layers:
        m = mask_ring.filter(ImageFilter.GaussianBlur(s(blur))) if blur else mask_ring
        m = m.point(lambda v, g=gain: int(min(255, v * g)))
        light = ImageChops.add(light, ImageChops.multiply(grad, Image.merge("RGB", (m, m, m))))
    return light

def render(subtitle=True):
    # ---- tile (macOS-style: 824 px rounded square centred in 1024, with a soft shadow) ----
    canvas = Image.new("RGBA", (N, N), (0, 0, 0, 0))
    tile_box = [s(100), s(100), s(924), s(924)]
    rad = s(186)
    shadow = Image.new("L", (N, N), 0)
    ImageDraw.Draw(shadow).rounded_rectangle([tile_box[0], tile_box[1] + s(14), tile_box[2], tile_box[3] + s(14)], rad, fill=150)
    shadow = shadow.filter(ImageFilter.GaussianBlur(s(22)))
    canvas.paste((0, 0, 0, 255), (0, 0), shadow)

    tile_mask = Image.new("L", (N, N), 0)
    ImageDraw.Draw(tile_mask).rounded_rectangle(tile_box, rad, fill=255)
    bg = Image.new("RGB", (N, N), (9, 6, 20))
    glow = Image.new("L", (N, N), 0)
    ImageDraw.Draw(glow).ellipse([s(220), s(220), s(804), s(760)], fill=255)
    glow = glow.filter(ImageFilter.GaussianBlur(s(150)))
    bg = Image.composite(Image.new("RGB", (N, N), (46, 22, 86)), bg, glow)

    grad = gradient(N // 8, N // 8).resize((N, N), Image.BICUBIC)
    light = Image.new("RGB", (N, N), (0, 0, 0))

    # ---- the monogram: neon tubes along the outline of "DA" ----
    mono_size = 360 if subtitle else 470
    mono_y = 452 if subtitle else 512
    fnt = font("Inter-Black.otf", s(mono_size))
    sp = s(10)
    inner = text_mask("DA", fnt, N // 2, s(mono_y), spacing=sp)
    outer = text_mask("DA", fnt, N // 2, s(mono_y), stroke=s(13 if subtitle else 18), spacing=sp)
    ring = ImageChops.subtract(outer, inner)
    light = neon(light, ring, grad, [(55, 0.55), (20, 0.8), (4, 1.0), (0, 0.8)])
    core = ImageChops.subtract(text_mask("DA", fnt, N // 2, s(mono_y), stroke=s(8 if subtitle else 11), spacing=sp), inner)
    core = core.filter(ImageFilter.GaussianBlur(s(1.2)))
    light = ImageChops.add(light, ImageChops.multiply(Image.merge("RGB", (core, core, core)), Image.new("RGB", (N, N), (210, 200, 240))))
    # a faint glow inside the letters
    light = ImageChops.add(light, ImageChops.multiply(grad, Image.merge("RGB", [inner.point(lambda v: v // 9)] * 3)))

    if subtitle:
        # a thin neon rule and the full name underneath
        rule = Image.new("L", (N, N), 0)
        ImageDraw.Draw(rule).rounded_rectangle([s(352), s(648), s(672), s(654)], s(3), fill=255)
        light = neon(light, rule, grad, [(10, 0.7), (0, 1.0)])
        name = text_mask("DALI AUDIO", font("Inter-Bold.otf", s(64)), N // 2, s(724), spacing=s(10))
        light = neon(light, name, grad, [(14, 0.45), (0, 1.0)])
        light = ImageChops.add(light, Image.merge("RGB", [name.point(lambda v: v // 3)] * 3))

    tile = ImageChops.add(bg, light)
    # a thin glossy rim
    rim = Image.new("L", (N, N), 0)
    ImageDraw.Draw(rim).rounded_rectangle(tile_box, rad, outline=70, width=s(3))
    tile = ImageChops.add(tile, Image.merge("RGB", (rim, rim, rim)))
    canvas.paste(tile, (0, 0), tile_mask)
    return canvas

def main():
    canvas = render(True)
    bold = render(False)        # small Windows sizes (16-48 px): the monogram alone, bigger, so it still reads
    out = canvas.resize((1024, 1024), Image.LANCZOS)
    icon_dir = os.path.join(ROOT, "Resources", "icon")
    os.makedirs(icon_dir, exist_ok=True)
    for size in (1024, 512, 256):
        out.resize((size, size), Image.LANCZOS).save(os.path.join(icon_dir, f"icon_{size}.png"))
    # Windows .ico: full tile (crop the macOS margin so the icon fills the square on Windows)
    crop = (s(84), s(84), s(940), s(940))
    big = canvas.crop(crop)
    small = bold.crop(crop)
    imgs = [(small if z <= 48 else big).resize((z, z), Image.LANCZOS) for z in (256, 128, 64, 48, 32, 24, 16)]
    ico = os.path.join(ROOT, "Installer", "Windows", "DaliVisual.ico")
    imgs[0].save(ico, format="ICO", sizes=[im.size for im in imgs], append_images=imgs[1:])
    # Windows installer wizard pictures (BMP, 100 % and 200 % scale)
    win_dir = os.path.join(ROOT, "Installer", "Windows")
    for k, z in ((1, 55), (2, 110)):
        im = Image.new("RGB", (z, z), (255, 255, 255))
        t = big.resize((z, z), Image.LANCZOS)
        im.paste(t, (0, 0), t)
        im.save(os.path.join(win_dir, f"wizard_small_{k}x.bmp"))
    for k, (w, h) in ((1, (164, 314)), (2, (328, 628))):
        side = Image.new("RGB", (w, h), (9, 6, 20))
        g = Image.new("L", (w, h), 0)
        ImageDraw.Draw(g).ellipse([-w // 2, h // 6, w + w // 2, h - h // 6], fill=255)
        g = g.filter(ImageFilter.GaussianBlur(w // 4))
        side = Image.composite(Image.new("RGB", (w, h), (46, 22, 86)), side, g)
        z = int(w * 0.8)
        t = big.resize((z, z), Image.LANCZOS)
        side.paste(t, ((w - z) // 2, (h - z) // 2), t)
        side.save(os.path.join(win_dir, f"wizard_large_{k}x.bmp"))
    print("icons written to", icon_dir, "and", ico)

if __name__ == "__main__":
    main()
