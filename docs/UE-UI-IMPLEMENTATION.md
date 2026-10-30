> 修正版状态：2026-10-30 用户报告旧包截字与 GPU 高占用。本轮 Sarasa 字体、布局与预览预算修复尚未 UE 重编译/运行；下述历史 Windows 成功仅对应旧提交。

# UE 原生界面实现说明

## 当前交付边界

`DywxSurvivors.uproject` 是 UE 5.8 C++ 工程源码，使用原生 Slate 实现主菜单、角色选择、地图选择与战斗 HUD。所有按钮、文本、数值条、卡片、返回及弹窗均为独立控件；不是在整张概念图上加点击热区。

**win-dev 已用 UE 5.8.4 完成 UHT/UBT 编译及 Windows Development 打包。** 实际独立启动和 UI 视觉/焦点验收仍未运行；完整证据与运行方法见 `WINDOWS-DEMO.md`。dot 阶段的独立导航状态测试、资源校验与审查记录见 `UI-VALIDATION.md`。当前交付是界面预览，不是完整可玩游戏。

- 主菜单：开始、道典说明、实际界面字号设置、带取消优先的退出确认。
- 角色：四名角色可查看介绍；断岳行者可继续，其他三名明确为构想，不能启动不存在的玩法。美术是平面预览素材，非已绑定三维角色。
- 地图：栖雷古坛可选，两个后续洞天不可进入。快速／传统保留选中状态，无尽由外部解锁状态控制。模式尚未驱动玩法。
- HUD：生命、波次、时间、三术法等级、经验、遗珍和遁术状态；预览固定示例数据，显著显示“HUD 预览”。暂停菜单可返回主菜单，选择保留在本次运行内。
- 1920×1080 设计画布等比适配。其他长宽比可能留边；尚未在 UE 视觉检查。真实字体使用打包的 Sarasa UI SC，不抄生图小字。

## 启动与编译

需要已安装且可使用的 UE 5.8 与对应 C++ 工具链。本交付不自动下载引擎、不接受许可，也不改变系统设置。

编辑器打开 `DywxSurvivors.uproject`，允许正常生成／编译项目模块，然后在默认 Entry 地图点击 Play。配置指定 `/Script/DywxUI.DywxUIGameMode`，控制器启动时将原生界面加入本地 viewport，无需人工创建 Widget Blueprint 或地图 uasset。

Linux 命令示例（路径按实际引擎位置设置）：

```bash
export UE_ROOT=/path/to/UnrealEngine
"$UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh" DywxSurvivorsEditor Linux Development "$PWD/DywxSurvivors.uproject" -WaitMutex
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" "$PWD/DywxSurvivors.uproject"
```

Windows 命令示例：

```powershell
& "$env:UE_ROOT\Engine\Build\BatchFiles\Build.bat" DywxSurvivorsEditor Win64 Development "$PWD\DywxSurvivors.uproject" -WaitMutex
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor.exe" "$PWD\DywxSurvivors.uproject"
```

Windows 构建已在 win-dev 的 UE 5.8.4 中实际执行。打包复现入口为 `scripts/package-windows.ps1`；具体工具链、修复、证据和仍未执行的项目见 `WINDOWS-DEMO.md`。构建通过不能替代视觉验收。

## 代码入口

| 路径 | 职责 |
|---|---|
| `Source/DywxUI/Private/SDywxRoot.*` | 四屏布局、原生控件、输入与弹窗、资源缓存 |
| `Source/DywxUI/Public/DywxUIState.h` | 无引擎依赖的导航与锁定策略 |
| `Source/DywxUI/Public/DywxHUDState.h` | Blueprint 可写 HUD 数据快照 |
| `Source/DywxUI/Public/DywxUIPlayerController.h` | 玩法接入入口与事件 |
| `Source/DywxUI/Private/DywxUIGameMode.cpp` | 无 Pawn 的 UI 预览游戏模式 |
| `Config/DefaultGame.ini` | `Content/UI` 原始图片／字体 NonUFS staging |
| `tests/ui_state_test.cpp` | 实际可执行的状态策略测试 |

`Content/UI/Images` 内 PNG 作为独立背景／图标读取，不需要先导入 uasset。首次使用缓存纹理，并用 `TStrongObjectPtr` 保持 UObject 生命周期。字体用项目相对路径读取。正式大规模内容可再迁入 UE 资产管线；本次避免为几个界面额外引入插件或构建系统。

## 真实玩法接入

控制器 API 均在游戏线程调用：

- `OnRunRequested(CharacterIndex, MapIndex, ModeIndex)`：点击进入洞天发出；模式 0 快速、1 传统、2 无尽。默认先显示明确标注的静态 HUD 预览。玩法层可以监听后创建真实场景，再调用 `BeginLiveHUD(State)`。
- `BeginLiveHUD(State)`：进入实时 HUD，移除静态战斗背景与“预览”提示。显示真实世界，开启 GameAndUI 输入；不会生成敌人、移动角色或启动自动攻击。
- `UpdateBattleHUD(State)`：更新真实生命、经验、时间、波次、技能等级、遗珍与冷却；不模拟这些数据。UI 对 NaN、无穷、负数和越界比例做防护。
- `OnDashRequested`：实时 HUD 中按空格或点击遁术时发出，前提是显示的冷却已结束；玩法层仍须权威校验并更新冷却。UI 不扣资源、不假装执行遁术。
- `OnPauseChanged(bool)`：暂停／恢复请求。**玩法层负责实际世界暂停与恢复**；UI 本身只显示菜单并切换 UIOnly 输入，避免暂停菜单期间继续移动。多人游戏可自行决定语义。
- `ShowMainMenu()` 与 `OnReturnToMenu`：返回菜单并发出恢复／清理通知。不会替玩法销毁关卡或保存进度。
- `SetEndlessUnlocked(bool)`：由真实进度决定是否开放无尽；不会伪造通关记录。

当前默认 GameMode 没有 Pawn，没有游戏计时器，也没有完整战斗逻辑。集成已有游戏时应使用／派生该控制器或移植挂载接口，并在玩法 GameMode 明确设置它；不要把预览 GameMode 当作完整游戏模式。

## 交互检查

- 鼠标点击原生按钮；Tab 使用 Slate 焦点导航；Enter 确认当前按钮／页面主操作；Esc 关闭弹窗或回退，战斗中打开暂停。
- 页面级快捷键忽略按键重复；退出弹窗的一般 Enter 只取消，真正退出要明确点选“确认退出”。
- UI／弹窗使用 UIOnly，实时无弹窗 HUD 才使用 GameAndUI。控制器也绑定 Escape，避免点击世界后失去打开菜单的能力。
- 替换地图前后销毁 UI 时，EndPlay 从 viewport 移除根控件，释放资源；无后台监听、MCP 服务或全局设置变更。
- 未实现存档、音频、窗口设置、商店、背包或局外成长。字号仅在本次运行中生效。

## 本地测试

```bash
bash scripts/test-ui-state.sh
```

测试实际编译无引擎依赖头文件并执行：非法跳转、构想角色与锁定地图、模式锁定、重复操作、返回与暂停、选择保留、非法数值比例。它不编译 Slate/UHT，也不验证 UE 的 UI 树、字体绘制、打包资源或键鼠焦点行为。

UHT/UBT 完整编译与打包已通过。仍须检查默认 Entry 地图独立启动、4 屏视觉与焦点、100%/110% 字号、1280×720/1920×1080/超宽屏、真玩法接入，以及打包运行时的图片与字体绘制。

## 参考

- [Epic：在游戏中使用 Slate](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-slate-in-game-in-unreal-engine)
- [Epic：运行时图片导入](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FImageUtils/ImportFileAsTexture2D)
- [Epic：Slate UV 区域](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/SlateCore/FSlateBrush/SetUVRegion)
- [Epic：C++ 编码标准](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)

玩法 A/B 仍未统一，本实现只落地已要求的 UI，不通过控件空位默认引入任何一套争议机制。
