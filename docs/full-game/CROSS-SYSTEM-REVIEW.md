# 完整系统交叉审查（代码集成阶段）

审查范围：Campaign / Economy / Arena / 扩容后的 Progression / Simulation / SaveCodec 接线。位置以函数名为主，避免并行集成时行号漂移。依据为 `gameplay-design-v0.1.md` 与 `mvp-prototype-config-v0.1.md`；后者明确数值优先于自行猜测。以下“已修复”不表示 Unreal 实机与人类体验验收已经通过。

## 已发现并修复的问题

| 编号 | 位置 | 影响、复现/反例 | 修复与验证 |
|---|---|---|---|
| C1 | `Campaign.cpp::ScheduleWave(Quick)` | 自审发现完整Quick错误复用暂定新曲线：首波120凡妖+1精英，而配套§9.4/9.5是240凡妖+10磷火、0精英。经验预算不足且提前引入精英；血量/人口曲线也偏表。 | Quick恢复原表：人口40/70/100/130、血量1/1.5/2.2/3.2、伤害1/1.15/1.3/1.5、精英0/0/1/2及6职责预算。`mvp_campaign_tests.cpp`新增首/末波数值断言。传统/无尽仍为明确暂定值。 |
| E1 | `Economy.cpp::TreasureProbabilities`；`Simulation.cpp::StartCampaign` | 宝匣原只有气运/保底乘数，同一气运、保底下风险0与风险4概率完全一样。玩法要求风险提高高品阶权重。 | `EconomyConfig::rarityWeight`（有限[1,10]）加入非凡品权重；Simulation传入风险系数，SaveCodec序列化该值。回归比较1与1.6时仙/道概率严格提高。 |
| E2 | `Economy.cpp::BeginReforge/ResolveReforge/RestoreState` | 灵品已有`Power,Fire,Trigger6`，重铸Power从同品阶样本抽到Trigger5会产生两条触发；配套§8.1明确最多1条触发。 | 候选、接受与读档三个边界均验证替换后Trigger≤1；100种子×10次重铸回归。没有擅自禁止重复属性或重复共鸣；文档未给这种全局禁令。 |
| E3 | `Economy.cpp::RestoreState` | 构造待揭示宝匣与未售商品同一实体id，原来能恢复，后续购买/揭示可能丢物或重复状态。另pending reforge.oldAffix可与当前槽不同。 | 检查宝匣与库存商品、装备/袋、炼化撤销实体id冲突；重铸旧槽一致性与候选结构验证。非法恢复原子拒绝，回归确认。 |
| E4 | `Economy.cpp::CommitRestExperience`；`Simulation.cpp::FinalizeRestExperience/PrepareCheckpoint` | 炼化XP暂存，保存准备的副本执行EndRest才升级；副本出现待选顿悟被拒绝，而原局没有候选，无法完成存档准备。 | 保存前可在真实调息状态提交XP（不离开洞府、不重抽商品），显示顿悟后玩家选择；XP撤销此时终止、灵石撤销保留。独立两次AddRawXp避免合计整数溢出。重复提交不重复发奖，回归覆盖。 |
| P1 | `Progression.cpp::RollRarity` | 精英/灵兽权重`{0,98.3,1.5,.2}`全部非零项乘同一气运×风险，归一化完全抵消。提升风险对保证灵品掉落实际没作用。 | 保证灵品池保留Spirit基线，只放大仙/道权重；普通池仍放大非凡品。内容owner已修，复读代码确认循环从`elite ? 2 : 1`开始。 |
| P2 | `Progression.cpp::SetContentUnlocks/LegalOffers/GenerateRelic`；`Simulation.cpp::StartCampaign` | 初稿中SpellPool/RelicPool传承购买仅设置位，完整模式直接开放全部内容，没有实效。 | 内容owner增加候选池解锁状态，Simulation传账号位；基础/第三修士路线保留可用，扩展内容由购买开启。需随完整内容测试持续验证。 |
| S1 | `Simulation.cpp::ResolveDeaths` | 灵潮×1.5使用`floor((1.5-1)*1)=0`额外灵石，普通妖魔从不增收；改成普通round又会恒定2倍。 | 改为拾取物灵石并使用模拟RNG随机取整，期望1.5，波末统一吸取；SaveCodec保存模拟RNG。复读接线确认。 |
| S2 | `Simulation.cpp` 异象warning分支、`AddEffect` | 效果池满时异象预警AddEffect失败，随后仍直接Hurt；原预警还未标hostile，无法使用危险预警替换策略。违反威胁先预警。 | 标记hostile并记录预警实际呈现成功；失败脉冲不伤玩家。复读代码确认；仍需实机视觉检查预警清晰度。 |
| D1 | `CampaignProfile::seenAnomalies`；`SaveCodec` profile字段 | 异象遇见状态只存在本局，退出后道典会重新灰色，违背逐页点亮。 | 增加账号六项发现位，SaveCodec持久化。复读确认`DywxMVPWorld::SaveProfile`合并当前encountered，`SDywxRoot`道典读取账号位或当前位。 |

## 已核实不是缺陷／需明确展示的约束

1. **20波仙品100%保底合法。** `Progression::RollRarity`与`Economy::TreasureProbabilities`达到阈值返回仙品，并非误把“逐步抬高”写成必出：配套配置§8.9明确“连续20波未出仙品，下一件随机掉落必为仙品”。洞主奖励不计入随机掉落保底；道品无保底。
2. **炼化灵石后消费可能无法撤销。** `Economy::UndoRefine`若当前余额小于该笔炼化所得返回false；反例：炼化得15、买货后剩0，不能既保留买入物又免费复原被炼化物。事务没有先扣款再失败的问题。复读确认UI已显示“没有可撤销记录，或所得灵石已花费、余额不足”，而非无声失败。若要求任意时点无条件撤销，应另设计交易链回滚，不能偷偷允许负余额。
3. **重铸数值来自同品阶合法生成器。** `BeginReforge`复用`GenerateRelic(rarity)`，不是手写无边界随机值。Mystery与Unique拒绝重铸；保留旧词条仍支付此次尝试费用、下次价格递增，这与“可保留原来的”不矛盾。重复普通属性/共鸣没有文档硬禁令，因此不自行限制。
4. **3波机缘保底是候选出现，而非强制选择。** `Campaign::Offer`在任意连续3次候选中保证洞府、第三层起宝匣；第一/二层不能为满足保底而泄漏第三层宝匣。快速没有机缘候选。
5. **死亡结算与明确公式。** 默认道韵为波数×模式×风险并向下取整，无尽另计洞主；击杀记录展示但默认不加奖，遵从末尾“奖励口径”而非前文模糊描述。传统风险1、到达10波验证结果23，防止float归一化导致22。
6. **永久加成预算更保守。** 账号全部项目合计10%，气运无购买枚举。便利项独立不占预算；修士熟练度仅展示。`ApplyPermanentBonuses`对损坏超额数据再次截断，必须仅在构筑重算基底上应用一次。
7. **临时编译失败不可当终态缺陷。** 集成早期曾因`AnomalySystem`尚未添加SaveCodec friend导致编译失败；重新读取后接口已完成，`test-mvp-save.sh`通过。记录此事件仅说明并行审核必须复查，不列为剩余问题。

## 保存与事务接线检查

- `ReserveRunId`/`ProfileStore::AllocateRunId`单调分配并通过writer持久化；每修士固定高水位`settledThrough[3]`防旧快照重复领奖，避免无限账本。不同修士可任意顺序结算。
- `PrepareCheckpoint`只接受完整传统/无尽调息，必须选完机缘、洞主奖、宝匣揭示/重铸；未选顿悟及已抽候选可原样保存；在副本上投影到下一波或洞主开头。快速、战斗中保存拒绝。
- `RunCheckpoint`保存模拟RNG、Progression、Economy、Campaign机会RNG/选择结果、Arena、Anomaly内部时间与循环、冷却与实体序号，不能只存面板数值。
- `ProfileStore::ConsumeCheckpoint`先在writer事务契约下提交槽位消费成功，才返回blob供恢复；注入writer失败时原槽不丢。UE真实文件替换、断电耐久性尚未验证。每修士一个槽、账号共享传承。`SaveCodec::ValidateCheckpoint`复验阶段、英雄、境界/实体、经济与地图。
- 本审查检查到的字段变更`rarityWeight`与`seenAnomalies`已经进入SaveCodec字段清单。新增字段必须版本一致，不能只改头文件遗漏编码。

## 地图/异象核查与未声称完成的验收

`Arena.cpp::GenerateArena/ValidateArena`使用中心安全圈、外围连续路线、障碍面积≤25%、网格连通检查、边缘无单格终端检查和有限尝试后清障fallback；种子控制布局。`AnomalySystem`仅由模拟战斗tick推进，暂停/整理不推进，六种固定可读规则提供风险与收益，无直接削弱已成型道象。

这不等于三种洞天美术、镜头遮挡、音效与场景材质已有人类验收；部分洞天差异通过生态权重/命名/法宝倾向/布局表现。传统10×198秒=33分钟妖潮是**配置目标**，加约2分钟洞主是预算估计，不是已经用真人完整玩到35分钟的测量。100+同屏流畅性与最终爽感需要真实Unreal构建与性能/人类试玩。

## 本轮执行的验证

- `scripts/test-mvp-campaign.sh`：通过，包括300种子机缘、101波无尽、风险/结算/非法输入及Quick数值回归。
- `scripts/test-mvp-economy.sh`：通过，新增风险概率、恢复冲突、重铸触发上限、保存前XP提交回归。
- `ASAN_OPTIONS=detect_leaks=0 SANITIZE=1 scripts/test-mvp-economy.sh`：通过。当前环境LeakSanitizer不支持ptrace，地址与UB检查仍开启。
- `scripts/test-full-content.sh`：通过。
- `scripts/test-mvp-save.sh`：通过。
- `scripts/test-full-game-integration.sh`：最终重跑通过（三级系统、10波/无尽洞主、结算、经济、布局）。
- Quick数值修复后`ASAN_OPTIONS=detect_leaks=0 SANITIZE=1 scripts/test-mvp-campaign.sh`再次通过。

## 审查后的硬需求补齐（冻结前已接线）

- Campaign新增无尽独立本地Top20，按波数/击杀排序、并列id固定，重复结算先由高水位拒绝，纯核覆盖跨修士和20上限；不混入Quick/Traditional、不联网。
- Campaign.Restore补充无尽历史Boss计数校验，拒绝例如第23波却未通过任何Boss的坏快照。
- 完整Quick显式6波A/B配置与默认4波共存，配套B表时间/血量/人口不再丢失，回归完整6波到Boss及结算。
- `CampaignProfile.seenBosses[3]`区分三处洞主发现，不能遇见一种就全点亮；已复读确认SaveCodec字段+roundtrip测试、World按cave实际Boss亲历合并、UI独立灰亮均已接线。

冻结后的自然10例结果（含真正新号层1）见`NATURAL-PLAY-CHECK.md`：机制推进证据与平衡失败/超时分开记录，不以全解锁fixture替代新人验证。
