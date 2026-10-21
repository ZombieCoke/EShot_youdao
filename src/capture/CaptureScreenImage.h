#pragma once

#include <QImage>
#include <QPainter>
#include <QRect>
#include <QVector>

// KWin's CaptureScreen returns the output's actual scale. Under XWayland,
// QScreen::devicePixelRatio() can instead report 1 for every output.
struct CaptureScreenImage
{
    QRect logical;
    QImage image;
    qreal scale = 1.0;
};

qreal captureSelectionScale(const QVector<CaptureScreenImage> &screens,
                            const QRect &logicalSelection);
QSize captureSelectionSize(const QVector<CaptureScreenImage> &screens,
                          const QRect &logicalSelection);
QImage composeCaptureImages(const QVector<CaptureScreenImage> &screens,
                            const QRect &logicalSelection);
// XWayland 在混合缩放下会把第二个输出的位置按设备像素报、尺寸按逻辑像素报，
// 于是 QScreen::geometry() 的并集比真实桌面宽出一截。这里用截图本身的物理尺寸
// 和缩放重建布局；只有发现 Qt 的尺寸自相矛盾时才返回重建结果，否则返回空矩形
// 表示"Qt 的布局可用，别动"。
QRect rebuildCaptureLayout(QVector<CaptureScreenImage> *screens, const QRect &qtUnion);
void paintCaptureImages(QPainter *painter,
                        const QVector<CaptureScreenImage> &screens,
                        const QPoint &canvasOrigin);
