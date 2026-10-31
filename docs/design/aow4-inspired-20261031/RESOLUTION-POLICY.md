# PC 三分辨率策略（直接 Slate）

## 范围与项目选择

支持矩阵：**1280×720、1920×1080、2560×1440**，每档验证默认字号与110%字号。本项目把常见PC“2K”明确解释为2560×1440，而非影院2048×1080。保留1920×1080设计画布、16:9比例和 `SScaleBox::ScaleToFit`；非16:9留边，不拉伸画面。

这是参考社区实践后的**项目策略**，不是一个普遍强制“社区标准”，也不是无障碍认证：

- [Epic DPI Scaling](https://dev.epicgames.com/documentation/en-us/unreal-engine/dpi-scaling-in-unreal-engine)介绍的是UMG自动缩放与常用Shortest Side曲线。本项目直接使用Slate，不能仅添加UMG曲线便声称它会生效，也不额外重复乘一次该曲线。
- [Unreal Garden：Resolution-independent UIs](https://unreal-garden.com/tutorials/ui-resolution/)建议优先验证最低支持分辨率、长文本与玩家自定义缩放。这里以720p作为下限，并保留现有110%文字选项。
- [XAG101：Text display](https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/101)要求通过真实画面测量字形高度；名义字体point/em并不等于实际可见字形高度。本补丁没有原生截图，不能宣称通过XAG。

## 可执行字号策略

纯C++定义：`Source/DywxUI/Public/DywxUILayoutPolicy.h`。

```
resolutionScale = clamp(min(viewportWidth/1920, viewportHeight/1080), 2/3, 4)
userScale = clamp(finite user setting, 1.0, 1.1)
pointSize = max(round(authoredPointSize * userScale),
                ceil(18 * 72/96 / resolutionScale * userScale))
```

18px是项目选择的**名义em下限**，不是“所有字形都有18px墨迹高度”的保证。按96DPI近似换算：

| 分辨率 | 参考画布缩放 | 最小point（100% /110%） |
|---|---:|---:|
|1280×720|2/3|21 /23|
|1920×1080|1|14 /15|
|2560×1440|4/3|11 /12|

大标题和原本超过下限的文字保留作者层级。只在小于1080p时扩展必要HUD槽与底部托盘，避免rank/XP重叠；1080p与1440p保留概念布局锚点。720p关键按钮点击区至少48参考画布像素高（约32显示像素），这是本项目鼠标目标选择，不是所有滚动条/装饰元素的通用标准。

`SDywxRoot::Tick`读取当前viewport尺寸；仅字号下限整数或小视口布局分支变化时重建表现。最小化/零尺寸时保留当前状态。此路径不发Session命令，不清空顿悟选择，不改变手动/失焦暂停。重建仍使用既有UIEpoch保护旧回调。

非法尺寸/NaN输入有界回退；小于720p只做有界降级，不承诺仍满足字号/目标尺寸；大于支持矩阵和非16:9仅保留比例，未纳入本次验收。OS缩放、Slate Application Scale、窗口DPI及实际font rasterization是独立因素，不能用当前公式替代原生观察。

## 验证和资源边界

- `scripts/test-ui-layout-policy.sh`：真实生产header的独立C++边界与有限值测试。
- `scripts/test_font_layout.py`：编译策略probe，使用实际Sarasa字体，对三分辨率×100/110%测量文字宽度、行高、墨迹包围盒及可用槽；不在Python静默另写一套生产公式。自动高度/滚动内容与固定槽分开说明。
- 概念UI契约检查720p分支、安全区、分离的封存/滚动区域、图片行padding和resize无游戏命令；既有导航/暂停回归保持。
- 测试输出的glyph ink可能小于18px，尤其数字和圆点；名义em不能冒充字形或对比度验收。
- **NOT_RUN：UE编译、原生Slate输入、三档真实渲染、OS/DPI组合、实际像素对比和性能。按用户要求未调用Windows验证。**
- 本策略不重采样原画。低于目标分辨率的PNG由渲染器放大，不因此称为“原生2K”；原始尺寸与哈希以`Art/UIConcepts/assets.json`和实际PNG为准。高分辨率字体由TTF提供，但实际渲染清晰度仍未原生验证。
