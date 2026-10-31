# 道衍万象

东方神话风格的 UE5 原生幸存者玩法原型。当前分支以 `docs/gameplay-design-v0.1.md` 与 `docs/mvp-prototype-config-v0.1.md` 为实施依据。

## 当前状态，请先读

**当前交付是已实现并做过离线验证的完整功能原型源码与资源，不是已验收的 Windows 游戏包。**

历史提交 `8ef559d` 只有四界面演示，曾通过 Windows 编译，但用户实际运行发现截字和 GPU 高占用。此后字体、布局、帧率预算已经重写，且加入了真正的战斗与成长系统。当前新源码尚未在 UE 重新编译、启动、烹饪或测试 GPU。旧 EXE 不包含这些修复，也不是本 MVP。

本分支当前已扩展到完整设计的功能原型：
- 快速、传统、无尽三种模式；四/六波快速配置、传统十波、无尽每五波洞主与本地排名。
- 三位修士、三处洞天、六元素五形态、30 基础术法与6融合、11心法、80法宝主题变体。
- 分层解锁、传承与劫数、机缘、灵石坊市、祭炼、重铸、宝匣、封存/跳过，以及完整法宝整理。
- 种子地图、六类异象、各洞天两阶段洞主；原生HUD、数值道典、失焦暂停与帧率预算。
- 每修士一个传统/无尽波间存档，包含随机流；验证并消费后续接，快速模式不存档。

新增内容的未定数值明确标为暂定；80件法宝共享部分触发/玄机模板，不是80种独特机制。原型低模美术并非概念图最终质量。

[完整设计交接](docs/full-game/HANDOFF.md) · [逐条需求复核](docs/full-game/REQUIREMENTS-COVERAGE-FINAL.md) · [UE官方资料审查](docs/full-game/UE-OFFICIAL-REVIEW.md) · [新自然操作样例](docs/full-game/NATURAL-PLAY-CHECK.md)

## 下一次可玩验证的范围

先验收两位初始修士、雷泽、快速模式、新号内容的完整闭环，不再以增加框架或内容数量代替试玩。已实现的其它模式与内容不删除，暂不阻塞这轮体验判断。见 [两局真人试玩与记录](docs/playtest/FIRST-PLAYABLE.md)、[实际复用决策](docs/playtest/REUSE-DECISIONS.md) 和 [自动节奏诊断](docs/playtest/20261031-pacing-diagnosis/README.md)。

## 当前代码分层

- `Source/DywxCore`：纯 C++ 规则与校验，可独立编译测试。
- `Source/DywxRuntime`：独占模拟推进的世界会话、命令入口、存档适配。
- `Source/DywxUI`：Slate、Controller、本地焦点与渲染/音效表现，不拥有模拟。

UI 不再取得可变核心对象。当前仅实现 Standalone；只读核心查询不是网络 DTO，未实现多人复制。设计取舍及官方/社区依据见 [UE 分层架构](docs/architecture/UE-LAYERING.md)。模块拆分后必须重新生成 UE 工程文件，并完成 UHT/UBT 与 Windows DLL 链接验收。

## 完全脱离 UE / UI 运行

`standalone/CMakeLists.txt` 可构建纯 C++ 静态库 `Dywx::Core`、`dywx_headless` 自然模拟程序及 CTest 命令回归，不需要 Unreal SDK。其他表现层可直接链接同一库。构建与输出说明见 [独立核心与无界面演练](standalone/README.md)。

## 在 dot 可执行的检查

```sh
bash scripts/test-full-game-all.sh
ASAN_OPTIONS=detect_leaks=0 bash scripts/test-full-game-all.sh --sanitize
```

需要 C++20 编译器、Python、Pillow 与 fontTools；当前 dot 已有。宿主 ptrace 导致 LeakSanitizer 无法使用，关闭泄漏检测不等于证明无泄漏。所有这些检查仍不是 Unreal 编译或实机画面证明。

新完整模式的固定样例有胜利、死亡与采样上限，见新报告；不能引用旧 MVP 通关轨迹证明当前全部模式或三位修士已平衡。实际 UE 编译、画面、GPU与人类试玩仍未验证。

## 在已有 UE 5.8 工具链的 Windows 构建

```powershell
powershell -File scripts/package-windows.ps1 -EngineRoot 'D:\UE_5.8' -ArchiveDirectory 'D:\dywx-package' -EvidenceDirectory 'D:\dywx-build-evidence'
```

路径按实际安装修改。脚本先编译 Editor 模块，再生成必需材质、导入音频，最后 Cook/Stage/Archive；任何前置资源门禁失败都不应发包。当前新增流程尚待首次 UE 运行验证。不要只拷贝 EXE，须保留完整运行目录。

## Linux 原生构建入口

```sh
UE_ROOT=/path/to/UnrealEngine bash scripts/build-ui.sh
UE_ROOT=/path/to/UnrealEngine bash scripts/bootstrap-mvp-assets.sh
UE_ROOT=/path/to/UnrealEngine bash scripts/package-linux.sh /absolute/output
```

Linux 入口输出 Linux 包，不是 Windows EXE。它们不会自动下载安装引擎、创建账号或修改系统安全设置。无引擎时明确退出 NOT_RUN。

## 资源与设计

- [UE 世界适配与资源流程](docs/mvp/UE-WORLD-ADAPTER.md)
- [UI/输入接线](docs/mvp/UI-BRIDGE-VERIFICATION.md)
- [道典与发现记录](docs/mvp/CODEX-DISCOVERY-IMPLEMENTATION.md)
- [资源来源与字体许可](Art/ASSET-PROVENANCE.md)
- [Blender 模型与步态源文件](Art/Blender/MVP/README.md)
- [原始玩法文档索引](docs/README.md)

Blender 预览和逻辑轨迹视频均明确标为离线演示，不是 UE 实机录像。美术仍为轻量原型资源，不宣称达到概念图级细节。
