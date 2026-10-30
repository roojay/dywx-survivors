# MVP 声音资源

十枚短促程序化音效，由本项目 Python 脚本通过振荡器、包络和固定种子噪声合成。没有外部采样、第三方音乐、API 调用或授权费用。

运行 `python3 Art/Audio/build_sfx.py` 可重建；源 WAV 位于 `Content/MVP/AudioSource/`。UE bootstrap 将它们导入 `/Game/MVP/Audio/` 后由世界适配器播放。默认限制同时发声数及触发频率，避免高密度击杀变成音量叠加。

这些是原型反馈声，不是最终配乐或高保真仙侠音效。音频文件格式和幅度已检查，UE 实际响度、延迟和混音仍未运行验证。
