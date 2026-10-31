# 证据索引与范围

检视日期：2026-10-31。以下 ID 为本报告本地编号，**不是 REA 工具返回的 Evidence ID**。菜单图片已逐张查看；局内结论由协作研究者的截图观察汇总。截图日期、文章日期和软件版本分别记录，不互相替代。未进行新增联网取证。

## 菜单证据

| ID | 屏幕／本地来源 | 原始来源、日期／版本 | 可支持与限制 |
|---|---|---|---|
| M1 | 主菜单；`../../aow4_refs/main_menu_2025.jpg` | [Paradox 论坛原图](https://forumcontent.paradoxplaza.com/public/1347819/20250825212143_1.jpg)；文件名日期 2025-08-25；图中 v1.011.002.111115，1920×1080 | 支持纵向导航、人物与场景占比；非当前全部 DLC／主菜单变体证明。 |
| M2 | 领域选择；`../../aow4_refs/realm_selection_official.jpg` | [官方开发日志 4](https://www.paradoxinteractive.com/games/age-of-wonders-4/news/age-of-wonders-4-dev-diary-4)，2023-02-09；构建号未知 | 支持左右分区、选中项及可见说明；发行前素材不保证现版相同。 |
| M3 | 领域设置；`../../aow4_refs/realm_setup.png` | [Gamepressure：Creating maps](https://www.gamepressure.com/age-of-wonders-4/creating-maps/z210b78)，2023-05-05；构建号未知 | 支持窄设置栏及底部进阶／下一步入口；未打开进阶设置。 |
| M4 | 派系选择；`../../aow4_refs/faction_selection.jpg` | [Gamepressure：Creating factions](https://www.gamepressure.com/age-of-wonders-4/creating-factions/za10c3c)，2023-05-30；构建号未知 | 支持列表、编辑／创建入口和角色预览；未验证动作。 |
| M5 | 种族／特质；`../../aow4_refs/race.jpg` | [Gamepressure：Races](https://www.gamepressure.com/age-of-wonders-4/races/za10baf)；构建号未知 | 支持形态／模型／特质三分区；不据此声称现版选项完整。 |
| M6 | 外观；`../../aow4_refs/appearance.jpg` | [Gamepressure：Creating factions](https://www.gamepressure.com/age-of-wonders-4/creating-factions/za10c3c)，2023-05-30；构建号未知 | 支持步骤条、滑杆与预览；未操作旋转、缩放或滑杆。 |
| M7 | 最终确认；`../../aow4_refs/finalize.jpg` | 同 M6；构建号未知 | 支持右侧命名／简介表单；不代表整个流程已逐步走通。 |

## 局内证据

- **W1 世界地图**：[官方 Steam 商店](https://store.steampowered.com/app/1669000/Age_of_Wonders_4/)的[截图 0](https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/1669000/5fdcf024343a66fa879c17c6de096bdd79145aa5/ss_5fdcf024343a66fa879c17c6de096bdd79145aa5.1920x1080.jpg?t=1791308733)。本地 `/tmp/aow4_research/steam0.jpg`，链接来自保留的 `steam.json`。构建号、截图拍摄日期未知。支持薄资源栏、左上任务与右下行动提示，不证明运行时响应。
- **T1 战术 HUD**：[评测页](https://www.inkl.com/news/age-of-wonders-4-review)及[原图](https://cdn.mos.cms.futurecdn.net/w3LCpS8PWyUrEbinXeLWEE.jpg)。本地 `/tmp/aow4_research/combat_review.jpg`。这是媒体截图，**非官方来源**；构建号未知。支持底部单位／能力栏、右侧统帅及可见向上技能说明，不证明悬停或键盘触发机制。
- **E1 事件／说明**：[官方开发日志 6](https://www.paradoxinteractive.com/games/age-of-wonders-4/news/age-of-wonders-4-dev-diary-6)，2023-02-23；本地 `/tmp/aow4_research/event.webp` 与 `tooltip.webp`。构建号未知，发行前素材。支持左文右人、纵向选项与嵌套说明；**不支持本项目“暂停顿悟”机制已经存在于 AoW4**。

## REA 方法来源

已读取官方仓库主分支，tree SHA `23de10bb2849cdce8ddbe70504aa7cc57de75a98`，skill metadata version 58：

1. [SKILL.md](https://github.com/morluto/rea/blob/23de10bb2849cdce8ddbe70504aa7cc57de75a98/.agents/skills/reverse-engineer-anything/SKILL.md)：问题清单、最小概览、分离观察／推断／未知、保留检索边界与未决问题。
2. [Evidence workflows](https://github.com/morluto/rea/blob/23de10bb2849cdce8ddbe70504aa7cc57de75a98/.agents/skills/reverse-engineer-anything/references/evidence-workflows.md)：保留来源、覆盖与限制；有限验证不能外推全局等价。
3. [Runtime observation](https://github.com/morluto/rea/blob/23de10bb2849cdce8ddbe70504aa7cc57de75a98/.agents/skills/reverse-engineer-anything/references/runtime-observation.md)：区分被动观察与实际执行动作。
4. [Native desktop observation](https://github.com/morluto/rea/blob/23de10bb2849cdce8ddbe70504aa7cc57de75a98/docs/native-investigation.md#native-desktop-observation)：原生 UI 工具需 macOS 14+、指定窗口及权限；浏览器 CDP 工具也不是 Windows 原生游戏控制器。

这里只应用证据组织方法，未安装、注册或调用 REA，未声称实时逆向或点击验证。设计取舍对应 `UI-ANALYSIS.md`；生成提示见 `PROMPTS.md`，五图文件身份见 `manifest.json`。资料覆盖为局部，不构成全游戏审计。
