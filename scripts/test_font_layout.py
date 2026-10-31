#!/usr/bin/env python3
"""Offline Sarasa text metrics smoke test; NOT a Slate layout/render test.

Run: python3 scripts/test_font_layout.py --json Art/font-layout-metrics.json
Requires Pillow and a C++20 compiler (CXX or g++).

Uses the actual bundled fonts at nominal 96 DPI, rounding the UE font point
size using the compiled production policy, then rounding pixel size upward.
It checks 720p/1080p/1440p profiles at 100/110%, requiring at least 4 reference
pixels of vertical slack after line metrics and padding. Slot anchors are checked against source
so changing a tested slot cannot silently leave this fixture stale.

Pillow/FreeType shaping, metrics, DPI and fallback are not equivalent to Slate.
This does NOT verify glyph rasterization, shadows, clipping, focus/disabled
appearance, brush sizing, paragraph wrapping, or actual 720p/1080p/1440p output. All
resolutions use a shared 1920x1080 reference canvas in the application; actual
SScaleBox/layout behavior still requires Windows UE runtime screenshots.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import os
import subprocess
import tempfile
from pathlib import Path

from PIL import ImageFont, __version__ as pillow_version

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Source/DywxUI/Private/SDywxRoot.cpp"
FONT_DIR = ROOT / "Content/UI/Fonts"
# id, text, point size, width, height, bold, total horizontal/vertical padding,
# exact source slot anchor (width/height are pre-ScaleBox reference-canvas units).
CASES = [
    # Concept screens retain native text; the title is now a cropped logo image.
    ("upgrade-passive-title", "1  炼火心经", 30, 535, 58, True, 0, 0, "At(265,RowY+6,535,58,"),
    ("fusion-action", "破境 · 雷火焚天掌", 30, 510, 80, True, 34, 10, "At(1360,120,510,100,Fusions)"),
    ("main-help", "Enter 确认   ·   Tab 切换焦点   ·   Esc 返回", 20, 950, 44, False, 0, 0, "At(70,1020,950,44,"),
    ("main-footer", "道衍万象 · 初入洞天", 18, 420, 44, False, 0, 0, "At(1450,1020,420,44,"),
    ("map-detail-heading", "◇  雷泽洞天", 32, 450, 65, True, 0, 0, "At(95,570,450,65,"),
    ("upgrade-pending", "择一术法 · 待领悟 99 次", 20, 900, 44, False, 0, 0, "At(105,278,900,44,"),
    ("main-primary", "开始修行", 30, 440, 80, True, 32, 8, "At(120,375,440,80,"),
    ("main-continue", "续接存档", 24, 440, 70, True, 32, 9, "At(120,475,440,70,"),
    ("codex-button", "道典", 24, 440, 70, True, 32, 9, "At(120,655,440,70,"),
    ("legacy-button", "传承", 24, 440, 70, True, 32, 9, "At(120,565,440,70,"),
    ("settings-button", "设置", 24, 440, 70, True, 32, 9, "At(120,745,440,70,"),
    ("quit-button", "退出", 24, 440, 70, True, 32, 9, "At(120,835,440,70,"),
    ("back-button", "Esc  返回", 24, 190, 60, True, 32, 9, "At(60,1005,190,60,"),
    ("map-mode", "无尽", 20, 150, 54, True, 16, 8, "At(80+I*158,790,150,54,"),
    ("map-caption", "雷泽洞天", 24, 306, 52, True, 0, 0, "At(80,200+I*115,470,110,"),
    ("hero-row", "灵契山人", 24, 258, 52, True, 0, 0, "At(80,200+I*125,470,110,"),
    ("upgrade-confirm", "确认领悟", 30, 430, 80, True, 32, 8, "At(100,860,430,80,"),
    ("upgrade-paused", "战斗已暂停", 30, 430, 75, True, 16, 16, "At(750,25,430,75,"),
    ("hud-xp", "修为 999 / 999", 16, 270, 40, False, 0, 0, "At(bSmallViewport?1100:1155,bSmallViewport?1000:995,bSmallViewport?320:270,bSmallViewport?44:40,"),
    ("hud-level", "境界 99", 18, 170, 38, False, 0, 0, "At(360,78,bSmallViewport?190:170,bSmallViewport?44:38,"),
    ("upgrade-rank", "4 重 → 5 重", 22, 535, 44, False, 0, 0, "At(265,RowY+64,535,44,"),
    ("passive-heading", "专属心法 · 炼体真诀", 20, 355, 45, False, 0, 0, "At(180,820,355,45,"),
    ("upgrade-seal", "封存 · 3", 16, 155, 48, False, 10, 4, "At(825,RowY+104,155,48,"),
    ("dash-cooldown", "12.3 秒", 14, 170, 34, False, 0, 0, "At(1690,1010,170,bSmallViewport?44:34,"),
    ("hud-health", "气血 9999 / 9999    护盾 999", 20, 600, 44, False, 0, 0, "At(28,0,600,44,"),
    ("hud-spell-cooldown", "12.3", 16, 76, 34, False, 0, 0, "At(678+I*120,SpellY+26,76,bSmallViewport?44:34,"),
    ("hud-spell-rank", "•••••", 14, 82, 32, False, 0, 0, "At(675+I*120,RankY,82,bSmallViewport?44:32,"),
    ("hud-portrait-name", "巡星羽客", 16, 155, 36, False, 0, 0, "At(485,bSmallViewport?970:1000,155,bSmallViewport?44:36,"),
    ("dash-button", "遁术 Space", 16, 205, 52, False, 10, 4, "At(1660,955,205,52,"),
    ("battle-pause", "暂停 Esc", 16, 120, 40, False, 10, 4, "At(bSmallViewport?1720:1770,22,bSmallViewport?170:120,bSmallViewport?48:40,"),
    ("battle-codex", "道典 C", 16, 160, 48, False, 10, 4, "At(40,990,160,48,"),
    ("battle-wave", "第 99 / 99 波   ·   99:59   ·   斩妖 99999", 20, 700, 46, False, 0, 0, "At(650,24,700,46,"),
    ("screen-heading", "选择修士", 42, 450, 80, True, 0, 0, "At(95,105,450,80,"),
    ("map-title", "选择洞天", 42, 450, 80, True, 0, 0, "At(95,105,450,80,"),
    ("hud-resonance", "同道象达到 2 / 4 点共鸣", 18, 600, 40, False, 0, 0, 'Paragraph(TEXT("同道象达到 2 / 4 点共鸣'),
    ("rest-refine", "炼化筛选结果 · 先预览", 24, 470, 60, True, 32, 9, "At(45,775,470,60,"),
    ("rest-lock", "锁定 / 解锁", 24, 245, 60, True, 32, 9, "At(515,705,245,60,"),
    ("rest-ready", "迎战 · 炼化修为入账", 30, 810, 70, True, 34, 10, "At(1080,750,810,70,"),
    ("upgrade-reroll", "重悟 · 剩余 2 次", 24, 430, 80, True, 32, 9, "At(550,860,430,80,"),
    ("refine-confirm", "确认炼化", 24, 395, 60, True, 32, 9, "At(1080,740,395,60,"),
    ("save-exit", "保存并离开", 24, 596, 65, True, 32, 9, 'Add(Button(TEXT("保存并离开")'),
    ("saved-hero", "请先续接存档", 30, 420, 80, True, 34, 10, 'At(1440,950,420,80,'),
    ("economy-refresh", "刷新洞府 · 免费传承或15灵石", 24, 596, 65, True, 32, 9, 'Button(TEXT("刷新洞府 · 免费传承或15灵石")'),
    ("economy-reforge", "重铸 · 999灵石 · 道象共鸣", 24, 596, 65, True, 32, 9, 'TEXT("重铸 · %d灵石 · ")'),
    ("economy-relic-heading", "玄霜镇海印·藏真", 21, 596, 55, False, 0, 0, "Label(CN(RelicCatalog()[R.catalogId].name),21,Gold)"),
    ("codex-numeric-line", "第五重：伤害999 · 冷却12.0秒", 21, 570, 55, False, 0, 0, '570,21,Seen ? Paper : Muted'),
    ("opportunity-title", "雷泽洞天 · 遗留洞府", 24, 810, 65, True, 32, 9, 'Button(CN(Details.title)'),
    ("opportunity-reward-line", "所得修为 +15，灵石 +20（初版数值）。", 20, 780, 55, False, 0, 0, 'Paragraph(CN(Details.description),780,20,Paper)'),
    ("anomaly-benefit-line", "【当前雷象有利：可吸纳引雷】", 21, 570, 55, False, 0, 0, 'TEXT("【当前雷象有利：可吸纳引雷】 ")'),
    ("boss-realm-title", "洞主破相！ 流波夔君 · 第 2 相   99999 / 99999", 17, 650, 40, False, 0, 0, 'At(bSmallViewport?610:635,72,bSmallViewport?700:650,bSmallViewport?44:40,'),
    ("local-ranking-heading", "无尽波数排行 · 本地，不联网", 24, 596, 55, False, 0, 0, 'Label(TEXT("无尽波数排行 · 本地，不联网"),24,Gold)'),
    ("local-ranking-line", "20 · 灵契山人 · 第 99999 波", 20, 560, 55, False, 0, 0, '560,20,Paper)'),
    ("anomaly-hud-header", "引雷 · 16秒", 16, 400, 36, False, 0, 0, 'At(28,125,400,bSmallViewport?44:36,'),
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
    # Compile a tiny probe against the production policy, not a Python reimplementation.
    policy = ROOT / "Source/DywxUI/Public/DywxUILayoutPolicy.h"
    resolutions = [(1280,720),(1920,1080),(2560,1440)]
    sizes = sorted({case[2] for case in CASES} | {20})
    with tempfile.TemporaryDirectory(prefix="dywx-font-policy-") as temp:
        probe = Path(temp)/"probe.cpp"
        probe.write_text('#include "DywxUILayoutPolicy.h"\n#include <iostream>\nint main(){float w,h,u;int size;while(std::cin>>w>>h>>size>>u){auto s=dywx::ReferenceScale(w,h);std::cout<<s<<" "<<dywx::FontPoints(size,s,u)<<"\\n";}}')
        binary = Path(temp)/"probe"
        subprocess.run([os.environ.get("CXX","g++"),"-std=c++20","-I",str(policy.parent),str(probe),"-o",str(binary)],check=True)
        requests = [(w,h,size,user) for w,h in resolutions for user in (1.0,1.1) for size in sizes]
        output = subprocess.run([str(binary)],input="".join(f"{w} {h} {size} {user}\n" for w,h,size,user in requests),text=True,capture_output=True,check=True).stdout.splitlines()
        metrics = {key:(float(row.split()[0]),int(row.split()[1])) for key,row in zip(requests,output)}
        assert len(metrics)==len(requests)
    results = []
    for viewport_width,viewport_height in resolutions:
      for scale in (1.0, 1.1):
        for name, text, size, width, height, bold, pad_x, pad_y, anchor in CASES:
            # Source-pinned 720p branch overrides. Other profiles retain authored slots.
            if viewport_height==720:
                width,height={"hud-xp":(320,44),"hud-level":(190,44),"hud-spell-cooldown":(76,44),"hud-spell-rank":(82,44),"hud-portrait-name":(155,44),"battle-pause":(170,48),"boss-realm-title":(700,44),"anomaly-hud-header":(400,44),"dash-cooldown":(170,44)}.get(name,(width,height))
            # These are AutoHeight tooltip content, not fixed canvas slot claims.
            if name in ("hud-resonance","anomaly-hud-numbers","hover-comparison-heading"):
                height=max(height,48)
            reference_scale,points = metrics[(viewport_width,viewport_height,size,scale)]
            pixels = math.ceil(points * 96 / 72)
            style = "Bold" if bold else "Regular"
            font = ImageFont.truetype(str(fonts[style]), pixels)
            ascent, descent = font.getmetrics()
            advance = font.getlength(text)
            ink = font.getbbox(text)
            fits = advance + pad_x <= width and ascent + descent + pad_y + 4 <= height
            results.append({
                "id": name, "text": text, "scale": scale, "resolution": [viewport_width,viewport_height], "font_style": style,
                "reference_scale": reference_scale, "points_after_rounding": points, "metric_pixel_size": pixels,
                "slot": {"width": width, "height": height, "source_anchor": anchor},
                "total_padding": {"x": pad_x, "y": pad_y},
                "advance": advance, "ascent": ascent, "descent": descent,
                "scaled_advance": advance*reference_scale, "scaled_line_height": (ascent+descent)*reference_scale,
                "scaled_ink_width": (ink[2]-ink[0])*reference_scale, "scaled_ink_height": (ink[3]-ink[1])*reference_scale,
                "remaining_width": round(width - advance - pad_x, 4),
                "remaining_height": height - ascent - descent - pad_y,
                "passed": fits,
            })
            print(f"{'PASS' if fits else 'FAIL'} {viewport_height}p {scale:.0%} {name}: "
                  f"{advance + pad_x:.2f}/{width} width; {ascent + descent + pad_y}/{height} height; ink {(ink[3]-ink[1])*reference_scale:.1f}px")
    catalog = (ROOT / "Source/DywxCore/Private/MVP/ContentCatalog.cpp").read_text()
    assert 'At(95,875,440,95,SNew(SScrollBox)' in source
    assert 'Paragraph(CN(Passive.description),410,20,Paper)' in source
    for viewport_width,viewport_height in resolutions:
      for scale in (1.0, 1.1):
        reference_scale,points=metrics[(viewport_width,viewport_height,20,scale)]
        font = ImageFont.truetype(str(fonts["Regular"]), math.ceil(points*96/72))
        line_height = sum(font.getmetrics())
        for passive in ("BodyScripture", "SwordScripture", "BeastScripture"):
            match = re.search(r'PassiveId::'+passive+r',\s*"([^"]+)",\s*"([^"]+)"', catalog)
            assert match, passive
            text = match[2]
            lines, line = [], ""
            # Conservative per-character CJK wrap approximation, NOT Slate shaping.
            for char in text:
                if line and font.getlength(line+char)>410:
                    lines.append(line); line=""
                line += char
            lines.append(line)
            fits = line_height+4 <= 95 and all(font.getlength(line)<=410 for line in lines)
            results.append({"id": "passive-multiline-"+passive, "scale": scale, "resolution": [viewport_width,viewport_height],
                            "text": text, "lines": lines, "content_height": len(lines)*line_height,
                            "viewport_height": 95, "scroll_available": True, "passed": fits})
            print(f"{'PASS' if fits else 'FAIL'} {scale:.0%} {passive}: {len(lines)} lines, {len(lines)*line_height}px content, 95px scroll viewport")
    passed = all(result["passed"] for result in results)
    evidence = {
        "test": "offline Sarasa font advance/line metrics smoke test",
        "scope": f"{len(CASES)} representative lines plus 3 catalog passive paragraphs at 720p/1080p/1440p, 100% and 110% text size",
        "runtime_validation": "NOT_RUN: this is not UE/Slate layout or visual verification",
        "exclusions": ["Slate shaping/fallback/DPI", "shadows", "paragraph wrapping", "focus and disabled appearance", "brush/layout rendering", "actual 720p/1080p/1440p output"],
        "reference_canvas": [1920, 1080], "resolutions": resolutions, "assumed_font_dpi": 96,
        "policy_sha256": hashlib.sha256(policy.read_bytes()).hexdigest(),
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
