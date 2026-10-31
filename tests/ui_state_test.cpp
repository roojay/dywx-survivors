#include "DywxUIState.h"
#include "DywxCombatFeedback.h"
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
    assert(!n.SelectCharacter(2)); assert(!n.SelectCharacter(1));
    n.heroes[1]=n.heroes[2]=true; assert(n.SelectCharacter(2)); assert(n.SelectCharacter(1));
    assert(n.SelectCharacter(0)); assert(n.ChooseMap()); assert(!n.ChooseMap());
    assert(!n.SelectMap(1)); assert(!n.SelectMap(-1)); assert(n.SelectMap(0));
    assert(!n.SelectMode(Mode::Endless)); assert(!n.SelectMode(static_cast<Mode>(99)));
    assert(n.SelectMode(Mode::Traditional)); n.riskMask=3; assert(n.SelectMode(Mode::Quick)); assert(n.riskMask==0);
    assert(n.Back()); assert(n.screen==Screen::Character); assert(n.mode==Mode::Quick);
    assert(n.ChooseMap()); assert(n.Enter()); assert(n.screen==Screen::Battle); assert(!n.liveBattle);
    assert(!n.Enter()); assert(!n.SelectCharacter(1)); assert(!n.SelectMode(Mode::Quick));
    assert(n.Pause()); assert(!n.Pause()); assert(n.modal==Modal::Pause);
    assert(n.Back()); assert(n.modal==Modal::None); assert(n.screen==Screen::Battle);
    assert(n.Back()); assert(n.modal==Modal::Pause);
    n.ReturnToMenu(); assert(n.screen==Screen::Main&&n.modal==Modal::None); assert(n.mode==Mode::Quick);
    n.modal=Modal::Quit; assert(!n.Start()); assert(n.Back()); assert(n.modal==Modal::None);
    assert(n.Start()); assert(n.ChooseMap()); n.endlessUnlocked=true;
    assert(n.SelectMode(Mode::Endless)); n.realms[2]=true; assert(n.SelectMap(2)); assert(n.Enter());
    n.ReturnToMenu(); assert(n.Start()); assert(n.ChooseMap());
    n.mode=static_cast<Mode>(99); assert(!n.Enter()); n.mode=Mode::Traditional;
    n.modal=Modal::Legacy; assert(!n.Enter()); assert(!n.SelectMap(0)); assert(n.Back());
    n.realms[2]=false; assert(!n.Enter()); assert(n.SelectMap(0)); assert(n.Enter());
    n.ReturnToMenu();assert(n.Start());n.savedRuns[n.character]=true;
    assert(!n.ChooseMap());assert(n.screen==Screen::Character);
    assert(n.SelectCharacter(n.character==0?1:0));assert(n.ChooseMap());
    n.savedRuns[n.character]=true;assert(!n.Enter());
    n.savedRuns[n.character]=false;assert(n.Enter());
    // Live focus loss supersedes all overlays; repeated loss cannot toggle it off.
    n.liveBattle=true;
    for(auto modal : {Modal::None,Modal::Economy,Modal::Codex,Modal::Settings,Modal::Legacy,Modal::Quit}) {
        n.modal=modal;assert(n.FocusLoss());assert(n.modal==Modal::Pause);
        assert(!n.FocusLoss());assert(n.modal==Modal::Pause);
        // Foreground has no navigation mutation; only an explicit action closes pause.
        assert(n.Back());assert(n.modal==Modal::None);
    }
    n.ReturnToMenu();n.modal=Modal::Codex;assert(!n.FocusLoss());assert(n.modal==Modal::Codex);
    assert(CanOpenRestServices(false,true)); assert(!CanOpenRestServices(false,false));
    assert(CanOpenRestServices(true,false)); assert(CanOpenRestServices(true,true));
    assert(SafeFraction(5,10)==0.5f); assert(SafeFraction(-1,10)==0);
    assert(SafeFraction(20,10)==1); assert(SafeFraction(1,0)==0);
    assert(SafeFraction(1,-1)==0); assert(SafeFraction(std::numeric_limits<float>::quiet_NaN(),10)==0);
    assert(SafeFraction(5,std::numeric_limits<float>::quiet_NaN())==0);
    assert(SafeFraction(std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity())==0);
    // Damage presentation consumes facts, never resolves a possibly removed actor.
    mvp::DamageFeedback hit;
    assert(DamageLabel(hit)=="未明来源" && DamageAdvice(hit).empty());
    for(auto kind : {mvp::DamageKind::Contact,mvp::DamageKind::Charge,mvp::DamageKind::Explosion,
                    mvp::DamageKind::Projectile,mvp::DamageKind::HostileArea,mvp::DamageKind::Orbit,
                    mvp::DamageKind::Lightning,mvp::DamageKind::Fissure}) {
        hit.kind=kind;
        assert(DamageLabel(hit)!="未明来源" && !DamageAdvice(hit).empty());
        hit.source=9999; // Source lifetime cannot affect the text.
        assert(!DamageLabel(hit).empty());
    }
    hit.kind=mvp::DamageKind::HostileArea; hit.effectKind=mvp::EffectKind::Burn;
    assert(DamageLabel(hit)=="灼烧区域");
    hit.effectKind=mvp::EffectKind::FireWall; assert(DamageLabel(hit)=="火墙");
    hit.effectKind=mvp::EffectKind::Circle; assert(DamageLabel(hit)=="敌方术法区域");
    hit.kind=mvp::DamageKind::Unknown; assert(DamageLabel(hit)=="未明来源" && DamageAdvice(hit).empty());
    hit.kind=static_cast<mvp::DamageKind>(999); assert(DamageLabel(hit)=="未明来源" && DamageAdvice(hit).empty());
    std::cout << "PASS: navigation, locked content, repeated actions, back/pause, selection retention, invalid fractions\n";
}
