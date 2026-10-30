# 波间存档与账号持久化

## 范围与流程

- 传统/无尽允许波间保存，每修士一个slot；快速拒绝。`Simulation::PrepareCheckpoint`在副本中完成整理收益和机缘推进，取下一波/洞主入口，原局不变。
- 波边界不保存战斗实体。玩家生命/护盾/遁术状态、施法冷却/计数、触发冷却/增益、陷阱计时、统计、完整地图与异象排程保留，洞主进入时按原RNG和下一个实体ID确定性生成。
- 若炼化修为在副本触发顿悟，存档保留已经抽好的合法候选；不重新抽取。读取恢复下一波开头的同一候选与RNG。
- `GetCheckpoint`只读校验；World先在候选Simulation中恢复并确认可运行，再 `ConsumeCheckpoint`。消费成功才切到该Simulation。消费失败不启动；重复消费失败。
- 保存/分配runId/消费均通过ProfileStore候选副本提交。writer失败不替换内存账号，不返回可启动的runId或已消费blob。

## 格式

`SaveCodec`是纯C++显式字段编码，无原生对象内存dump/指针/第三方库。版本2封包包含magic、版本、记录类型、长度、FNV-1a完整性校验；这是误损检测，不是加密签名或反作弊。

每个数值按明确小端64bit编码，浮点转IEEE double编码；布尔仅接受0/1，所有浮点拒绝NaN/Inf。全文件32MiB上限，容器数量/字符串长度先检查再分配，拒绝截断/尾随数据/非法枚举/重复物品身份/英雄错配/已结算runId。异常地图尺寸在导航校验前限界，异象时钟/配置也校验，避免坏档进入无限调度。

Progression保存完整随机生成器、nextRelic、已合成消耗位、封存候选、候选池解锁、永久加成基线、劫数权重、偏好法宝池、洞主奖励序号。Economy保存其完整state（含RNG/币/未知灵纹/重铸次数）；在波入口库存等整理交易已关闭。Campaign保存run状态、模式/层级/机缘RNG；不会只用seed重roll。

账号保存道韵、解锁、永久点数、个人成绩/熟练度、每修士已结算高水位、单调nextRunId、异象和原道典发现、全局仙品波数保底及三个独立slot。`AllocateRunId`必须成功提交后才返回非0。

## UE适配与兼容

新路径 `Saved/MVP/profile-v2.dat`。仅新文件不存在时读取旧 `profile.json` version1；兼容旧可选发现字段，旧文件保持不动。损坏/未知版本/过大文件使 `IsWritable=false`，禁止把故障档覆盖成空账号。

UE adapter仅在dirty时写盘，写到临时文件后请求 `IFileManager::Move`替换。这里保留原工程的临时文件替换模式。**尚未运行UE，也未证明所有目标平台的Move具备原子替换/断电耐久性**。纯核的故障注入测试验证的是“writer原子提交契约”下的事务行为，不等于平台磁盘故障验证。发布前必须补UE实机文件替换、写满/权限失败/退出重启、强杀或断电窗口测试；不能宣称已完成这些验收。

## 验证命令

- `scripts/test-mvp-save.sh`：纯codec/profile事务测试。
- `ASAN_OPTIONS=detect_leaks=0 SANITIZE=1 scripts/test-mvp-save.sh`：ASan+UBSan（容器ptrace不支持LSan）。
- `python3 scripts/test-mvp-profile-contract.py`：仅UE adapter源码合同，不是UE编译/IO测试。
- 全游戏整合测试另验证Prepare原局不变、Resume继续相同敌人生成与统计。
