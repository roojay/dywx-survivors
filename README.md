# 完整源码 + 增量恢复检查点

**一次性源码灾备，不是 EXE/Release，不代表 main 已同步。不要把这个归档分支合并到 main。**

本分支复用已验证的 ff3452e 全量源码 bundle 的全部20块及恢复脚本，仅增加 `delta.bundle`、`delta-manifest.json` 和本说明。全量与增量一起可以恢复到 `803e9d19772d87290842aadf2aea791bba673e7a`（tree `51ff5aa217931196f9f6f237bdfe3491c11187cf`）；增量不能单独使用。所有资源保留。

## 在全新目录恢复

将 `checkpoint/20261031-design-closure-803e9d1` 替换为此检查点实际分支；Windows 可使用 `py -3` 代替 `python3`。

```sh
git clone --single-branch --branch checkpoint/20261031-design-closure-803e9d1 https://github.com/roojay/dywx-survivors.git dywx-checkpoint
cd dywx-checkpoint
python3 restore.py
python3 -c "import json,pathlib,hashlib; m=json.loads(pathlib.Path('delta-manifest.json').read_text()); b=pathlib.Path('delta.bundle').read_bytes(); ok=len(b)==m['bytes'] and hashlib.sha256(b).hexdigest()==m['sha256']; print('delta SHA256 matches:',ok); raise SystemExit(0 if ok else 1)"
git bundle verify source.bundle
git clone --branch feat/playable-mvp source.bundle ../dywx-restored
git -C ../dywx-restored bundle verify ../dywx-checkpoint/delta.bundle
git -C ../dywx-restored fetch ../dywx-checkpoint/delta.bundle refs/heads/feat/playable-mvp:refs/heads/recovered-803e9d1
git -C ../dywx-restored switch recovered-803e9d1
git -C ../dywx-restored fsck --full
git -C ../dywx-restored rev-parse HEAD HEAD^{tree}
```

每一步成功后再执行下一步；如果 SHA 校验失败立即停止。新目录不得已存在。最后输出应匹配上述目标 commit/tree。增量通过不带 `+` 的 fetch 写入新命名分支，不强制覆盖现有分支；恢复脚本也不会覆盖不同已有bundle。不会自动推送或操作任何现有开发仓库。

## 测试边界

完整 test-full-game-all.sh --sanitize 退出0，新增三项 regression 普通/ASan/UBSan 通过；默认10例与新号8例 natural 的普通/sanitized/CMake CSV及trace一致。测试摘要在恢复后的源码 docs/full-game/design-closure-20261031/REVIEW.md。仅说明本轮功能修复与这些测试，不能宣称平衡或真人体验达标；UE 编译/运行仍为 NOT_RUN，LSan 未启用。

归档额外核验：全量SHA、所有增量 prerequisites 在已归档base祖先链、空仓先导入全量再verify/fetch增量、fsck、目标commit/tree一致。GitHub验证包含blob上传返回Git SHA、本地SHA256、远端tree/ref与大小；connector不支持二进制完整下载读回，不能宣称逐字节远端下载验证。源文件、字体及原history均保留。

源码审核、UE编译与运行验证后，另行通过已认证开发环境执行正常Git同步；不要将归档分支本身推至main。
