# 全功能第二轮覆盖审计（非最终验收签字）

审计时间：2026-10-30 23:11–23:15 UTC；对象为 `c77483e` 后正在整合的工作区（多实施者并行修改，不能把本文件名理解为全绿签字）。只读检查实现调用链，对照 `gameplay-design-v0.1.md` 与 `mvp-prototype-config-v0.1.md`。本轮不依赖实施报告、不以目录数量/按钮/源码字符串代替执行。基线审计见 `REQUIREMENTS-AUDIT.md`。

## 结论先行

完整核心已经从“Quick原型”扩展成有真实状态转换的三模式、经济、传承、分层与波间单次消费存档。仍有**明确需求缺口/接线遗漏**，已逐项立即报告父任务和UI owner：第一层洞府祭炼入口错误禁用；异象完整数值/有利标记/影响光晕；无尽按波排名；心法参与破境；洞天专属洞主与专属法宝池；渐进仙品保底。后续修复必须以新代码或行为证据关闭，不因已派修复就标通过。

下列“源码闭环”只表示读到有效调用与状态变化；所有UE呈现、可点击、打包和真实性能仍 **NOT_RUN**。

## 本轮发现的未闭环需求（按发现时状态）

| ID | 优先级 | 发现与直接证据 | 修复/复核口径 |
|---|---|---|---|
| F01 | P0 | `SDywxRoot::RestPanel` 唯一“洞府/重铸/宝匣”入口使用 `HasFeature(Economy)` 禁用。层1传统局仍有Cave，`Economy::Identify`只需rest+cave，`ReceiveRelic`仍隐藏仙道灵纹；因此层1用户无法祭炼 | 已交UI。选中洞府即应允许进入，商店/重铸各自按层控制。需新账号传统局获得仙品→选Cave→祭炼UI实测 |
| F02 | P1 | `CampaignProfile::heroes` 只有跨模式混合 `bestWave`；`Campaign::Settle`取max；Legacy按插入顺序显示英雄，没有无尽成绩排序 | 已交root。实现独立无尽存活波数记录及排序（本地即可），且保存/加载、不同角色成绩不混淆；不能把“最佳10波”标签当排名 |
| F03 | P1 | Codex异象调用 `AnomalyDescription`，丢 `DescribeAnomaly`已有精确倍率/半径/伤害/时间；没有异象对当前构筑有利提示；World敌人只盾/异变光效，没异象影响光晕 | 已交UI/World。显示真实本局配置，按真实收益条件提示；受影响妖魔同色光晕；全六条预列灰色/亲历点亮 |
| F04 | P1 | `FusionRecipe`仅两SpellId；三条 `FusionCatalog`全双圆满术法；`Progression::CanFuse`只检查两术法满重，候选倾斜亦只有另一术法 | 已交root。设计明确支持“圆满术法+指定心法”，需要至少一个真实可执行类别与对应合法池/配方/道典/融合/保存测试；不要求强行达到每修士3条后期目标 |
| F05 | P1 | `CaveCatalog`有3个bossName，但 `Simulation::UpdateEnemies` Boss统一引雷/火墙/冲撞，无洞天行为分支；HUD只“洞主第n相”，道典只有流波夔君 | 已交root/UI。每洞天专属洞主不是仅命名配置，需实际生态/首领机制差异；名称要进入实际HUD与道典。每位2阶段本身已存在 |
| F06 | P1 | `SetRelicPool`被实际调用，但 `GenerateRelic`只是preferred ID重复加入权重，任何洞天仍抽所有同阶法宝；`BossRewards`也用全局池 | 已交root。这是偏好池，不是文档的“专属法宝池”。可定义共享池+本洞天专属池，保证专属部分在其他洞天不出现；明确展示池与概率 |
| F07 | P1 | `Progression::RollRarity`只有20波硬保底与luck/risk倍率，没有随未出仙品波数逐步提高；`Economy::TreasureProbabilities`反而已有pity增长 | 已交root。完整设计渐进保底与配套20波硬保底可同时满足；不能用硬保底替代渐进机制 |
| F08 | P2 | Legacy显示HeroSword20道韵可买，`PurchaseUnlock(HeroSword)`永远false（修士2已初始开放）；重铸UI仅排除Mystery但Unique也不可重铸 | 已交UI。初始开放项不应展示可购买；Unique显示不可重铸/不列入候选 |
| F09 | P1 | 道典敌人/异变仍模糊描述，缺200灵甲/30%减伤/40%分裂等配置数值；水灭火与对灼烧+25%已执行但未见道典交互条目 | 已交UI。所有规则透明，不仅法宝/术法条目透明；六职责及各洞天生态信息应与运行配置一致 |
| F10 | P2 | 完整模式 `StartCampaign`强制`sixWaves=false`；六波A/B仅旧`Start`测试入口可用 | 原型实验接入差距，不等同于三模式核心缺失。若声称配套AB均可玩，需把实验配置接正式Quick流程或明确实验仅离线尚未UE验证 |
| F11 | P1 | 每洞天专属机缘池先前仅Catalog静态数据 | root已先发现并交Simulation。复读时已出现`Simulation::DescribeOpportunity`按洞天选择3类碎片、确定性key不耗RNG；仍须UI真正调用并展示选择后内容，不把新描述API本身当闭环 |

## 按原始设计章节核对的覆盖矩阵

| 原始要求 | 本轮直接证据 | 当前判断 |
|---|---|---|
| 操作仅移动+遁术、自动术法、无主动神通 | Simulation Tick/Cast，Controller输入 | 源码闭环；手感UE待测 |
| Quick 4波调息/Traditional 10波机缘/Endless每5波洞主后继续 | Campaign ScheduleWave/CompleteWave/BeginNextWave/CompleteBoss；Simulation同步phase；集成测试到无尽第11波 | 源码闭环，不只是模式按钮 |
| Quick/Traditional初始开放、Endless传统首胜 | Campaign Start/HasFeature/FeatureTier，World StartFullRun，Navigation | 源码闭环；角色2初始开放属于原型延续，无强制仅1角色开局要求 |
| 层1/2/3和整理工具始终开放 | Progression ConfigureFullGame/SetSystemLayer；Economy layer2/layer3；Anomalies Reset；UI封存/跳过/整理 | 核心门控已接；F01需修；形态共鸣按账号层而非默认true |
| 6术法/4心法/5重，3选1，重悟+传承额外1 | LegalOffers/ChooseUpgrade/Reroll/ConfigureFullGame | 源码闭环 |
| 封存3次、跳过换修为灵石 | SealOffer/SkipUpgrade/TakeSkipStones→Simulation CreditStones | 已有真实命令；应回归不可重复消费同次顿悟 |
| 候选道象倾斜、配方倾斜、气运递减 | MakeOffers/Affinity/EffectiveLuck；OnKill与SpawnWave | 部分；F04心法配方缺口；高阶心法品级本身未定义，不能凭空要求一套稀有度系统 |
| 6元素5形态，2/4共鸣、融合继承计数 | Types/ContentCatalog + Progression Derive + Simulation form/element执行 | 真正有新行为，不只是扩充enum；30基础术法/额外融合数量是建议内容量 |
| 御兽跟随游走/符箓最快威胁/水灭火 | Simulation Companion/Talisman/Water分支；full_spell_behavior_tests实际断言位置、目标和伤害 | 源码+专门行为测试路径；战斗辨识与效果UE待测 |
| 四档法宝、随机主题灵纹、触发/玄机/独有/互斥、6槽+无限袋 | GenerateRelicFromCatalog、Compatible、GrantRelic、Equip | 源码闭环；“80”不是仅复制名，但扩充物品美术和体验未验 |
| 品阶颜色音效/仙道光柱/灵兽方位 | World RelicGrade0..3、CollectedGrade调音、RelicBeam；BattleScreen四方向BeastOffset | 有真正渲染/音频调用；音色区分、屏外箭头位置、颜色可读性必须UE验证 |
| 待祭炼、快速即时显纹 | Economy ReceiveRelic hidden_清affix；Identify恢复并处理互斥；quick分支 | 核心正确，F01入口缺陷 |
| 炼化两币固定汇率、锁定/预览/撤销/排序/筛选/对比/换装 | Economy Refine/Undo；RestPanel调用；对比有全11属性+会心伤害和红绿改善方向 | 源码闭环；悬停为纯文字全属性，着色面板在点击选择后，严格“悬停并排涨红跌绿”交互仍不完全等价，应UE核对/完善 |
| 所有妖魔稳定灵石、波末全屏吸取、局末清零 | ResolveDeaths生成stone pickup；CollectPickups；Economy EndRun | 源码闭环；收入/价格能否买2–4件待实测 |
| 洞府3–5件至少灵品、Quick3件无刷新、道品不卖、付费/免费刷新 | Economy GenerateShop/Refresh/EnterCave + UI交易命令 | 源码闭环；商城灵纹直接可见，不经过未知祭炼 |
| 重铸逐次涨价、新旧保留、玄机不可重铸 | BeginReforge/ResolveReforge/reforges_ + UI | 源码闭环；F08无效Unique按钮 |
| 宝匣免费机缘+额外付费上限5、揭示、概率公开、共享仙品保底 | OpenTreasure/RevealTreasure/TreasureProbabilities + UI | 有真实两步揭示，无动画效果验收；道品无保底 |
| 大妖/事件/洞府/宝匣三选一、3波出现保底、暂停不限时、手动下一波 | Campaign Offer强制Cave/Chest；ChooseOpportunity召唤真实Elite；Simulation阻止未解决机缘开始下一波 | 源码闭环；F11具体洞天事件UI待复核 |
| 独立生态、环境、专属池、每局演化、异象风险回报 | Cave roleWeights实际乘budget；GenerateArena/Constrain用于实体及World RebuildArena；6个Anomaly modifiers/events实际进入spawn/damage/xp/stones/vision | 地图/异象非装饰；F05/F06专属内容仍不足；F03透明提示不足 |
| 全图连通、中心开阔、≤25%障碍、边界切角、出生5秒、灵脉/陷阱/兽路线 | Arena ValidateArena BFS/ObstacleFraction/Contains/route；Simulation terrainTimer与Steer | 源码闭环；实际导航/视觉边界UE待测 |
| 六职责、4异变、两变Boss、所有威胁预警 | Simulation Enemy/Effect执行，AddEffect溢出保护 | 基础已有；F05每洞天Boss不足；预警可读性不可离线声称 |
| 道典灰列/亲历点亮/失败保留/配方全公开 | Codex完整法术1–5重/Relic范围/Fusion描述；Profile跨局发现；Anomaly encountered | 比基线完整，但F03/F09仍缺；待祭炼实际roll不应通过对比泄露 |
| 隐性叙事/不暴露科技 | 初版制作口号已去除；DescribeOpportunity新增仙侠碎片 | 专属事件文字已生成；UI调用与选择后可读待复核 |
| 道韵胜败结算、系数、风险、无尽Boss额外奖、幂等 | Campaign Reward/Settle+settledThrough；World SaveProfile先持久化再接受候选Profile | 源码闭环；不把重载/再次点击计重复奖励 |
| 传承解锁候选池/地图/角色/便利，永久≤10%、无气运 | PurchaseUnlock/PurchaseBonus；StartCampaign应用base、extraRerolls/contentUnlocks；FreeCaveRefresh | 源码闭环；F08 UI死按钮 |
| 劫数选项、风险真实战斗/掉落/结算、阶梯解锁 | GetTrialEffects被SpawnWave/Spawn/Anomaly/Progression/Economy/Reward使用 | 源码闭环，不只是选项数据 |
| 波间存档、恢复下一待迎战波开头、读档删除、Quick禁止 | PrepareCheckpoint复制投影next.BeginNextWave；SaveCodec校验；World ResumeRun先Candidate恢复验证，再ConsumeCheckpoint持久提交，最后ActivateRun | 源码闭环；保留RNG/经济/灵纹未知/次数/波次，真实UE磁盘故障仍待测 |
| 每修士1局、共享账号、单独展示成绩熟练度 | ProfileData checkpoints[3]按hero；StartFullRun阻止已有slot；Settle按hero更新；Legacy只展示不加数值 | 单slot已实现；F02无尽排名独立缺失；中途战斗不允许存档符合设计 |
| PC/2.5D/100+同屏、命名美术一致、爽感/再开率/曲线 | World相机/渲染；enemyCap；offline calibrations | 只能源码及离线模拟验证；UE帧耗、GPU、实际玩家体验/问卷未执行 |

## 不应冒充硬性未完的建议项／真正延期项

- 3洞天、3修士、30术法、约80法宝是建议首发数量；当前已实现相应数据量但仍必须检查类别的行为真实性。每修士至少3条路线明确为后期扩充目标，不要求本轮凭空做9条配方。
- 所有秒数、倍率、商价、掉率和收益目标属于原型待校准数值。配置有差别不自动判失败；但需要说明差别并验证效果，而不是无说明改动。当前新异象采用暂定参数，不能在道典仍展示旧表数字。
- 灵潮在主设计要求更多灵石，在配套表建议更多修为；当前实现更多灵石，属于主循环明确要求，不能为“缺XP+20%”判硬阻塞，但应记录采用口径。
- 本命法宝带出明确延后；移动端、联机明确后续；线上排名服务没有明确要求，本地无尽按存活波排名即可；三套完整新怪模型不是文档定量硬门槛，但剪影/职责可辨必须UE验证。
- 核心体验指标（前3分钟死亡率、90–130秒Boss、18–20顿悟、破境率/掉率、再开意愿）需要可靠模拟或真人样本。不能用一次test kill证明平衡目标满足。

## 最终验收边界

本报告只进行源码/测试路径审查，不拥有UE Editor、编译器、cook或运行画面。最终必须跑全套离线回归、存档失败注入和跨进程恢复，然后在UE完成三模式可玩路径、第一层祭炼、机缘与交易、无尽Boss连续循环、首次胜利解锁、每角色存读、防重领、屏幕分辨率/大字体/焦点/滚动与100+妖魔性能验收。

**禁止结论：**“所有功能已全绿”“UE已通过”“内容数量达标所以完整需求满足”。允许结论：已实现核心闭环、列出本表尚未关闭项及真实NOT_RUN门禁。

## 23:15 UTC 修复复读附录（覆盖上文“发现时状态”，不覆盖UE门禁）

已直接复读到而非只收到口头完成报告：

- **F01源码接线已修**：RestPanel调用`CanOpenRestServices(EconomyUnlocked,CaveSelected)`；Economy modal独立Layer2。第一层选Cave可进入祭炼，商店按层禁用。UE点击仍未验。
- **F03部分已修**：Codex改用真实`DescribeAnomaly`，雷象持有/站在当前阵台才显示有利；World增加`AnomalyAffectsEnemy`和分色`AnomalyHalo`实际渲染。**仍发现BattleScreen没有当前异象名称/图标/完整效果栏**，只有打开Codex才能看到规则；已立即交root追加。
- **F04核心已修、UI回归待修**：FusionCatalog新增3条圆满术法+心法路线，CanFuse/Fuse真读passive。现BattleScreen仍循环同修士配方并覆盖单一`Fusion`变量，导致旧双术法配方不可从UI选择；UI owner已报root请求多配方入口。不能仅据6条配方表宣称六条可玩。
- **F08源码已修**：HeroSword标“初始开放”且禁用购买；重铸列表同时排除Mystery和Unique。
- **F11描述已接UI**：RestPanel现在调用DescribeOpportunity绘制卡片，并在选择后保留片段内容。事件按洞天选择实际不同标题/叙事，奖励仍统一暂定值；未规定各事件必须不同数值，不因此伪造新缺陷。
- **F02/F07正在变更**：读到`Campaign::Settle`写endlessRanking并排序截20、Progression新增RarityProbabilities渐进实现；尚未在此次附录完成它们的存储/UI端到端复读，须root最终回归。
- **F05只修了名称部分**：HUD读Cave bossName，Codex列三洞主；目前新增条目却共同使用`Known.seenEnemies[Boss]`，遇任意洞主就点亮所有洞主，违反按亲历点亮，已交root。专属Boss真实机制和F06池过滤未在此附录确认关闭。
- **F09仍未修**：UI owner明确冻结前未新增妖魔/异变精确数值及水互动条目；必须如实保留。

本审计者独立运行结果（23:13–23:15）：`test-mvp-arena.sh`通过1000seed连通/边界/6异象门控，`test-mvp-economy.sh`通过，`test-mvp-save.sh`通过（含profile事务）。这是离线行为测试，不是UE验收，不覆盖随后尚在变更的所有文件。

### 23:16 UTC 继续复核

- F05：非雷泽Boss已实际调用`UpdateCaveBoss`，赤山火浪/环火、青岚根缚/不同随从的两阶段机制进入运行分支，不再只是名字差异；参数暂定，待平衡和UE预警验收。
- F06：`StartCampaign`已改用`RealmRelicPool`；`Progression::LegalRelic`保留原型共享15件，扩展内容必须已解锁且位于本洞天池；GenerateRelic与BossRewards同用过滤。源码功能类别已补齐。
- F07：真实`DropRarityProbabilities`在完整模式每未出仙品波把仙品权重提高10%（暂定），RollRarity直接取该概率，20波硬保底保留；非仅概率文案。
- F02：已读到Campaign的本地top20排序与Legacy展示；新排名/独立Boss发现字段在23:16读取的SaveCodec宏中尚未序列化，root正在收尾。不得在持久化完成前关闭跨重启覆盖。
- UI owner已收到一次性剩余清单：六配方可达、敌人/异变/水互动数值、严格悬停红绿并排对比、明确Boss转阶段提示。当前异象HUD/独立Boss发现和真实掉落概率展示由root追加交其处理；完成后仍须实际UE验证。

## 冻结快照最终源码复核 — 2026-10-30 23:20 UTC

**本节优先于前面动态工作区的发现状态。** 只读来源：`/tmp/dywx-full-frozen-20261030`；本轮仅修改原仓库本审计文档，不改快照和生产源码/测试。父任务正在该快照执行全量ASan门禁，本审计不抢先代报其结果。

复核文件SHA256：
- `Private/SDywxRoot.cpp`：`2f2bf47ae92df50aceee81741f4af5a289c0bfc321286f1015de9f713472f5a9`
- `Private/MVP/Simulation.cpp`：`e30ddd05fd3048ec71f0d176c3619ca41f875301914c247846b70baf5e484e5a`
- `Private/MVP/SaveCodec.cpp`：`3a3b1f375af02b8c81eec46cc74cff970399636559dcb4275fc58357489dbb80`

### 原11项逐条裁决

| 项 | 冻结源码证据（均在`Source/DywxUI`下） | 最终源码覆盖判断 |
|---|---|---|
| F01 第一层祭炼 | `Private/SDywxRoot.cpp:704`以`CanOpenRestServices(Economy,CaveSelected)`开放服务；Economy modal的Identify按Cave而非Layer2，商店独立Layer2；`Economy::Identify`恢复隐藏affixes并检查互斥 | **已接线**。不是仅放开入口后操作仍被层2拒绝 |
| F02 无尽排名 | `Private/MVP/Campaign.cpp:177–198`只无尽结算写rank、按波数/击杀排序、截20，幂等ledger在此前检查；`Private/SDywxRoot.cpp:484–489`实际展示；`SaveCodec.cpp:75–76,102–113`读写与排序/重复/长度验证；`tests/mvp_save_tests.cpp:56–65`有往返与非法排序断言 | **已接线，包含持久化**。每英雄HeroRecord仍独立且只展示；不是把通用bestWave冒充排行榜 |
| F03 异象透明和标记 | `SDywxRoot.cpp:286–306`Battle实时六象名/汉字徽记/剩余时间/关键数值+完整DescribeAnomaly动态tooltip，按active显示；Codex:455–465完整真实配置与有条件有利提示；World `AnomalyAffectsEnemy`、`AnomalyHalo`和Render逐敌调用 | **已接线**。UI大小、展开与实机颜色可读性仍UE待验；不是只在Codex补一句文本 |
| F04 心法配方/六配方 | `ContentCatalog::FusionCatalog`三条双术法+三条心法；`Progression.cpp:208,265–276`倾斜、持有检查、仅消耗指定术法、保留心法、继承计数；`SDywxRoot.cpp:319–339`逐配方创建独立按钮并捕获各自Fusion，非覆盖单变量；`full_content_tests.cpp:56–73`有心法融合与倾斜行为 | **已接线**。旧双术法配方未被新配方遮掉 |
| F05 专属洞主与发现 | `Simulation.cpp:1017`非雷泽分支调用`UpdateCaveBoss`；1835起实际火浪/环火和根缚/专属随从；World SaveProfile:204–209仅记录当前cave Boss；SaveCodec:76序列化seenBosses；Codex:447逐洞天读取独立flag与static DescribeBossMechanics；Battle:283显示当前洞主和4秒破相提示 | **已接线**。Boss美术/咆哮与红边等配套表现、阶段提示辨识度仍需UE核对，不据文字声明全部视听完成 |
| F06 专属法宝池 | `Simulation.cpp:118`调用RealmRelicPool；`ContentCatalog.cpp:531`按物品realm生成；`Progression.cpp:682–694`原型15件共享、扩展必须本realm池且已解锁，随机/坊市/宝匣复用GenerateRelic，BossRewards同LegalRelic；`full_content_tests.cpp:79–84`三洞天65扩展条目互斥且掉落/首领奖励过滤 | **已接线**。不是只给preferred权重 |
| F07 渐进/硬保底与公开概率 | `Progression.cpp:643–663`完整模式仙品权重每未出波+10%暂定，20波硬保底，RollRarity直接采样同一概率；Codex:451调用DescribeDropOdds普通/保底来源；测试有0/10/19/20波概率比较与真掉落 | **已接线**。具体每5/15局目标频率须统计，不据概率代码直接称已达成 |
| F08 两个死按钮 | Legacy:479修士2标初始开放且禁购；Economy Item同时排除Mystery和Unique，原来永远失败的入口已移除 | **已接线** |
| F09 数值透明/交互 | Codex:446–453实际调用Simulation::DescribeEnemy/DescribeMutation/DescribeElementInteractions；返回基础HP/速度/伤害/时序、200灵甲/30%/40%分裂、雷火水交互；不是孤立未调用的描述函数 | **数值/规则已接线，但保留一项信息呈现缺口**：DescribeEnemy:160–172没有名称/职责标题，Codex也不补前缀，七条是匿名基础数值+行为。需要名称映射才能明确辨认对象，已立即报告root |
| F10 六波A/B实验 | `Simulation.cpp:101,106,126`将RunConfig.sixWaves映射CampaignConfig.quickSixWaves并设6波；`Campaign.cpp:35–49`调度6波时长/强度/上限；SaveCodec:77持久化；`full_game_integration_tests.cpp:165`完整Campaign六波序列 | **运行核心已接线**。纠正此前“仍强制4波”的动态工作区旧结论。默认玩家菜单仍4波，不把无实验开关当新硬门槛 |
| F11 专属机缘 | `Simulation::DescribeOpportunity`按实际cave和确定性seed/wave/index选该洞天碎片；RestPanel:690–700实际用于选项描述及选择后保留内容；ChooseOpportunity执行真实XP/灵石/洞府/大妖/宝匣分支 | **已接线**。各事件奖励暂同值不违反未给具体数值的设计，不人为新增必须各事件不同数值的门槛 |

### 额外两项原审计呈现要求复核

- **悬停并排对比已真接**：`SDywxRoot::Comparison:126–146`构建当前/候选/差值四列、全11属性及会心伤害，红升绿降并将遁息降低视为改善；`Button:164`接自定义SToolTip；袋条目:736将`Comparison(Current,Compare,Tip)`传入。不是只有点击后的红绿面板。
- **存档与分层仍保留真实门控**：World先验证Candidate恢复再事务ConsumeCheckpoint，Profile三slot按hero；层1Cave祭炼例外不会连带开放Layer2商店/Layer3重铸；保存并离开只在成功写入后回菜单。源码条件复读通过，真实UE落盘/强退/失败恢复仍未验。

### 尚需明确说明的需求口径差异

冻结源`Simulation.cpp:877`在完整模式下火系命中直接`e.shield=0`；`DescribeElementInteractions`也明确写直接破盾。它符合完整玩法文档“火打雷盾过载破除”，但**不同于配套参数表9.3的对盾伤害×2**（原型路径仍保留原规则）。这是可观察的版本口径差异，不是未接线；交付说明应记录，不能悄悄声称两份文档此条完全逐字一致。若以配套数值表作为该行为唯一验收标准，则此条尚未严格符合，需设计裁决/统一。

### 未验收与禁止夸大

1. **UE / 真机**：编译、UHT、cook、启动、真实Slate点击/焦点/滚动/tooltip、分辨率与大字号、防遮挡、音效/光晕/预警/转阶段、真实磁盘存读与重启、100+妖魔CPU/GPU/帧率仍 **NOT_RUN**。
2. **真人体验**：前3分钟安全和无聊度、共鸣跃升感、御兽/符箓等形态可辨、走位遁术存在感、主动再开意愿及各选择分布未验。
3. **平衡统计**：每局仙/道频率、洞主90–130秒、18–20顿悟、30–40%破境、全模式单位时间道韵、正常收入能买2–4件尚需统计与校准；临时低BossHP/直接击杀的测试不能当这些目标证据。
4. **本审计最终措辞**：此前11项主要功能缺口已有实际源码链路；F09保留匿名敌人条目呈现问题，火破盾存在两文档口径差异。不得据此写“所有功能需求与验收均已完成”。父任务ASan最终结果应另引用真实输出。

### 父任务最后增量复核

匿名妖魔条目已在实际 `SDywxRoot.cpp` 道典循环添加名称与职责标题；数值仍调用统一 `DescribeEnemy`。该两行展示增量通过 UI 契约及70项字体测量，不改变纯核自然回放。完整模式火象直接破盾与旧MVP对盾×2的口径区别已在完整交接中明示，没有静默宣称两者数值完全一致。
