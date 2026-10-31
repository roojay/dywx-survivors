# 为首次真实试玩服务的复用决策

2026-10-31，对 `2e5eb1e` 分层版本审查；这是一份实施边界，不是UE实机验收。原则：现有规则和表现已分离，先验证玩法，不因“成熟”重新搭底座。所有外部资源需单独确认许可、版本和发布范围；免费不等于开源或允许公开再分发。

| 当前部分 | 决策 | 触发条件 / 最小动作 |
|---|---|---|
| DywxCore / Runtime / UI分层 | 保留 | 继续headless验证规则；本轮不引GAS、ECS或另一套规则引擎。 |
| 运行时静态作者网格 | 本轮保留，候选局部替换 | `DywxMVPWorld::LoadGeometry/BuildMesh`读取约9.15MB JSON并运行时BuildFromMeshDescriptions。只先选一个角色、敌人、地形做编辑器预导入→Cook实验；Windows UE5.8验证后才切。 |
| ISM实例批量显示 | 保留 | 可换资源加载而不换现有实例显示与核心。 |
| Slate UI | 保留 | 先实机修布局、反馈和可读性；不为换技术整套改UMG/CommonUI。 |
| 手工键盘输入 | 首轮保留 | 验证失焦、暂停、恢复、按住与释放；真实手柄/重映射需求出现后用Enhanced Input，不继续堆专用输入框架。 |
| SaveCodec / ProfileStore | 保留 | 纯C++字段codec有边界校验；换USaveGame不替代事务/迁移/业务不变量。测出写盘卡顿再仅优化UE存储适配层，保留成功后提交及顺序。 |
| 手写网格VFX | 仅规划有限替换 | 以后用内建Niagara模板或许可确认的内容提升命中/冲刺/升级反馈。现有危险预警和伤害真值继续由当前规则控制。 |
| 性能测量 | 现在直接复用UE官方工具 | 仅五个CPU scope + Unreal Insights；见[采集流程](UE-PROFILING.md)，无新性能框架。 |
| 多人/大规模框架替换 | 延期 | 当前只实现Standalone。分层使未来适配更容易，但不宣称已经具备复制、预测、回滚或权威服务器。 |

## 静态网格候选：复用现有官方资产导入能力

现有 `scripts/bootstrap-mvp-assets.py` 已通过 UE Editor Python `AssetImportTask` 导入原创音频并保存材质，`scripts/package-windows.ps1` 已串联 Editor build、bootstrap和Cook。以后扩展同一路径，不引入另一套运行时网格插件。

最小实验：Blender导出三个样本→编辑器导入 `/Game/MVP/Meshes/...`→检查顶点色/法线/pivot/比例/朝向→验证Cook引用与打包加载→对照当前视觉与启动trace。保持当前路径直到实机验证通过；不要在无UE环境直接删除JSON加载器或承诺GPU改善。简单诊断图形可以继续程序生成。官方FBX管线支持顶点色，文档写明FBX2020.2，导出版本需匹配验证：[Static Mesh Pipeline](https://dev.epicgames.com/documentation/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine?lang=en-US)。

## 成熟表现候选与许可门禁

Epic的[Niagara Examples Pack官方介绍](https://dev.epicgames.com/community/learning/tutorials/v7z2/unreal-engine-niagara-examples-pack-introduction)提供命中、尾迹、buff等示例和Effect Types/Data Channels/Scalability实践。可借鉴其分级控制思路，但不因此迁移玩法逻辑。

[Fab列表](https://www.fab.com/listings/0e188eca-4e54-4fb2-a9ed-d8b8a565e600?lang=en)显示免费，当前抓取却未给出有效License terms文本，并显示“Allows usage with AI: No”；官方教程标UE5.7，不能推断5.8兼容。因此本轮**不下载、不接受新条款、不送入生成模型、不提交其源资产到GitHub**。接入前核实实际许可/再分发限制及5.8独立试包。无法确认就使用项目原创资源和引擎内建能力，不以此阻塞试玩。

## 不盲搬社区项目

- [Epic PythonSamples](https://github.com/EpicGames/PythonSamples)已归档，README原示例基于UE4.26：只作思路参考，不能包装为最新生产依赖，也不在未确认许可时复制其源码。
- [ActionRoguelike](https://github.com/tomlooman/ActionRoguelike)适合研究事件/UI/资源管理实践，但其主线实验性特性、授权与本游戏独立核心不同；不整项目移植，不把它默认当MIT依赖。
- [Enhanced Input](https://dev.epicgames.com/documentation/unreal-engine/enhanced-input-in-unreal-engine)是满足手柄/映射上下文需求时的引擎内建首选；焦点与UI输入回归仍须自己验证。
- [官方Save/Load](https://dev.epicgames.com/documentation/en-us/unreal-engine/saving-and-loading-your-game-in-unreal-engine)提供同步/异步保存能力，但不证明本游戏事务、异常写入、旧档迁移已经正确；已有纯核心codec不为此重写。

本轮只落实性能可观测性和这些取舍；是否好玩仍须真实人类试玩。尚无本轮UE编译、Cook、GPU、画面或流畅度通过证据。
