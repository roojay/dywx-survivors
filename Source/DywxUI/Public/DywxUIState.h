#pragma once
// Dependency-free navigation policy. Shared by native widgets and executable tests.
namespace dywx
{
enum class Screen { Main, Character, Map, Battle };
enum class Mode { Quick, Traditional, Endless };
enum class Modal { None, Pause, Quit, Codex, Settings, Legacy, Economy };
struct Navigation
{
    Screen screen = Screen::Main;
    Modal modal = Modal::None;
    int character = 0;
    int map = 0;
    Mode mode = Mode::Quick;
    bool endlessUnlocked = false;
    bool liveBattle = false;
    bool heroes[3] = {true,false,false};
    bool realms[3] = {true,false,false};
    bool savedRuns[3] = {false,false,false};
    unsigned riskMask = 0;
    bool Start() { if (screen != Screen::Main || modal != Modal::None) return false; screen = Screen::Character; return true; }
    bool SelectCharacter(int value) { if (screen != Screen::Character || modal != Modal::None || value < 0 || value > 2 || !heroes[value]) return false; character = value; return true; }
    bool ChooseMap() { if (screen != Screen::Character || modal != Modal::None || character < 0 || character > 2 || !heroes[character] || savedRuns[character]) return false; screen = Screen::Map; return true; }
    bool SelectMap(int value) { if (screen != Screen::Map || modal != Modal::None || value < 0 || value > 2 || !realms[value]) return false; map = value; return true; }
    bool SelectMode(Mode value)
    {
        if (screen != Screen::Map || modal != Modal::None) return false;
        if (value != Mode::Quick && value != Mode::Traditional && value != Mode::Endless) return false;
        if (value == Mode::Endless && !endlessUnlocked) return false;
        mode = value; if(mode==Mode::Quick)riskMask=0; return true;
    }
    bool Enter() { if (screen != Screen::Map || modal != Modal::None || character < 0 || character > 2 || !heroes[character] || savedRuns[character] || map < 0 || map > 2 || !realms[map] || (mode != Mode::Quick && mode != Mode::Traditional && mode != Mode::Endless) || (mode == Mode::Endless && !endlessUnlocked)) return false; screen = Screen::Battle; liveBattle = false; return true; }
    bool Back()
    {
        if (modal != Modal::None) { modal = Modal::None; return true; }
        switch (screen) {
        case Screen::Character: screen = Screen::Main; return true;
        case Screen::Map: screen = Screen::Character; return true;
        case Screen::Battle: modal = Modal::Pause; return true;
        default: return false;
        }
    }
    bool Pause() { if (screen != Screen::Battle || modal != Modal::None) return false; modal = Modal::Pause; return true; }
    // Focus loss supersedes every live modal with an explicit Continue path.
    // No focus-gained transition exists: foreground restoration never resumes a run.
    bool FocusLoss()
    {
        if(screen!=Screen::Battle || !liveBattle || modal==Modal::Pause) return false;
        modal=Modal::Pause; return true;
    }
    void ReturnToMenu() { screen = Screen::Main; modal = Modal::None; liveBattle = false; }
};
// Identification belongs to the chosen cave, even before the economy layer.
inline bool CanOpenRestServices(bool economyUnlocked, bool caveSelected) { return economyUnlocked || caveSelected; }
inline float SafeFraction(float value, float maximum)
{
    // Comparisons deliberately reject NaN and invalid/zero denominators.
    if (!(maximum > 0.0f) || !(value >= 0.0f)) return 0.0f;
    const float ratio = value / maximum;
    return ratio >= 1.0f ? 1.0f : (ratio >= 0.0f ? ratio : 0.0f);
}
}
