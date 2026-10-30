# NPR 资产与 UE 材质兼容性（静态检查）

2026-10-30。本轮保留现有 UE 材质、灯光与数据契约，不为离线效果增加运行时成本。

## 真正进入运行时的内容

- `scene.json` 的顶点位置、法线、线性 RGB 顶点色、三角形随资产更新；单材质槽及 31 个 mesh 名称保持不变。
- `M_Surface` 为 opaque / default-lit：`VertexColor.RGB × Tint` 接 Base Color，常量粗糙度 **0.86**；同色乘 **0.22** 的 emissive 仅作夜景可读性底色，**不是环境光或间接光模拟**。
- RGBA 的 A 仍为 **1**，没有挪作粗糙度或透明度。曾评估借用 A，但本次 Blender 金属装饰 0.84 / 哑光 0.90 与 UE 0.86 差距很小，不值得更改语义。后续不能不经迁移就重用该通道。
- 现有 `MoonLight` 冷主光与 `WarmRim` 暖补光已构成双灯关系，均不投射动态阴影；不增加每敌人灯光、描边、后处理材质或体积雾，不改战斗区可见性。
- `M_VFX` 继续独立 unlit / opaque 路径，`Glow` 不受本轮调整影响。

## 不应冒充已移植的内容

Blender 的金属度 **0.22**、装饰 **0.84** / 哑光 **0.90** 粗糙度分组、局部自发光、Area/Point lights、阴影、AgX 与 compositor **不会随 JSON 导出**。UE 使用统一 0.86 粗糙度且没有金属度连接。这是明确的近似，不是外观一致性承诺。颜色明暗分面是已导出的 RGB，和仅留在 Blender 节点树里的效果不同。

当前离线 PNG 用于检查造型/色块方向，不证明 UE 阴影、材质编译、最终曝光或 GPU 帧时。未因离线截图盲目降低 0.22 可读性底色。

## 可重复验证

```sh
python3 scripts/test-mvp-art-material-contract.py
python3 scripts/validate-mvp-render-contract.py
```

新增检查核对 31 个 mesh、字段集合、RGBA/法线数量、alpha=1、现有材质连接和双灯无阴影约束，并按三角形面积计算英雄/敌群/地面基础色亮度关系。该指标不等于最终屏幕对比度；至少 1.5 倍的基础色亮度关系只是防止资产意外抹平角色/背景层次的保守门槛。

**未运行：** UE Editor Python bootstrap、shader compile、cook、游戏内 render、GPU 性能验证。必须在真实 UE 环境复查英雄白衣与深色敌群、火焰与危险边界，以及暗部细节；不能用这些静态检查替代。

## Epic 官方依据（2026-10-30 核对）

- [Constant Material Expressions / VertexColor](https://dev.epicgames.com/documentation/en-us/unreal-engine/constant-material-expressions-in-unreal-engine)：RGB 与 A 为独立输出；顶点色来自 Static Mesh。
- [Material Inputs](https://dev.epicgames.com/documentation/en-us/unreal-engine/material-inputs-in-unreal-engine)：Roughness 控制高光粗糙程度，Metallic/Emissive/Opacity 是不同输入。没有给金属度接线不能声称已导出 Blender 金属响应。
- [MaterialEditingLibrary（UE 5.6 Python）](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MaterialEditingLibrary?application_version=5.6)：创建表达式及连接材质输入的现有 API；文档核对不是在此环境运行成功的证据。

本轮没有引入新材质节点或猜测的 Unreal API。
