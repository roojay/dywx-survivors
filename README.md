# 道衍万象

东方神话风格的 UE5 原生界面原型。

## 当前交付

`DywxSurvivors.uproject` 包含主菜单、角色选择、洞天／模式选择、战斗 HUD 的 C++ Slate 实现。控件、中文文本、导航、弹窗与状态绑定独立于背景图；背景、头像、金色按钮与六枚 Blender 图标均随工程提供。

**当前不是已验收的可玩游戏。** dot 环境没有 Unreal Editor。已运行独立 C++ 状态测试和资源校验；UE 编译、UHT、编辑器启动、打包、实际界面与性能全部未运行。战斗页默认明确展示静态 HUD 示例，通过接口接入真实玩法后才隐藏预览背景。

- [启动、构建与玩法接口](docs/UE-UI-IMPLEMENTATION.md)
- [验证记录与待验收项目](docs/UI-VALIDATION.md)
- [资源来源与许可](Art/ASSET-PROVENANCE.md)
- [Blender 图标源文件与复现](Art/Blender/README.md)
- [原有设计文档](docs/README.md)

## dot 可执行检查

```sh
bash scripts/test-ui-state.sh
python3 scripts/validate_assets.py
```

独立状态测试与 UE 工程均使用 C++20；资源检查需要 Python、Pillow 与 fontTools。当前 dot 已具备这些检查依赖；工程运行时不依赖 Python。

已有 Linux UE5 安装时：

```sh
UE_ROOT=/absolute/path/to/UnrealEngine bash scripts/build-ui.sh --check
UE_ROOT=/absolute/path/to/UnrealEngine bash scripts/build-ui.sh
```

脚本不会下载引擎、改变系统配置或将缺少引擎伪装为构建通过。完整启动方式见实施说明。
