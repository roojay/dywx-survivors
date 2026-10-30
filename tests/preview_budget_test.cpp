#include "DywxPreviewBudget.h"
#include <cassert>
#include <initializer_list>

int main()
{
    using Dywx::PreviewFPS;
    using Dywx::PreviewBaseline;
    static_assert(PreviewBaseline(144.f, -1.f, 60.f) == 144.f); // No ownership yet.
    static_assert(PreviewBaseline(144.f, 60.f, 60.f) == 144.f); // Own setting.
    static_assert(PreviewBaseline(144.f, 60.f, 30.f) == 30.f); // New lower preference.
    static_assert(PreviewBaseline(144.f, 15.f, 120.f) == 120.f); // New higher preference.
    static_assert(PreviewBaseline(144.f, 5.f, 0.f) == 0.f); // New unlimited preference.
    // Background external edit -> foreground -> minimized -> release baseline.
    float Baseline = PreviewBaseline(144.f, 15.f, 30.f);
    assert(PreviewFPS(true, false, Baseline) == 30.f);
    Baseline = PreviewBaseline(Baseline, 30.f, 30.f);
    assert(PreviewFPS(false, true, Baseline) == 5.f);
    Baseline = PreviewBaseline(Baseline, 5.f, 5.f);
    assert(Baseline == 30.f);
    // Raising/removing a cap must still impose the preview limit, even when the
    // desired budget matches our previous value (the controller forces a write).
    assert(PreviewFPS(true, false, PreviewBaseline(30.f, 60.f, 120.f)) == 60.f);
    assert(PreviewFPS(true, false, PreviewBaseline(30.f, 60.f, 0.f)) == 60.f);
    static_assert(PreviewFPS(true, false, 0.f) == 60.f);
    static_assert(PreviewFPS(false, false, 0.f) == 15.f);
    static_assert(PreviewFPS(false, true, 0.f) == 5.f);
    static_assert(PreviewFPS(true, true, 120.f) == 5.f);
    static_assert(PreviewFPS(true, false, 30.f) == 30.f);
    static_assert(PreviewFPS(false, false, 10.f) == 10.f);
    static_assert(PreviewFPS(true, false, -1.f) == 60.f);
    // Minimize -> background restore -> foreground never leaves an unlimited cap.
    for (const float PreviousCap : {0.f, 30.f, 60.f, 144.f})
    {
        assert(PreviewFPS(false, true, PreviousCap) <= 5.f);
        assert(PreviewFPS(false, false, PreviousCap) <= 15.f);
        assert(PreviewFPS(true, false, PreviousCap) <= 60.f);
    }
}
