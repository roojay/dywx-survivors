#include "MVP/Arena.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <sstream>
namespace dywx::mvp {
namespace {
constexpr float Pi=3.14159265358979323846f;
float Length(Vec2 p) { return std::sqrt(p.x*p.x+p.y*p.y); }
float SegmentDistance(Vec2 p,Vec2 a,Vec2 b){Vec2 d=b-a;float n=d.x*d.x+d.y*d.y;float t=n>0?std::clamp(((p.x-a.x)*d.x+(p.y-a.y)*d.y)/n,0.f,1.f):0;return Length(p-(a+d*t));}
struct Rng { std::uint64_t s; std::uint64_t Next(){s+=0x9e3779b97f4a7c15ULL; auto z=s; z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL; z=(z^(z>>27))*0x94d049bb133111ebULL; return z^(z>>31);} float Unit(){return float(Next()>>40)/16777216.f;} };
float Safe(float v,float fallback,float low,float high){return std::isfinite(v)?std::clamp(v,low,high):fallback;}
}
const std::array<CaveDefinition,3>& CaveCatalog(){
 static const std::array<CaveDefinition,3> c{{
 {CaveId::ThunderMarsh,"雷泽洞天","流波夔君","青金雷泽",{"泽畔魑魅","雷纹赤狰","泽火毕方","青磷火","雷泽阵灵","泽中蜃"},{1,.9f,.8f,1,1,.7f},{0,4,8,12},{"雷泽遗刻","旋龟引路","雷池洗玉"}},
 {CaveId::EmberMountain,"赤山洞天","赤山离火君","朱砂玄岩",{"赤山魈","赤岩狰","离火毕方","赤磷火","熔岩阵灵","炎霞蜃"},{1,1.3f,1.3f,1.4f,.6f,.6f},{1,5,9,13},{"火羽遗痕","赤泉问道","熔岩古匣"}},
 {CaveId::MistForest,"青岚洞天","青岚蜃君","苍翠月白",{"林间魑魅","青纹赤狰","雾羽毕方","幽磷火","古木阵灵","青岚蜃"},{1,.7f,1.2f,.7f,1.4f,1.5f},{2,6,10,14},{"古木残篇","白鹿引路","雾海问心"}}
 }}; return c;
}
std::string_view AnomalyName(AnomalyId id){static constexpr std::array<std::string_view,6> names{"雷泽引雷","灵潮涨落","地脉震荡","灵雾","月蚀","阵台共鸣"}; auto i=static_cast<std::size_t>(id);return i<names.size()?names[i]:"";}
std::string_view AnomalyDescription(AnomalyId id){static constexpr std::array<std::string_view,6> text{
 "灵脉定期引雷，先现预警；雷象修士入雷可免此次雷伤并暂获雷象增伤。",
 "妖潮更加密集，妖魔掉落的灵石增加；坊市和炼化汇率不变。",
 "灵脉周期裂开并预警，冲锋妖魔更多；裂隙同时伤及妖魔。",
 "雾中远眺受限，远程妖魔更多；灵兽更常现身，术法射程不变。",
 "夜行妖魔威力提高，击杀修为增加。",
 "灵脉阵台显现，驻足阵台威力提高；固守位置须承受妖潮围攻。"};auto i=static_cast<std::size_t>(id);return i<text.size()?text[i]:"";}
bool ArenaLayout::Contains(Vec2 p,float radius) const {
 if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(radius))return false;
 const float r=std::max(0.f,radius),x=std::abs(p.x),y=std::abs(p.y);
 return x<=config.halfWidth-r && y<=config.halfHeight-r && x+y<=config.halfWidth+config.halfHeight-config.cornerCut-r*1.414214f;
}
bool ArenaLayout::Walkable(Vec2 p,float r) const {if(!Contains(p,r))return false; for(auto& o:obstacles)if(Length(p-o.position)<o.radius+r)return false;return true;}
Vec2 ArenaLayout::Constrain(Vec2 p,float radius) const {
 if(!std::isfinite(p.x)||!std::isfinite(p.y))p={};
 const float r=Safe(radius,.45f,0,std::min(config.halfHeight,config.halfWidth)*.4f);
 for(int pass=0;pass<8;++pass){
  p.x=std::clamp(p.x,-config.halfWidth+r,config.halfWidth-r);p.y=std::clamp(p.y,-config.halfHeight+r,config.halfHeight-r);
  float excess=std::abs(p.x)+std::abs(p.y)-(config.halfWidth+config.halfHeight-config.cornerCut-r*1.414214f);
  if(excess>0){p.x-=std::copysign(excess*.5f,p.x);p.y-=std::copysign(excess*.5f,p.y);}
  for(auto& o:obstacles){Vec2 d=p-o.position;float len=Length(d),needed=o.radius+r+.001f;if(len<needed)p=o.position+(len>.0001f?d*(needed/len):Vec2{needed,0});}
  if(Walkable(p,r))return p;
 } return {}; // Bounded safe fallback: generation reserves the central clear zone.
}
float ArenaLayout::ObstacleFraction()const{float a=0;for(auto& o:obstacles)a+=Pi*o.radius*o.radius;return a/(4*config.halfWidth*config.halfHeight-2*config.cornerCut*config.cornerCut);}
bool ValidateArena(const ArenaLayout& l){
 if(!l.Walkable({},l.config.centralClearRadius)||l.ObstacleFraction()>l.config.maxObstacleFraction+.00001f)return false;
 const float g=l.config.gridSize;
 if(!std::isfinite(g)||g<=0||!std::isfinite(l.config.halfWidth)||!std::isfinite(l.config.halfHeight)||l.config.halfWidth<=0||l.config.halfHeight<=0||2*l.config.halfWidth/g>1023||2*l.config.halfHeight/g>1023)return false;
 const int w=int(2*l.config.halfWidth/g)+1,h=int(2*l.config.halfHeight/g)+1;
 if(w<2||h<2||w>1024||h>1024)return false;
 std::vector<unsigned char> walk(w*h),seen(w*h);int total=0,start=-1;
 for(int y=0;y<h;++y)for(int x=0;x<w;++x){int i=y*w+x;Vec2 p{-l.config.halfWidth+x*g,-l.config.halfHeight+y*g}; if(l.Walkable(p,l.config.navigationClearance)){walk[i]=1;total++;start=i;}}
 if(start<0)return false;
 // The edge band must not have terminal one-cell branches that trap pursuit.
 for(int y=0;y<h;++y)for(int x=0;x<w;++x){
  if(!walk[y*w+x])continue;
  float px=-l.config.halfWidth+x*g,py=-l.config.halfHeight+y*g;
  if(std::abs(px)<l.config.halfWidth-l.config.peripheralMargin && std::abs(py)<l.config.halfHeight-l.config.peripheralMargin)continue;
  int neighbors=0;
  for(auto d:std::array<std::array<int,2>,4>{{{1,0},{-1,0},{0,1},{0,-1}}}){int nx=x+d[0],ny=y+d[1];if(nx>=0&&ny>=0&&nx<w&&ny<h&&walk[ny*w+nx])++neighbors;}
  if(neighbors<2)return false;
 }
 std::queue<int> q;q.push(start);seen[start]=1;int count=0;
 while(!q.empty()){int i=q.front();q.pop();++count;int x=i%w,y=i/w;for(auto d:std::array<std::array<int,2>,4>{{{1,0},{-1,0},{0,1},{0,-1}}}){int nx=x+d[0],ny=y+d[1];if(nx<0||ny<0||nx>=w||ny>=h)continue;int j=ny*w+nx;if(walk[j]&&!seen[j]){seen[j]=1;q.push(j);}}}
 if(count!=total)return false;
 for(std::size_t i=0;i<l.beastRoute.size();++i){
  Vec2 a=l.beastRoute[i],b=l.beastRoute[(i+1)%l.beastRoute.size()];
  if(!l.Walkable(a,l.config.navigationClearance))return false;
  for(auto o:l.obstacles)if(SegmentDistance(o.position,a,b)<o.radius+l.config.navigationClearance)return false;
 }
 for(auto p:l.edgeSpawns)if(!l.Walkable(p,l.config.navigationClearance))return false;
 return true;
}
ArenaLayout GenerateArena(std::uint64_t seed,CaveId cave,const ArenaConfig& input){
 ArenaLayout l;l.seed=seed;l.cave=cave;l.config=input;auto& c=l.config;
 c.halfWidth=Safe(c.halfWidth,48,24,120);c.halfHeight=Safe(c.halfHeight,27,18,80);
 c.cornerCut=Safe(c.cornerCut,5,0,8);c.centralClearRadius=Safe(c.centralClearRadius,10,3,12);
 c.gridSize=Safe(c.gridSize,1,.5f,2);c.navigationClearance=Safe(c.navigationClearance,.6f,.1f,1);
 c.peripheralMargin=Safe(c.peripheralMargin,4,3,8);c.minObstacleRadius=Safe(c.minObstacleRadius,1,.5f,3);
 c.maxObstacleRadius=Safe(c.maxObstacleRadius,2.5f,c.minObstacleRadius,4);
 c.maxObstacleFraction=Safe(c.maxObstacleFraction,.25f,0,.25f);
 c.obstacleCount=std::clamp(c.obstacleCount,0,96);c.placementAttempts=std::clamp(c.placementAttempts,0,2048);c.veinCount=std::clamp(c.veinCount,1,32);
 Rng rng{seed^((std::uint64_t(cave)+1)*0x7856ab321ULL)};
 const float rx=c.halfWidth-c.peripheralMargin,ry=c.halfHeight-c.peripheralMargin;
 // This reserved octagonal outer loop has no terminal branches or sharp corner traps.
 for(int i=0;i<32;++i){float a=2*Pi*i/32;Vec2 p{rx*std::cos(a),ry*std::sin(a)};l.beastRoute.push_back(p);l.edgeSpawns.push_back(p);}
 for(int i=0;i<c.placementAttempts&&int(l.obstacles.size())<c.obstacleCount;++i){float r=c.minObstacleRadius+(c.maxObstacleRadius-c.minObstacleRadius)*rng.Unit();Vec2 p{(rng.Unit()*2-1)*(rx-4),(rng.Unit()*2-1)*(ry-4)};
  if(Length(p)<c.centralClearRadius+r+1||!l.Contains(p,r+3))continue;
  bool safe=true;for(auto o:l.obstacles)if(Length(p-o.position)<r+o.radius+2*c.navigationClearance+2*c.gridSize)safe=false;
  for(std::size_t j=0;j<l.beastRoute.size();++j)if(SegmentDistance(p,l.beastRoute[j],l.beastRoute[(j+1)%l.beastRoute.size()])<r+2.5f)safe=false;
  if(!safe)continue;
  l.obstacles.push_back({p,r});if(l.ObstacleFraction()>c.maxObstacleFraction)l.obstacles.pop_back();
 }
 for(int i=0;i<c.veinCount;++i){std::size_t index=(i*32/c.veinCount+int(rng.Next()%3))%32;l.veins.push_back(l.beastRoute[index]);l.hazards.push_back(l.beastRoute[index]);}
 if(!ValidateArena(l)){l.obstacles.clear();l.usedFallback=true;}
 return l;
}
std::string DescribeAnomaly(AnomalyId id,const AnomalyConfig& c){
 std::ostringstream s;s<<AnomalyName(id)<<"："<<AnomalyDescription(id)<<" 持续 "<<c.activeDuration<<" 秒；轮换间隔 "<<c.interval<<" 秒。";
 switch(id){
 case AnomalyId::Lightning:s<<"预警 "<<c.warning<<" 秒，落雷伤害 "<<c.lightningDamage<<"，半径 "<<c.hazardRadius<<" 格，落雷后 "<<c.pulseInterval<<" 秒再次预警；雷象威力 ×"<<c.thunderReward<<"，持续 "<<c.thunderRewardDuration<<" 秒。";break;
 case AnomalyId::SpiritTide:s<<"妖潮数量 ×"<<c.tideSpawn<<"，灵石掉落 ×"<<c.tideCurrency<<"。";break;
 case AnomalyId::Fissure:s<<"预警 "<<c.warning<<" 秒，裂隙伤害 "<<c.fissureDamage<<"，半径 "<<c.hazardRadius<<" 格；冲锋妖魔权重 ×"<<c.fissureChargers<<"。";break;
 case AnomalyId::Fog:s<<"远眺 "<<c.fogVision<<" 格，远程妖魔权重 ×"<<c.fogRanged<<"，灵兽出现权重 ×"<<c.fogBeasts<<"。";break;
 case AnomalyId::Eclipse:s<<"夜行妖魔威力 ×"<<c.eclipseEnemyPower<<"，修为获取 ×"<<c.eclipseXp<<"。";break;
 case AnomalyId::Array:s<<"阵台半径 "<<c.arrayRadius<<" 格，站立时威力 ×"<<c.arrayPower<<"。";break;
 case AnomalyId::Count:break;
 }return s.str();
}
void AnomalySystem::Reset(std::uint64_t seed,const ArenaLayout& layout,bool layerTwo,const AnomalyConfig& input){
 layout_=layout;config_=input;state_={};state_.enabled=layerTwo;cycle_=0;clock_=0;thunderUntil_=0;strikeAt_=-1;
 auto& c=config_;c.interval=Safe(c.interval,24,2,3600);c.activeDuration=Safe(c.activeDuration,16,1,c.interval);
 c.warning=Safe(c.warning,1.2f,.1f,c.activeDuration);c.pulseInterval=Safe(c.pulseInterval,4,c.warning+.1f,3600);
 c.hazardRadius=Safe(c.hazardRadius,2.2f,.1f,10);c.lightningDamage=Safe(c.lightningDamage,22,0,10000);c.fissureDamage=Safe(c.fissureDamage,16,0,10000);
 c.thunderReward=Safe(c.thunderReward,1.3f,1,10);c.thunderRewardDuration=Safe(c.thunderRewardDuration,5,0,120);
 c.tideSpawn=Safe(c.tideSpawn,1.35f,1,10);c.tideCurrency=Safe(c.tideCurrency,1.5f,1,10);c.fissureChargers=Safe(c.fissureChargers,1.8f,1,10);
 c.fogVision=Safe(c.fogVision,14,4,1000);c.fogRanged=Safe(c.fogRanged,1.8f,1,10);c.fogBeasts=Safe(c.fogBeasts,2,1,10);
 c.eclipseEnemyPower=Safe(c.eclipseEnemyPower,1.3f,1,10);c.eclipseXp=Safe(c.eclipseXp,1.35f,1,10);c.arrayPower=Safe(c.arrayPower,1.25f,1,10);c.arrayRadius=Safe(c.arrayRadius,3,.1f,10);
 for(int i=0;i<6;++i)order_[i]=static_cast<AnomalyId>(i);
 Rng rng{seed^0x9e8ca6713ULL};for(int i=5;i>0;--i)std::swap(order_[i],order_[rng.Next()%std::uint64_t(i+1)]);
 nextStart_=c.interval;nextPulse_=0;endAt_=0;
}
std::vector<ArenaHazardEvent> AnomalySystem::Step(float dt,Vec2 player,bool hasThunder){
 std::vector<ArenaHazardEvent> events;
 if(!state_.enabled||!std::isfinite(dt)||dt<=0)return events;
 // Engine fixed steps are tiny; clamp pathological caller input to bound event allocation.
 const double until=clock_+std::min(dt,3600.f);
 while(true){
  double next=state_.active?endAt_:nextStart_;
  if(state_.active&&(state_.id==AnomalyId::Lightning||state_.id==AnomalyId::Fissure)){
   if(strikeAt_>=0)next=std::min(next,strikeAt_);else if(nextPulse_+config_.warning<=endAt_)next=std::min(next,nextPulse_);
  }
  if(next>until)break;
  clock_=next;
  if(!state_.active){
   state_.active=true;state_.id=order_[cycle_%6];state_.encountered[static_cast<std::size_t>(state_.id)]=true;
   state_.anchor=layout_.veins.empty()?Vec2{12,0}:layout_.veins[cycle_%layout_.veins.size()];
   ++cycle_;endAt_=clock_+config_.activeDuration;nextPulse_=clock_;strikeAt_=-1;
  }else if(strikeAt_>=0&&strikeAt_<=clock_){
   bool absorbed=state_.id==AnomalyId::Lightning&&hasThunder&&Length(player-state_.anchor)<=config_.hazardRadius;
   if(absorbed)thunderUntil_=clock_+config_.thunderRewardDuration;
   events.push_back({state_.id,state_.anchor,config_.hazardRadius,state_.id==AnomalyId::Lightning?config_.lightningDamage:config_.fissureDamage,false,absorbed});strikeAt_=-1;nextPulse_=clock_+config_.pulseInterval;
  }else if(clock_>=endAt_){state_.active=false;nextStart_=endAt_+std::max(0.f,config_.interval-config_.activeDuration);strikeAt_=-1;
  }else{
   events.push_back({state_.id,state_.anchor,config_.hazardRadius,0,true,false});strikeAt_=clock_+config_.warning;
  }
 }
 clock_=until;state_.elapsed=float(clock_);state_.remaining=state_.active?float(std::max(0.,endAt_-clock_)):0;
 state_.warningRemaining=strikeAt_>=0?float(std::max(0.,strikeAt_-clock_)):0;
 return events;
}
AnomalyModifiers AnomalySystem::Modifiers(Vec2 player)const{
 AnomalyModifiers m;if(!state_.enabled)return m;
 if(clock_<thunderUntil_)m.thunderPower=config_.thunderReward;
 if(!state_.active)return m;
 switch(state_.id){
 case AnomalyId::Lightning:break;
 case AnomalyId::SpiritTide:m.spawn=config_.tideSpawn;m.currency=config_.tideCurrency;break;
 case AnomalyId::Fissure:m.chargers=config_.fissureChargers;break;
 case AnomalyId::Fog:m.visionRange=config_.fogVision;m.ranged=config_.fogRanged;m.beasts=config_.fogBeasts;break;
 case AnomalyId::Eclipse:m.nightEnemyPower=config_.eclipseEnemyPower;m.xp=config_.eclipseXp;break;
 case AnomalyId::Array:if(Length(player-state_.anchor)<=config_.arrayRadius)m.playerPower=config_.arrayPower;break;
 case AnomalyId::Count:break;
 }return m;
}
} // namespace dywx::mvp
