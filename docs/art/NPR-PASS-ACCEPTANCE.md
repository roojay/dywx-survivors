# NPR 增量美术验收

2026-10-31（UTC+8）。用户要求学习 80.lv 文章并继续完善现有资源。

本轮读取文章正文；未取得嵌入视频，未声称逐步骤复现。文章强调两点照明、合成统一与氛围效果，本轮优先采用前两项，避免浓雾遮挡战斗。来源：https://80.lv/articles/watch-this-video-to-learn-how-to-create-better-npr-art-in-blender

## 已落实与证据

- 实际几何/面色改进：`../npr-mesh-refinement.md`；运行时读取更新的 `Content/MVP/Geometry/scene.json`，不是把整张效果图放入战斗。
- 同镜头前后对照：`../../Art/Blender/MVP/npr/before-after.png`。保留原舞台主角 1.2 倍示意缩放，不是游戏镜头。
- 本轮实际轨迹抽帧：`../../Art/Blender/MVP/npr/trace-check/` 中 0、49、99 帧。由原有同一自然通关轨迹和更新网格重新离线渲染，保留危险圈与弹幕；没有改模拟数据。第 49 帧显示红色预警仍可分辨，但主角较小、地砖格缝仍突出，不能视为完整实战验收。
- 材质兼容性：`NPR-MATERIAL-COMPATIBILITY.md`。保持原有 alpha=1 与 UE 材质接线，未把 Blender 合成器、AgX 或薄雾谎称已移植。
- 聚合离线检查加入 `scripts/test-mvp-art-material-contract.py`，覆盖资源颜色与现有材质静态契约。几何、步态、当前两份 Blender 源文件的基础网格一致性也已校验。
- 本轮最终 `bash scripts/test-mvp-all.sh --sanitize` 通过；ASan/UBSan 未报告异常。LeakSanitizer 仍因宿主限制未运行。这些检查不编译 UE。

## 独立视觉结论

中性炭灰地面、旧铜、主角明度焦点以及弱化 Boss 鬃毛是可见的增量改善。仍是切面低模原型，而非已达到概念图的绘画质感。Boss 正面偏暗、地砖过于规则，后续需在实际 UE 画面中继续调整。

## 性能与验收边界

全部资源三角形 45,680→43,066，但分裂顶点 34,850→99,854，JSON 4,857,902→9,147,202 字节。不得把本轮描述为性能优化。实际 UE 加载、内存、GPU 帧时间仍未测；若目标硬件压力增大，应优先减少小怪不可见的切面/面色分裂，而不是继续堆后期。

所有 Blender 图片均为离线预览。当前版本尚未 UE 编译、打包或实机运行；不能宣称已交付流畅精美的 EXE。
