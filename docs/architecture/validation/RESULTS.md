# 分层与无界面验收

日期：2026-10-31。行为对照基线：3504a9dbfdeb54ef9079220c9c196329736ddc9f。

## 通过

- 冻结完整源码执行 `bash scripts/test-full-game-all.sh --sanitize`，退出0。新增核心命令边界与模块/职责契约已纳入。日志 `full-sanitizer.log`。
- 普通与 ASan/UBSan 自然十例的 results、trace、源码清单逐字节相同。
- 分层后 results/trace 与基线 `docs/full-game/dot-review-20261031/natural-fixed/` 逐字节相同，仍3胜4死3观察上限。
- standalone CMake Release 构建纯静态核心和headless程序；CTest 1/1命令测试通过（Release保留assert）。`cmake-ctest.log`。
- CMake headless 同样输出完整十例，结果/trace与3504a9d逐字节相同，重复运行一致。
- 独立外部 CMake consumer 链接 `Dywx::Core` 成功运行，无UE/Runtime/UI依赖。
- 保存/结算9个迁移函数保持原candidate事务语义，Profile及六个领域实现文件内容未变；领域新增部分为受控命令封装。
- 最终生产Source、测试与脚本与运行冻结副本一致，diff检查通过。

## 中间失败与修正

第一次总门禁在旧 `validate-mvp-render-contract.py` 失败：仍要求表现Actor声明迁往Runtime的存档API。没有通过删除断言掩盖；改为验证Runtime归属、World无此职责，并保留原Allocate/restore/consume/commit时序断言。九项Python契约/资源检查全通过后重冻，完整重跑通过。

## 未验证与限制

- ASan/UBSan启用，LeakSanitizer未启用；不是无泄漏证明。
- 正则/source contract不能证明UE编译和运行。本轮无UHT/UBT、Windows模块DLL链接、Cook、引擎生命周期、实际输入/焦点、GPU测试。
- 真实游戏仍单玩家，Runtime明确限定Standalone，不宣称已有联网或跨平台锁步。
- const领域查询为进程内适配，不是可直接传给网络客户端的安全DTO。
- 已有新号难度/御兽时间问题仍在；结构等价不代表平衡目标达标。
- GitHub完整推送仍因可用Git写入认证受阻。未新建Space代码备份。
