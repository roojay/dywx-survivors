# 最终离线汇总检查

在当前 dot Linux 环境对整合后的源码执行，2026-10-30 UTC。核查源文件哈希见 `final-tested-source-sha256.json`。

执行：`bash scripts/test-mvp-all.sh --sanitize`，退出码 0。

通过：导航、菜单/游戏帧率预算策略、成长/法宝、核心战斗及触发、已获得术法等级行为、发现数据、独立场景/边界/饱和压力、中文字体尺寸、UI/焦点/存档源码契约、素材/网格与打包资源门禁、Python 语法。

补充执行：`python3 Art/Blender/MVP/validate_walk_runtime.py`，退出码 0；步态距离推进与实例数保留上限的静态/算术检查通过。它不是实际 GPU 测试。

ASan/UBSan 没有报告诊断。由于宿主 ptrace，使用 `detect_leaks=0`，没有通过 LeakSanitizer 的结论。

最后两位修士的自然规则通关记录与最终源码指纹、轨迹 SHA 均经比对一致。详见 PLAYTHROUGH-CALIBRATION.md。轨迹录像是 Blender 离线可视化，不是 Unreal 运行证明。

仍未运行：当前版本 UHT/UBT、UE 材质/音频导入、Cook/Package、实际窗口与输入/字号、GPU/帧率/显存、人类试玩及美术验收。
