#include <QtTest/QTest>

#include "capture/CaptureScreenImage.h"

namespace {
QImage pixelPattern(const QSize &size)
{
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < size.height(); ++y) {
        auto *row = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < size.width(); ++x)
            row[x] = (x + y) % 2 ? qRgb(255, 255, 255) : qRgb(0, 0, 0);
    }
    return image;
}

QVector<CaptureScreenImage> mixedScreens()
{
    return {
        {QRect(0, 0, 32, 24), pixelPattern(QSize(64, 48)), 2.0},
        {QRect(32, 0, 32, 16), pixelPattern(QSize(32, 16)), 1.0}
    };
}
}

class CaptureScreenImageTests : public QObject
{
    Q_OBJECT

private slots:
    void exports1080pOutputAtNativeResolution();
    void exportsHiDpiOutputAtNativeResolution();
    void cropsUnscaledOutputWithoutChangingAnyPixels();
    void previewsUnscaledOutputWithoutChangingAnyPixels();
    void preservesCrossOutputSelectionWithSharpPixelDuplication();
    void handlesNegativeOutputPositionsAndDesktopGaps();
    void keepsFractionalScaleSelectionSizeStable();
    void rebuildsMixedDpiLayoutWhenQtMixesUnits();
    void keepsConsistentLayoutUntouched();
    void ignoresInvalidAndUntouchedOutputs();
};

void CaptureScreenImageTests::exports1080pOutputAtNativeResolution()
{
    const QVector<CaptureScreenImage> screens = {
        {QRect(0, 0, 1920, 1200), pixelPattern(QSize(3840, 2400)), 2.0},
        {QRect(1920, 0, 1920, 1080), pixelPattern(QSize(1920, 1080)), 1.0}
    };
    const QImage result = composeCaptureImages(screens, screens[1].logical);
    QCOMPARE(result.size(), QSize(1920, 1080));
    QCOMPARE(result, screens[1].image);
    QCOMPARE(captureSelectionScale(screens, screens[1].logical), 1.0);
}

void CaptureScreenImageTests::exportsHiDpiOutputAtNativeResolution()
{
    auto screens = mixedScreens();
    screens[0].image.setDevicePixelRatio(2.0);
    const QImage result = composeCaptureImages(screens, screens[0].logical);
    QCOMPARE(result.size(), QSize(64, 48));
    QCOMPARE(result.devicePixelRatio(), 1.0);
    QCOMPARE(result.pixel(1, 2), screens[0].image.pixel(1, 2));
}

void CaptureScreenImageTests::cropsUnscaledOutputWithoutChangingAnyPixels()
{
    const auto screens = mixedScreens();
    const QRect selection(35, 2, 17, 9);
    const QImage result = composeCaptureImages(screens, selection);
    QCOMPARE(result, screens[1].image.copy(QRect(3, 2, 17, 9)));
    QCOMPARE(captureSelectionSize(screens, selection), QSize(17, 9));
}

void CaptureScreenImageTests::previewsUnscaledOutputWithoutChangingAnyPixels()
{
    const auto screens = mixedScreens();
    QImage preview(QSize(64, 24), QImage::Format_ARGB32_Premultiplied);
    preview.fill(Qt::black);
    QPainter painter(&preview);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    paintCaptureImages(&painter, screens, QPoint());
    QVERIFY(painter.testRenderHint(QPainter::SmoothPixmapTransform));
    painter.end();
    QCOMPARE(preview.copy(screens[1].logical), screens[1].image);
}

void CaptureScreenImageTests::preservesCrossOutputSelectionWithSharpPixelDuplication()
{
    const auto screens = mixedScreens();
    const QImage result = composeCaptureImages(screens, QRect(30, 2, 6, 4));
    QCOMPARE(result.size(), QSize(12, 8));
    for (int y = 0; y < result.height(); ++y) {
        for (int x = 0; x < result.width(); ++x) {
            const QRgb expected = x < 4
                ? screens[0].image.pixel(60 + x, 4 + y)
                : screens[1].image.pixel((x - 4) / 2, 2 + y / 2);
            QCOMPARE(result.pixel(x, y), expected);
        }
    }
}

void CaptureScreenImageTests::handlesNegativeOutputPositionsAndDesktopGaps()
{
    const QVector<CaptureScreenImage> screens = {
        {QRect(-40, -8, 32, 16), pixelPattern(QSize(32, 16)), 1.0},
        {QRect(0, 0, 32, 24), pixelPattern(QSize(64, 48)), 2.0}
    };
    QCOMPARE(composeCaptureImages(screens, QRect(-37, -6, 17, 9)),
             screens[0].image.copy(QRect(3, 2, 17, 9)));
    const QRect canvas(-40, -8, 72, 32);
    const QImage result = composeCaptureImages(screens, canvas);
    QCOMPARE(result.size(), QSize(144, 64));
    QCOMPARE(result.pixel(68, 20), qRgb(0, 0, 0));
    QImage preview(canvas.size(), QImage::Format_ARGB32_Premultiplied);
    preview.fill(Qt::black);
    QPainter painter(&preview);
    paintCaptureImages(&painter, screens, canvas.topLeft());
    painter.end();
    QCOMPARE(preview.copy(QRect(0, 0, 32, 16)), screens[0].image);
}

void CaptureScreenImageTests::keepsFractionalScaleSelectionSizeStable()
{
    const QVector<CaptureScreenImage> screens = {
        {QRect(-64, 0, 64, 32), pixelPattern(QSize(80, 40)), 1.25}
    };
    for (int x = -63; x < -40; ++x) {
        const QRect selection(x, 4, 13, 7);
        QCOMPARE(captureSelectionSize(screens, selection), QSize(16, 9));
        QCOMPARE(composeCaptureImages(screens, selection).size(), QSize(16, 9));
    }
}

void CaptureScreenImageTests::ignoresInvalidAndUntouchedOutputs()
{
    auto screens = mixedScreens();
    screens.append({QRect(32, 0, 32, 16), QImage(), 3.0});
    screens.append({QRect(32, 0, 32, 16), screens[1].image, 0.0});
    QCOMPARE(captureSelectionScale(screens, screens[1].logical), 1.0);
    QVERIFY(composeCaptureImages(screens, QRect(100, 100, 10, 10)).isNull());
    QVERIFY(composeCaptureImages(screens, QRect()).isNull());
}

void CaptureScreenImageTests::rebuildsMixedDpiLayoutWhenQtMixesUnits()
{
    // 实测来自一台 KDE Wayland 机器：DP-3 2560x1440@1.3、HDMI-A-1 1920x1080@1.25。
    // Qt 把第二块屏的位置按 XWayland 设备像素报（2561），尺寸却按逻辑像素报，
    // 于是并集是 4107x1115，而真实桌面只有 3506x1108。
    QVector<CaptureScreenImage> screens = {
        {QRect(0, 0, 1970, 1108), pixelPattern(QSize(2560, 1440)), 1.3},
        {QRect(2561, 0, 1546, 869), pixelPattern(QSize(1920, 1080)), 1.25}
    };
    const QRect rebuilt = rebuildCaptureLayout(&screens, QRect(0, 0, 4107, 1115));
    QCOMPARE(rebuilt, QRect(0, 0, 3506, 1108));
    QCOMPARE(screens[0].logical, QRect(0, 0, 1970, 1108));
    QCOMPARE(screens[1].logical, QRect(1970, 0, 1536, 864));
}

void CaptureScreenImageTests::keepsConsistentLayoutUntouched()
{
    // Qt 报的尺寸和"物理像素 ÷ 缩放"一致时不该动它，否则会破坏本来正确的平台。
    QVector<CaptureScreenImage> screens = {
        {QRect(0, 0, 1970, 1108), pixelPattern(QSize(2560, 1440)), 1.3},
        {QRect(1970, 0, 1536, 864), pixelPattern(QSize(1920, 1080)), 1.25}
    };
    QVERIFY(!rebuildCaptureLayout(&screens, QRect(0, 0, 3506, 1108)).isValid());
    QCOMPARE(screens[0].logical, QRect(0, 0, 1970, 1108));
    QCOMPARE(screens[1].logical, QRect(1970, 0, 1536, 864));
}

QTEST_APPLESS_MAIN(CaptureScreenImageTests)

#include "CaptureScreenImageTests.moc"
