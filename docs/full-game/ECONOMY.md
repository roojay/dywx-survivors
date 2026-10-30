# 局内经济实现与接线合同

`Economy.h/.cpp` 是可执行领域模块，不代表整套 UI 已验收。依赖 Progression 的受限 friend 访问用于同事务背包变更、祭炼和属性重算；没有 const_cast。

## 已实现

- 第1层只能炼化修为；第2层灵石/坊市/炼化币；第3层重铸/宝匣。所有交易再检查整理期，洞府重铸/祭炼额外检查洞府。
- 快速每次调息3件、禁止刷新；非快速洞府3–5件（默认4）至少一灵品，永不售道品，所有售品已祭炼。
- 单调货物 token、有限库存，已售/过期 token/不足金额不消费。每个洞府可授予一次免费刷新；重复 BeginRest 不能重置库存或免费次数。
- 固定掉落凡妖1、异种4、大妖20；炼化币5/15/50/120；修为调用原表4/12/40/100；灵品30/仙品110，不人为降价。
- 批量炼化先预览、去重、忽略锁定/不存在/装备中的物品。修为待离开整理入账，可撤销；币立即到账，同次整理内有足够余额可撤销，不允许负余额回滚。离开整理清除撤销历史。
- 付费重铸一条非玄机、非独有效果灵纹，按法宝累计涨价，展示旧/新选项，保留旧也计费；不允许并行重复扣费；未选择即离开默认保留旧。待选中的物品不会被炼化。
- 宝匣所有品阶权重公开，气运有原系统递减封顶；仙品使用 Progression 的共享波数保底，非按开匣数量保底，道品无保底。每局至多5次额外付费，价格递增，免费机缘每restId只能领一次。Open/Reveal分步，重复Reveal无效；离开自动交付尚未揭示的已得奖励。
- 非快速仙品及以上拾取先隐藏并不生效，洞府祭炼恢复；若显出玄机冲突自动入袋。快速拾取即祭炼。

## 集成顺序

1. 每局 `Reset(seed, quick, layer2, layer3, config)`。层级只从可信传承解锁读取；不要通过UI传任意授权bool。
2. 击杀/拾取结算 `CreditKill(tier)`；额外机缘 `CreditStones(amount)`。灵石是局内状态，Reset清零。不能同时在kill和pickup各付一次。
3. 所有法宝奖励 `ReceiveRelic(progression, relic)`，而不是绕过Economy的GrantRelic；坊市自动传已鉴定标记。
4. `Progression.BeginRest()` 后 `Economy.BeginRest(p, monotonicRestId, caveSelected, freeRefreshUnlocked)`。选机缘前不要用false提前BeginRest，否则不可再重复BeginRest升级成洞府。
5. UI使用 ShopGood.token 购买，显示价格与sold；所有炼化改用本模块，`PendingXp()`用于预览。重铸展示PendingReforge，宝匣展示TreasureProbabilities再Open/Reveal，未知装备以IsUnidentified显示“待祭炼”。
6. 离开先 `Economy.EndRest(p)` 后 `Progression.EndRest()`；不能仍同时调用旧炼化API，避免两套撤销栈。

## RNG / 未定配置

Economy独立mt19937用于商品品阶、宝匣品阶、重铸候选选择；实际物品/灵纹用Progression生成器，因此两者RNG均为继续存档所必需。当前模块可按值复制；磁盘codec由整合层提供，尚无独立磁盘格式。

尚未在设计文档给定的默认值均为可配置暂定：凡品售价10、洞主掉币100、刷新15、重铸20+15×该物品次数、宝匣30+20×已额外开启次数、洞府4件。基础宝匣权重采用原型掉落70/29.05/0.6/0.35，仙品权重随未获仙品波数线性增加，20波强制仙品；这些是测试可见的暂定调优值，不宣称已实现“每5/15局”统计目标。商品仙品5%、灵品55%、凡品40%，首格最低灵品。长期购买2–4件目标仍需整合实机数据校准。

## 验证

`scripts/test-mvp-economy.sh`：严格警告构建，种子可复现、层级门槛、库存幂等/旧token、锁定/去重/撤销、保底/公开概率/付费上限、祭炼/重铸拒绝路径。

`ASAN_OPTIONS=detect_leaks=0 SANITIZE=1 scripts/test-mvp-economy.sh`：ASan+UBSan。此容器ptrace环境使LeakSanitizer不可用；仅关闭泄漏检测，不关闭地址或未定义行为检测。

## 存档与新增接线

`EconomyState ExportState()`含版本、配置、mt19937完整状态、当前restId/单调商品token、库存/售出、钱包、炼化待修为与撤销栈、未祭炼灵纹、各物品重铸次数、待选择重铸、待揭示宝匣、付费次数和免费机缘领取记录。`RestoreState(state, progression)`做边界/有限数值/枚举/身份/重复交易/随机数格式校验，通过后一次替换，不重新生成。需要先恢复对应Progression状态（含RNG与nextRelic），再恢复Economy；两者由上层存档事务统一提交，文件版本/校验/一次性消耗由存档层负责。

同次整理先选机缘时用 `EnterCave` 开放洞府而不重开rest；首次调用可获免费刷新，重复调用拒绝。`ChooseBossReward`替代Progression同名方法以确保非快速仙品未知灵纹正确隐藏。`EndRun()`在胜利/陨落时清货币及未完成交易、保留未知灵纹metadata供结算展示，无尽洞主结束不调用。
