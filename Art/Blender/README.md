# Blender UI 图标资源

六个原始程序化建模图标，为《道衍万象》界面提供独立透明图层。形状遵循金色斩击、银色刃环／贯穿箭、冷色遁术与石／镜遗珍的概念方向；它们是可替换的首轮制作资源，不是最终美术验收。

## 文件

| PNG（位于 `Content/UI/Images/`） | 对应内容 |
|---|---|
| `icon-slash.png` | 断岳三叠，三片金色弧刃 |
| `icon-orbit.png` | 回风刃环，内外银色旋刃 |
| `icon-arrow.png` | 穿云重矢，银色箭锋与金色尾羽 |
| `icon-dash.png` | 遁术，奔跑剪影与速度线 |
| `icon-stone.png` | 磨砺石，带金色矿脉的切面石块 |
| `icon-mirror.png` | 须弥镜，青色镜面与金色环饰 |

- 全部为 512 × 512 RGBA PNG，无文字，无按钮底板。支持以约 72–100 像素展示；按钮圆环与状态应由真正的 UI 控件绘制。
- `ui-icons.blend`：完整几何、材质、正交相机和三盏灯。六个集合按 `slash/orbit/arrow/dash/stone/mirror` 命名。保存时仅 slash 集合可见；单独启用所需集合可编辑和渲染。
- `build_ui_icons.py`：重建全部几何与材质、渲染六图并保存源文件。不需要外部模型、纹理、插件或联网。
- `verify_ui_icons.py`：只读检查尺寸、RGBA、透明背景、边距、非空像素与 SHA-256。需要 Pillow。
- `validation.json`、`render.log`、`source-validation.log`：本次本地检查记录。

## 重建

在仓库根目录，用现有官方 Blender 执行：

```sh
blender --background --factory-startup --offline-mode --disable-autoexec --python Art/Blender/build_ui_icons.py
python3 Art/Blender/verify_ui_icons.py
```

脚本按自身位置定位仓库。重建会覆盖本目录下 `ui-icons.blend` 及上述六张 PNG；保留自定义修改前应另存副本。

本次使用 Blender 5.2.3 LTS / Cycles CPU，32 samples，AgX。不同 Blender 版本或渲染器可能产生外观差异。源场景实测重新打开通过，共 41 个对象、7 个材质，六个图标集合均存在。

## 实现与验证边界

全部图形由本项目脚本生成实际几何，不是从概念图裁切或将文字烘焙成图标，无第三方素材授权依赖。PNG 已目视检查，并通过透明度、内容非空及边缘安全区检查。渲染期间系统只读音频与缩略图缓存产生警告，但 PNG 和 `.blend` 输出成功，进程退出码为 0；没有改动系统权限、启动 MCP 或开放端口。

尚未在 Unreal Editor 中导入或验证实际 UI 显示。若改为 Texture2D 导入，建议 UI 纹理组、保留 alpha 并实际检查 sRGB、缩放、透明边缘；不要将 PNG 文件检查当成 UE 渲染验收。
