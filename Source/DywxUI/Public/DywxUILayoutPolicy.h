#pragma once
// Direct-Slate presentation policy, deliberately independent of UMG's DPI curve.
#include <algorithm>
#include <cmath>
namespace dywx {
inline double ReferenceScale(double width, double height) {
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0) return 1.0;
    // 720p is the minimum supported viewport. Smaller windows still letterbox /
    // scale down, but must never allocate unbounded fonts to compensate.
    return std::clamp(std::min(width / 1920.0, height / 1080.0), 2.0 / 3.0, 4.0);
}
inline int FontPoints(int requested, double referenceScale, double userScale = 1.0) {
    if (!std::isfinite(referenceScale) || referenceScale <= 0) referenceScale = 1.0;
    referenceScale = std::clamp(referenceScale, 2.0 / 3.0, 4.0);
    if (!std::isfinite(userScale)) userScale = 1.0;
    userScale = std::clamp(userScale, 1.0, 1.1);
    const int authored = int(std::lround(std::clamp(requested, 1, 128) * userScale));
    // Project choice: >=18px nominal em after reference-canvas scaling at96DPI.
    // Actual glyph ink, OS/application DPI and native rendering require capture;
    // this is NOT an accessibility certification or a universal community rule.
    const int floor = int(std::ceil(18.0 * 72.0 / 96.0 / referenceScale * userScale));
    return std::max(authored, floor);
}
}
