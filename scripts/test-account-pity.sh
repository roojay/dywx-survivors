#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
work="$(mktemp -d /tmp/dywx-account-pity.XXXXXX)"
trap 'rm -rf "$work"' EXIT
flags=()
if [[ "${SANITIZE:-0}" == 1 ]]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer -g); fi
g++ -std=c++20 -Wall -Wextra -Werror -pedantic "${flags[@]}" -DDYWXCORE_STANDALONE=1 -I Source/DywxCore/Public tests/account_pity_regressions.cpp Source/DywxCore/Private/MVP/{ContentCatalog,Progression,Economy,Campaign,Arena,SaveCodec,Simulation}.cpp -o "$work/tests"
"$work/tests"
python3 - <<'PY'
from pathlib import Path
runtime=Path('Source/DywxRuntime/Private/MVP/DywxRunSession.cpp').read_text()
resume=runtime.split('bool UDywxRunSession::ResumeRun(int32 Hero)',1)[1].split('bool UDywxRunSession::SaveProfile()',1)[0]
steps=['Candidate.RestoreCheckpoint(', 'Profile.Apply(Candidate.GetProgression()); Candidate.RefreshProgression();', 'Profile.ConsumeCheckpoint(', 'Simulation=MoveTemp(Candidate); ActivateRun();']
positions=[resume.index(step) for step in steps]
assert positions==sorted(positions)
assert 'if(!Profile.ConsumeCheckpoint(HeroId(Hero),Consumed)) return false;' in resume
profile=Path('Source/DywxRuntime/Private/MVP/DywxMVPProfile.cpp').read_text()
assert 'void FDywxMVPProfile::Apply(dywx::mvp::Progression& P)const{P.ImportAccountProgress(GetKnown());}' in profile
save=runtime.split('bool UDywxRunSession::SaveRun()',1)[1].split('bool UDywxRunSession::ResumeRun(',1)[0]
steps=['Simulation.PrepareCheckpoint(', 'auto CandidateProfile=Profile;', 'ObserveRunInto(CandidateProfile);', 'CandidateProfile.SaveCheckpoint(', 'Profile=MoveTemp(CandidateProfile);']
positions=[save.index(step) for step in steps]
assert positions==sorted(positions)
assert 'if(!CandidateProfile.SaveCheckpoint(HeroId(Checkpoint.campaign.hero),Bytes)) return false;' in save
assert 'SettleCampaign' not in save and 'Simulation=' not in save
observe=runtime.split('void UDywxRunSession::ObserveRunInto(',1)[1].split('bool UDywxRunSession::SaveProfile()',1)[0]
for field in ['CandidateProfile.Observe(GetSnapshot().progression)', 'seenAnomalies', 'seenBosses']:
    assert field in observe
assert runtime.count('ObserveRunInto(CandidateProfile);')==2
print('PASS source contract: validated candidate -> account import -> durable consume -> activation; checkpoint + account discoveries commit together')
print('NOT_RUN: native Unreal compilation and platform file IO')
PY
