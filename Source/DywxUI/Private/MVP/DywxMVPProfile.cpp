#include "MVP/DywxMVPProfile.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace {
FString ProfilePath() {
  return FPaths::ProjectSavedDir() / TEXT("MVP/profile.json");
}
template <size_t N>
void ReadFlags(const FJsonObject &Json, const TCHAR *Field,
               std::array<bool, N> &Flags) {
  const TArray<TSharedPtr<FJsonValue>> *Values = nullptr;
  if (!Json.TryGetArrayField(Field, Values) || !Values)
    return;
  for (int32 Index = 0; Index < Values->Num() && Index < static_cast<int32>(N);
       ++Index)
    if ((*Values)[Index].IsValid() && (*Values)[Index]->Type == EJson::Boolean)
      Flags[static_cast<size_t>(Index)] = (*Values)[Index]->AsBool();
}
template <size_t N>
void WriteFlags(FJsonObject &Json, const TCHAR *Field,
                const std::array<bool, N> &Flags) {
  TArray<TSharedPtr<FJsonValue>> Values;
  for (bool Value : Flags)
    Values.Add(MakeShared<FJsonValueBoolean>(Value));
  Json.SetArrayField(Field, Values);
}
template <size_t N>
bool MergeFlags(std::array<bool, N> &Destination,
                const std::array<bool, N> &Source) {
  bool Changed = false;
  for (size_t Index = 0; Index < N; ++Index) {
    Changed |= Source[Index] && !Destination[Index];
    Destination[Index] = Destination[Index] || Source[Index];
  }
  return Changed;
}
} // namespace

bool FDywxMVPProfile::Load() {
  Known = {};
  bDirty = false;
  const FString Path = ProfilePath();
  const int64 Bytes = IFileManager::Get().FileSize(*Path);
  if (Bytes < 0 || Bytes > 65536)
    return false;
  FString Text;
  TSharedPtr<FJsonObject> Json;
  if (!FFileHelper::LoadFileToString(Text, *Path) ||
      !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) ||
      !Json.IsValid())
    return false;
  double Version = 0, Pity = 0;
  if (!Json->TryGetNumberField(TEXT("version"), Version) || Version != 1)
    return false;
  if (Json->TryGetNumberField(TEXT("pityWaves"), Pity) && FMath::IsFinite(Pity))
    Known.pityWaves = static_cast<int>(FMath::Clamp(Pity, 0.0, 20.0));
  ReadFlags(*Json, TEXT("spells"), Known.seenSpells);
  ReadFlags(*Json, TEXT("relics"), Known.seenRelics);
  ReadFlags(*Json, TEXT("fusions"), Known.seenFusions);
  ReadFlags(*Json, TEXT("passives"), Known.seenPassives);
  ReadFlags(*Json, TEXT("enemies"), Known.seenEnemies);
  ReadFlags(*Json, TEXT("mutations"), Known.seenMutations);
  ReadFlags(*Json, TEXT("resonances"), Known.seenResonances);
  return true;
}

void FDywxMVPProfile::Apply(dywx::mvp::Progression &Progression) const {
  Progression.SetPityWaves(Known.pityWaves);
  Progression.MergeDiscoveries(Known);
}

void FDywxMVPProfile::Observe(const dywx::mvp::ProgressionState &State) {
  const int Pity = FMath::Clamp(State.pityWaves, 0, 20);
  bDirty |= Pity != Known.pityWaves;
  Known.pityWaves = Pity;
  bDirty |= MergeFlags(Known.seenSpells, State.seenSpells);
  bDirty |= MergeFlags(Known.seenRelics, State.seenRelics);
  bDirty |= MergeFlags(Known.seenFusions, State.seenFusions);
  bDirty |= MergeFlags(Known.seenPassives, State.seenPassives);
  bDirty |= MergeFlags(Known.seenEnemies, State.seenEnemies);
  bDirty |= MergeFlags(Known.seenMutations, State.seenMutations);
  bDirty |= MergeFlags(Known.seenResonances, State.seenResonances);
}

bool FDywxMVPProfile::Save() {
  if (!bDirty)
    return true;
  auto Json = MakeShared<FJsonObject>();
  Json->SetNumberField(TEXT("version"), 1);
  Json->SetNumberField(TEXT("pityWaves"), Known.pityWaves);
  WriteFlags(*Json, TEXT("spells"), Known.seenSpells);
  WriteFlags(*Json, TEXT("relics"), Known.seenRelics);
  WriteFlags(*Json, TEXT("fusions"), Known.seenFusions);
  WriteFlags(*Json, TEXT("passives"), Known.seenPassives);
  WriteFlags(*Json, TEXT("enemies"), Known.seenEnemies);
  WriteFlags(*Json, TEXT("mutations"), Known.seenMutations);
  WriteFlags(*Json, TEXT("resonances"), Known.seenResonances);
  FString Text;
  const auto Writer = TJsonWriterFactory<>::Create(&Text);
  if (!FJsonSerializer::Serialize(Json, Writer))
    return false;
  const FString Path = ProfilePath(), Temporary = Path + TEXT(".tmp");
  if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true) ||
      !FFileHelper::SaveStringToFile(
          Text, *Temporary,
          FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) ||
      !IFileManager::Get().Move(*Path, *Temporary, true, false, false, true))
    return false;
  bDirty = false;
  return true;
}
