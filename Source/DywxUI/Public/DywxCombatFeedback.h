#pragma once
#include "MVP/Types.h"
#include <string_view>

// Presentation wording for authoritative damage facts. No entity lookup: the
// attacker may already be gone by the time the result screen is displayed.
namespace dywx {
inline std::string_view DamageLabel(const mvp::DamageFeedback& hit)
{
    using mvp::DamageKind;
    switch (hit.kind) {
    case DamageKind::Contact: return "妖物接触";
    case DamageKind::Charge: return "妖物冲锋";
    case DamageKind::Explosion: return "爆裂冲击";
    case DamageKind::Projectile: return "妖弹";
    case DamageKind::HostileArea:
        if (hit.effectKind == mvp::EffectKind::Burn) return "灼烧区域";
        if (hit.effectKind == mvp::EffectKind::FireWall) return "火墙";
        return "敌方术法区域";
    case DamageKind::Orbit: return "环绕灵珠";
    case DamageKind::Lightning: return "异象天雷";
    case DamageKind::Fissure: return "异象裂隙";
    case DamageKind::None: case DamageKind::Unknown: return "未明来源";
    }
    return "未明来源";
}
inline std::string_view DamageAdvice(const mvp::DamageFeedback& hit)
{
    using mvp::DamageKind;
    switch (hit.kind) {
    case DamageKind::Contact: return "留意周围退路，避免被妖群包围。";
    case DamageKind::Charge: return "留意冲锋预警，侧向走位避开路线。";
    case DamageKind::Explosion: return "离开爆裂预警范围，避免贴近爆心。";
    case DamageKind::Projectile: return "留意飞行妖弹，横向走位避开。";
    case DamageKind::HostileArea: return "尽快离开敌方术法覆盖的区域。";
    case DamageKind::Orbit: return "与环绕灵珠保持距离，避开其轨迹。";
    case DamageKind::Lightning: return "留意天雷落点预警，及时离开。";
    case DamageKind::Fissure: return "避开异象裂隙，绕行寻找安全位置。";
    case DamageKind::None: case DamageKind::Unknown: return "";
    }
    return "";
}
}
