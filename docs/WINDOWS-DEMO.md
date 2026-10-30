# Windows 界面演示包

2026-10-30 在 win-dev 使用已安装的 Unreal Engine 5.8.4（CL 58902052）完成真实 UHT/UBT、Cook、Stage 和 Archive。配置为 Win64 Development。

## 运行

1. 将整个 ZIP 解压到本地目录，例如 `D:\Games\DywxUI`。保留 `Windows` 内所有文件和子目录。
2. 双击 `Windows\DywxSurvivors.exe`。运行演示包不需要安装 Unreal Editor 或 Visual Studio。
3. 若系统提示缺少 VC++ 运行库，可由你手动运行随包的 `Windows\Engine\Extras\Redist\en-us\vc_redist.x64.exe`。构建过程中没有在你的系统安装该运行库。
4. 需要窗口模式时，可在 `Windows` 目录执行：

```powershell
.\DywxSurvivors.exe -windowed -ResX=1280 -ResY=720
```

不要只复制根目录的 EXE。它是启动入口；实际游戏 EXE、DLL、Content、PAK/IoStore 和配置均在同一个完整目录中。需要约 640 MiB 解压空间，另留少量运行日志空间。

## 可体验内容

开始修行 → 选择断岳行者 → 选择栖雷古坛及快速/传统模式 → HUD 预览。鼠标点击，Tab 切换焦点，Enter 执行页面主操作，Esc 返回或打开暂停菜单。主菜单提供道典、字号设置和退出确认。

战斗是显式标注的静态 HUD 示例，没有角色移动、敌人、自动攻击、存档或完整游戏逻辑。其他角色可看构想，后续地图和未解锁无尽不能开局。

## 构建与验证

- 引擎：UE 5.8.4，安装路径 `D:\epic\UE_5.8`。
- 编译器：UBT 实际选择 MSVC 14.50.35725，工具目录 `14.50.35717`；Windows SDK 10.0.26100.0。
- Editor/Game UHT、UBT 编译与链接通过；AutomationTool `ExitCode=0 (Success)`。
- Cook 报告 `Success - 0 error(s), 0 warning(s)`；Stage 和 Archive 完成。
- 修复 `RequestRun` 的参数遮蔽编译错误，不改变交互逻辑。
- 引擎仍给出字体构造器 C4996 弃用警告，当前 UE 5.8.4 构建通过。
- 原始 Archive：58 文件、669,210,292 字节、0 PDB；15 个 UI 文件与源码哈希一致。ZIP 另附本说明、源码版本信息和文件校验清单。
- [运行文件清单与 SHA256](windows-build/package-manifest.json)
- [源码发布核对](windows-build/source-publication-verification.json)
- [构建状态及日志证据](windows-build/build-summary.json)
- [构建日志摘录](windows-build/build-evidence.txt)

本轮没有启动打包游戏或交互编辑器做验收。独立启动、真实字体绘制、四屏视觉、焦点、DPI/分辨率、玩法和性能仍未验证；成功编译/打包不能视为这些项目通过。

## 复现

在现有且完整的 UE 5.8/MSVC/Windows SDK 环境中：

```powershell
.\scripts\package-windows.ps1 `
  -EngineRoot 'D:\epic\UE_5.8' `
  -ArchiveDirectory 'D:\Builds\DywxUI\package' `
  -EvidenceDirectory 'D:\Builds\DywxUI\logs'
```

脚本不会安装工具链、接受许可或上传任何文件。`EvidenceDirectory\UAT` 专供 AutomationTool 使用，工具会清理该日志子目录；不要指向存放其他资料的目录。构建日志和 PDB 保留在构建工作区，用户 ZIP 不包含 PDB。
