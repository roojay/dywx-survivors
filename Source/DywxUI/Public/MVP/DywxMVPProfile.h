#pragma once
#include "CoreMinimal.h"
#include "MVP/Progression.h"

// Local discovery/pity only: never persists run power, inventory or current
// combat.
class FDywxMVPProfile {
public:
  bool Load();
  void Apply(dywx::mvp::Progression &Progression) const;
  void Observe(const dywx::mvp::ProgressionState &State);
  bool Save();
  const dywx::mvp::ProgressionState &GetKnown() const { return Known; }

private:
  dywx::mvp::ProgressionState Known;
  bool bDirty = false;
};
