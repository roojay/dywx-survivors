# 概念图 UI 资源与参考交接检查点

**仅传输资源/参考源码增量，不是UI实现完成、EXE或Release。不要将此归档分支直接合并到main。**

目标源码 commit `3c71b20705e212954cd8e0c27ada2a06a62fa325`，tree `c3672aeac364127ce1345e6a36c913f178fb20b0`。基线为正常源码 `55e5fcb021da7bfa9490d55778e92d46cf650618`，不依赖旧fullbundle链。包含12个干净UI分层资源、5张已选概念参考图、原始提示词/清单/实施计划及PNG结构和哈希校验。所有已提交资源保留；未包含新的UI/玩法实现代码，仅新增资源测试并接入聚合测试。

## 独立目录恢复

每一步成功后才继续，新目录不得已存在；Windows将python3替换为py -3。

```sh
git clone --single-branch --branch checkpoint/20261031-concept-ui-3c71b20 https://github.com/roojay/dywx-survivors.git dywx-concept-transfer
cd dywx-concept-transfer
python3 restore.py
git clone --branch main https://github.com/roojay/dywx-survivors.git ../dywx-concept-restored
git -C ../dywx-concept-restored cat-file -t 55e5fcb021da7bfa9490d55778e92d46cf650618
git -C ../dywx-concept-restored bundle verify ../dywx-concept-transfer/art-delta.bundle
git -C ../dywx-concept-restored fetch ../dywx-concept-transfer/art-delta.bundle refs/heads/feat/playable-mvp:refs/heads/import-concept-ui-3c71b20
git -C ../dywx-concept-restored switch import-concept-ui-3c71b20
git -C ../dywx-concept-restored fsck --full
git -C ../dywx-concept-restored rev-parse HEAD HEAD^{tree}
```

最后两个输出必须匹配上面目标commit/tree。restore.py只验证/拼接固定文件名，不运行shell/Git，不覆盖不同已有bundle。校验失败立即停止。Windows负责人也可先确认现有源码工作区安全，再fetch到全新命名分支；不强制覆盖任何现有分支或工作区。本归档不会自动合并、push或更新main。

## Windows后续与验证边界

资源在 Art/UIConcepts、Content/UI/Images/concept-*；设计说明在 docs/design/aow4-inspired-20261031。请先阅读清单、alpha/crop说明与实施计划，再按五张概念图实际实现UI。12项资源已由生产者/主任务解码和视觉核查，PNG结构/hash检查通过；55e基线暂停修复普通/ASan验证通过。这些不证明UE新UI已完成或画面已验收；仍须Windows完成代码、UE编译及运行截图验证。

归档另经独立空仓导入base、bundle verify/fetch/fsck、目标commit/tree验证，分块SHA256/Git SHA及总SHA记录于manifest。远端验证对象SHA/大小/tree/ref和文本逐字读回；connector有UTF8限制，不宣称完整二进制远端下载复验。main由Windows在实际实现验证后正常同步，不由本传输任务修改。
