# UI 排版与预览性能修复：独立复核

日期：2026-10-30。基线：`origin/main`（`8ef559d`）。复核对象为
`fix/ui-layout-performance` 工作区修改。本文是源码与资源复核，**不是实机验收报告**。

## 用户证据与修复对应关系

用户提供的 2048×1152 截图中，主菜单四个控件均缺少最后一个字：开始修行、道典、设置、退出游戏；金色主按钮框被压缩，次按钮显示为不符合目标风格的灰色矩形。旧源码在居中按钮内对通用标签启用了 `AutoWrapText(true)`。

修复将按钮／短标签设为不换行、按钮内容横向填满并居中；长文另用明确宽度的段落；完整金框改用裁切图像而非九宫格。上述变化针对源码层面的合理原因，尚无修复后 Slate 实际截图证明效果。

## 独立执行的检查

- `bash scripts/test-ui-state.sh`：PASS，原生导航策略测试。
- `bash scripts/test-preview-budget.sh`：PASS，包含新增外部上限变更策略测试。
- `python3 scripts/validate_assets.py --output /tmp/dywx-independent-assets.json`：17 项 PASS，包括两款 Sarasa 字体各覆盖校验器提取的 238 个汉字、许可证及松散资源打包配置。
- `python3 scripts/test_font_layout.py --json /tmp/dywx-independent-font-metrics.json`：最终冻结源码上 24 项 PASS，最小实际纵向余量 6px（脚本要求至少 4px），覆盖代表性固定槽位在 100%／110% 字号下的离线度量。FreeType 度量并不等同于 Slate；不验证阴影、焦点与禁用外观。
- `git diff --check`：复核时 PASS。

## 生命周期与代码检查

- 预览保存弱视口引用，每 0.25 秒读取原生窗口状态；没有循环启动外部命令、枚举进程或查询 GPU。
- 世界渲染禁用仅在进入预览时设置，离开预览时恢复原值。实时 HUD 走释放路径，其暂停弹窗不会重新启用预览限制。
- 控制器结束时移除应用激活回调和 ticker，恢复预览持有的状态。
- 不可用内容仍由按钮禁用与导航策略双重限制。根控件焦点重设及普通 SButton 键盘行为保留，但实际焦点提示、键盘路由、禁用文字可读性未实测。

## 发现并修正的问题

首版预算代码仅在释放时避免覆盖外部帧率上限；若预览期间外部设置更改上限，随后切换焦点会重新覆盖它。

**源码复核已确认修正：** `PreviewBaseline` 在重新计算预算之前采纳新的外部上限；即使目标与之前写入值相同，也会在检测到外部改动时重新写入预算；退出时保留新的基线。新增较低、较高、无限制及状态变化测试独立复跑 PASS。引擎实际 CVar 优先级与所有权行为仍待运行验收。

截至复核，未发现其他已确认的静态阻断缺陷。

## API 核验与未执行项

官方 [UGameViewportClient API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UGameViewportClient)
包含本次使用的窗口、光标及输入访问接口；官方
[FGenericWindow API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/ApplicationCore/FGenericWindow)
包含前台与最小化查询。未发现高置信度的新 API 阻断问题；这不替代 UHT／UBT 编译。

**not_run：** 本次修复的 UE 编译／UHT、编辑器启动、Windows 打包、Slate 实际渲染与输入、真实玩法接入、实测 FPS 与 GPU 使用率。历史 Windows 包早于本次修复。用户反馈的 GPU 99% 尚未复现，不宣称已经降低 GPU 占用。待运行验收矩阵见 `preview-performance-validation.md`。
