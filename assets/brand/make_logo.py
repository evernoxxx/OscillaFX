#!/usr/bin/env python3
"""OscillaFX brand assets, generated from assets/brand/butterfly-source.png.

Run from anywhere:  python3 assets/brand/make_logo.py
Needs only Pillow (no numpy). Output (all deterministic, no random grain):

  packaging/icon/OscillaFX.png            1024x1024 app icon (CRT glass tile, teal phosphor butterfly)
  packaging/icon/OscillaFX-small-{16,32,64}.png   simplified high-contrast icon for tiny sizes
  assets/brand/logo-mark.png              transparent butterfly mark, 1024 (+ logo-mark@{512,256,128,64}.png)
  assets/brand/logo-lockup.png            mark + OSCILLAFX wordmark, transparent
  assets/brand/logo-mark-green.png        original green variant of the mark
  assets/brand/icon-green.png             original green variant of the icon
  assets/brand/tray-glyph{,@2x}.png       18 / 36 px black template silhouette (menu bar)
  assets/brand/tray-glyph-40.png          40 px template source used by the app
  app/Source/ui/BrandAssets.h             192px mark + 40px tray glyph as C++ byte arrays

The butterfly artwork is never redrawn: its luminance is extracted, auto-levelled and re-coloured with
the CRT palette (app/Source/ui/crt/CrtPalette.h): body teal, near-white cyan core, deep blue-green halo.
"""
import os
import sys
from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont, ImageOps

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
SRC = os.path.join(HERE, "butterfly-source.png")
ICON_DIR = os.path.join(ROOT, "packaging", "icon")
HEADER = os.path.join(ROOT, "app", "Source", "ui", "BrandAssets.h")


def hexrgb(v):
    return ((v >> 16) & 255, (v >> 8) & 255, v & 255)


# ---- CRT palette (mirror of CrtPalette.h) ---------------------------------------------------
FACE, FACE_CENTRE = hexrgb(0x0B181B), hexrgb(0x112A2D)
BODY, CORE, HALO = hexrgb(0x25D0C0), hexrgb(0xD8FFF8), hexrgb(0x0B8A96)
GRID = hexrgb(0x2A9D94)
# original green variant
G_BODY, G_CORE, G_HALO = hexrgb(0x22E070), hexrgb(0xF4FFB0), hexrgb(0x118A3C)

GAMMA = 1.95  # >1 keeps the green wing bodies teal and leaves white-cyan to the yellow veins
MARK_PX = 192  # embedded mark size (keeps BrandAssets.h small)
WORK = 1024  # working canvas for the butterfly layer
# square crop of the 782x1024 source that holds the butterfly and its halo, centred on the wings
CROP = (8, 108, 768, 868)


# ---- helpers --------------------------------------------------------------------------------
def tint(band, rgb, gain=1.0):
    """Scale a greyscale band into an RGB image tinted with rgb (0..255) and gain."""
    chans = [band.point(lambda v, c=c: min(255, int(v * c / 255.0 * gain))) for c in rgb]
    return Image.merge("RGB", chans)


def add(*imgs):
    out = imgs[0]
    for i in imgs[1:]:
        out = ImageChops.add(out, i)
    return out


def percentile(img, pct):
    h = img.histogram()
    target = sum(h) * pct
    acc = 0
    for i, n in enumerate(h):
        acc += n
        if acc >= target:
            return i
    return 255


def smoothstep_lut(lo, hi, gamma=1.0):
    lut = []
    for v in range(256):
        t = min(1.0, max(0.0, (v - lo) / float(hi - lo)))
        t = t * t * (3 - 2 * t) if gamma == 1.0 else t ** gamma
        lut.append(int(round(t * 255)))
    return lut


def dot_mask(size, period, depth):
    """Ordered dot-matrix: lit cell with a one-pixel gap each way. Regular, never random."""
    cell = Image.new("L", (period, period), 255)
    px = cell.load()
    gap = int(255 * (1 - depth))
    for i in range(period):
        px[i, period - 1] = gap
        px[period - 1, i] = gap
    mask = Image.new("L", size)
    for y in range(0, size[1], period):
        for x in range(0, size[0], period):
            mask.paste(cell, (x, y))
    return mask


def scanlines(size, period, depth):
    row = Image.new("L", (size[0], period), 255)
    ImageDraw.Draw(row).line([(0, period - 1), (size[0], period - 1)], fill=int(255 * (1 - depth)))
    out = Image.new("L", size)
    for y in range(0, size[1], period):
        out.paste(row, (0, y))
    return out


def rounded_mask(size, rect, radius, ss=4):
    big = Image.new("L", (size[0] * ss, size[1] * ss), 0)
    x0, y0, x1, y1 = [v * ss for v in rect]
    ImageDraw.Draw(big).rounded_rectangle([x0, y0, x1 - 1, y1 - 1], radius=radius * ss, fill=255)
    return big.resize(size, Image.LANCZOS)


# ---- butterfly energy -----------------------------------------------------------------------
def load_source():
    return Image.open(SRC).convert("RGB")


def energy(src, size=WORK):
    """Luminance of the source, auto-levelled to 0..255 (noise floor removed with a soft knee)."""
    crop = src.crop(CROP).resize((size, size), Image.LANCZOS)
    lum = crop.convert("L")
    lo = percentile(lum, 0.62) + 6          # background plus halo floor
    hi = percentile(lum, 0.995)
    lum = lum.point(smoothstep_lut(lo, hi)).point(lambda v: int(255 * (v / 255.0) ** GAMMA))
    lum = lum.filter(ImageFilter.UnsharpMask(radius=2.2, percent=60, threshold=2))
    return crop, lum


def phosphor_layers(e, body, core, halo, dots=True, dot_period=4):
    """Return (sharp, bloom) RGB layers for energy image e (to be screened over the glass)."""
    ramp = ImageOps.colorize(e, black=(0, 0, 0), mid=body, white=core, blackpoint=6, midpoint=125, whitepoint=252)
    if dots:
        m = dot_mask(e.size, dot_period, 0.20).filter(ImageFilter.GaussianBlur(0.35))
        ramp = ImageChops.multiply(ramp, Image.merge("RGB", [m, m, m]))
    bloom = add(
        tint(e.filter(ImageFilter.GaussianBlur(5)), halo, 0.75),
        tint(e.filter(ImageFilter.GaussianBlur(16)), halo, 0.80),
        tint(e.filter(ImageFilter.GaussianBlur(46)), halo, 0.70),
        tint(e.filter(ImageFilter.GaussianBlur(3)), body, 0.16),
    )
    return ramp, bloom


def green_layers(crop, e, halo):
    """Original colours: source RGB re-levelled by the same mask, bloom in green."""
    r, g, b = crop.split()
    sharp = Image.merge("RGB", [ImageChops.multiply(c, e.point(lambda v: min(255, int(v * 1.25)))) for c in (r, g, b)])
    sharp = ImageChops.lighter(sharp, ImageOps.colorize(e.point(lambda v: v * v // 255), (0, 0, 0), (60, 220, 100), (255, 255, 160),
                                                        blackpoint=8, midpoint=120, whitepoint=235))
    bloom = add(
        tint(e.filter(ImageFilter.GaussianBlur(5)), halo, 1.15),
        tint(e.filter(ImageFilter.GaussianBlur(16)), halo, 1.05),
        tint(e.filter(ImageFilter.GaussianBlur(46)), halo, 0.90),
    )
    return sharp, bloom


def make_layers(green=False, dots=True):
    src = load_source()
    crop, e = energy(src)
    if green:
        s, b = green_layers(crop, e, G_HALO)
    else:
        s, b = phosphor_layers(e, BODY, CORE, HALO, dots=dots)
    return e, s, b


# ---- icon -----------------------------------------------------------------------------------
def glass_tile(size):
    """Dark teal-grey CRT glass with faint uniform glow, vignette, graticule and scanlines. RGB."""
    grad = Image.radial_gradient("L").resize((size, size), Image.BICUBIC)  # 0 centre .. 255 corner
    face = ImageOps.colorize(grad.point(lambda v: min(255, int(v * 1.55))), FACE_CENTRE, FACE)
    # edge vignette: soft darkening towards the rim, strongest in the corners (barrel-ish falloff)
    vig = grad.point(lambda v: int(255 * (1 - 0.55 * (max(0, v - 90) / 165.0) ** 2)))
    face = ImageChops.multiply(face, Image.merge("RGB", [vig] * 3))
    # faint graticule: centre cross + 2 divisions each way, 1 device px, ~7 % strength
    grid = Image.new("L", (size, size), 0)
    d = ImageDraw.Draw(grid)
    for k in range(-2, 3):
        p = size // 2 + k * size // 5
        d.line([(p, 0), (p, size)], fill=34 if k else 48, width=max(1, size // 512))
        d.line([(0, p), (size, p)], fill=34 if k else 48, width=max(1, size // 512))
    face = ImageChops.screen(face, tint(grid, GRID, 1.0))
    return face


def reflection(size, mask):
    """Soft crescent of glass reflection up-left, plus a thin catch-light along the top edge."""
    refl = Image.new("L", (size * 2, size * 2), 0)
    d = ImageDraw.Draw(refl)
    d.ellipse([-size * 0.55, -size * 0.75, size * 1.15, size * 0.80], fill=255)
    d.ellipse([-size * 0.55 + size * 0.07, -size * 0.75 + size * 0.10, size * 1.15 + size * 0.04, size * 0.80 + size * 0.08], fill=0)
    refl = refl.filter(ImageFilter.GaussianBlur(size * 0.012)).resize((size, size), Image.LANCZOS)
    fade = Image.linear_gradient("L").resize((size, size)).transpose(Image.FLIP_TOP_BOTTOM)
    refl = ImageChops.multiply(refl, fade).point(lambda v: int(v * 0.11))
    return ImageChops.multiply(refl, mask)


def build_icon(e, sharp, bloom, simple=False, canvas=1024):
    body = int(canvas * 824 / 1024)
    off = (canvas - body) // 2
    radius = int(body * 0.2237)
    mask_full = rounded_mask((canvas, canvas), (off, off, off + body, off + body), radius)

    tile = glass_tile(body)
    # butterfly: the crop's butterfly spans ~60% of its width; scale so wings fill ~68% of the tile
    scale = (body * (0.90 if simple else 0.88)) / WORK
    n = int(WORK * scale)
    s_img = sharp.resize((n, n), Image.LANCZOS)
    b_img = bloom.resize((n, n), Image.LANCZOS)
    if simple:  # bolder glow so it survives 16 px
        b_img = ImageChops.add(b_img, b_img)
        s_img = ImageChops.add(s_img, tint(e.resize((n, n), Image.LANCZOS).filter(ImageFilter.MaxFilter(5)), BODY, 0.55))
    layer = Image.new("RGB", (body, body), (0, 0, 0))
    # centre the wings (their centre sits at ~(0.51, 0.50) of the crop)
    layer.paste(add(s_img, b_img), ((body - n) // 2, (body - n) // 2 - int(body * 0.005)))
    tile = ImageChops.screen(tile, layer)

    if not simple:
        sl = scanlines((body, body), 4 if canvas >= 1024 else 3, 0.07)
        tile = ImageChops.multiply(tile, Image.merge("RGB", [sl] * 3))

    full = Image.new("RGB", (canvas, canvas), (0, 0, 0))
    full.paste(tile, (off, off))
    # inner rim: dark bezel edge + hairline highlight so the glass reads as recessed
    rim_outer = mask_full
    rim_inner = rounded_mask((canvas, canvas), (off + 6, off + 6, off + body - 6, off + body - 6), radius - 6)
    rim = ImageChops.subtract(rim_outer, rim_inner).filter(ImageFilter.GaussianBlur(1.2))
    full = ImageChops.multiply(full, Image.merge("RGB", [ImageChops.invert(rim.point(lambda v: int(v * 0.65)))] * 3))
    if not simple:
        full = ImageChops.screen(full, tint(reflection(canvas, mask_full), (210, 255, 250)))
    hi = ImageChops.subtract(rounded_mask((canvas, canvas), (off + 1, off + 1, off + body - 1, off + body - 1), radius - 1),
                             rounded_mask((canvas, canvas), (off + 3, off + 3, off + body - 3, off + body - 3), radius - 3))
    full = ImageChops.screen(full, tint(hi.point(lambda v: int(v * 0.16)), BODY))

    # soft drop shadow in the transparent margin
    shadow = mask_full.filter(ImageFilter.GaussianBlur(canvas * 0.012))
    shadow = ImageChops.offset(shadow, 0, int(canvas * 0.012)).point(lambda v: int(v * 0.45))
    out = Image.new("RGBA", (canvas, canvas), (0, 0, 0, 0))
    out.putalpha(shadow)
    tile_rgba = full.convert("RGBA")
    tile_rgba.putalpha(mask_full)
    return Image.alpha_composite(out, tile_rgba)


# ---- transparent mark -----------------------------------------------------------------------
def mark_rgba(sharp, bloom):
    """Unpremultiply the additive-on-black render: alpha = brightest channel, colour = rgb / alpha."""
    from PIL import ImageMath
    rgb = ImageChops.screen(sharp, bloom)
    r, g, b = rgb.split()
    a = ImageChops.lighter(ImageChops.lighter(r, g), b)
    af = a.convert("F")
    out = []
    for c in (r, g, b):
        q = ImageMath.lambda_eval(lambda x: x["c"] * 255.0 / (x["a"] + 1.0), c=c.convert("F"), a=af)
        out.append(q.convert("L"))
    return Image.merge("RGBA", out + [a])


# ---- wordmark -------------------------------------------------------------------------------
def load_font(size):
    for path, idx in (("/System/Library/Fonts/Supplemental/Futura.ttc", 0), ("/System/Library/Fonts/Supplemental/DIN Alternate Bold.ttf", 0)):
        if os.path.exists(path):
            try:
                return ImageFont.truetype(path, size, index=idx)
            except Exception:
                try:
                    return ImageFont.truetype(path, size)
                except Exception:
                    pass
    return ImageFont.load_default()


def tracked_mask(text, font, tracking_px, size):
    m = Image.new("L", size, 0)
    d = ImageDraw.Draw(m)
    x = 0
    for ch in text:
        d.text((x, 0), ch, font=font, fill=255)
        x += d.textlength(ch, font=font) + tracking_px
    return m, x - tracking_px


def make_lockup(mark, green=False):
    H = 640
    font = load_font(210)
    cream = hexrgb(0xE4D4AE)
    w1, w2 = "OSCILLA", "FX"
    mk = mark.resize((H, H), Image.LANCZOS)
    box = Image.new("L", (3000, 420), 0)
    track = 34
    m1, x1 = tracked_mask(w1, font, track, box.size)
    m2, x2 = tracked_mask(w2, font, track, box.size)
    gap = track * 1.4
    total = int(x1 + gap + x2)
    bbox_h = font.getbbox("O")
    cap_top, cap_bot = bbox_h[1], bbox_h[3]
    text_w, text_h = total + 8, cap_bot - cap_top + 8
    W = H - 40 + 30 + text_w + 40
    out = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    out.alpha_composite(mk, (0, 0))
    tx, ty = H - 40 + 30, (H - text_h) // 2

    def place(mask, dx):
        full = Image.new("L", (W, H), 0)
        full.paste(mask.crop((0, cap_top, text_w, cap_bot + 8)), (int(tx + dx), ty))
        return full

    t1 = place(m1, 0)
    t2 = place(m2, x1 + gap)
    # OSCILLA: warm engraved cream with a 2 px lip; FX: teal phosphor with a tight bloom
    lip = ImageChops.offset(t1, 0, 3).point(lambda v: int(v * 0.35))
    out.alpha_composite(Image.merge("RGBA", [lip.point(lambda v: 0)] * 3 + [lip]))
    out.alpha_composite(Image.merge("RGBA", [t1.point(lambda v, c=c: c) for c in cream] + [t1]))
    core, body, halo = (G_CORE, G_BODY, G_HALO) if green else (CORE, BODY, HALO)
    glow = ImageChops.add(t2.filter(ImageFilter.GaussianBlur(10)), t2.filter(ImageFilter.GaussianBlur(26)))
    out.alpha_composite(Image.merge("RGBA", [glow.point(lambda v, c=c: c) for c in halo] + [glow.point(lambda v: min(255, int(v * 0.8)))]))
    body_img = Image.merge("RGBA", [t2.point(lambda v, c=c: c) for c in body] + [t2])
    out.alpha_composite(body_img)
    inner = t2.filter(ImageFilter.MinFilter(9)).filter(ImageFilter.GaussianBlur(2))
    out.alpha_composite(Image.merge("RGBA", [inner.point(lambda v, c=c: c) for c in core] + [inner.point(lambda v: int(v * 0.85))]))
    return out


# ---- tray glyph -----------------------------------------------------------------------------
def tray_masks(e):
    """Solid black template silhouette: threshold the leveled luminance, close seams, fill holes, smooth."""
    body = e.point(lambda v: 255 if v > 5 else 0).filter(ImageFilter.MedianFilter(7))
    body = body.filter(ImageFilter.MaxFilter(13)).filter(ImageFilter.MinFilter(13))   # outline of the whole butterfly
    flood = body.copy()
    ImageDraw.floodfill(flood, (0, 0), 128)
    body = flood.point(lambda v: 0 if v == 128 else 255)
    m = body
    m = m.filter(ImageFilter.GaussianBlur(6)).point(lambda v: 255 if v > 128 else 0)  # round off roughness
    bbox = m.getbbox()
    m = m.crop(bbox)
    side = max(m.size)
    sq = Image.new("L", (side, side), 0)
    sq.paste(m, ((side - m.size[0]) // 2, (side - m.size[1]) // 2))
    return sq


def glyph(sq, px, pad_frac=0.06):
    inner = int(round(px * (1 - 2 * pad_frac) * 4))
    big = sq.resize((inner, inner), Image.LANCZOS)
    canvas = Image.new("L", (px * 4, px * 4), 0)
    canvas.paste(big, ((px * 4 - inner) // 2, (px * 4 - inner) // 2))
    a = canvas.resize((px, px), Image.LANCZOS)
    out = Image.new("RGBA", (px, px), (0, 0, 0, 0))
    out.putalpha(a)
    return out


# ---- header ---------------------------------------------------------------------------------
def png_bytes(img, **kw):
    import io
    b = io.BytesIO()
    img.save(b, "PNG", optimize=True, **kw)
    return b.getvalue()


def write_header(mark256, glyph40):
    def arr(name, data):
        rows = []
        for i in range(0, len(data), 32):
            rows.append("    " + ",".join(str(c) for c in data[i:i + 32]) + ",")
        return "inline const unsigned char %s[] = {\n%s\n};\ninline constexpr int %sSize = %d;\n" % (name, "\n".join(rows), name, len(data))

    with open(HEADER, "w") as f:
        f.write("// GENERATED by assets/brand/make_logo.py - do not edit.\n#pragma once\n\n")
        f.write("// OscillaFX butterfly mark (192 px RGBA PNG, teal phosphor) and 40 px black menu-bar template glyph.\n")
        f.write("namespace BrandAssets\n{\n")
        f.write(arr("markPng", png_bytes(mark256)))
        f.write("\n")
        f.write(arr("trayGlyphPng", png_bytes(glyph40)))
        f.write("}\n")


# ---- main -----------------------------------------------------------------------------------
def main():
    os.makedirs(ICON_DIR, exist_ok=True)
    e, sharp, bloom = make_layers(green=False)
    _, sharp_nodot, bloom_nodot = make_layers(green=False, dots=False)

    icon = build_icon(e, sharp, bloom)
    icon.save(os.path.join(ICON_DIR, "OscillaFX.png"))
    simple = build_icon(e, sharp_nodot, bloom_nodot, simple=True)
    for px in (16, 32, 64):
        simple.resize((px, px), Image.LANCZOS).save(os.path.join(ICON_DIR, "OscillaFX-small-%d.png" % px))

    mark = mark_rgba(sharp, bloom)
    mark.save(os.path.join(HERE, "logo-mark.png"))
    for px in (512, 256, 128, 64):
        mark.resize((px, px), Image.LANCZOS).save(os.path.join(HERE, "logo-mark@%d.png" % px))
    make_lockup(mark).save(os.path.join(HERE, "logo-lockup.png"))

    ge, gs, gb = make_layers(green=True)
    mark_rgba(gs, gb).save(os.path.join(HERE, "logo-mark-green.png"))
    build_icon(ge, gs, gb).save(os.path.join(HERE, "icon-green.png"))

    sq = tray_masks(e)
    glyph(sq, 18).save(os.path.join(HERE, "tray-glyph.png"))
    glyph(sq, 36).save(os.path.join(HERE, "tray-glyph@2x.png"))
    g40 = glyph(sq, 40)
    g40.save(os.path.join(HERE, "tray-glyph-40.png"))

    write_header(mark.resize((MARK_PX, MARK_PX), Image.LANCZOS), g40)
    print("ok")


if __name__ == "__main__":
    sys.exit(main())
