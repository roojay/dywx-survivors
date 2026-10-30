> 当前源码已进入修复阶段：旧版实际运行出现截字和 GPU 高占用。历史编译/打包通过不代表当前字体、布局与限帧修复已运行验证。当前复查见 UI-REPAIR-REVIEW.md 与 preview-performance-validation.md。

# 原生 UE UI 验证记录

日期：2026-10-30。范围：主菜单、角色选择、地图选择与战斗 HUD。dot 阶段校验与随后 win-dev 的真实 UE 构建分开记录；本文不是实机视觉验收证明。

## win-dev Windows 构建

- UE 5.8.4 / CL 58902052，MSVC 14.50.35725（工具目录 14.50.35717），Windows SDK 10.0.26100.0。
- Editor 和 Game 的 UHT、UBT 编译/链接通过。修复了 `RequestRun` 参数 `Character` 遮蔽 UE `AController::Character` 触发的 C4458 编译错误。
- Win64 Development Cook、Stage、Archive 通过，AutomationTool 退出码 0。Cook 明确报告 0 errors、0 warnings。
- 原始运行目录 58 文件、669,210,292 字节；无 PDB。15 个 `Content/UI` 文件与源码 SHA256 完全一致，包含两套中文字体、许可和全部 PNG。
- 运行目录含 AMD64 PE EXE、依赖 DLL、PAK/IoStore、配置及 x64 VC 运行库安装包。清单和证据见 [Windows 演示说明](WINDOWS-DEMO.md)。
- 保留一个 UE 5.8 支持但已弃用的文件路径字体构造器警告 C4996；编译成功，后续升级引擎时应迁移到 FCompositeFont。

## 已执行

- `bash scripts/test-ui-state.sh`：通过。使用 g++ 编译无 UE 依赖的导航状态策略，覆盖页面顺序、重复操作、构想角色/地图/无尽锁定、返回与暂停、选择保留及异常进度分数。独立策略测试与 UE 模块均指定 C++20；此测试不编译 UE 相关代码。
- `python3 scripts/validate_assets.py --output Art/asset-validation.json`：资源和配置检查通过；具体检查项、文件 SHA-256、尺寸、透明区域与字体覆盖见 JSON。共 17 项检查：4 张背景、6 张图标、四角色图集、金边按钮、2 个字体、字体许可、NonUFS 打包目录与 runtime 模块声明。最终字体扫描覆盖 237 个源码汉字。
- PNG 完整读取通过；图标为 512×512 RGBA，透明背景、内容非空且保留边距。背景为 1672×941，接近 16:9。字体覆盖源码中出现的全部汉字；这不是 UE 字体布局或视觉可读性验收。
- 独立静态审查发现的问题已在源码中修正：UE 模块最低 C++20、菜单/弹层使用 UIOnly 防止输入漏入战斗、控制器接收失焦后的 Esc/空格、忽略长按重复导航和默认不确认退出。四角色图集使用等比例显示；纹理强引用在 widget 生命周期内保留。
- 已检查四张原始概念图。实现结构将背景、文本、按钮、状态条分离，不使用整图热区伪装交互。具体视觉接近程度仍需实际运行比较。

金边按钮按 UV 纵向 0.21–0.77 使用，显著 alpha（>1/255）边界完全落在裁切范围内；范围外仅有最多 1/255 的生成噪点。四角色图集按水平四等分取 UV，四区均非空；此检查不替代 Slate 九宫格和采样边缘实测。

## dot 阶段环境记录与当前未执行项目

dot 阶段 `bash scripts/build-ui.sh --check` 返回 `NOT_RUN: UE_ROOT is unset`，退出码 2。随后经用户授权在 win-dev 完成了上面的真实构建，不再将 UHT/UBT 或 Cook/Package 标为未运行。

以下均为 **not_run**：

1. 编辑器交互打开、PIE 与打包后独立程序启动。
2. 打包运行时的 PNG/字体绘制（文件存在性和哈希已验证）。
3. Slate 实际渲染、鼠标命中、Tab/手柄导航、焦点与跨分辨率布局。
4. 真实战斗数据驱动、暂停接入、地图加载、角色动画与 GPU 性能。

静态测试和成功打包都无法证明上述项目通过。工程目标 UE 5.8，已构建版本为 UE 5.8.4；具体环境见 Windows 构建记录。

## 有引擎后必须检查

- 菜单开始→角色→地图→HUD 预览，逐级返回，选择模式后返回再进入应保留选择。
- 构想角色可查看但不可开局；后续地图禁用；无尽锁定及 `SetEndlessUnlocked` 后状态正确。
- 连击、按住 Enter/Esc、Tab 后确认，不应重复请求开局或意外退出；退出默认取消。
- 预览暂停打开/关闭、返回菜单后重开；实时 HUD 的游戏输入只在无弹层时透传。点击场景后 Esc 仍可打开菜单，遁术事件不应重复触发。
- `BeginLiveHUD` 后背景消失、显示真实世界；`UpdateBattleHUD` 不抢焦点，并能处理空技能数组、负数、零分母和非有限输入。
- 玩法消费 `OnRunRequested`、`OnDashRequested`、`OnPauseChanged` 与 `OnReturnToMenu`。UI 本身不负责模拟战斗、真正暂停世界或切换地图。
- 在 1280×720、1920×1080、超宽窗口及大字号下检查所有标题、按钮、HUD 数字和锁定说明无截断。
- 完成打包后从包启动，核对 `Content/UI` 的背景、图标与字体均正确显示；不能只检查编辑器版本。

## 核对的官方 API

- [Slate 在游戏中的接入](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-slate-in-game-in-unreal-engine)
- [运行时文件导入纹理 FImageUtils::ImportFileAsTexture2D](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/FImageUtils/ImportFileAsTexture2D)
- [FSlateFontInfo](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/SlateCore/FSlateFontInfo)
- [Epic C++ 编码规范及 C++20 最低标准](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)
