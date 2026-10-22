# EShot

面向 Windows 与 Linux 的原生截图、标注、OCR、以图搜索、上传、GIF 与录屏工具。

[English](README.md) | **简体中文**

[![Build](https://github.com/ZombieCoke/EShot_youdao/actions/workflows/build.yml/badge.svg)](https://github.com/ZombieCoke/EShot_youdao/actions/workflows/build.yml)
[![Platforms](https://img.shields.io/badge/platform-Windows%2010%2F11%20%7C%20Linux-4b8bbe)](#平台支持)
[![Qt](https://img.shields.io/badge/Qt-6.x-41cd52)](https://www.qt.io/)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)

> [!NOTE]
> 本仓库是 [Benoks/EShot](https://github.com/Benoks/EShot) 的分支，维护在 [ZombieCoke/EShot_youdao](https://github.com/ZombieCoke/EShot_youdao)。与上游相比有三处不同：
>
> - **OCR 默认使用有道在线识别。** 一次请求同时返回识别行、逐行坐标和译文，结果窗口可以双语对照，也能把服务端排好版的译文图直接贴回选区。本地 Tesseract 仍保留为离线引擎。
> - **翻译模式有独立热键。** 绑定一次之后，按下即截图、直接翻译并把结果原位贴回，不经过 OCR 对话框。
> - **混合缩放多屏布局按物理像素重建**，修正了 KDE Wayland 下画布偏移、截图画面左移、贴图不在原位的问题。
>
> 本分支只产出 Linux 构建。Windows 用户请使用上游发布的安装包。

> [!IMPORTANT]
> EShot 支持 KDE Plasma 6 Wayland 与 GNOME Wayland。KDE Plasma 是主要的测试目标；GNOME 在日常使用中稳定，但真实环境下的测试覆盖面较小。其它桌面环境与合成器不受支持，通常无法正常工作。

> [!WARNING]
> **Linux AppImage：AppImage 本身就是安装器，赋予可执行权限后直接运行即可。**
> 不要把它交给 Gear Lever、AppImageLauncher、AppImageUpdate 或任何其它 AppImage 管理器与更新器。首次启动时，EShot 自带的开箱设置会安装可选系统软件包，把 EShot 装进应用菜单（`~/.local/opt/EShot`），并持续更新这份副本。交给管理器会多出一份不受 EShot 更新的拷贝，最终可能出现重复的菜单项、快捷方式和自启动项。如果已经被管理器接管，先从管理器中移除 EShot，再重新运行下载的 AppImage。Arch/CachyOS 用户建议优先使用 [AUR 包](#arch-linux-与-cachyos)。

EShot 把完整的截图流程收在一个紧凑的托盘应用里：框选区域、标注、复制或保存、提取文字、以图搜索、上传、置顶显示，或者录成 GIF/MP4。

## 截图

| Windows | Linux（CachyOS / KDE Plasma 6） |
| --- | --- |
| <img src="docs/images/eshot-windows.png" alt="EShot on Windows" width="760"> | <img src="docs/images/eshot-linux.png" alt="EShot on CachyOS with KDE Plasma 6" width="760"> |

## 平台支持

| 平台 | 状态 | 本分支的分发方式 |
| --- | --- | --- |
| Linux x86_64（KDE Plasma 6 Wayland） | 稳定，主要目标 | AppImage 与源码构建 |
| Linux x86_64（GNOME Wayland） | 稳定，测试面较小 | AppImage 与源码构建 |
| Windows 10/11 x64 与 ARM64 | 仅源码构建 | 请使用上游安装包 |
| 其它 Linux 桌面 | 不支持 | 通常无法正常工作 |

## 功能特性

- 区域与整屏捕获，支持多显示器与高 DPI，Windows 上还支持窗口选择
- 紧凑的选区浮层，动作与快捷键均可配置
- 画笔、箭头、直线、矩形、椭圆、文字、荧光笔、平滑模糊、马赛克、序号、橡皮擦、取色器
- 直接在画布上缩放和旋转已绘制的文字与图形
- 使用荧光笔时按住 `Shift` 可锁定到最近的坐标轴
- 撤销/重做、选区锁定、颜色控制、工具栏可见性可配置
- 默认使用有道在线 OCR，一次请求返回识别行、逐行坐标与译文，本地 Tesseract 作为离线引擎保留
- 对选区使用 Google Lens 或 Yandex Images 以图搜索
- 截图上传至 Catbox、Uguu、Litterbox、TmpFiles.org、temp.sh、Allwebs、Radikal Cloud、Google Drive、Yandex Disk
- 截图置顶显示
- 选区 GIF 录制
- MP4 录制，可配置帧率、质量、时长、桌面音频与麦克风音频
- Linux 与 Windows 上均可真实选择麦克风设备
- 全局截图热键，以及截图/GIF/录屏的直达热键
- 录制过程中的暂停、停止、取消快捷键可配置
- 设置导入导出与版本更新检查
- 已安装的 AppImage 支持自更新

## 截图流程

从托盘图标、`Print Screen`、自定义全局快捷键、直达动作快捷键或命令行发起截图。框选区域后，用浮层工具栏标注、OCR、搜索、上传、置顶、复制、保存或开始录制。

框选过程中双击某块屏幕即可捕获整屏。浮层会自适应狭窄空间与带缩放的混合多屏布局。

## 安装

本分支不发布 release。请从源码构建 AppImage（见 [从源码构建](#从源码构建)），产物是 `packages/EShot-v<版本>-x86_64.AppImage`。上游构建仍发布在 [Benoks/EShot releases](https://github.com/Benoks/EShot/releases/latest)。

> [!IMPORTANT]
> 本分支的 AppImage 在 Ubuntu 24.04 上构建，需要 **glibc 2.38 或更高版本**。Ubuntu 24.04/24.10/25.04 与较新的 Fedora 可以启动；Debian 12、Ubuntu 22.04 及更旧的发行版无法运行。

### Windows

本分支不产出 Windows 二进制。可从源码构建（见 [从源码构建](#从源码构建)），或直接使用上游安装包。

1. 下载上游的 `EShot_Setup_v<版本>_x64.exe` 或 ARM64 安装包。
2. 运行安装程序，按需勾选 FFmpeg/OCR 组件。
3. 从开始菜单或系统托盘启动 EShot。

安装程序默认只装给当前用户，不需要管理员权限。为所有用户安装是可选项，可能需要管理员账户；每个账户都可以在 EShot 设置里开启开机自启动。已有的全用户安装在被更新时仍保持该模式；如果想改成单用户安装，需要先卸载旧版本。对全用户安装执行卸载时，只能清除当时已加载的配置里的单用户自启动项。Windows 可能会阻止对以管理员身份运行的程序以及 UAC 安全桌面进行捕获或键鼠焦点操作。要捕获这类窗口，请在设置中开启 **以管理员身份启动**：经过一次管理员授权提示后，EShot 会以提权状态启动，之后不再询问。

每个 release 同时附带 x64 与 ARM64 的便携 ZIP。

### Linux：KDE Plasma 6 与 GNOME Wayland

KDE Plasma 6 Wayland 与 GNOME Wayland 是受支持的 Linux 目标。KDE Plasma 是主要测试环境。GNOME 在日常使用中稳定，只是真实环境的测试覆盖面仍然较小。与任何 Wayland 桌面一样，portal 版本会影响截图、快捷键、托盘集成与录制行为。

不要用 Gear Lever、AppImageLauncher 或其它 AppImage 管理器打开 AppImage；EShot 会自行完成集成与更新（见开头的警告）。

1. 下载 `EShot-v<版本>-x86_64.AppImage`。
2. 赋予可执行权限：

   ```bash
   chmod +x EShot-v*-x86_64.AppImage
   ```

3. 从终端启动：

   ```bash
   ./EShot-v*-x86_64.AppImage
   ```

   在 KDE Plasma 上，文件变为可执行后通常可以直接图形化启动。原版 GNOME Files 不会把 AppImage 注册为应用，双击可能只会弹出"选择打开方式"的提示而不是启动 EShot。在 GNOME 上，用上面两条命令首次启动最可靠。请避免使用 AppImageLauncher、Gear Lever 这类管理器；它们会移动文件，并跳过 EShot 自带的安装与更新流程。

4. 完成图形化的首次运行向导。它可以通过系统包管理器安装 FFmpeg/GStreamer、PipeWire portal 组件、Tesseract、所需 OCR 语言包，以及可选的菜单集成。
5. 用 **将 Print Screen 分配给 EShot** 来接管 `Print Screen`。KDE 会保留 Spectacle 的其它快捷键。GNOME 在可用时使用全局快捷键 portal，在较旧版本上则安装一个仅属于 EShot 的自定义快捷键。

#### AppImage 首次启动与集成

AppImage 是便携的。直接打开它并不会把 EShot 安装到系统全局。欢迎界面只在首次启动时出现；完成或跳过可选设置都会标记为已完成，之后不再出现。

勾选 **将 EShot 添加到应用菜单并安装快捷方式** 后，设置过程会把 AppImage 复制到 `~/.local/opt/EShot/EShot.AppImage`，创建应用菜单项，并在设置成功后从这份托管副本重启。此后请从应用菜单启动 EShot。下载目录里的原文件仍是一份独立的便携副本，菜单项可用之后可以删掉。如果不勾选该选项，则不会复制任何文件，你继续手动启动同一个 AppImage。

AppImage 内已经打包了 EShot 与 Qt。可选的媒体、OCR 与桌面集成软件包仍是系统包。跳过的依赖之后可以在 **设置 → 打开 Linux 依赖设置** 里补装。想要常规的应用菜单项和包管理器更新，原生包（例如 AUR 包）会更省心。

每个 release 还附带 `EShot-v<版本>-linux-x64.tar.gz`，适合偏好解包即用的用户。与 AppImage 不同，这个归档依赖发行版提供的兼容 Qt 与运行时库。

集成后的 AppImage 以当前用户身份存放在 `~/.local/opt/EShot` 下。EShot 会检查更新，并始终保留手动更新的入口。如果单用户 Windows 安装或集成后的 AppImage 落后至少两个稳定版本，EShot 会下载对应的 release 资源，校验 GitHub 提供的 SHA-256 摘要，静默更新并重启。全用户 Windows 安装的更新需要管理员权限。AUR 包（`eshot-bin`）与 `.tar.gz` 归档通过包管理器或重新下载来更新；EShot 对它们只提示新版本，不会自行替换。

### 关闭托管便携版的自动更新检查

从 4.7.0 起，在 EShot 可执行文件旁放一个名为 `.portable` 的空文件，即可关闭自动更新检查与静默更新。对 AppImage 来说，把该文件放在原始 `.AppImage` 文件旁边。这样可以让包管理器负责更新，EShot 不再在后台检查。手动检查更新仍然可用。删除该标记文件即可恢复正常更新行为。这个标记不会改变 EShot 保存设置的位置。

### Arch Linux 与 CachyOS

用 AUR 助手安装 EShot 维护的 AUR 二进制包：

```bash
yay -S eshot-bin
```

该包通过 pacman 安装 release 版 AppImage 及其桌面入口。它拉取的是 `Benoks/EShot` 上游的 AppImage，不是本分支的构建。请用常规的 AUR 助手更新，而不是 EShot 的 AppImage 更新器。

### Linux 卸载

使用 **设置 → 通用 → 从本机移除 EShot**，或者在桌面会话的终端里运行 `--uninstall`。两种方式都会先请求正在运行的实例退出，然后：

- 把 Print Screen 交还给桌面：GNOME 会恢复自己的截图界面快捷键，并移除 EShot 的自定义键位；在 KDE 上，EShot 的全局快捷键会被移除，如果 Print Screen 原本被 EShot 占用，Spectacle 会重新拿回它；
- 移除自启动项与 KWin 截图权限项；
- 对 AppImage 安装，还会移除集成副本（`~/.local/opt/EShot`）、应用菜单项以及 EShot 的图标。

```bash
# AppImage
~/.local/opt/EShot/EShot.AppImage --uninstall

# AUR：先清理，再移除软件包
eshot --uninstall
sudo pacman -R eshot-bin
```

对 AUR 包与 `.tar.gz` 归档，由包管理器或解压目录管理的文件不会被改动。`~/.config/EShot` 中的设置以及你保存的截图和录像都会保留；不需要时请自行删除。

### Linux 运行说明

- Wayland 下的截图与录制使用 XDG Desktop Portal 与 PipeWire。
- KDE 全局快捷键使用 KGlobalAccel。GNOME 在可用时使用全局快捷键 portal，并可以安装一个仅属于 EShot 的 GNOME 自定义快捷键作为兼容回退。
- 原版 GNOME 不提供传统托盘图标。在没有托盘集成时，从应用列表启动 EShot 会打开一个紧凑的控制窗口，包含截图、设置、关于和退出。自启动仍然静默。`Print Screen` 与自定义快捷键不依赖托盘，照常工作。
- GNOME 的截图通过其安全的截图 portal 传递。EShot 会在启动时预热该服务，但 portal 仍会给选择面板带来一小段延迟。这随 GNOME 与 portal 版本变化，在 Wayland 上属于预期行为。
- GNOME 与 KDE Wayland 使用 XWayland 选区浮层，使文字输入、焦点、单画布多屏框选、贴图与录制控件的位置表现一致。在 KDE 上，设置 `ESHOT_CAPTURE_BACKEND=wayland` 可以启用实验性的按输出原生表面，混合 DPI 下预览更清晰，但无法放置贴图与录制框。
- GIF 录制使用 GStreamer 做 portal 捕获，用 FFmpeg 做最终 GIF 编码。
- 启用音频时，MP4 录制需要 GStreamer 的 AAC 编码器。
- 录屏权限由桌面 portal 处理。当 portal 支持持久会话时，EShot 会为每块屏幕分别保存恢复令牌。
- 录制区域必须位于单块屏幕之内。如果 portal 弹出屏幕选择器，请选择包含该区域的那块屏幕。
- 可选依赖设置优先使用桌面提供的 PackageKit 会话安装器，之后回退到原生的 pacman、apt 或 dnf 流程。在 Fedora 上，已启用仓库不提供的软件包（例如 RPM Fusion 的包）会被跳过，而不是让整个安装失败。
- Fedora：设置过程从 Fedora 自己的仓库安装 `ffmpeg-free` 与 GStreamer 插件。MP4 录制需要 RPM Fusion `gstreamer1-plugins-ugly` 提供的 `x264enc`；请在运行设置前启用 [RPM Fusion](https://rpmfusion.org/Configuration)（free 与 nonfree），或之后再单独安装该包。

## 以图搜索

在设置中选择 **Google Lens** 或 **Yandex Images**。EShot 只把选区上传到临时公共图床，然后用这个临时链接打开所选的搜索服务。

不要对私密或敏感截图使用以图搜索。

## OCR

OCR 默认使用 **有道在线识别**。一次发往 `ocrtran.youdao.com` 的请求会返回每一行识别结果及其坐标，同时附带译文，因此结果窗口可以双语对照、分别复制，或者把服务端渲染好的译文图贴回选区。识别语言默认 `auto`，由服务端自行判断语种。

首次识别会把选区图片上传到 `ocrtran.youdao.com`。该接口不是有道官方公开接口，可能随时失效，所以 EShot 会在首次上传前询问一次并记住答案。选择拒绝会切回本地 Tesseract 并保存这个选择。

本地 **Tesseract** 仍作为离线引擎保留；在 OCR 对话框的引擎下拉里选择即可。首次运行向导可以安装英文、系统语言以及额外的语言数据。缺失的语言仍会显示但处于禁用状态，方便看出缺的是哪个依赖。

失败会按原样报告，不会静默降级到 Tesseract，因此你始终能知道图片有没有离开本机。

翻译模式有独立热键，默认不绑定。在 **设置 > 热键 > 动作热键** 中绑定后，可以一次完成截图与翻译，不打开 OCR 对话框。该热键不受"框选后自动翻译"开关影响，但仍遵守首次上传确认。

## 上传服务

匿名图床无需凭据。Google Drive 与 Yandex Disk 需要 OAuth 访问令牌；Allwebs 与 Radikal Cloud 需要服务 API 密钥。OAuth 令牌保存在操作系统的凭据存储中，绝不会写入 EShot 的明文设置文件。

### Google Drive 令牌配置

1. 打开 [Google OAuth 2.0 Playground](https://developers.google.com/oauthplayground)。
2. 选择 Drive API v3 作用域：

   ```text
   https://www.googleapis.com/auth/drive.file
   ```

3. 授权该作用域并把授权码换成令牌。
4. 复制 `access_token`，或者复制完整的 JSON 响应。
5. 粘贴到 EShot 的 Google Drive 令牌输入框。

OAuth Playground 签发的访问令牌会过期。如果之后上传返回 HTTP 401，重新生成一个令牌即可。

## 默认快捷键

大部分快捷键都可以在设置中修改。

| 快捷键 | 动作 |
| --- | --- |
| `Print Screen` | 开始区域截图 |
| 双击某块屏幕 | 捕获整屏 |
| `Enter` / `Ctrl+C` | 复制选区 |
| `Ctrl+S` | 保存选区 |
| `Esc` | 取消或关闭 |
| `P` | 画笔 |
| `A` | 箭头 |
| `L` | 直线 |
| `R` | 矩形 |
| `C` | 椭圆 |
| `T` | 文字 |
| `H` | 荧光笔 |
| `B` | 模糊 |
| `N` | 序号 |
| `X` | 橡皮擦 |
| `D` | 半透明矩形 |
| `I` | 取色器 |
| `Ctrl+Z` / `Ctrl+Shift+Z` | 撤销 / 重做 |

## 命令行

```powershell
# Windows
EShot.exe --capture
EShot.exe --save "C:\path\to\capture.png"
EShot.exe --silent
```

```bash
# Linux
EShot --capture
EShot --save "$HOME/Pictures/capture.png"
EShot --silent
EShot --quit
EShot --uninstall  # 交还 Print Screen 并移除桌面集成

# 原生包同时安装小写的启动器：
eshot --capture
```

## 问题与反馈

- **使用提问与求助：** 到 [Discussions → Q&A](https://github.com/Benoks/EShot/discussions/categories/q-a) 提问。
- **想法与功能请求：** 发到 [Discussions → Ideas](https://github.com/Benoks/EShot/discussions/categories/ideas)，其他人可以投票。
- **缺陷：** 提交 [issue](https://github.com/Benoks/EShot/issues/new)。请附上 EShot 版本、操作系统，Linux 上还要说明桌面环境（KDE/GNOME）与会话类型（Wayland/X11）。

## 从源码构建

核心依赖：

- CMake 3.16+
- C++17 编译器
- Qt 6 的 Core、GUI、Widgets、Network，Linux 上还需要 DBus

### Linux

仓库自带 Ubuntu/Debian 与 CachyOS/Arch 的依赖安装脚本：

```bash
./scripts/linux/install-ubuntu-deps.sh
# 或者
./scripts/linux/install-cachyos-deps.sh

./scripts/linux/check-linux-runtime.sh
./scripts/linux/build-linux.sh
./dist-linux/bin/EShot
```

构建 AppImage：

```bash
./scripts/linux/build-appimage.sh
```

脚本产出的文件是 `packages/EShot-v<版本>-x86_64.AppImage`。它会显式把 `libssl.so.3` 装进包内，因为 Qt 在运行时才加载 OpenSSL 后端，而 `linuxdeploy --plugin qt` 不会自动带上它；缺少这个库时所有 HTTPS 请求都会以 `TLS initialization failed` 失败。

运行测试（需要显示器，无头环境请用 xvfb 包一层）：

```bash
ctest --test-dir build-linux --output-on-failure
xvfb-run -a ctest --test-dir build-linux --output-on-failure
```

### Windows

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
```

用 Inno Setup 6 构建安装包：

```powershell
iscc EShot_Setup.iss
```

## 仓库内容

本仓库只跟踪源码、测试、打包脚本，以及随版本一起发布的文档。其余内容都留在开发者本机，并写在 `.gitignore` 中：

| 类别 | 路径 |
| --- | --- |
| 中间产物 | `build*/`、`EShot.AppDir/`、`packages/`、`dist/`、`installer_output/`、`*.AppImage` |
| 计划说明 | `docs/specs/`、`docs/plans/` |
| 项目目标与路线图草稿 | `docs/goals/`、`GOALS.md`、`ROADMAP.md` |

不跟踪它们意味着新克隆下来的仓库里不会残留与代码脱节的产物，草稿也可以随时重写而不必污染提交历史。

## 第三方组件

EShot 使用 Qt、Tesseract OCR、FFmpeg、GStreamer、PipeWire、XDG Desktop Portal 与 Inno Setup。授权细节见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

## 许可证

EShot 以 [MIT License](LICENSE) 发布。
