# UI 资源来源与使用边界

本次资源用于《道衍万象》的主菜单、角色选择、地图选择与战斗 HUD 原型。

- `Content/UI/Images/*-background.png`：使用内置生图工具，以已交付的四张 UI 概念图为参考生成。移除烘焙文字、按钮、边框，由真实 UE 控件覆盖。完整提示词保留在 `Art/Generated/`。
- `main-background.png` 对应主菜单概念；`character-background.png` 对应断岳行者预览。角色仍为图像占位，不是可动画的三维角色模型。
- `map-background.png` 对应栖雷古坛选图页；`battle-background.png` 仅供显式 HUD 预览，绝不是已实现战斗、敌群或实时三维场景。
- `icon-*.png`：本次通过 Blender 几何、材质与灯光独立制作，源文件与可复现脚本位于 `Art/Blender/`。透明图标是实际渲染输出，未把概念图切成图标。
- `Content/UI/Fonts/NotoSansSC-Regular.otf` 与 `NotoSerifSC-Regular.otf`：从 dot 预装的 Noto CJK TTC 中抽取简体中文 SC 字面（索引 2），保留完整字形与原字体内部命名。SIL Open Font License 1.1，随附 `Content/UI/Fonts/NOTO-LICENSE.txt`。上游 https://github.com/notofonts/noto-cjk 。未从不明来源下载商业字体。
- 概念对照图保留在 `docs/ui-concepts/`，不作为整屏按钮热区或交互成品。

生成资源不代表具备第三方品牌、人物或其他权利的独占授权。正式发行前仍须进行项目自己的美术和许可审查。

本次没有下载 UE 引擎或把生图渲染当作 UE 运行截图。UE 编译、打包与视觉验收需在可用引擎中另行完成。

补充生成资源：
- `character-portraits.png`：四列角色头像纹理，运行时按 UV 四等分；顺序为断岳行者、巡星羽客、司霆方士、归灵祝师。后面三名只作为构想预览，未实现为战斗角色。
- `button-gold.png`：透明金色边框按钮底图，标签由真实控件显示；原始尺寸 2172×724，中心竖向透明留边通过运行时 UV 处理，不是文字截图。
