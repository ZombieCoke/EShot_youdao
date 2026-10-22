# EShot

Native screenshot, annotation, OCR, visual-search, upload, GIF, and video capture for Windows and Linux.

**English** | [简体中文](README.zh-CN.md)

[![Build](https://github.com/ZombieCoke/EShot_youdao/actions/workflows/build.yml/badge.svg)](https://github.com/ZombieCoke/EShot_youdao/actions/workflows/build.yml)
[![Platforms](https://img.shields.io/badge/platform-Windows%2010%2F11%20%7C%20Linux-4b8bbe)](#platform-support)
[![Qt](https://img.shields.io/badge/Qt-6.x-41cd52)](https://www.qt.io/)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)

> [!NOTE]
> This is a fork of [Benoks/EShot](https://github.com/Benoks/EShot), maintained at [ZombieCoke/EShot_youdao](https://github.com/ZombieCoke/EShot_youdao). Compared with upstream it differs in three places:
>
> - **OCR runs on Youdao online recognition by default.** One request returns the recognized lines, their bounding boxes and the translated text, so the result dialog can show both languages and paste the server-rendered translation back over the selection. Local Tesseract stays available as an offline engine.
> - **Translation mode has its own hotkey.** Bind it once and a capture goes straight to translation and pins the result in place, without opening the OCR dialog.
> - **Mixed-DPI multi-monitor layouts are rebuilt from physical pixels** on KDE Wayland, fixing a canvas offset that shifted the capture and misplaced pinned images.
>
> Builds are produced for Linux only. Windows users should use the upstream releases.

> [!IMPORTANT]
> EShot supports KDE Plasma 6 Wayland and GNOME Wayland. KDE Plasma is the primary tested Linux target, while GNOME support is stable for normal use but has received less broad real-world testing. Other desktop environments and compositors remain unsupported and are unlikely to work correctly.

> [!WARNING]
> **Linux AppImage: the AppImage is the installer. Just make it executable and run it.**
> Do **not** add it to Gear Lever, AppImageLauncher, AppImageUpdate or any other AppImage manager or updater. On first launch EShot's own setup installs the optional system packages, adds EShot to the application menu (`~/.local/opt/EShot`) and keeps that copy updated. A manager makes a second copy that does not get EShot's updates and can end up with duplicate menu entries, shortcuts and autostart. If a manager already took it, remove EShot from the manager and run the downloaded AppImage again. On Arch/CachyOS, prefer the [AUR package](#arch-linux-and-cachyos).

EShot keeps the complete screenshot workflow in one compact tray application: select a region, annotate it, copy or save it, extract text, search the image, upload it, pin it above other windows, or record it as GIF/MP4.

## Screenshots

| Windows | Linux (CachyOS / KDE Plasma 6) |
| --- | --- |
| <img src="docs/images/eshot-windows.png" alt="EShot on Windows" width="760"> | <img src="docs/images/eshot-linux.png" alt="EShot on CachyOS with KDE Plasma 6" width="760"> |

## Platform support

| Platform | Status | Distribution in this fork |
| --- | --- | --- |
| Linux x86_64 (KDE Plasma 6 Wayland) | Stable, primary target | AppImage and source build |
| Linux x86_64 (GNOME Wayland) | Stable, less broadly tested | AppImage and source build |
| Windows 10/11 x64 and ARM64 | Source builds only | Use the upstream installers |
| Other Linux desktops | Unsupported | Unlikely to work correctly |

## Features

- Region and monitor capture with multi-monitor and high-DPI handling, plus window selection capture on Windows
- Compact selection overlay with configurable actions and shortcuts
- Pen, arrow, line, rectangle, ellipse, text, highlighter, smooth blur, pixelate, counter, eraser, and eyedropper tools
- Resize and rotate selected text and shapes directly on the canvas
- Hold `Shift` while drawing with the highlighter to lock it to the nearest axis
- Undo/redo, selection locking, color controls, and configurable toolbar visibility
- Youdao online OCR by default, returning recognized lines, per-line boxes and the translation in one request, with local Tesseract available as an offline engine
- Google Lens or Yandex Images visual search for the selected region
- Screenshot uploads to Catbox, Uguu, Litterbox, TmpFiles.org, temp.sh, Allwebs, Radikal Cloud, Google Drive, and Yandex Disk
- Always-on-top pinned captures
- Selected-region GIF recording
- MP4 recording with configurable FPS, quality, duration, desktop audio, and microphone audio
- Real microphone-device selection on Linux and Windows
- Global capture and direct screenshot/GIF/video hotkeys
- Configurable pause, stop, and cancel recording shortcuts
- Settings import/export and automatic release checks
- Self-update support for installed AppImages

## Capture workflow

Start a capture from the tray icon, `Print Screen`, a custom global shortcut, a direct-action shortcut, or the command line. Select a region and use the overlay toolbar to annotate, OCR, search, upload, pin, copy, save, or begin recording.

Double-clicking a screen during selection captures that complete monitor. The overlay adapts to tight spaces and scaled multi-monitor layouts.

## Install

This fork does not publish releases. Build the AppImage from source (see [Build from source](#build-from-source)); the output is `packages/EShot-v<version>-x86_64.AppImage`. Upstream builds stay at [Benoks/EShot releases](https://github.com/Benoks/EShot/releases/latest).

> [!IMPORTANT]
> The AppImage built from this fork is produced on Ubuntu 24.04 and needs **glibc 2.38 or newer**. It starts on Ubuntu 24.04/24.10/25.04 and current Fedora. Debian 12, Ubuntu 22.04 and older distributions cannot run it.

### Windows

This fork ships no Windows binaries. Build them from source (see [Build from source](#build-from-source)) or use the upstream installer.

1. Download the upstream `EShot_Setup_v<version>_x64.exe` or the ARM64 installer.
2. Run the installer and choose the optional FFmpeg/OCR components you need.
3. Launch EShot from the Start menu or system tray.

The installer defaults to the current user and does not require administrator permission. An all-users install is optional and may require an administrator account; each account can enable auto-start from EShot Settings. Existing all-users installations stay in that mode when updated; uninstall the old installation first if you want to switch to a per-user install. For all-users installs, uninstall can remove per-user auto-start entries only from profiles loaded at that time. Windows may restrict capture or keyboard focus over applications running as administrator and the UAC secure desktop. To capture them, turn on **Start as administrator** in Settings: after one administrator prompt, EShot starts elevated without further prompts.

Portable x64 and ARM64 ZIP archives are also attached to each release.

### Linux: KDE Plasma 6 and GNOME Wayland

KDE Plasma 6 Wayland and GNOME Wayland are supported Linux targets. KDE Plasma is the primary tested environment. GNOME is stable for normal use, although its wider real-world testing coverage is still smaller. As with any Wayland desktop, portal versions can affect capture, shortcuts, tray integration, and recording behavior.

Do not open the AppImage with Gear Lever, AppImageLauncher or another AppImage manager; EShot integrates and updates itself (see the warning at the top).

1. Download `EShot-v<version>-x86_64.AppImage`.
2. Mark it executable:

   ```bash
   chmod +x EShot-v*-x86_64.AppImage
   ```

3. Start it from a terminal:

   ```bash
   ./EShot-v*-x86_64.AppImage
   ```

   KDE Plasma commonly offers a graphical launch flow after the file is executable. Stock GNOME Files does not register AppImages as applications, so double-clicking can show an "open with" prompt instead of starting EShot. Running the two commands above is the reliable first-launch method on GNOME. Avoid AppImage managers such as AppImageLauncher or Gear Lever for EShot; they move the file and skip EShot's own setup and updates.

4. Complete the graphical first-run wizard. It can install FFmpeg/GStreamer, PipeWire portal components, Tesseract, selected OCR languages, and optional application-menu integration through the system package manager.
5. Use **Use Print Screen for EShot** to assign `Print Screen`. KDE keeps Spectacle's other shortcuts. GNOME uses the Global Shortcuts portal when available and an EShot-only custom shortcut on older GNOME releases.

#### AppImage first launch and integration

The AppImage is portable. Opening it does not install EShot system-wide. The Welcome screen appears only on the first launch; completing or skipping the optional setup marks it complete, so it does not return on later launches.

When **Add EShot to the application menu and install shortcuts** is selected, the setup copies the AppImage to `~/.local/opt/EShot/EShot.AppImage`, creates an application-menu entry, and restarts from that managed copy after successful setup. Launch EShot from the application menu after that point. The original file in Downloads remains a separate portable copy and can be removed once the menu entry works. If the option is not selected, nothing is copied and you continue launching the same AppImage manually.

The AppImage bundles EShot and Qt. Optional media, OCR, and desktop-integration packages remain system packages. Skipped dependencies can be installed later from **Settings → Open Linux dependency setup**. A native package is the smoother option for users who want an ordinary application-menu entry and package-manager updates; on Arch that means building this fork locally, because the AUR packages track upstream.

Each release also includes `EShot-v<version>-linux-x64.tar.gz` for users who prefer an unpacked build. Unlike the AppImage, this archive relies on compatible Qt and runtime libraries installed by the Linux distribution.

Integrated AppImages are stored for the current user under `~/.local/opt/EShot`. EShot checks for updates and always keeps manual updating available. If a per-user Windows installation or integrated AppImage is at least two stable releases behind, EShot downloads the matching release asset, verifies its GitHub SHA-256 digest, updates silently, and restarts. All-users Windows installations require an administrator for updates. Upstream's AUR package (`eshot-bin`) and the `.tar.gz` archive are updated through the package manager or a new download; EShot only reports new releases there and does not replace them itself.

### Disable automatic update checks for managed portable installs

Starting with 4.7.0, place an empty file named `.portable` beside the EShot
executable to disable automatic update checks and silent updates. For an
AppImage, place the file beside the original `.AppImage` file. This lets a
package manager handle updates without EShot checking in the background.
Manual update checks remain available. Removing the marker restores the normal
update behavior. The marker does not change where EShot stores settings.

### Arch Linux and CachyOS

The AUR packages `eshot-bin` and `eshot-git` follow **upstream Benoks/EShot**, not this fork. Installing them gives you a build without the Youdao OCR engine, the dedicated translate hotkey, and the mixed-DPI capture fix described above, so treat them as plain upstream EShot.

To run this fork on Arch or CachyOS, pick one of these.

**AppImage built from this repository.** Mounting it needs FUSE 2:

```bash
sudo pacman -S --needed fuse2
chmod +x EShot-v4.7.0-x86_64.AppImage
./EShot-v4.7.0-x86_64.AppImage
```

Without FUSE, run it unpacked instead:

```bash
./EShot-v4.7.0-x86_64.AppImage --appimage-extract-and-run
```

**Build from source.** The dependency script installs the toolchain and the runtime packages:

```bash
bash scripts/linux/install-cachyos-deps.sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/bin/EShot
```

That covers `base-devel`, `cmake`, `ninja`, `qt6-base`, `qt6-wayland`, `libx11`, `libxkbcommon`, `libsecret`, FFmpeg, Tesseract, and the PipeWire/GStreamer stack. `libsecret` matters at configure time: the build requires its headers, and the same package provides them.

### Uninstall on Linux

Use **Settings → General → Remove EShot from this system**, or run `--uninstall` from a terminal in your desktop session. Either way EShot asks a running instance to quit, then:

- gives Print Screen back to the desktop: GNOME gets its screenshot UI shortcut back and EShot's custom keybinding is removed; on KDE, EShot's global shortcuts are removed and Spectacle gets plain Print back if EShot had taken it;
- removes the autostart entry and the KWin screenshot permission entry;
- for AppImage installs, also removes the integrated copy (`~/.local/opt/EShot`), the application menu entry and EShot's icons.

```bash
# AppImage
~/.local/opt/EShot/EShot.AppImage --uninstall

# AUR: clean up first, then remove the package
eshot --uninstall
sudo pacman -R eshot-bin
```

For the AUR package and the `.tar.gz` archive, files owned by the package manager or the extracted folder are left alone. Settings in `~/.config/EShot` and your saved screenshots and recordings are kept; delete them yourself if you no longer need them.

### Linux runtime notes

- Wayland screenshots and recordings use XDG Desktop Portal and PipeWire.
- KDE global shortcuts use KGlobalAccel. GNOME uses the Global Shortcuts portal where available and can install an EShot-only GNOME custom shortcut as a compatibility fallback.
- Stock GNOME does not expose legacy tray icons. When no tray integration is available, launching EShot from the application list opens a compact control window with Capture, Settings, About, and Quit. Autostart remains silent. `Print Screen` and custom shortcuts continue to work independently of the tray.
- GNOME screenshots are delivered through its secure Screenshot portal. EShot warms that service at startup, but the portal still adds a short delay before the selection panel appears. This varies by GNOME and portal version and is expected on Wayland.
- GNOME and KDE Wayland use an XWayland selection overlay so text entry, focus, one-canvas multi-monitor selection and the placement of pinned images and recording controls behave consistently. On KDE, `ESHOT_CAPTURE_BACKEND=wayland` opts into experimental per-output native surfaces, which give sharper previews on mixed-DPI setups but cannot place pinned images or the recording frame.
- GIF recording uses GStreamer for portal capture and FFmpeg for final GIF encoding.
- MP4 recording requires a GStreamer AAC encoder when audio is enabled.
- Screen recording permission is handled by the desktop portal. EShot stores a separate restore token per monitor when the portal supports persistent sessions.
- A recorded region must fit inside one monitor. If the portal opens a monitor chooser, select the monitor containing the region.
- The optional dependency setup uses the PackageKit session installer when the desktop provides it, then falls back to the native pacman, apt or dnf workflow. On Fedora, packages the enabled repositories do not provide (such as RPM Fusion packages) are skipped instead of failing the whole install.
- Fedora: the setup installs `ffmpeg-free` and the GStreamer plugins from Fedora's own repositories. MP4 recording needs `x264enc` from RPM Fusion's `gstreamer1-plugins-ugly`; enable [RPM Fusion](https://rpmfusion.org/Configuration) (free and nonfree) before running the setup, or install that package afterwards.

## Visual search

Choose **Google Lens** or **Yandex Images** in Settings. EShot uploads only the selected region to a temporary public image host and opens the chosen visual-search provider with that temporary URL.

Do not use visual search for private or sensitive screenshots.

## OCR

OCR runs on **Youdao online recognition** by default. A single request to `ocrtran.youdao.com` returns every recognized line with its bounding box plus the translated text, so the result dialog can show the original and the translation side by side, copy either of them, or paste the server-rendered translation back over the selected region. The recognition language defaults to `auto`, letting the server detect the script. The translation target is chosen in the same dialog, remembered across runs, and reused by the translation hotkey.

The first recognition uploads the selected region to `ocrtran.youdao.com`. That endpoint is not an official public Youdao API and may stop working at any time, so EShot asks for confirmation once before the first upload and remembers the answer. Declining the notice switches the engine back to local Tesseract and stores that choice.

Local **Tesseract** remains available as an offline engine; select it in the engine dropdown of the OCR dialog. The first-run wizard can install English, the system language, and additional language data. Missing languages remain visible but disabled so the missing dependency is clear.

Failures are reported as they are and never fall back to Tesseract silently, so it is always clear whether the image left the machine.

Translation mode has its own hotkey, unbound by default. Bind it in **Settings > Hotkeys > Action hotkeys** to capture a region and translate it in one step, without opening the OCR dialog. The hotkey ignores the "translate after selection" setting but still respects the first-upload confirmation.

## Upload services

Anonymous providers work without credentials. Google Drive and Yandex Disk require OAuth access tokens; Allwebs and Radikal Cloud require service API keys. OAuth tokens are stored in the operating system credential store, never in EShot's plain-text settings file.

### Google Drive token setup

1. Open [Google OAuth 2.0 Playground](https://developers.google.com/oauthplayground).
2. Select the Drive API v3 scope:

   ```text
   https://www.googleapis.com/auth/drive.file
   ```

3. Authorize the scope and exchange the authorization code for tokens.
4. Copy the `access_token`, or copy the complete JSON response.
5. Paste it into EShot's Google Drive token field.

OAuth Playground access tokens expire. Generate a new token if an upload later returns HTTP 401.

## Default shortcuts

Most shortcuts can be changed in Settings.

| Shortcut | Action |
| --- | --- |
| `Print Screen` | Start region capture |
| Double-click a screen | Capture the complete monitor |
| `Enter` / `Ctrl+C` | Copy selection |
| `Ctrl+S` | Save selection |
| `Esc` | Cancel or close |
| `P` | Pen |
| `A` | Arrow |
| `L` | Line |
| `R` | Rectangle |
| `C` | Circle |
| `T` | Text |
| `H` | Highlighter |
| `B` | Blur |
| `N` | Counter |
| `X` | Eraser |
| `D` | Semi-transparent rectangle |
| `I` | Eyedropper |
| `Ctrl+Z` / `Ctrl+Shift+Z` | Undo / redo |

## Command line

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
EShot --uninstall  # restore Print Screen and remove desktop integration

# Native packages also install the lowercase launcher:
eshot --capture
```

## Questions, ideas and bug reports

- **Questions and help:** ask in [Discussions → Q&A](https://github.com/Benoks/EShot/discussions/categories/q-a).
- **Ideas and feature requests:** post them in [Discussions → Ideas](https://github.com/Benoks/EShot/discussions/categories/ideas), where others can vote on them.
- **Bugs:** open an [issue](https://github.com/Benoks/EShot/issues/new). Include your EShot version, OS and, on Linux, your desktop (KDE/GNOME) and session (Wayland/X11).


## Build from source

Core requirements:

- CMake 3.16+
- C++17 compiler
- Qt 6 Core, GUI, Widgets, Network, and DBus on Linux

### Linux

Ubuntu/Debian and CachyOS/Arch dependency helpers are included:

```bash
./scripts/linux/install-ubuntu-deps.sh
# or
./scripts/linux/install-cachyos-deps.sh

./scripts/linux/check-linux-runtime.sh
./scripts/linux/build-linux.sh
./dist-linux/bin/EShot
```

Build the AppImage:

```bash
./scripts/linux/build-appimage.sh
```

The script writes `packages/EShot-v<version>-x86_64.AppImage`. It installs `libssl.so.3` into the bundle explicitly, because Qt loads the OpenSSL backend at runtime and `linuxdeploy --plugin qt` does not pull it in; without it every HTTPS request fails with `TLS initialization failed`.

Run the tests (they need a display, so wrap them on a headless machine):

```bash
ctest --test-dir build-linux --output-on-failure
xvfb-run -a ctest --test-dir build-linux --output-on-failure
```

### Windows

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
```

Build the installer with Inno Setup 6:

```powershell
iscc EShot_Setup.iss
```

## Repository contents

Only sources, tests, packaging scripts and the documentation that ships with a release are tracked here. Everything else stays on the developer's machine and is listed in `.gitignore`:

| Category | Paths |
| --- | --- |
| Intermediate build output | `build*/`, `EShot.AppDir/`, `packages/`, `dist/`, `installer_output/`, `*.AppImage` |
| Design notes and implementation plans | `docs/specs/`, `docs/plans/` |
| Project goals and roadmap drafts | `docs/goals/`, `GOALS.md`, `ROADMAP.md` |

Keeping them untracked means a fresh clone contains nothing that can drift away from the code, and drafts can be rewritten freely without adding noise to history.

## Third-party components

EShot uses Qt, Tesseract OCR, FFmpeg, GStreamer, PipeWire, XDG Desktop Portal, and Inno Setup. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for licensing details.

## License

EShot is released under the [MIT License](LICENSE).
