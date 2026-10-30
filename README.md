# 道衍万象

东方神话风格的 UE5 原生界面原型。

## 当前交付

`DywxSurvivors.uproject` 包含主菜单、角色选择、洞天／模式选择、战斗 HUD 的 C++ Slate 实现。控件、中文文本、导航、弹窗与状态绑定独立于背景图；背景、头像、金色按钮与六枚 Blender 图标均随工程提供。

**当前为待 UE 复验的修正版源码，不是新验收通过的 EXE。** 原版本 `8ef559d` 曾在 win-dev 通过 UE 5.8.4 编译打包，但用户运行后发现按钮截字、布局偏离及 GPU 99% 占用。本轮切换 Sarasa UI SC，修正文本/按钮布局，并加入仅预览模式的帧率预算与遮挡世界渲染抑制；由于 win-dev 已关机，当前修改只做 dot 离线测试，尚未重新编译或实机验证。旧包不得作为修正版使用。

- [本轮修复独立复查](docs/UI-REPAIR-REVIEW.md)
- [预览性能修复与待验收项目](docs/preview-performance-validation.md)
- [启动、构建与玩法接口](docs/UE-UI-IMPLEMENTATION.md)
- [Windows 演示包运行说明与构建证据](docs/WINDOWS-DEMO.md)
- [验证记录与待验收项目](docs/UI-VALIDATION.md)
- [资源来源与许可](Art/ASSET-PROVENANCE.md)
- [Blender 图标源文件与复现](Art/Blender/README.md)
- [原有设计文档](docs/README.md)

## dot 可执行检查

```sh
bash scripts/test-ui-state.sh
bash scripts/test-preview-budget.sh
python3 scripts/validate_assets.py
python3 scripts/test_font_layout.py
```

独立状态测试与 UE 工程均使用 C++20；资源检查需要 Python、Pillow 与 fontTools。当前 dot 已具备这些检查依赖；工程运行时不依赖 Python。

已有 Linux UE5 安装时：

```sh
UE_ROOT=/absolute/path/to/UnrealEngine bash scripts/build-ui.sh --check
UE_ROOT=/absolute/path/to/UnrealEngine bash scripts/build-ui.sh
```

脚本不会下载引擎、改变系统配置或将缺少引擎伪装为构建通过。完整启动方式见实施说明。
