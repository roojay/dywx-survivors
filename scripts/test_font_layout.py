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
    # Candidate title width is card 260 minus panel padding 12; height is the
    # inner-card bound, not an assertion of Slate's AutoHeight title allocation.
    ("upgrade-passive-title", "1  炼火心经", 24, 248, 208, True, 32, 9, "At(1080+I*270,290,260,220,Panel(Card,Ink,6))"),
    ("fusion-action", "破境 · 雷火焚天掌", 30, 510, 80, True, 34, 10, "At(1360,120,510,100,Fusions)"),
    ("main-title", "道衍万象", 82, 610, 155, True, 0, 0, "At(110,155,610,155,"),
    ("main-primary", "开始修行", 30, 490, 90, True, 32, 8, "At(120,455,490,90,"),
    ("codex-button", "道典", 24, 440, 70, True, 32, 9, "At(145,580,440,70,"),
    ("legacy-button", "传承", 24, 440, 70, True, 32, 9, "At(145,685,440,70,"),
    ("quit-button", "退出游戏", 24, 440, 70, True, 32, 9, "At(145,790,440,70,"),
    ("back-button", "Esc  返回", 24, 190, 60, True, 32, 9, "At(60,1005,190,60,"),
    ("map-mode", "无尽 · 每五波洞主", 30, 580, 85, True, 34, 10, "At(60+I*620,475,580,85,"),
    ("map-caption", "雷泽洞天 · 未解锁", 24, 580, 80, True, 34, 10, "At(60+I*620,190,580,80,"),
    ("dash-button", "遁术 Space", 16, 155, 38, False, 10, 4, "At(1250,995,155,38,"),
    ("battle-pause", "暂停 Esc", 16, 120, 40, False, 10, 4, "At(1770,22,120,40,"),
    ("battle-codex", "道典 C", 16, 110, 40, False, 10, 4, "At(1640,22,110,40,"),
    ("battle-wave", "第 99 / 99 波   ·   99:59   ·   斩妖 99999", 16, 520, 34, False, 0, 0, "At(800,24,520,34,"),
    ("screen-heading", "选择修士", 50, 800, 100, True, 0, 0, "At(55,45,800,100,"),
    ("map-title", "选择洞天与劫数", 50, 800, 100, True, 0, 0, "At(55,55,800,100,"),
    ("hud-resonance", "同道象达到 2 / 4 点共鸣", 18, 600, 40, False, 0, 0, 'Paragraph(TEXT("同道象达到 2 / 4 点共鸣'),
    ("rest-refine", "炼化筛选结果 · 先预览", 24, 470, 60, True, 32, 9, "At(45,775,470,60,"),
    ("rest-lock", "锁定 / 解锁", 24, 245, 60, True, 32, 9, "At(515,705,245,60,"),
    ("rest-ready", "迎战 · 炼化修为入账", 30, 810, 70, True, 34, 10, "At(1080,750,810,70,"),
    ("upgrade-reroll", "重悟 · 剩余 2 次", 24, 430, 60, True, 32, 9, "At(1460,520,430,60,"),
    ("refine-confirm", "确认炼化", 24, 395, 60, True, 32, 9, "At(1080,740,395,60,"),
    ("save-exit", "保存并离开", 24, 596, 65, True, 32, 9, 'Add(Button(TEXT("保存并离开")'),
    ("saved-hero", "存档待续 · 请先续接", 30, 500, 75, True, 34, 10, 'At(600,920,500,75,'),
    ("economy-refresh", "刷新洞府 · 免费传承或15灵石", 24, 596, 65, True, 32, 9, 'Button(TEXT("刷新洞府 · 免费传承或15灵石")'),
    ("economy-reforge", "重铸 · 999灵石 · 道象共鸣", 24, 596, 65, True, 32, 9, 'TEXT("重铸 · %d灵石 · ")'),
    ("economy-relic-heading", "玄霜镇海印·藏真", 21, 596, 55, False, 0, 0, "Label(CN(RelicCatalog()[R.catalogId].name),21,Gold)"),
    ("codex-numeric-line", "第五重：伤害999 · 冷却12.0秒", 21, 570, 55, False, 0, 0, '570,21,Seen ? Paper : Muted'),
    ("opportunity-title", "雷泽洞天 · 遗留洞府", 24, 810, 65, True, 32, 9, 'Button(CN(Details.title)'),
    ("opportunity-reward-line", "所得修为 +15，灵石 +20（初版数值）。", 20, 780, 55, False, 0, 0, 'Paragraph(CN(Details.description),780,20,Paper)'),
    ("anomaly-benefit-line", "【当前雷象有利：可吸纳引雷】", 21, 570, 55, False, 0, 0, 'TEXT("【当前雷象有利：可吸纳引雷】 ")'),
    ("boss-realm-title", "洞主破相！ 流波夔君 · 第 2 相   99999 / 99999", 17, 650, 40, False, 0, 0, 'At(635,72,650,40,'),
    ("local-ranking-heading", "无尽波数排行 · 本地，不联网", 24, 596, 55, False, 0, 0, 'Label(TEXT("无尽波数排行 · 本地，不联网"),24,Gold)'),
    ("local-ranking-line", "20 · 灵契山人 · 第 99999 波", 20, 560, 55, False, 0, 0, '560,20,Paper)'),
    ("anomaly-hud-header", "引雷 · 16秒", 16, 400, 36, False, 0, 0, 'At(28,80,400,36,'),
    ("anomaly-hud-numbers", "预警1.2秒 · 伤害22 · 半径2.2格", 18, 600, 42, False, 0, 0, 'WrapTextAt(600)'),
    ("hover-comparison-heading", "有效属性       当前装备    候选换装    差值", 18, 570, 42, False, 0, 0, 'Label(TEXT("有效属性       当前装备    候选换装    差值"),18,Gold)'),
    ("rest-title", "调息 · 战斗暂停   炼化待入账修为 9999", 26, 980, 55, True, 0, 0, "At(45,290,980,55,"),
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
        "scope": f"{len(CASES)} representative label/button/scroll-content lines at 100% and 110% text size",
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
