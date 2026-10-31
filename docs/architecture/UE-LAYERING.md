# UE 架构分层（2026-10-31）

## 决策

用户明确的首要目标是：底层独立于 UI 和 UE，能无界面模拟演练，上层表现可以替换或适配其他环境。联机只是未来可选适配，不是本轮目标。UE Runtime 是使用规则核的一个宿主，不是运行规则核的必要条件。

保留已能独立测试的纯 C++ 规则核，按职责与生命周期拆分，避免为未来联机提前搬入整套 Lyra、GAS 或复杂网络框架。本轮基线为 `3504a9d`。这是当前项目的取舍，不是 Epic 唯一强制模板。

### DywxCore

标准库领域模型、战斗、成长、经济、地图、存档 DTO/codec 与规则校验。公共头使用专门导出宏：UE 构建由 UBT 提供 DLL API，离线构建显式声明 standalone，不隐式吞掉缺失的 UE 导出定义。模块入口和仅 UE 构建启用的 HAL/Platform.h 导出适配依赖 UE Core，以确保 NoPCHs 下 DLL 导出宏可用；领域算法不依赖 UObject、Engine、Slate 或磁盘平台 API。离线编译不进入该 ABI 适配分支。

UI 原先直接调用 Progression 的装备、卸下、锁定、排序、融合改为 Simulation 的明确命令入口，统一校验阶段/暂停状态并同步快照。UI 上的按钮可用性不是规则的唯一执行位置。

### DywxRuntime

`UDywxRunSession` 采用 world-lifetime `UTickableWorldSubsystem`，唯一持有并推进本地 Simulation；负责输入命令、账号适配器、整理存档、恢复、定期观察和结算事务。它不依赖 Slate 或表示 Actor。

只在 Game/PIE 且 Standalone 模式创建并运行。网络模式明确拒绝，不会静默让客户端各跑一份所谓权威世界。这个限制是诚实的未实现边界，不是联机功能。

`FDywxMVPProfile` 是 UE 文件系统适配器，保留现有磁盘版本/候选事务；不放入纯 Core。本轮仍由 world session 持有，每次世界生命周期从磁盘恢复账号，未新增 GameInstanceSubsystem。未来确有跨地图持续账号/会话需求，再把持久化服务提升为 GameInstance 生命周期，不能让跨世界对象引用旧 World 的 Actors。

### DywxUI / presentation

Slate 与 Controller 只读查询、提交语义命令，不取得 mutable Simulation/Progression/Economy。`ADywxMVPWorld` 保留类名以避免无谓资产迁移，仅负责相机、实例网格、音效与特效，显式接收所属 Controller，不查询 FirstPlayerController。

Controller 管理本地输入、菜单、窗口焦点和帧率预算。会话在 Step 前调用弱 UObject 绑定的本地采样回调，先完成失焦清输入/暂停，再推进模拟，不能依赖 Actor/Subsystem 恰好以某顺序 tick。重新聚焦不会自动恢复战斗。

运行时和表现的职责分开：隐藏画面或停止表现 tick 不应改变规则核的推进权。菜单停止会话必须显式停止 runtime 与 presentation，而不是仅隐藏 Actor。

## 当前查询边界与多人后续

本轮只读 `const Simulation` 查询用于进程内单机表现，防止 UI 写入。它**不是安全的网络 DTO**：领域对象仍含经济未揭晓信息与内部状态。真正联网时需单独按可见权限构造玩家 read model，不可序列化整个 Simulation/Economy::ExportState 下发客户端。

当前依然是单玩家 Snapshot/Input/Progression。未来多人还需要：

1. 引入 PlayerId 对应输入、玩家状态和命令权限；区分局内状态与可信服务端账号。
2. 客户端通过拥有连接的 PlayerController/Pawn 发 Server RPC；服务器验证阶段、余额、物品归属与过期选择，再调用核心命令。
3. GameMode 管理服务端局规则；GameState/PlayerState 或专门复制适配发布共享/个人状态，不能各自运行第二套权威模拟。
4. 命令需携带会话身份和适当的选项 revision，处理重发/迟到/重开；不能每帧 snapshot 变化都拒绝合法选择。
5. 明确多人暂停、升级选择、掉落归属、断线恢复规则。
6. 根据实测需求选择预测/纠错与复制方案。固定步长与固定种子不等于已保证跨平台锁步。

## 不引入的系统

不在本轮引入 GAS、Lyra Experience/GameFeature、CommonUI、UMG Viewmodel 插件、Mass、Iris 复制、房间后端或自建通用事件总线。需要时依据规模和平台重新评估。5.8 Iris release notes 与旧概览徽标可能不同步，不以旧 Experimental 标签作永久判断。

## 依据

官方一手资料：
- [Gameplay Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine)
- [Subsystems](https://dev.epicgames.com/documentation/unreal-engine/programming-subsystems-in-unreal-engine)
- [Unreal Engine Modules](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-modules)
- [GameMode and GameState](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-mode-and-game-state-in-unreal-engine)
- [Networking Overview](https://dev.epicgames.com/documentation/unreal-engine/networking-overview-for-unreal-engine)
- [RPC](https://dev.epicgames.com/documentation/unreal-engine/remote-procedure-calls-in-unreal-engine)
- [CommonUI design guidelines](https://dev.epicgames.com/documentation/unreal-engine/design-guidelines-for-using-commonui-in-unreal-engine)
- [UE 5.8 Release Notes](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-release-notes?lang=en-US)

社区工程一手参考：
- [Tom Looman ActionRoguelike 源码](https://github.com/tomlooman/ActionRoguelike)
- [作者说明，更新于 2026-09-08](https://tomlooman.com/unreal-engine-sample-game-action-roguelike/)

借鉴其模拟数据/表现分离、事件驱动及按需自定义系统；不把示例或实验优化视为本项目已经验证。没有安装或运行外部项目代码。

## 验收口径

离线完整测试、独立命令回归和架构 source contract 用于验证规则与依赖边界。源码路径变化需新生成证据，历史日志/hash不改写。UHT、UBT、Editor DLL 链接、Cook、真实焦点/窗口/输入、GPU仍需Windows UE环境，不以正则契约替代实机验收。

## 独立构建入口

`standalone/CMakeLists.txt` 提供 `Dywx::Core` 静态库与 `dywx_headless`，后者复用真实的十例自然玩法驱动，不是另写简化游戏。其他前端可直接链接同一目标，自动继承公开include/C++20/standalone定义。详见 `../../standalone/README.md`。在不安装 Unreal 的 dot 环境中，CMake 配置/构建、CTest 1/1 和外部独立消费者链接运行已通过。

自然演练普通/ASan/UBSan的结果与轨迹一致；CMake headless 结果/轨迹也与重构前3504a9d逐字节一致（3胜4死3观察上限）。这证明既有固定样例规则未因分层而变化，不证明平衡目标达标或UE适配运行正确。证据见 `validation/`。
