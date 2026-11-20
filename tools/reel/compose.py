# Komposition Repetitor-Reel: 1080x1920, 30 fps, GUI-Frames + Captions + Cursor + Endkarte
# Aufruf (im Ordner mit out/): python3 compose.py   ->  repetitor_reel.mp4
import json, math, subprocess, sys
from PIL import Image, ImageDraw, ImageFont, ImageFilter

W, H, FPS = 1080, 1920, 30
D = json.load(open("out/events.json"))
BAR = D["bar"]; TOTAL = D["total"]; GUI_UNTIL = D["guiUntil"]
R0 = D["rects"]
FONT = "/usr/share/fonts/opentype/inter/"
def font(w, s): return ImageFont.truetype(FONT + {"black": "Inter-Black.otf", "bold": "Inter-Bold.otf", "med": "Inter-Medium.otf", "semi": "Inter-SemiBold.otf"}[w], s)

BG1, BG2 = (23, 24, 28), (15, 16, 19)
TEXT, DIM = (236, 238, 242), (125, 130, 140)
SLOT = [(255, 122, 47), (79, 195, 247), (105, 240, 174), (179, 136, 255)]
URL = "github.com/danikoenig/Repetitor/releases"

def union(*rs):
    x0 = min(r[0] for r in rs); y0 = min(r[1] for r in rs)
    x1 = max(r[0] + r[2] for r in rs); y1 = max(r[1] + r[3] for r in rs)
    return [x0, y0, x1 - x0, y1 - y0]
def pad(r, p): return [r[0] - p, r[1] - p, r[2] + 2 * p, r[3] + 2 * p]

gui = R0["gui"]
FOCUS = {
    "full":  [0, 0, gui[2], gui[3]],
    "scale": pad(union(R0["pitch"], R0["scale"]), 10),
    "loop":  pad(union(R0["loop"], R0["row1"], R0["row2"]), 8),
    "rv":    pad(union(R0["loop"], R0["vary"]), 10),
    # rechte Haelfte: Kopfzeile (NEW) bis Kanal 4, mit VARY-Karte und Step-Anzeigen
    "notes": [R0["loop"][0] - 10, 4, gui[2] - R0["loop"][0] + 6, R0["row4"][1] + R0["row4"][3] + 8 - 4],
}

SCENES = [  # start, ende, titel, untertitel, fokus, farbe
    (0.0, 1 * BAR, "", "", "full", SLOT[0]),
    (1 * BAR, 3 * BAR, "RANDOM NOTES", "always in key  ·  four voices", "notes", SLOT[0]),
    (3 * BAR, 5 * BAR, "PICK A SCALE", "27 scales, or click your own notes", "scale", (255, 213, 79)),
    (5 * BAR, 7 * BAR, "LIKE IT? LOOP IT", "turn Length down  ·  1 to 32 steps", "loop", (77, 208, 225)),
    (7 * BAR, 8.5 * BAR, "ADD RHYTHM", "random patterns that repeat", "rv", (77, 208, 225)),
    (8.5 * BAR, 10 * BAR, "LET IT DRIFT", "skip  ·  octave  ·  fifth", "notes", (255, 77, 94)),
]

def scene_at(t):
    for i, s in enumerate(SCENES):
        if s[0] <= t < s[1]: return i
    return len(SCENES) - 1

def ease(x): x = max(0.0, min(1.0, x)); return x * x * (3 - 2 * x)

BOX_W, BOX_TOP, BOX_MAXH = 1020, 590, 1100
def crop_at(t):
    i = scene_at(t)
    cur = FOCUS[SCENES[i][4]]
    if i == 0: return cur
    prev = FOCUS[SCENES[i - 1][4]]
    u = ease((t - SCENES[i][0]) / 0.5)
    return [prev[k] + (cur[k] - prev[k]) * u for k in range(4)]

def box_for(crop):
    s = BOX_W / crop[2]
    h = crop[3] * s
    if h > BOX_MAXH:
        s = BOX_MAXH / crop[3]; h = BOX_MAXH
    w = crop[2] * s
    x = (W - w) / 2
    y = BOX_TOP + (BOX_MAXH - h) * 0.32
    return x, y, w, h, s

# --- statischer Hintergrund
def make_bg():
    bg = Image.new("RGB", (W, H))
    d = ImageDraw.Draw(bg)
    for y in range(H):
        u = y / H
        d.line([(0, y), (W, y)], fill=tuple(int(BG1[k] + (BG2[k] - BG1[k]) * u) for k in range(3)))
    glow = Image.new("RGB", (W, H), (0, 0, 0))
    g = ImageDraw.Draw(glow)
    for (cx, cy), c in zip([(0, 0), (W, 0), (0, H), (W, H)], SLOT):
        g.ellipse([cx - 520, cy - 520, cx + 520, cy + 520], fill=tuple(int(v * 0.16) for v in c))
    glow = glow.filter(ImageFilter.GaussianBlur(160))
    return Image.fromarray(__import__("numpy").clip(__import__("numpy").asarray(bg, dtype="int16") + __import__("numpy").asarray(glow, dtype="int16"), 0, 255).astype("uint8"))
BG = make_bg()

def logo(d, x, y, q, gap):
    # vier Step-Balken in den Kanalfarben (q = Gesamthoehe)
    bw = q * 0.26
    for s, hh in enumerate([0.55, 1.0, 0.4, 0.75]):
        xx = x + s * (bw + gap)
        d.rounded_rectangle([xx, y + q * (1 - hh), xx + bw, y + q], radius=bw * 0.3, fill=SLOT[s])

def centered(d, y, text, f, fill):
    w = d.textlength(text, font=f)
    d.text(((W - w) / 2, y), text, font=f, fill=fill)

# --- Cursor
CUR = D["cursor"]
def cursor_at(t):
    if not CUR or t < CUR[0][0]: return None
    for k in range(1, len(CUR)):
        if t <= CUR[k][0]:
            a, b = CUR[k - 1], CUR[k]
            u = ease((t - a[0]) / max(1e-6, b[0] - a[0]))
            return (a[1] + (b[1] - a[1]) * u, a[2] + (b[2] - a[2]) * u, bool(a[3]) and t < b[0] or bool(b[3]) and u > 0.999 or (bool(a[3]) and bool(b[3])))
    last = CUR[-1]
    return (last[1], last[2], bool(last[3]))
press_times = [CUR[k][0] for k in range(1, len(CUR)) if CUR[k][3] and not CUR[k - 1][3]]

def draw_cursor(img, x, y, pressed, t):
    d = ImageDraw.Draw(img, "RGBA")
    for pt in press_times:
        if 0 <= t - pt < 0.35:
            r = 14 + (t - pt) * 150; a = int(200 * (1 - (t - pt) / 0.35))
            d.ellipse([x - r, y - r, x + r, y + r], outline=(255, 255, 255, a), width=4)
    if pressed:
        d.ellipse([x - 20, y - 20, x + 20, y + 20], fill=(255, 255, 255, 60))
    s = 1.9
    pts = [(0, 0), (0, 22), (5.5, 17), (9.5, 26), (13, 24.5), (9, 16), (16, 16)]
    poly = [(x + px * s, y + py * s) for px, py in pts]
    d.polygon([(px + 2, py + 3) for px, py in poly], fill=(0, 0, 0, 110))
    d.polygon(poly, fill=(255, 255, 255, 255), outline=(20, 20, 24, 255))
    d.line(poly + [poly[0]], fill=(20, 20, 24, 255), width=3)

# --- Frames
frame_cache = {}
def gui_frame(f):
    return Image.open(f"out/frames/{f:04d}.jpg").convert("RGB")

F_CAP_T, F_CAP_S = font("black", 88), font("med", 40)
F_TITLE, F_TSUB = font("black", 122), font("semi", 44)
F_SMALL = font("semi", 34)
F_CHIP = font("bold", 26)

def render(f):
    t = f / FPS
    img = BG.copy()
    d = ImageDraw.Draw(img, "RGBA")

    if t < GUI_UNTIL + 0.6:
        gf = gui_frame(min(f, int(GUI_UNTIL * FPS) - 1))
        crop = crop_at(t)
        bx, by, bw, bh, s = box_for(crop)
        region = gf.crop((int(crop[0]), int(crop[1]), int(crop[0] + crop[2]), int(crop[1] + crop[3]))).resize((int(bw), int(bh)), Image.LANCZOS)
        i = scene_at(t)
        if i == 0:   # Titel: GUI abgedunkelt im Hintergrund
            region = Image.blend(region, Image.new("RGB", region.size, BG2), 0.55)
        # Schatten + Rahmen
        sh = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        ImageDraw.Draw(sh).rounded_rectangle([bx + 6, by + 18, bx + bw + 6, by + bh + 24], radius=26, fill=(0, 0, 0, 150))
        img.paste(sh.filter(ImageFilter.GaussianBlur(22)), (0, 0), sh.filter(ImageFilter.GaussianBlur(22)))
        mask = Image.new("L", region.size, 0)
        ImageDraw.Draw(mask).rounded_rectangle([0, 0, region.size[0] - 1, region.size[1] - 1], radius=22, fill=255)
        img.paste(region, (int(bx), int(by)), mask)
        d = ImageDraw.Draw(img, "RGBA")
        d.rounded_rectangle([bx, by, bx + bw, by + bh], radius=22, outline=(255, 255, 255, 30), width=2)

        # Caption
        sc = SCENES[i]
        if i == 0:
            u = ease(t / 0.5)
            logo(d, W / 2 - 62, 250 - 30 * (1 - u), 130, 12)
            centered(d, 420, "REPETITOR", F_TITLE, TEXT)
            centered(d, 570, "random notes that always fit", F_TSUB, DIM)
        else:
            u = ease((t - sc[0]) / 0.35)
            a = int(255 * u)
            yoff = 26 * (1 - u)
            d.rounded_rectangle([W / 2 - 40, 300 + yoff, W / 2 + 40, 308 + yoff], radius=4, fill=sc[5] + (a,))
            tw = d.textlength(sc[2], font=F_CAP_T)
            fcap = F_CAP_T if tw < W - 80 else font("black", int(88 * (W - 80) / tw))
            centered(d, 335 + yoff, sc[2], fcap, TEXT + (a,))
            centered(d, 450 + yoff, sc[3], F_CAP_S, DIM + (a,))
            # Marke oben
            logo(d, 72, 108, 48, 5)
            d.text((158, 112), "REPETITOR", font=font("black", 40), fill=TEXT)

        # Cursor
        c = cursor_at(t)
        if c and i > 0:
            cx = bx + (c[0] - crop[0]) * s; cy = by + (c[1] - crop[1]) * s
            if by - 40 < cy < by + bh + 40:
                draw_cursor(img, cx, cy, c[2], t)

        d = ImageDraw.Draw(img, "RGBA")
        centered(d, 1790, "free  ·  MIDI plugin  ·  VST3 + AU  ·  Mac", F_SMALL, DIM)

    # Endkarte
    if t >= GUI_UNTIL:
        u = ease((t - GUI_UNTIL) / 0.6)
        end = BG.copy()
        e = ImageDraw.Draw(end, "RGBA")
        logo(e, W / 2 - 86, 430, 180, 16)
        centered(e, 660, "REPETITOR", font("black", 116), TEXT)
        centered(e, 805, "random notes that always fit", F_TSUB, DIM)
        lines = ["4 voices  ·  27 scales  ·  your own scale", "loop what you like  ·  1 to 32 steps", "rhythm  ·  skip  ·  octave  ·  fifth"]
        for k, ln in enumerate(lines):
            centered(e, 930 + k * 62, ln, font("semi", 40), TEXT)
        centered(e, 1170, "MIDI FX  ·  VST3 + AU  ·  Bitwig  ·  Logic  ·  Ableton", font("bold", 36), SLOT[1])
        fu = font("bold", 38)
        tw = e.textlength(URL, font=fu)
        bx0 = (W - tw) / 2 - 36
        e.rounded_rectangle([bx0, 1270, W - bx0, 1370], radius=24, fill=(29, 31, 36, 255), outline=SLOT[0] + (255,), width=3)
        centered(e, 1296, URL, fu, TEXT)
        centered(e, 1410, "free download  ·  link in bio", font("med", 36), DIM)
        centered(e, 1700, "by Dani Koenig", font("semi", 36), DIM)
        img = Image.blend(img, end, u) if u < 1 else end
    return img

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "preview":
        for ts in sys.argv[2:]:
            render(int(float(ts) * FPS)).save(f"/tmp/prev_{float(ts):05.2f}.jpg", quality=88)
        sys.exit(0)
    n = int(round(TOTAL * FPS))
    p = subprocess.Popen(["ffmpeg", "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{W}x{H}", "-r", str(FPS),
                          "-i", "-", "-c:v", "libx264", "-preset", "veryfast", "-crf", "18", "-pix_fmt", "yuv420p", "video_tmp.mp4"],
                         stdin=subprocess.PIPE)
    for f in range(n):
        p.stdin.write(render(f).tobytes())
        if f % 150 == 0: print("compose", f, "/", n, flush=True)
    p.stdin.close(); p.wait()
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", "video_tmp.mp4", "-i", "out/audio.wav", "-c:v", "copy",
                    "-c:a", "aac", "-b:a", "256k", "-shortest", "-movflags", "+faststart", "repetitor_reel.mp4"], check=True)
    print("video done")
