# 源码恢复检查点：反馈与可靠性修复

**一次性源码灾备，不是 EXE/Release，不代表 main 已同步。不要将这个归档分支合并到 main。**

源码目标 `6a1031d38b2e1246bc97ec3600c5159ca41b7bbf`，tree `73e913a3615cbc8484164944939d87c0d2438d1d`。
复用全量 ff3452e 的20块及第一增量 delta.bundle（803e9d1），仅新增第二增量 delta-6a1031d.bundle 与清单。所有已提交资源和原history保留；三个bundle必须按顺序恢复，第二增量不能独立使用。

## 安全恢复

使用全新、不存在的目录；每一步成功后才执行下一步。Windows 将 python3 换成 py -3。

```sh
git clone --single-branch --branch checkpoint/20261031-feedback-6a1031d https://github.com/roojay/dywx-survivors.git dywx-checkpoint
cd dywx-checkpoint
python3 restore.py
python3 -c "import pathlib,json,hashlib; names=['delta-manifest.json','delta-6a1031d-manifest.json']; ms=[json.loads(pathlib.Path(n).read_text()) for n in names]; ok=all(len(pathlib.Path(m['bundle_name']).read_bytes())==m['bytes'] and hashlib.sha256(pathlib.Path(m['bundle_name']).read_bytes()).hexdigest()==m['sha256'] for m in ms); print('Both delta SHA256 match:',ok); raise SystemExit(0 if ok else 1)"
git bundle verify source.bundle
git clone --branch feat/playable-mvp source.bundle ../dywx-restored
git -C ../dywx-restored bundle verify ../dywx-checkpoint/delta.bundle
git -C ../dywx-restored fetch ../dywx-checkpoint/delta.bundle refs/heads/feat/playable-mvp:refs/heads/recovered-803e9d1
git -C ../dywx-restored bundle verify ../dywx-checkpoint/delta-6a1031d.bundle
git -C ../dywx-restored fetch ../dywx-checkpoint/delta-6a1031d.bundle refs/heads/feat/playable-mvp:refs/heads/recovered-6a1031d
git -C ../dywx-restored switch recovered-6a1031d
git -C ../dywx-restored fsck --full
git -C ../dywx-restored rev-parse HEAD HEAD^{tree}
```

最后两项应为上述目标commit/tree。校验失败立即停止；不要对现有开发仓执行这些恢复命令。restore.py 不覆盖不同已有bundle，fetch 不使用强制标志，并写入新命名分支。没有自动推送或修改main。

## 验证边界

冻结的 test-full-game-all.sh --sanitize 退出0；CMake Release CTest 22/22；CMake与ASan默认10例/新号8例结果及trace逐字节一致，原803e已有字段/trace未变。详见恢复后源码 docs/full-game/feedback-reliability-20261031/REVIEW.md。

本轮是功能反馈与可靠性修复，不声称真人好玩、平衡或UE完成。UE编译/运行仍NOT_RUN；纯核和静态契约不能证明UHT/UBT/Cook、最终画面、GPU或真人体验。

归档另经全量及两层增量按序导入、prerequisite核验、fsck及目标commit/tree比较。GitHub验证包括上传返回Git blob SHA、远端tree对象标识/大小及文本逐字读回；connector有UTF-8限制，不声称二进制完整下载复验。之后需另行使用认证开发环境完成常规源码同步，不要将归档分支推到main。
