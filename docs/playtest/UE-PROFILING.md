# UE 5.8 实机性能采集（待执行，不是性能结果）

本轮只加入五个静态 CPU scope，不改规则、推进顺序或资源管线。离线源码检查不能证明 UE 编译、GPU 性能或帧率达标；以下流程须在有完整 UE 5.8 的 Windows 上执行。不要把 headless 吞吐量当作游戏帧率。

## 1. 先准备真正的包

按仓库 README 使用 `scripts/package-windows.ps1` 生成 Win64 **Development** 完整目录。保留 UBT/UAT 日志及 bootstrap receipt；构建/Cook 失败时停止，不使用历史演示 EXE 代替。保存当前 commit、包路径、Windows/驱动、CPU/GPU、内存、分辨率、窗口模式、画质、VSync、实际帧率限制以及显示器刷新率。

先无 tracing 跑一轮基线，再用相同配置采集；追踪本身有开销。首次启动与热启动分开标记，不删除用户缓存或存档来伪造冷启动。如需新档，对原档先备份并明确记录测试副本。

## 2. 从进程启动采集到本地文件

官方 UE 5.8 Reference 确认 `-trace=<channels>` 和 `-tracefile=<filename>`。以下 PowerShell 路径只是例子，换成真实包内 EXE；先建立可写目录，文件名每次唯一：

```powershell
New-Item -ItemType Directory -Force 'D:\dywx-perf' | Out-Null
& 'D:\dywx-package\Windows\DywxSurvivors.exe' '-trace=cpu,gpu,frame,bookmark,log,file,loadtime' '-tracefile=D:\dywx-perf\startup-and-play-001.utrace' -log
```

用本地文件采集不需要外部 trace server、端口开放或管理员权限。正常退出游戏后确认 `.utrace` 非空，再拖入 `Engine\Binaries\Win64\UnrealInsights.exe` 的 Session Browser。不要仅以文件存在判断采集成功：必须看到 Game/Render 线程、帧轨道及下表 scope。若 GPU 轨道没有数据，记录 **NOT_CAPTURED** 并检查当前 RHI/驱动/构建支持，不能将空轨道解读成零成本。若 scope 缺失，核对实际运行的是新 Development 包以及 cpu channel 已启用。

基线先不开 `-statnamedevents`；需要更细引擎 timer 时另采一轮并注明新增开销。无需内存/网络/context-switch 等无关通道。正式包是否包含 trace 取决于构建配置，不把 Development 的可观测性推断给 Shipping。

## 3. 固定采样场景

| 场景 | 操作和证据 |
|---|---|
| 启动/首入战斗 | 从启动参数开始追踪；记录菜单可交互、首次进入战斗时刻。检查 `DywxWorld_LoadGeometry`，与后续热启动分开比较。 |
| 菜单 | 前台静置至少30秒；截图记录实际FPS/限帧，不只看任务管理器GPU百分比。 |
| 密集战斗 | 同角色、地图、模式、种子/可复现配置、相似敌人/弹幕规模，连续采样至少60秒；记录波次、升级/暂停排除区间。不能声称人工操作轨迹完全确定。 |
| 存档 | 运行跨过至少三次当前10秒自动存档周期，并覆盖一次波次/阶段切换；观察 SaveProfile 与慢帧是否重合，查日志是否写入失败。 |
| 失焦/最小化 | 先前台稳定30秒，Alt-Tab失焦30秒，最小化30秒，再恢复30秒；记录各转换的墙钟时间，确认暂停/恢复和帧率预算，没有误推进/卡住输入。 |

可在 Development 控制台使用 `stat unit` 看 Frame / Game / Draw / GPU 毫秒（以及平台提供的RHI项），`stat fps` 看FPS；若该包无控制台入口，不声称命令执行成功，改用 Insights 线程与GPU轨道并单独记录缺失项。可用官方 `trace.bookmark` 标记转换；打开控制台和截图自身会影响帧，分析时排除这些瞬间。失焦期间用外部时间记录，不反复切回去截屏破坏场景。

## 4. 本项目 scope 的含义

| Timer | 测量边界 / 不代表什么 |
|---|---|
| `DywxSession_SimulationStep` | 仅 `Simulation.Step` 调用，不含输入准备与随后存档；没有侵入纯 Core。 |
| `DywxSession_SaveProfile` | 整个会话保存尝试，含候选复制、观察/结算、编码/同步写盘路径及提前返回；出现timer不等于实际写盘或成功。 |
| `DywxWorld_LoadGeometry` | JSON解析和其中触发的运行时网格构建；不是整个启动、音频/材质/场景总耗时。 |
| `DywxWorld_Render` | 游戏线程表现更新，包括嵌套Flush；不是GPU渲染耗时。 |
| `DywxWorld_Flush` | ISM更新的CPU提交成本；不含随后渲染线程/GPU执行全部成本。 |

在 Timing Insights 中选择相同场景区间，报告帧时间 median / p95 / max（毫秒、样本量、区间），并看最慢帧的 Game/Render/GPU 轨道和上述timer。Render inclusive 已含 Flush，不要相加重复计费。文件通道可辅助看存档IO；`loadtime` 面向包加载，不能指望它自动解释手工JSON解析。慢帧恰好重合只能提示关联，须重复采样并检查其他线程/加载工作再归因。

若限定60FPS，16.67ms是整帧预算，不代表所有线程可各花16.67ms后相加；线程可能重叠，等待/限帧也影响帧时。GPU占用99%既不是单独的失败证据，也不证明流畅。记录任务管理器数据作为补充，优先报告真实帧时、波动及前后台行为。

## 5. 交付证据和停止条件

交付：commit + 构建/Cook日志 + 上述环境信息 + 采样时间段 + `.utrace` 本地位置 + timer/帧时汇总 + 必要截图 + 错误日志。原始trace可能含路径/日志且较大，不默认提交Git或公开上传。仓库只记录脱敏摘要与确切测试状态。

通过采集准备的条件是五个timer在对应路径出现且轨道可分析；**不等于性能验收通过**。发现显著卡顿才决定优化哪一层；未跑UE时所有实机结论标 NOT_RUN，GPU缺失标 NOT_CAPTURED。

## 官方依据（2026-10-31核对，文档显示 UE 5.8）

- [Unreal Insights Reference：通道、CLI、CPU/GPU追踪开关](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-insights-reference-in-unreal-engine-5)
- [Trace Quick Start：打开utrace](https://dev.epicgames.com/documentation/en-us/unreal-engine/trace-quick-start-guide-in-unreal-engine)
- [Trace：bookmark、loadtime与通道范围](https://dev.epicgames.com/documentation/en-us/unreal-engine/trace-in-unreal-engine-5)
- [Developer Guide：优先使用内建静态CPU scope](https://dev.epicgames.com/documentation/en-us/unreal-engine/developer-guide-to-tracing-in-unreal-engine)
- [Performance Profiling：区分Game/Draw/GPU瓶颈](https://dev.epicgames.com/documentation/en-us/unreal-engine/introduction-to-performance-profiling-and-configuration-in-unreal-engine)
