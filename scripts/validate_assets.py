#!/usr/bin/env python3
"""Read-only resource contract checks. Does not compile or render Unreal UI.

Requires Pillow and fontTools. Run from any directory; optional --output JSON.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image
from fontTools.ttLib import TTFont

ROOT = Path(__file__).resolve().parents[1]


def validate() -> dict:
    checks: list[dict] = []

    def check(name: str, passed: bool, detail: object = "") -> None:
        checks.append({"name": name, "passed": bool(passed), "detail": detail})

    image_dir = ROOT / "Content/UI/Images"
    for name in ("main", "character", "map", "battle"):
        path = image_dir / f"{name}-background.png"
        try:
            with Image.open(path) as image:
                image.load()
                width, height = image.size
                extrema = image.convert("RGB").getextrema()
                check(path.name, width >= 1280 and height >= 720
                      and abs(width / height - 16 / 9) < .01
                      and any(high > low for low, high in extrema),
                      {"size": image.size, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
        except Exception as error:
            check(path.name, False, str(error))

    for name in ("slash", "orbit", "arrow", "dash", "stone", "mirror"):
        path = image_dir / f"icon-{name}.png"
        try:
            with Image.open(path) as image:
                image.load()
                alpha = image.getchannel("A") if image.mode == "RGBA" else None
                bounds = alpha.getbbox() if alpha else None
                check(path.name, image.size == (512, 512) and alpha is not None
                      and alpha.getextrema() == (0, 255) and bounds is not None
                      and bounds[0] > 0 and bounds[1] > 0 and bounds[2] < 512 and bounds[3] < 512,
                      {"size": image.size, "mode": image.mode, "visible_bounds": bounds,
                       "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
        except Exception as error:
            check(path.name, False, str(error))

    # Inspect atlas subregions without creating altered image files.
    path = image_dir / "character-portraits.png"
    try:
        with Image.open(path) as image:
            image.load()
            width, height = image.size
            slices = []
            for index in range(4):
                region = image.crop((round(index * width / 4), 0, round((index + 1) * width / 4), height)).convert("RGB")
                slices.append(any(high > low for low, high in region.getextrema()))
            check(path.name, width >= 1024 and height >= 512 and abs(width / height - 2) < .01 and all(slices),
                  {"size": image.size, "uv_ranges": [[index / 4, 0, (index + 1) / 4, 1] for index in range(4)],
                   "nonempty_slices": slices, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    except Exception as error:
        check(path.name, False, str(error))

    path = image_dir / "button-gold.png"
    try:
        with Image.open(path) as image:
            image.load()
            alpha = image.getchannel("A") if image.mode == "RGBA" else None
            bounds = alpha.point(lambda value: 255 if value > 1 else 0).getbbox() if alpha else None
            width, height = image.size
            # Matches the native Slate brush's V=.21..77 UV crop; visible art must fit.
            # Generated alpha=1/255 noise is ignored; it is deliberately outside the UV crop.
            check(path.name, alpha is not None and alpha.getextrema() == (0, 255) and bounds is not None
                  and bounds[0] >= 0 and bounds[2] <= width
                  and bounds[1] >= height * .21 and bounds[3] <= height * .77,
                  {"size": image.size, "mode": image.mode, "visible_bounds_alpha_above_1": bounds,
                   "uv_range": [0, .21, 1, .77], "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    except Exception as error:
        check(path.name, False, str(error))

    # Check all Han glyphs in native source, not merely a hand-picked sample.
    text = "\n".join(path.read_text(encoding="utf-8") for path in (ROOT / "Source").rglob("*")
                     if path.suffix in (".h", ".cpp"))
    han = {ord(char) for char in text if 0x3400 <= ord(char) <= 0x9FFF}
    for name in ("SarasaUiSC-Regular.ttf", "SarasaUiSC-Bold.ttf"):
        path = ROOT / "Content/UI/Fonts" / name
        try:
            with TTFont(path) as font:
                cmap = font.getBestCmap()
                missing = sorted(han.difference(cmap))
                check(name, bool(han) and not missing,
                      {"han_glyphs_checked": len(han), "missing": "".join(map(chr, missing))})
        except Exception as error:
            check(name, False, str(error))
    license_path = ROOT / "Content/UI/Fonts/SARASA-OFL.txt"
    check("font license present", license_path.is_file() and "OPEN FONT LICENSE" in license_path.read_text().upper())

    packaging = (ROOT / "Config/DefaultGame.ini").read_text()
    check("loose UI files included in packaging", 'DirectoriesToAlwaysStageAsNonUFS=(Path="UI")' in packaging)
    project = json.loads((ROOT / "DywxSurvivors.uproject").read_text())
    check("runtime module declared", any(item["Name"] == "DywxUI" and item["Type"] == "Runtime"
                                         for item in project["Modules"]))
    return {"scope": "asset and configuration checks only; not an Unreal build or runtime test",
            "passed": all(item["passed"] for item in checks), "checks": checks,
            "not_run": ["Unreal compilation/UHT", "editor launch", "packaging", "Slate rendering/input", "GPU performance"]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = validate()
    serialized = json.dumps(report, ensure_ascii=False, indent=2)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(serialized + "\n", encoding="utf-8")
    print(serialized)
    raise SystemExit(0 if report["passed"] else 1)
