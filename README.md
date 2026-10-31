# 一次性可恢复源码检查点 / Recoverable source checkpoint

**这是 GitHub 源码灾备，不是 EXE、Release 或正常源码分支；main 尚未同步。不要将此归档分支合并到 main。**

完整源码及所有已提交资源、字体和 feat/playable-mvp 可达历史，封装在自包含 Git bundle 中；没有删除或替换游戏资源。源码目标 commit `ff3452e1f697a43c434844e284dd864eea404395`，tree `388fcac0c0f86996c1b8f38abb58c6ae73f5be41`。manifest.json 记录总大小、SHA256、每块 SHA256 和 Git blob SHA。此方式仅用于一次性恢复，不作为常规开发流程。

Standalone sanitized gate、natural normal/sanitized、CMake 通过（执行记录由主任务提供）；归档另经过空仓导入、git fsck、commit/tree 对比。**UE 尚未编译，未证明 Windows EXE/GPU 性能。**

## 恢复（Git + Python 3，无额外 Python 依赖）

使用全新的目录，避免影响任何现有开发仓库。在 Windows 可将 python3 改为 py -3。

```sh
git clone --single-branch --branch checkpoint/20261031-playtest-ff3452e https://github.com/roojay/dywx-survivors.git dywx-checkpoint
cd dywx-checkpoint
python3 restore.py
git bundle verify source.bundle
git clone --branch feat/playable-mvp source.bundle ../dywx-restored
git -C ../dywx-restored fsck --full
git -C ../dywx-restored rev-parse HEAD HEAD^{tree}
```

最后两项应为上面的目标 commit/tree。restore.py 只检查/拼接固定名称文件，不运行 shell/Git，不修改用户仓库，不覆盖不同的已有 source.bundle；如果中途失败留下不完整文件，检查后手工移走再重试。不要以 python -O 运行（校验使用 assert）。

恢复之后先审核源码、编译和测试，再手动添加 GitHub remote、获取最新 main，确认兼容且为 fast-forward 后做正常推送。**此脚本不自动推送、不强制更新、不改变 main。** 不要将归档分支本身推到 main。
