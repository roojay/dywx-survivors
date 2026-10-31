# 可复用源码与架构参考

核查日期：2026-10-31。目标是尽快交付游戏，不是为框架迁移而迁移。

## 决策

**保留 DywxCore 的独立 C++ 规则核，UE 负责宿主、输入、UI 与表现；不推倒重建为 Orion、Lyra 或 GAS 示例。** 这是针对本项目现状的工程判断，不是这些项目作者的结论。具体边界见 [UE-LAYERING.md](UE-LAYERING.md)。

复用优先级：Orion 的局部 UE 接入模式 > 有测量依据时选用 EnTT > 按需参考 Lyra 外围系统 > 确认采用 GAS 后查 tranek。没有任何候选被本次调查证明为可直接替换的、生产成熟的 UE5.8 幸存者完整底座。

## 四个聚焦候选

### 1. ActionRoguelike / Project Orion：首选参考，不整体搬入

- [作者仓库与 README](https://github.com/tomlooman/ActionRoguelike) 将 Project Orion 定义为同一 ActionRoguelike 项目，主分支明确为 UE5.8。它由课程项目持续扩展而来，不是已发行商业游戏源码。
- README 明确主分支包含实验系统：弹幕有池化及无 Actor 的数据导向实现，可能影响稳定性，部分实验尚不支持多人，默认通过编译开关关闭。不能把“包含优化实现”写成本项目已获得稳定性能收益。
- 最值得参考：事件驱动 UI、SaveGame、DataAsset/AssetManager 异步加载、刷怪配置、PSO 设置。外围模式局部迁移成本相对可控；整套 Action/Attribute/Actor 游戏规则替换现有规则核成本高，还会制造两个规则来源。
- **许可不是 MIT。** 本次根目录未见标准 LICENSE；README 明确允许用项目代码与内容构建自己的 UE 游戏，但这是作者具体表述，不应扩大解释为通用 MIT 授权。直接复制前记录对应版本的授权说明；有分发范围疑问先向作者确认。
- **资源另算：** README 特别限制 Epic Paragon 资源只能用于 UE，且不能借原游戏名称、角色或商标仿制/宣传续作等。不要把代码许可判断套到音频、模型、特效等每个资源。
- [提交记录](https://github.com/tomlooman/ActionRoguelike/commits/master/)本次网页可见 2026-09-06 的编译及崩溃修复。旧搜索摘要可能仍写 5.6；版本判断以此次读取的 README 为准。

### 2. EnTT：可真正引入的成熟小库，但不是幸存者底座

- [官方仓库](https://github.com/skypjack/entt) 提供独立于 UE 的现代 C++ header-only ECS、事件与资源等工具。README 列出 Minecraft、ArcGIS Runtime SDKs、Ragdoll 等使用者；这是作者提供的采用证据，不是对本项目性能的保证。
- **代码 MIT；文档 CC BY 4.0；logo CC BY-SA 4.0。** 不能把仓库所有内容统称 MIT。实际引入固定版本并保留 [LICENSE](https://github.com/skypjack/entt/blob/main/LICENSE)。
- 只在敌人/投射物实体管理、查询或存储复杂度有真实瓶颈时评估 registry/view；不要先把所有领域对象改 ECS。库本身不提供波次、升级、存档协议、UI 或 UE 渲染适配。
- 采用前做小范围等价试验：同 seed/命令轨迹结果一致、存档 round-trip 一致、代表性规模性能对比、standalone 与 UE 构建均通过。固定遍历/结算顺序不能因存储替换而意外改变。
- 本次未安装、编译或引入 EnTT，也未核验具体 release 的 UE5.8 构建兼容性。

### 3. Epic Lyra：成熟官方示例的外围参考，避免整体框架负担

- [官方文档](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-sample-game-in-unreal-engine) 当前为 UE5.8，说明项目随 UE 更新，是模块化学习示例和开发起点；包含多人、GAS、GameFeature、输入、UI 等系统。官方明确其 UI 系统可独立用于自己的游戏。
- 重点参考 UI/输入/设置及模块组织；将 Experience、GameFeatures、多人及 GAS 整套迁入单机幸存者，会新增大量与当前目标无关的依赖和维护工作。
- 从 [Epic 的 Fab 页面](https://www.fab.com/listings/93faede1-4434-47c0-85f1-bf27c0820ad0?lang=en) 获取官方项目；不要用非官方镜像的 LICENSE 推定 Epic 内容授权。
- **不是 MIT 开源。** 本次 Fab 页面可获取，但工具所见 License terms 为“-”，不能据此断言具体下载包统一适用哪一版 Fab 标准许可。下载时必须检查附带条款及代码/资源各自授权。
- [UE EULA](https://www.unrealengine.com/eula/unreal) 区分 Engine Code、Examples、Starter Content 与其他渠道 Content；不能仅因“免费官方示例”就把全部资源视为无限制可分发。本文没有完成逐文件许可审计。

### 4. tranek GASDocumentation：用法参考，不是完整游戏框架

- [GASDocumentation README](https://github.com/tranek/GASDocumentation/blob/master/README.md?plain=1) 当前声明 UE5.3，且不是官方文档；[代码/文档 LICENSE](https://github.com/tranek/GASDocumentation/blob/master/LICENSE) 为 MIT。可参考 AttributeSet、GameplayEffect、能力生命周期和 UI 监听，但 UE5.8 行为须与当前引擎源码/官方文档复核。
- [GASShooter](https://github.com/tranek/GASShooter) 是其进阶武器示例，仍声明 UE4.27，README 明确不是 production-ready；部分 TargetActor/ReticleActor 逐帧开销较重。
- GASShooter 虽有 MIT 代码许可，但资源来自 Epic ShooterGame、Infinity Blade 和作者本人，不能推定所有美术都是 MIT。
- 网页可见最近提交：[GASDocumentation 2024-04-06](https://github.com/tranek/GASDocumentation/commits/master/)、[GASShooter 2021-12-03](https://github.com/tranek/GASShooter/commits/master/)。不宣称已验证 UE5.8 即插即用。
- 当前无需仅为“成熟”引入 GAS；如果未来明确需要复杂联网预测/设计师能力工作流，再单独评估。不要让 GAS 和纯 C++ core 同时拥有伤害、升级、掉落的权威。

## 成熟游戏证明架构可行，不意味着源码可直接搬进 UE

[OpenTTD 官方专用服务器文档](https://wiki.openttd.org/en/Manual/Dedicated%20server) 明确支持无本地玩家客户端的后台服务器与 `-D` 启动。这是成熟游戏允许无图形客户端推进玩法的实例；它不能证明本项目规则核或 UE 适配已经正确，也不能证明 OpenTTD 所有规则完全不依赖自身平台层。

OpenTTD 的 [COPYING](https://github.com/OpenTTD/OpenTTD/blob/master/COPYING.md) 需与源文件许可一起检查。GPL 项目在这里**只作架构思路参考，不作为可直接复制或链接的 UE 模块来源**。[Epic 官方源码 FAQ](https://www.unrealengine.com/ue-on-github?lang=en-US) 明确限制会要求 UE 改受其他许可约束的 copyleft 组合，列出 GPL 等不兼容许可，并列出 MIT/BSD/Apache 等可接受的非 copyleft 类别。不要以“只复制一点”规避许可审查。

## 没有进入推荐名单的 Survivors 项目

本次有限检索未找到具备足够证据的“成熟开源 UE5.8 幸存者整套底座”，这不等于证明全网不存在。

例如 [UltimateRoguelikeFramework](https://github.com/CanTATARDev/UltimateRoguelikeFramework) 自述具备自动攻击、卡牌升级、刷怪和池化，但 README 是 All Rights Reserved / Fab EULA，且只自述测试 UE5.0–5.7；性能数字未经本次独立验证。因此不把它作为成熟宽松许可开源替代品推荐。

## 现在最有价值的落地动作

1. 先把开始、战斗、升级选择、死亡/胜利、重开/存档打包成完整可玩闭环，保持 core 是唯一规则来源。
2. 先解决已知体验短板，再从上述项目选一个局部模式；明确原始来源、固定 commit、许可、依赖和验收条件。
3. 每次局部移植同时验证独立 headless 与真实 UE 宿主。UE Server 无渲染运行不等于标准 C++ core 不依赖 UE；二者不能互相替代验收。
4. 不以 headless 通过推定 Cook、窗口焦点、输入、音效、动画、GPU 或打包表现通过。

## 核查与交付边界

本次只读取公开一手 README、LICENSE、官方文档及网页可见提交历史；未执行、复制或安装外部代码，未实机运行候选项目，未审计所有资源授权。提交日期来自网页索引可见内容，不是 GitHub live API HEAD；版本声明是作者声明，不是本次兼容性测试。工程取舍与来源事实已分别标注。实际复制前重新核对选定 commit 和随包条款。
