#pragma once

namespace Dywx
{
// If another owner changes the engine cap while preview is open, preserve that
// newer preference across the next preview state change and eventual release.
constexpr float PreviewBaseline(float SavedCap, float LastAppliedCap, float CurrentCap)
{
    return LastAppliedCap >= 0.f && CurrentCap != LastAppliedCap ? CurrentCap : SavedCap;
}

// Pure policy shared with the native test. Preserve a user's stricter existing cap.
constexpr float PreviewFPS(bool bApplicationActive, bool bMinimized, float PreviousCap)
{
    const float Budget = bMinimized ? 5.f : (bApplicationActive ? 60.f : 15.f);
    return PreviousCap > 0.f && PreviousCap < Budget ? PreviousCap : Budget;
}
}
