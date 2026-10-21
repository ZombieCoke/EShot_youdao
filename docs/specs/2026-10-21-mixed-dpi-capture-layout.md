# 混合缩放多屏下的截图布局偏移

## 现象

KDE Wayland + XWayland，两块屏缩放不同（DP-3 2560x1440 @1.3、HDMI-A-1 1920x1080 @1.25）：

- 主屏截图时画面整体左移，最左侧一条框不到；
- OCR 贴图、有道自动翻译贴图都不在原位；
- 贴图尺寸正常，只有位置偏。

## 根因

`QScreen::geometry()` 在这套环境下给位置和尺寸用了不同的像素密度。

| 来源 | DP-3 | HDMI-A-1 | 并集 |
|---|---|---|---|
| KWin（真实） | 0,0 1970x1108 | 1970,0 1536x864 | 3506x1108 |
| xrandr（XWayland 设备像素） | 2560x1440+0+0 | 1997x1123+2561+0 | 4558x1440 |
| Qt 报的（日志实测） | — | — | **4107x1115** |

`4107 = 2561 + 1997/1.3`：HDMI 的 X 起点保留了 XWayland 设备像素（2561），宽度却按逻辑像素折算。真实并集只有 3506，宽出去 601。

因为 AppImage 跑在 xcb 上，`LinuxScreenshotPolicy::useNativeScreenViews()` 要求 platformName 含 "wayland"，返回 false，走的是单窗口分支 `setGeometry(m_virtualDesktopRect)`。窗口比真实桌面宽 601 逻辑像素（约 781 设备像素），合成器摆进去时被顶偏，于是画面左移、左侧不可达。

贴图位置原本取自 `mapToGlobal(selRect.topLeft())`，用的是同一个被顶偏的窗口坐标，所以偏移量一致。

## 修法

1. `rebuildCaptureLayout()`（`src/capture/CaptureScreenImage.cpp`）：用截图自身的物理尺寸 ÷ KWin 缩放重建每个输出的逻辑矩形。只有在发现 Qt 报的尺寸和"物理像素 ÷ 缩放"对不上（容差 2 px）时才重建，其它平台保持原样。
2. `captureNativeScreenImages()` 采用重建结果作为 `m_virtualDesktopRect`，并同步修正 `m_captureMonitors` 的物理矩形。
3. 贴图位置改用 `globalPointFromCanvas()`（虚拟桌面原点 + 画布坐标），不再依赖可能被顶偏的 `mapToGlobal`。

重建时逻辑尺寸向上取整：宁可多 1 像素，也不要比真实桌面窄，窄了会露出没被覆盖的边。

## 验证

- `tests/CaptureScreenImageTests`：新增两例，一例用上面这组实测数字断言重建出 3506x1108，一例断言 Qt 布局本来就正确时不做任何改动。
- `xvfb-run -a ctest --test-dir build`：63/63 通过。
- 真机验证：主屏框选后画面不再偏移，左侧可截取，贴图回到原位。

## 已知边界

重建按"多屏是横着或竖着一排"排布（取跨度大的方向）。L 形或带空隙的非连续布局仍会退化成紧贴排布；这类排布下 Qt 报的坐标本身不可信，没有更可靠的来源。
