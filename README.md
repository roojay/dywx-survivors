# 美术重建源码传输检查点

**此分支仅传输源码增量，不是游戏源码main、EXE或Release。不要将本归档分支直接合并到main。**

目标源码 commit `1917c24d3776a4a927b02395d81b5ef98fe23e4f`，tree `3db08acbd5e9ce272fb1a16a02c9cdd42a4a9f1c`。
所需正常源码基线为 `6a71cd5327e49f4fd24b80f70b458e13e62a1699`，不依赖旧fullbundle或旧增量归档链。全部已提交资源（包含Blender、scene JSON与生成PNG）保留。manifest包含总SHA256、大小、每块SHA256/Git blob SHA以及prerequisites。

## 在独立目录恢复与验证

以下新目录不得已存在。每条命令成功后再继续；Windows可将python3换为py -3。

```sh
git clone --single-branch --branch checkpoint/20261031-art-1917c24 https://github.com/roojay/dywx-survivors.git dywx-art-transfer
cd dywx-art-transfer
python3 restore.py
git clone --branch main https://github.com/roojay/dywx-survivors.git ../dywx-art-restored
git -C ../dywx-art-restored cat-file -t 6a71cd5327e49f4fd24b80f70b458e13e62a1699
git -C ../dywx-art-restored bundle verify ../dywx-art-transfer/art-delta.bundle
git -C ../dywx-art-restored fetch ../dywx-art-transfer/art-delta.bundle refs/heads/feat/playable-mvp:refs/heads/import-art-1917c24
git -C ../dywx-art-restored switch import-art-1917c24
git -C ../dywx-art-restored fsck --full
git -C ../dywx-art-restored rev-parse HEAD HEAD^{tree}
```

最后两个输出必须等于上面目标commit/tree。restore.py只验证/拼接固定名称文件，不运行Git/shell，不覆盖不同已有bundle。如果中断留下半成品，检查后手工移走再重试。校验失败立即停止。

Windows已有正常源码仓也可由负责人先确认工作区与分支安全，再将bundle fetch到全新import分支；不要强制覆盖现有工作区。然后按正常流程合并Windows另行进行的Trace改动，解决冲突、编译/运行测试，再由Windows正常推送main。本归档不会执行任何自动合并、push或force更新。

## 测试边界

冻结的 test-full-game-all.sh 普通模式退出0；geometry/determinism/bounds及HUD76项检查通过。说明见恢复源码 docs/art/reference-rebuild-20261031/REVIEW.md。实际美术UE编译仍NOT_RUN，需Windows导入后确认；不能据纯核/静态检查宣称最终画面、GPU性能或美术验收已完成。

归档额外核验：独立空仓先导入正常源码base，再verify/fetch增量，fsck、目标commit/tree一致。远端核验对象SHA/大小/tree/ref及文本逐字读回；受connector UTF8限制，不宣称完整二进制下载复验。main不由本传输任务修改。
