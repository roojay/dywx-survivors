# 独立规则库与无界面演练

此入口只需要 CMake 3.20+ 和支持 C++20 的编译器；不查找 Unreal、Slate、窗口系统或游戏资源。显式编译 `DywxCore/Private/MVP` 的七个生产源文件，不编译 UE Module 入口，也不链接 Runtime/UI。没有另写一套简化规则。

## 构建、测试与演练

从仓库根目录运行：

```sh
cmake -S standalone -B build/standalone -DCMAKE_BUILD_TYPE=Release
cmake --build build/standalone --config Release --parallel 2
ctest --test-dir build/standalone -C Release --output-on-failure
./build/standalone/dywx_headless ./build/standalone/trace.csv > ./build/standalone/results.csv
```

Visual Studio 等多配置生成器的可执行文件位于 `build/standalone/Release/`，Windows 文件名有 `.exe` 后缀。最后一行相应改用该路径；CSV 重定向是 shell 功能。

- `DywxCore`：纯 C++20 静态库（别名 `Dywx::Core`）。
- `core_command_tests`：复用 `tests/core_command_tests.cpp` 的 CTest 测试；Release 也保持 `assert` 启用。默认 CTest 不运行较慢的自然演练。
- `dywx_headless`：直接复用 `tests/full_game_playthrough.cpp`，不是另一份策略或规则实现。首个位置参数为轨迹 CSV 路径；父目录必须存在。请显式传路径，既有源码的缺省值是 `/tmp/dywx-full-natural-trace.csv`。
- 标准输出是每场一行的结果 CSV（含表头），轨迹文件按约30模拟秒、波次/阶段、升级与选择变化记录，并包含地面未拾经验。异常写标准错误并返回1；正常完成返回0。`won`、`dead`、`sample_bound` 都是有效观测，返回0不代表全部通关。输出路径会被覆盖，不要复用需要保留的证据文件。

## 首轮新号快速演练

在轨迹路径后追加 `--quick-onboarding`，只运行两位初始修士 × 四个固定 seed（11/29/73/101），全部为雷泽、快速、新号第一层。

```sh
./build/standalone/dywx_headless ./build/standalone/onboarding-trace.csv --quick-onboarding > ./build/standalone/onboarding-results.csv
```

当前输出保留 `policy=0` 的原升级选择策略，并用独立 `harness_revision=pickup-distance-v1` 标记拾取走位修正。`first_upgrade` 指首次观测等级大于1，不是选择卡片的时间；负值表示观察期间未发生。策略修正前后的胜负不能当作生产玩法增强或回归。实验及失败样例见 [节奏诊断](../docs/playtest/20261031-pacing-diagnosis/README.md)。

## 其他表现层链接同一库

在宿主 CMake 项目中（路径按实际调整）：

```cmake
set(DYWX_BUILD_HEADLESS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE) # 仅当宿主也不需要 CTest 时设置
add_subdirectory(path/to/dywx-survivors/standalone dywx-core)
target_link_libraries(my_presentation PRIVATE Dywx::Core)
```

链接目标自动传播 C++20 要求、公开头文件目录及 `DYWXCORE_STANDALONE=1`，调用方可 `#include "MVP/Simulation.h"`。不要在 UE/UBT 模块中启用此宏；UE 继续使用既有 Build.cs 与模块导出契约。此入口不包含安装包、跨语言绑定或网络 DTO，新的表现层仍须自行适配输入、快照展示与存档 I/O。

## 演练范围与策略局限

固定运行10场：前9场为3模式 × 3修士，种子按修士固定11/29/73，洞天轮换；账户已解锁内容但永久属性、起始货币为零，风险0。最后1场为 seed11 新账号近战 Quick。局内使用生产模拟与合法操作，不注入伤害、经验或物品。

策略每0.1模拟秒读取全局快照并选择移动/遁术，构筑偏向近身火系范围与起手融合路线，不等同于真人可见范围或反应，也未为所有流派调优。Quick/Endless 输入预算1200秒，Traditional为2400秒；到预算记 `sample_bound` 而非胜利。程序以固定1/60秒步进尽快执行，不按真实时间等待。

这证明规则可以脱离 UI 运行，并提供可检查的轨迹；不证明所有模式可通关、玩法平衡、完整功能覆盖、UE 编译成功或 GPU 性能。详细历史策略与证据说明见 [自然操作烟测](../docs/full-game/NATURAL-PLAY-CHECK.md)；历史结果不能替代当前源码的新运行。本入口不自动生成源码哈希清单；需要冻结证据时继续使用既有 `scripts/calibrate-full-game.sh`。
