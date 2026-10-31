#include "MVP/DywxMVPProfile.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
namespace {
FString ProfilePath(){return FPaths::ProjectSavedDir()/TEXT("MVP/profile-v2.dat");}
bool WriteProfile(const std::string& Bytes){
  if(Bytes.size()>dywx::mvp::SaveCodec::MaxBytes)return false;
  const FString Path=ProfilePath(),Temporary=Path+TEXT(".tmp");
  TArray<uint8> Data;Data.Append(reinterpret_cast<const uint8*>(Bytes.data()),static_cast<int32>(Bytes.size()));
  return IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true)&&
    FFileHelper::SaveArrayToFile(Data,*Temporary)&&
    IFileManager::Get().Move(*Path,*Temporary,true,false,false,true);
}
template<size_t N>bool ReadFlags(const FJsonObject& Json,const TCHAR* Field,std::array<bool,N>& Flags){
  if(!Json.HasField(Field))return true; // Version 1 discovery fields were optional.
  const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
  if(!Json.TryGetArrayField(Field,Values)||!Values||Values->Num()>static_cast<int32>(N))return false;
  for(int32 Index=0;Index<Values->Num();++Index){
    if(!(*Values)[Index].IsValid()||(*Values)[Index]->Type!=EJson::Boolean)return false;
    Flags[static_cast<size_t>(Index)]=(*Values)[Index]->AsBool();
  }
  return true;
}
}
FDywxMVPProfile::FDywxMVPProfile():Store(WriteProfile){}
bool FDywxMVPProfile::Load(){
  bWritable=false;const FString Path=ProfilePath();
  if(IFileManager::Get().FileExists(*Path)){
    const int64 Size=IFileManager::Get().FileSize(*Path);TArray<uint8> Bytes;
    if(Size<=0||Size>static_cast<int64>(dywx::mvp::SaveCodec::MaxBytes)||!FFileHelper::LoadFileToArray(Bytes,*Path))return false;
    const std::string Blob(reinterpret_cast<const char*>(Bytes.GetData()),Bytes.Num());
    if(!Store.Load(Blob))return false;
    bWritable=true;return true;
  }
  const FString LegacyPath=FPaths::ProjectSavedDir()/TEXT("MVP/profile.json");
  if(!IFileManager::Get().FileExists(*LegacyPath)){bWritable=true;return true;}
  const int64 Size=IFileManager::Get().FileSize(*LegacyPath);FString Text;TSharedPtr<FJsonObject> Json;
  if(Size<=0||Size>65536||!FFileHelper::LoadFileToString(Text,*LegacyPath)||
    !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Json)||!Json.IsValid())return false;
  double Version=0,Pity=0;
  if(!Json->TryGetNumberField(TEXT("version"),Version)||Version!=1)return false;
  if(Json->HasField(TEXT("pityWaves"))&&(!Json->TryGetNumberField(TEXT("pityWaves"),Pity)||
     !FMath::IsFinite(Pity)||Pity<0||Pity>20||Pity!=double(static_cast<int>(Pity))))return false;
  dywx::mvp::ProgressionState Known;Known.pityWaves=static_cast<int>(Pity);
  if(!ReadFlags(*Json,TEXT("spells"),Known.seenSpells)||!ReadFlags(*Json,TEXT("relics"),Known.seenRelics)||
     !ReadFlags(*Json,TEXT("fusions"),Known.seenFusions)||!ReadFlags(*Json,TEXT("passives"),Known.seenPassives)||
     !ReadFlags(*Json,TEXT("enemies"),Known.seenEnemies)||!ReadFlags(*Json,TEXT("mutations"),Known.seenMutations)||
     !ReadFlags(*Json,TEXT("resonances"),Known.seenResonances)||!Store.ImportLegacy(Known))return false;
  // Keep the legacy file untouched; the first successful save creates version 2.
  bWritable=true;return true;
}
void FDywxMVPProfile::Apply(dywx::mvp::Progression& P)const{P.ImportAccountProgress(GetKnown());}
void FDywxMVPProfile::Observe(const dywx::mvp::ProgressionState& S){Store.Observe(S);}
bool FDywxMVPProfile::Save(){return bWritable&&Store.Save();}
std::uint64_t FDywxMVPProfile::AllocateRunId(){return bWritable?Store.AllocateRunId():0;}
bool FDywxMVPProfile::SaveCheckpoint(dywx::mvp::HeroId H,const std::string& B){return bWritable&&Store.SaveCheckpoint(H,B);}
bool FDywxMVPProfile::HasCheckpoint(dywx::mvp::HeroId H)const{return Store.HasCheckpoint(H);}
bool FDywxMVPProfile::GetCheckpoint(dywx::mvp::HeroId H,std::string& B)const{return Store.GetCheckpoint(H,B);}
bool FDywxMVPProfile::ConsumeCheckpoint(dywx::mvp::HeroId H,std::string& B){return bWritable&&Store.ConsumeCheckpoint(H,B);}
