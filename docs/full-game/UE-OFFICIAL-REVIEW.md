# UE 5.8 官方接口与完整玩法接线复审

审查日期：2026-10-30。基线 `c77483e` + 本次未提交完整玩法工作树。仅审查源码并运行明确列出的离线测试；没有安装、模拟或构建 UE stub。`UE_ROOT` 未设置，`UnrealEditor` 不在 PATH。**UHT / UBT / Shader Compile / Cook / packaged launch / GPU / 实际磁盘故障注入均 NOT_RUN。** 本文不是 UE 编译合格证。

## 确认问题与修复复核

### P1：经济弹层失焦后无显式恢复路径（已修复并离线复核）

- 原链：`DywxMVPWorld.cpp::RefreshRunBudget` 先暂停 simulation → Controller 消费请求 → `HandleFocusLoss` 调用普通 `Pause`；`Navigation::Pause` 在已有 Economy modal 时拒绝。关闭 Economy 不满足 `bModalPausedRun`，因而保留 paused 却没有暂停菜单。
- 复现：传统模式进入波间洞府 → 打开交易弹层 → Alt-Tab → 回窗口 → 返回整理。原实现此时交易/迎战受暂停拦截。
- 现修复：`DywxUIState.h:45-49` 的 `FocusLoss` 对任何 live Battle 弹层统一进入 Pause；`SDywxRoot.cpp:530-537` 保持 simulation 暂停、不清理 core 交易。重复失焦不会关闭暂停；只有用户明确继续才恢复。
- 独立运行 `test-ui-state.sh` 与 `test-mvp-focus-contract.py` PASS。实窗 Alt-Tab、最小化、点击和键盘恢复仍需 UE。
- 官方依据：[FInputModeUIOnly](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FInputModeUIOnly) 仅 UI 接收输入；[SWidget](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/SlateCore/SWidget) 的 focus-lost 不冒泡，不能假设按钮失焦会替代应用焦点状态机。

### P1：暂停菜单的保存入口必然被 paused 拦截（已修复，副本投影回归见下）

- 原 `SDywxRoot.cpp::ModalContent` 唯一保存按钮在 Pause modal；原 `Simulation.cpp::PrepareCheckpoint` 拒绝 paused，正常玩家保存必失败。
- 现 `PrepareCheckpoint` 为 const 投影，允许合法 paused Rest，XP/待选升级仅在 checkpoint 副本结算；`DywxMVPWorld.cpp::SaveRun` 已删除对真实 simulation 的 `FinalizeRestExperience` 调用。拒绝 Quick、战斗阶段和未解决交易的边界仍须保持。
- 因此写入失败不再先改变真实 Run 的 XP/交易；不能通过临时解除真实 Run 暂停来解决。
- 复现/验收：传统或无尽 → Rest → 合法处理机缘 → Esc → 保存并退出 → 重新启动 → 续接；加磁盘失败时原 pause、XP、背包、交易均不变。

### P1：已存 checkpoint 与结算 watermark 冲突（已修复并独立复核）

- `SaveCodec.cpp::ProfileValid` 拒绝 checkpoint.runId ≤ settledThrough；`Campaign.cpp::Settle` 提升该 watermark。原 `ProfileStore::Commit` 不清理旧 slot。
- 保存后仍继续本局终局，或保留旧 slot 另开同修士新局再终局，会使候选 Profile 永久编码失败，结算无法提交。
- 现已复核：`SDywxRoot.cpp:353-362` 保存成功 ShowMainMenu、失败保留 Pause；`DywxMVPWorld.cpp:74-75` 开新局先拒绝同修士未消费存档，Navigation/Controller/UI也防绕过；`SaveCodec.cpp:191-199` 仅在候选提交中清理已结算旧 slot，writer 成功才替换内存，不删除坏blob、不删除有效未完成slot，也未放宽反重放校验。

## 官方 API 核对：未发现可确信的签名/UHT阻断

| 范围 | 源码观察 | 官方核对/结论 |
|---|---|---|
| Controller/World/Profile 接口 | StartFullRun、SaveRun、ResumeRun、Purchase、CampaignProfile 参数与实现对齐；纯 C++ enum forward declaration 的默认底层类型与定义一致 | 新 std::/simulation 类型没有放进 UFUNCTION/UPROPERTY；generated.h 保持末尾 include。仍须真实 UHT，不能以源码代替。 |
| 模块依赖 | Engine/Core 类型位于 public；Slate/Json/MeshDescription 仅私有实现使用；SDywxRoot 为 private header | [模块教程](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-modules) 的 public/private 规则一致。未添加运行时 UnrealEd 依赖。 |
| Runtime mesh | World BuildMesh 设置 bFastBuild=true，检查 BuildFromMeshDescriptions 返回 bool，禁碰撞 | [UStaticMesh](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UStaticMesh?lang=en-US)、[构建参数](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UStaticMesh/FBuildMeshDescriptionsParams)：非编辑器要求 fast path，现满足。shader/RHI 可用性尚未验证。 |
| ISM | World Flush 每组件最后一次 transform 更新标记 dirty；static arena 在 RebuildArena ClearInstances，复用 component/material；pose slack 有界 | [ISM API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UInstancedStaticMeshComponent) 参数顺序匹配。不能误报“每个实例都强制重建render state”。批量 API 是可量测优化，不是正确性阻断。 |
| Slate 纹理所有权 | Brush 指向运行时 Texture，Textures 容器用 strong object guard；Owner为弱 UObject；Root由Controller共享拥有，EndPlay移除viewport并reset | [Object pointers](https://dev.epicgames.com/documentation/en-us/unreal-engine/object-pointers-in-unreal-engine) 支持 strong guard 保活。raw-this Slate lambda 的安全依赖当前 widget所有权树，UIEpoch仅防旧树动作，不是生存期guard；没有已证实UAF，须PIE反复结束/重启验证。 |
| bootstrap | PythonScriptPlugin/EditorScriptingUtilities仅Editor；-run=pythonscript -script；保存材质与SoundWave后写receipt | [官方Python教程](https://dev.epicgames.com/documentation/en-us/unreal-engine/scripting-the-unreal-editor-using-python)、[MaterialEditingLibrary](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MaterialEditingLibrary) 一致。NullRHI不是视觉/shader验收。 |
| cook/stage | /Game/MVP AlwaysCook涵盖动态LoadObject材质和音效；UI与MVP/Geometry NonUFS涵盖字体/PNG/JSON；Entry显式MapsToCook | [PackagingSettings](https://dev.epicgames.com/documentation/unreal-engine/API/Developer/DeveloperToolSettings/UProjectPackagingSettings) 的路径语义一致。源码声明≠stage产物已存在。 |

## 存档与显示：限制而非虚构故障

- Profile购买、结算、checkpoint消费使用 candidate/commit 顺序；保存失败返回false且不提交候选内存。坏文件保持不可写，启动阻止覆写；无静默重置账户。
- `DywxMVPProfile.cpp:10-16` tmp→`IFileManager::Move(replace=true)` 只能表述为“尝试替换且检查返回值”。[IFileManager官方API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Core/IFileManager) 未承诺跨平台原子替换、断电 durable 或旧文件在所有失败场景下仍完整。需Windows/Linux权限拒绝、磁盘满、崩溃时点、备份恢复测试，不能把writer mock当真实磁盘证明。
- `SDywxRoot::Button` UIEpoch防旧树命令；状态命令还核对phase/generation/交易token。新SScrollBox在操作后重建会重置滚动位置：这是可用性/实机检查项，不是已证实崩溃。
- 大字号、窄屏/超宽屏、三张升级卡及长词条滚动、层级入口显示、视口焦点和鼠标捕获必须在真实UE截图检查。字体静态/源码契约不能证明Slate布局。
- ISM逐实例更新、pose bucket增删、巨大波次负载需Insights/GPU计时；官方提供 [BatchUpdateInstancesTransforms](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UInstancedStaticMeshComponent/BatchUpdateInstancesTransforms?lang=en-US)，但未测量前不宣称性能不达标或优化已达标。

## 本轮实际执行

- `validate-mvp-render-contract.py` PASS：31 meshes、43,066 triangles、JSON/staging/source only。
- `test-mvp-art-material-contract.py` PASS：静态几何/材质源契约；shader/render/GPU NOT_RUN。
- `test-ui-state.sh` PASS；`test-mvp-focus-contract.py` PASS（含修复后重跑）。
- `bash -n scripts/package-linux.sh scripts/bootstrap-mvp-assets.sh` PASS（仅shell语法）。
- 初次 `test-mvp-profile-contract.py` FAIL（旧v1实现字符串契约）；owner更新后独立重跑PASS，覆盖可选v1 flags迁移、v2界限、写入/消费顺序；不代表真实IO。
- `test-mvp-save.sh` 独立重跑PASS：包含保存slot终局失效与writer失败保留的纯核事务回归。
- `test-full-game-integration.sh` 独立重跑PASS：含 paused Rest投影不改原状态、重复checkpoint字节一致、待选升级/offers保留、续接及boss入口。

## 最小真实引擎验收门槛

1. UE5.8 Editor UBT+UHT，保留日志；再执行bootstrap，检查两材质与十音效receipt/资产。
2. Development BuildCookRun，并从独立archive启动，禁用源码目录回退验证字体、图片、JSON、材质与音效。
3. 新局→波间选择/换装/交易→失焦→显式继续→保存离开→重启续接→终局结算；同修士旧档新局策略、坏档及写入失败。
4. 同一进程三洞天/三角色多次重开，观察旧静态实例无残留、GC无失效brush、窗口输入恢复、长列表可达。
5. 1080p/720p/超宽、大字号截图，正常/峰值负载CPU/GPU/FPS采样。以上均未在此执行。

## 2026-10-31 补充：非暂停 UIOnly 入口

- 源码证实：Economy 弹层在 Rest 中使用 `FInputModeUIOnly`，但不调用 `SetUIPaused(true)`，此前仅暂停入口清理 `PlayerInput` 和 queued dash。因此存在进入弹层前按住移动键、在弹层内松开后残留输入的风险；**实际 UE 键盘残留未复现，不作为实机已确认故障**。
- Controller 现在共用 `ClearGameplayInput`：暂停请求立即清理，UIOnly 模式入口也清理。不会放到 `PlayerTick`、升级选择或 HUD 轮询中。Economy 关闭仍恢复 GameAndUI 而不额外解除暂停；FocusLoss 仍替换为显式 Pause，必须明确继续。
- 本次离线通过：focus source-contract（新增非暂停 UIOnly 清理及禁止逐帧调用）、UI source-contract、navigation、preview-budget policy、70 项 100%/110% 字号字体度量、资源与 31-mesh/43,066-triangle render/material source-contract、bootstrap Python/shell 语法、diff whitespace。
- 未改动 world/VFX/cook 配置：动态材质/音效位于 AlwaysCook `/Game/MVP`；字体/图片与几何 JSON 由 NonUFS 声明覆盖。预算源码仍在模拟步前检查焦点、后台/最小化使用共享 cap 策略、停止时仅恢复自己仍持有的 cap；这些声明不是已生成 package 或 FPS/GPU 数据。
- 仍需实机验证：按住 W/Space→进入洞府→在弹层松开→返回→迎战；暂停/失焦/恢复同链路；720p/1080p/大字号/超宽截图。暂停时 `Render(0)` 仍遍历并上传 dynamic ISM transforms，属于可量测优化候选；本轮未凭静态分析宣称 GPU 超标，也未未经测量重构渲染刷新。
