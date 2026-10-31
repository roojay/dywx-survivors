> Superseded for runtime interaction and package delivery by [battle-entry crash fix validation](../windows-crashfix-20261031/REPORT.md). The original 739466 package crashes on first battle entry.

# Windows UE 验证记录（2026-10-31）

恢复源码为 `6a1031d38b2e1246bc97ec3600c5159ca41b7bbf`，树为 `73e913a3615cbc8484164944939d87c0d2438d1d`。从检查点 `d295df6fd8634afe7b99d0d524111da96a976ba5` 按 full → 803e → 6a 顺序恢复；分卷、bundle 哈希、依赖和 `git fsck --full` 全部通过。归档分支没有当作游戏源码编译。

本次 EXE 的源码提交为 `7394661062b11c7083a9066066b048ca3b1eb94c`。真实工具链：UE 5.8.4 CL58902052，MSVC 14.50.35725（安装目录14.50.35717），Windows SDK10.0.26100.0，Win64 Development。原项目和旧静态 UI 包均保留。

## 已完成

| 阶段 | 结果 | 证据 |
|---|---|---|
| UHT、Editor UBT、三个模块 DLL 链接 | PASS | 本机 `fullgame-build-winfix/UBT-Editor.log` |
| 材质、10个原始音效导入 | PASS | `Saved/MVP/material-bootstrap.json`；生成的12个uasset已纳入源码 |
| 游戏目标 UBT、Cook、Stage、Archive | PASS | `package-tail.log`，UAT exit0 |
| Windows CMake Release | PASS | `ctest-windows.log`，22/22 |
| Windows ASan | PASS（限定范围） | 仅重跑经济测试及损坏随机状态回归；不是全量ASan/UBSan |
| 架构/资源契约 | PASS | 本机 `architecture-contract.log`、`render-contract.log` |
| 新包初始化、Sarasa字体装载、自动退出 | PASS | 720p、1080p独立用户目录；Freetype确认Regular/Bold成功载入 |
| 交互、视觉、完整玩法 | NOT_RUN | 新应用窗口审批等待超时，未取得截图或发送输入 |

## 修复

- 三处变量遮蔽导致的 MSVC 编译错误，只改变局部变量名。
- CMake 核心与消费者统一 `/utf-8`，消除中文字符串的本机代码页编译错误。
- UE 5.8 VertexColor 的 RGB 输出无名称；材质生成改用第一个输出。
- 损坏随机状态触发 MSVC `std::mt19937` 提取器异常，异常关闭的 UE 会终止。增加有界数字、范围、原生状态词数和尾部校验，经济及进度存档共用；解析失败不替换已有状态。新增空值、截断、负数、溢出、多余数字/尾缀回归。不改变随机算法或玩法数值；原有标准库之间的存档格式差异未作迁移。
- 架构检查允许 Core 内部私有头文件，并继续检查其依赖和 UE 类型隔离；公开 API 导出要求保留。

## 实测性能范围

RTX3080。UE CSV采样，跳过启动帧，仅后台主菜单，世界渲染关闭。不是战斗性能或视觉布局验收。

| 条件 | 稳定帧数 | 实测FPS | 帧时间均值/P95 | GPU时间均值/P95 | 本地GPU内存均值 |
|---|---:|---:|---:|---:|---:|
| 1280×720，后台15上限 | 120 | 15.00 | 66.67/66.97 ms | 0.67/0.89 ms | 264.21 MB |
| 1920×1080，进程内更低10上限 | 90 | 10.00 | 100.00/100.17 ms | 1.46/1.63 ms | 282.22 MB |

10上限在退出时仍为10。没有改系统/显卡设置。60前台和5最小化只有源码策略，未完成持续测量；首帧日志的60读回不能视为前台性能通过。原始CSV及其哈希见两个performance JSON。本轮没有战斗测试fixture，也没有写入用户真实存档。

## 本地运行包

`deliverables/dywx-survivors-fullgame-win64-20261031.zip`，372915633字节，SHA256 `70ff2bd156849f0c3803e94befb33297f261de42875088a065d83afcc4c17f04`。ZIP CRC测试通过，61个条目，解压685913265字节。包含完整EXE、DLL、Content、配置、配套运行库安装程序及中文说明；无PDB。没有上传EXE、创建Release或Space备份。

完整解压后运行顶层 `DywxSurvivors.exe`。WASD/方向键移动，空格遁术，1/2/3升级，Esc暂停。测试参数可用 `-windowed -ResX=1280 -ResY=720`。保留整个目录；不要只复制EXE。

## 剩余验收与限制

角色/地图/模式、移动/遁术/升级、死亡提示、结算滚动和重试、机缘整理、失焦恢复、保存并离开/续接、720p/1080p实际中文字形与布局、战斗/100+敌人性能、60/15/5完整状态切换、真实文件替换/断电和人工平衡验收仍未完成。编译仍有非致命float转换及Slate字体构造弃用警告。

阻塞动作是 `cua.getApp({windowId:6949050})`（本次创建的游戏窗口），返回 `Computer Use app approval timed out`；审批状态pending/uncertain，没有截图或输入执行证据，未换工具绕过。该问题只阻塞交互/视觉验收。

Git首次正常用户发布在fetch前被目录所有权检查阻止（检出由沙箱用户创建），未执行push。采用正常用户新建的独立发布检出和已校验bundle，不改safe.directory、凭据、权限或网络。源码已正常快进推送至 main 的 `7394661062b11c7083a9066066b048ca3b1eb94c`；fetch后确认远端提交、全部相对路径/对象列表与15项关键资源哈希一致。详见 `source-publication-verification.json`。
