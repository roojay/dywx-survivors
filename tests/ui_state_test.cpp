#include "DywxUIState.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <iostream>
int main()
{
    using namespace dywx;
    Navigation n;
    assert(n.screen==Screen::Main);
    assert(!n.Enter()); assert(!n.ChooseMap()); assert(!n.Pause()); assert(!n.Back());
    assert(n.Start()); assert(!n.Start()); assert(n.screen==Screen::Character);
    assert(!n.SelectCharacter(-1)); assert(!n.SelectCharacter(4));
    assert(!n.SelectCharacter(2)); assert(n.SelectCharacter(1)); // exactly two playable heroes
    assert(n.SelectCharacter(0)); assert(n.ChooseMap()); assert(!n.ChooseMap());
    assert(!n.SelectMap(1)); assert(!n.SelectMap(-1)); assert(n.SelectMap(0));
    assert(!n.SelectMode(Mode::Endless)); assert(!n.SelectMode(static_cast<Mode>(99)));
    assert(!n.SelectMode(Mode::Traditional));
    assert(n.Back()); assert(n.screen==Screen::Character); assert(n.mode==Mode::Quick);
    assert(n.ChooseMap()); assert(n.Enter()); assert(n.screen==Screen::Battle); assert(!n.liveBattle);
    assert(!n.Enter()); assert(!n.SelectCharacter(1)); assert(!n.SelectMode(Mode::Quick));
    assert(n.Pause()); assert(!n.Pause()); assert(n.modal==Modal::Pause);
    assert(n.Back()); assert(n.modal==Modal::None); assert(n.screen==Screen::Battle);
    assert(n.Back()); assert(n.modal==Modal::Pause);
    n.ReturnToMenu(); assert(n.screen==Screen::Main&&n.modal==Modal::None); assert(n.mode==Mode::Quick);
    n.modal=Modal::Quit; assert(!n.Start()); assert(n.Back()); assert(n.modal==Modal::None);
    assert(n.Start()); assert(n.ChooseMap()); n.endlessUnlocked=true;
    assert(!n.SelectMode(Mode::Endless)); assert(n.Enter());
    assert(SafeFraction(5,10)==0.5f); assert(SafeFraction(-1,10)==0);
    assert(SafeFraction(20,10)==1); assert(SafeFraction(1,0)==0);
    assert(SafeFraction(1,-1)==0); assert(SafeFraction(std::numeric_limits<float>::quiet_NaN(),10)==0);
    assert(SafeFraction(5,std::numeric_limits<float>::quiet_NaN())==0);
    assert(SafeFraction(std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity())==0);
    std::cout << "PASS: navigation, locked content, repeated actions, back/pause, selection retention, invalid fractions\n";
}
