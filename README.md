# 道衍万象

东方神话风格的 UE5 原生幸存者 MVP。当前分支以 `docs/gameplay-design-v0.1.md` 与 `docs/mvp-prototype-config-v0.1.md` 为实施依据。

## 当前状态，请先读

**当前交付是已实现并做过离线验证的 MVP 源码与资源，不是已验收的 Windows 游戏包。**

历史提交 `8ef559d` 只有四界面演示，曾通过 Windows 编译，但用户实际运行发现截字和 GPU 高占用。此后字体、布局、帧率预算已经重写，且加入了真正的战斗与成长系统。当前新源码尚未在 UE 重新编译、启动、烹饪或测试 GPU。旧 EXE 不包含这些修复，也不是本 MVP。

本分支已经包含：
- 两位修士、走位/遁术、自动术法、六类妖魔/大妖、四波与可配置六波、两阶段洞主、死亡/胜利/重开。
- 十门术法、五心法、两条破境、元素/形态共鸣、十五法宝、六装备位与储物袋、调息换装/锁定/筛选/炼化预览及撤销。
- 真正的模拟状态驱动原生 UI 与 UE 实例化三维世界，不再用静态战斗背景假装游戏。
- Sarasa UI SC 字体、原型三维人物/妖魔/神坛资源、主角/群行妖魔步态姿势、术法/预警表现、十枚原始音效。
- 菜单及游戏的前台/后台/最小化帧率预算、失焦暂停、输入状态清理、发现记录与保底进度的轻量本地保存。

[当前交接与限制](docs/mvp/MVP-HANDOFF.md) · [实施范围](docs/mvp/IMPLEMENTATION-SCOPE.md) · [自然通关校准](docs/mvp/PLAYTHROUGH-CALIBRATION.md) · [独立测试覆盖](docs/mvp/COVERAGE.md)

## 在 dot 可执行的检查

```sh
bash scripts/test-mvp-all.sh
ASAN_OPTIONS=detect_leaks=0 bash scripts/test-mvp-all.sh --sanitize
```

需要 C++20 编译器、Python、Pillow 与 fontTools；当前 dot 已有。宿主 ptrace 导致 LeakSanitizer 无法使用，关闭泄漏检测不等于证明无泄漏。所有这些检查仍不是 Unreal 编译或实机画面证明。

两位修士均已在生产参数、实际刷怪/伤害/掉落下由八方向自动操作完成自然通关。记录与可复现命令见校准报告。它证明规则闭环可达，不证明人类操作体验、平衡完成或目标设备流畅。

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
