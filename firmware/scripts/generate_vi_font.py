#!/usr/bin/env python3
"""Generate a compact LVGL 9 font with Vietnamese glyphs from DejaVu Sans.
The TTF is used at build time only; firmware contains only raster glyph data.
License: DejaVu fonts license, see /usr/share/doc/fonts-dejavu-core/copyright.
"""
import sys
import unicodedata
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

FONT = Path("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")
OUT = Path(__file__).resolve().parent.parent / "main" / "sec_font_vi_13.c"
SIZE = 13
# Every ASCII glyph plus the precomposed vowels used in Vietnamese.
codepoints = list(range(32, 127))
codepoints += [c for c in range(0x00C0, 0x0100) if unicodedata.category(chr(c)) != "Cn"]
codepoints += [0x0102, 0x0103, 0x0110, 0x0111, 0x0128, 0x0129,
               0x0168, 0x0169, 0x01A0, 0x01A1, 0x01AF, 0x01B0]
codepoints += [c for c in range(0x1EA0, 0x1EFA) if unicodedata.category(chr(c)) != "Cn"]
codepoints = sorted(set(codepoints))
if not FONT.exists():
    sys.exit(f"Font source is missing: {FONT}; install fonts-dejavu-core")
font = ImageFont.truetype(str(FONT), SIZE)
bitmap, desc = [], ["    {0,0,0,0,0,0}, /* reserved glyph ID 0 */"]
for cp in codepoints:
    ch = chr(cp)
    x0, y0, x1, y1 = font.getbbox(ch, anchor="ls")
    w, h = x1-x0, y1-y0
    offset = len(bitmap)
    if w > 0 and h > 0:
        mask = Image.new("L", (w, h), 0)
        d = ImageDraw.Draw(mask)
        d.text((-x0, -y0), ch, font=font, fill=255, anchor="lt") if False else None
        # font.getmask returns the exact glyph bounding box raster.
        mask_bytes = list(font.getmask(ch, mode="L"))
        if len(mask_bytes) != w*h:
            sys.exit(f"Glyph size mismatch U+{cp:04X}")
        for a, b in zip(mask_bytes[::2], mask_bytes[1::2]):
            bitmap.append(((a*15+127)//255 << 4) | ((b*15+127)//255))
        if len(mask_bytes) & 1:
            bitmap.append(((mask_bytes[-1]*15+127)//255) << 4)
    adv = max(0, round(font.getlength(ch)*16))
    desc.append(f"    {{{offset},{adv},{w},{h},{x0},{-y1}}}, /* U+{cp:04X} */")
# ASCII is glyph IDs 1..95. Remaining codepoints are sparse-encoded.
extras = [cp for cp in codepoints if cp > 126]
relative = [cp-extras[0] for cp in extras]
chunks = lambda arr, n: "\n".join("    "+", ".join(str(v) for v in arr[i:i+n]) + "," for i in range(0,len(arr),n))
extra_range = extras[-1] - extras[0] + 1
src = f'''// AUTO-GENERATED at build time by scripts/generate_vi_font.py
// DejaVu Sans glyph outlines are rasterized to 4-bpp; no TTF is embedded.
#include "lvgl.h"
static const uint8_t glyph_bitmap[] = {{
{chunks(bitmap,16)}
}};
static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {{
{chr(10).join(desc)}
}};
static const uint16_t unicode_list_1[] = {{
{chunks(relative,12)}
}};
static const lv_font_fmt_txt_cmap_t cmaps[] = {{
    {{.range_start=32, .range_length=95, .glyph_id_start=1,
      .unicode_list=NULL, .glyph_id_ofs_list=NULL, .list_length=0,
      .type=LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY}},
    {{.range_start={extras[0]}, .range_length={extra_range},
      .glyph_id_start=96, .unicode_list=unicode_list_1,
      .glyph_id_ofs_list=NULL, .list_length={len(extras)},
      .type=LV_FONT_FMT_TXT_CMAP_SPARSE_TINY}}
}};
static const lv_font_fmt_txt_dsc_t font_dsc = {{
    .glyph_bitmap=glyph_bitmap, .glyph_dsc=glyph_dsc, .cmaps=cmaps,
    .kern_dsc=NULL, .kern_scale=0, .cmap_num=2,
    .bpp=4, .kern_classes=0, .bitmap_format=0
}};
const lv_font_t sec_font_vi_13 = {{
    .get_glyph_dsc=lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap=lv_font_get_bitmap_fmt_txt,
    .line_height=17, .base_line=4,
    .subpx=LV_FONT_SUBPX_NONE, .dsc=&font_dsc,
}};
'''
OUT.write_text(src,encoding="utf-8")
print(f"Wrote {OUT}: {len(codepoints)} glyphs, {len(bitmap)} bytes")
