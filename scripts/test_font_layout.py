#!/usr/bin/env python3
"""Offline Sarasa text metrics smoke test; NOT a Slate layout/render test.

Run: python3 scripts/test_font_layout.py --json Art/font-layout-metrics.json
Requires Pillow (python3 -m pip install Pillow).

Uses the actual bundled fonts at nominal 96 DPI, rounding the UE font point
size first, then conservatively rounding pixel size upward. It checks selected
fixed canvas slots at 100/110% text size, requiring at least 4 reference
pixels of vertical slack after line metrics and padding. Slot anchors are checked against source
so changing a tested slot cannot silently leave this fixture stale.

Pillow/FreeType shaping, metrics, DPI and fallback are not equivalent to Slate.
This does NOT verify glyph rasterization, shadows, clipping, focus/disabled
appearance, brush sizing, paragraph wrapping, or actual 720p/1080p output. Both
resolutions use a shared 1920x1080 reference canvas in the application; actual
SScaleBox/layout behavior still requires Windows UE runtime screenshots.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path

from PIL import ImageFont, __version__ as pillow_version

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Source/DywxUI/Private/SDywxRoot.cpp"
FONT_DIR = ROOT / "Content/UI/Fonts"
# id, text, point size, width, height, bold, total horizontal/vertical padding,
# exact source slot anchor (width/height are pre-ScaleBox reference-canvas units).
CASES = [
    ("main-title", "道衍万象", 82, 610, 155, True, 0, 0, "At(110,155,610,155,"),
    ("main-primary", "开始修行", 30, 490, 90, True, 32, 8, "At(120,455,490,90,"),
    ("codex-button", "道典", 24, 440, 70, True, 32, 9, "At(145,580,440,70,"),
    ("settings-button", "设置", 24, 440, 70, True, 32, 9, "At(145,685,440,70,"),
    ("quit-button", "退出游戏", 24, 440, 70, True, 32, 9, "At(145,790,440,70,"),
    ("back-button", "Esc  返回", 24, 190, 60, True, 32, 9, "At(60,1005,190,60,"),
    ("map-mode", "传统", 30, 145, 95, True, 34, 10, "At(1395+Index*160,530,145,95,"),
    ("map-caption", "栖雷古坛", 30, 395, 70, True, 34, 10, "At(X,930,395,70,"),
    ("dash-button", "遁术  空格", 24, 220, 60, True, 32, 9, "At(1195,985,220,60,"),
    ("screen-heading", "选择修士", 65, 800, 130, True, 0, 0, "At(55,45,800,130,"),
    ("map-title", "栖雷古坛", 56, 460, 110, True, 0, 0, "At(1410,190,460,110,"),
    ("hud-preview", "HUD 预览 · 静态示例数据 · 非实机战斗", 18, 650, 58, False, 16, 16, "At(660,145,650,58,"),
]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", type=Path, help="Write machine-readable evidence to this path")
    args = parser.parse_args()
    source = SOURCE.read_text(encoding="utf-8")
    # These are the common button metrics assumed by the fixture above.
    expected = ["SetNormalPadding(FMargin(16, 4))", "SetPressedPadding(FMargin(16, 5, 16, 3))"]
    missing = [s for s in expected + [case[-1] for case in CASES] if s not in source]
    if missing:
        parser.error("Source geometry changed; update/review the fixture: " + repr(missing))
    fonts = {style: FONT_DIR / f"SarasaUiSC-{style}.ttf" for style in ("Regular", "Bold")}
    results = []
    for scale in (1.0, 1.1):
        for name, text, size, width, height, bold, pad_x, pad_y, anchor in CASES:
            points = math.floor(size * scale + 0.5)
            pixels = math.ceil(points * 96 / 72)
            style = "Bold" if bold else "Regular"
            font = ImageFont.truetype(str(fonts[style]), pixels)
            ascent, descent = font.getmetrics()
            advance = font.getlength(text)
            fits = advance + pad_x <= width and ascent + descent + pad_y + 4 <= height
            results.append({
                "id": name, "text": text, "scale": scale, "font_style": style,
                "points_after_rounding": points, "metric_pixel_size": pixels,
                "slot": {"width": width, "height": height, "source_anchor": anchor},
                "total_padding": {"x": pad_x, "y": pad_y},
                "advance": advance, "ascent": ascent, "descent": descent,
                "remaining_width": round(width - advance - pad_x, 4),
                "remaining_height": height - ascent - descent - pad_y,
                "passed": fits,
            })
            print(f"{'PASS' if fits else 'FAIL'} {scale:.0%} {name}: "
                  f"{advance + pad_x:.2f}/{width} width; {ascent + descent + pad_y}/{height} height")
    passed = all(result["passed"] for result in results)
    evidence = {
        "test": "offline Sarasa font advance/line metrics smoke test",
        "scope": "12 representative fixed label/button slots at 100% and 110% text size",
        "runtime_validation": "NOT_RUN: this is not UE/Slate layout or visual verification",
        "exclusions": ["Slate shaping/fallback/DPI", "shadows", "paragraph wrapping", "focus and disabled appearance", "brush/layout rendering", "actual 720p/1080p output"],
        "reference_canvas": [1920, 1080], "assumed_font_dpi": 96,
        "minimum_vertical_slack": 4,
        "pillow_version": pillow_version,
        "font_sha256": {path.name: hashlib.sha256(path.read_bytes()).hexdigest() for path in fonts.values()},
        "source_sha256": hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
        "passed": passed, "case_count": len(results), "results": results,
    }
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(evidence, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"{'PASS' if passed else 'FAIL'}: {len(results)} offline checks. UE/Slate runtime validation NOT RUN.")
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
