#include "CaptureScreenImage.h"
#include "CaptureGeometry.h"

#include <QtMath>
#include <algorithm>

namespace {
bool isUsable(const CaptureScreenImage &screen)
{
    return screen.logical.isValid() && !screen.image.isNull()
        && qIsFinite(screen.scale) && screen.scale > 0.0;
}
}

qreal captureSelectionScale(const QVector<CaptureScreenImage> &screens,
                            const QRect &logicalSelection)
{
    qreal scale = 0.0;
    for (const CaptureScreenImage &screen : screens) {
        if (isUsable(screen) && screen.logical.intersects(logicalSelection))
            scale = qMax(scale, screen.scale);
    }
    return scale;
}

QSize captureSelectionSize(const QVector<CaptureScreenImage> &screens,
                          const QRect &logicalSelection)
{
    const qreal scale = captureSelectionScale(screens, logicalSelection);
    return scale > 0.0
        ? QSize(qRound(logicalSelection.width() * scale),
                qRound(logicalSelection.height() * scale))
        : QSize();
}

QImage composeCaptureImages(const QVector<CaptureScreenImage> &screens,
                            const QRect &logicalSelection)
{
    const qreal scale = captureSelectionScale(screens, logicalSelection);
    const QSize size = captureSelectionSize(screens, logicalSelection);
    if (size.isEmpty())
        return {};

    // A selection contained in one output is a direct native-pixel crop:
    // no resampling, even when another output has a higher scale.
    for (const CaptureScreenImage &screen : screens) {
        if (isUsable(screen) && qFuzzyCompare(screen.scale, scale)
            && screen.logical.contains(logicalSelection)) {
            const QRect source = physicalRectFromLogical(
                logicalSelection.translated(-screen.logical.topLeft()), screen.scale);
            QImage result = screen.image.copy(source);
            result.setDevicePixelRatio(1.0);
            return result;
        }
    }

    // A cross-output image needs one pixel density. Use the highest density
    // actually touched by the selection, with nearest-neighbour enlargement
    // of lower-density outputs rather than introducing interpolation blur.
    QImage result(size, QImage::Format_ARGB32_Premultiplied);
    if (result.isNull())
        return {};
    result.fill(Qt::black);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    for (const CaptureScreenImage &screen : screens) {
        if (!isUsable(screen))
            continue;
        const QRect intersection = screen.logical.intersected(logicalSelection);
        if (intersection.isEmpty())
            continue;
        const QRect source = physicalRectFromLogical(
            intersection.translated(-screen.logical.topLeft()), screen.scale);
        const QRect destination = physicalRectFromLogical(
            intersection.translated(-logicalSelection.topLeft()), scale);
        painter.drawImage(destination, screen.image, source);
    }
    return result;
}

QRect rebuildCaptureLayout(QVector<CaptureScreenImage> *screens, const QRect &qtUnion)
{
    if (!screens || screens->size() < 2)
        return {};
    for (const CaptureScreenImage &screen : *screens) {
        if (!isUsable(screen))
            return {};
    }

    // Qt 报的尺寸一旦和"物理像素 ÷ 缩放"对不上，就说明它给位置和尺寸用了不同
    // 的像素密度。XWayland 混合缩放正是这样：第二块屏的位置按设备像素报，
    // 尺寸却按逻辑像素报，并集于是比真实桌面宽出一截。
    bool inconsistent = false;
    for (const CaptureScreenImage &screen : *screens) {
        const QSize expected(qCeil(screen.image.width() / screen.scale),
                             qCeil(screen.image.height() / screen.scale));
        if (qAbs(screen.logical.width() - expected.width()) > 2
            || qAbs(screen.logical.height() - expected.height()) > 2) {
            inconsistent = true;
            break;
        }
    }
    if (!inconsistent)
        return {};

    // 位置的单位不可信，但左右/上下顺序仍然可信：按 Qt 的坐标排序，再用真实
    // 逻辑尺寸紧贴排布。多屏基本是横着或竖着一排，取跨度大的方向。
    QRect bounds = qtUnion;
    if (!bounds.isValid()) {
        for (const CaptureScreenImage &screen : *screens)
            bounds = bounds.united(screen.logical);
    }
    const bool horizontal = bounds.width() >= bounds.height();

    QVector<int> order;
    order.reserve(screens->size());
    for (int i = 0; i < screens->size(); ++i)
        order.append(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        const QRect &ra = (*screens)[a].logical;
        const QRect &rb = (*screens)[b].logical;
        if (horizontal)
            return ra.x() != rb.x() ? ra.x() < rb.x() : ra.y() < rb.y();
        return ra.y() != rb.y() ? ra.y() < rb.y() : ra.x() < rb.x();
    });

    QPoint cursor(0, 0);
    QRect packed;
    for (int index : order) {
        CaptureScreenImage &screen = (*screens)[index];
        // 向上取整：宁可多 1 像素也不要比真实桌面窄，窄了会露出没被覆盖的边。
        const QSize logical(qCeil(screen.image.width() / screen.scale),
                            qCeil(screen.image.height() / screen.scale));
        screen.logical = QRect(cursor, logical);
        packed = packed.united(screen.logical);
        if (horizontal)
            cursor.rx() += logical.width();
        else
            cursor.ry() += logical.height();
    }
    return packed;
}

void paintCaptureImages(QPainter *painter,
                        const QVector<CaptureScreenImage> &screens,
                        const QPoint &canvasOrigin)
{
    painter->save();
    // A proxy widget may be painted into multiple views. The QWidget's own
    // DPR is not authoritative; the redirected painter's device transform is.
    const QTransform transform = painter->deviceTransform();
    const qreal paintScale = qSqrt(transform.m11() * transform.m11()
                                  + transform.m12() * transform.m12());
    for (const CaptureScreenImage &screen : screens) {
        if (!isUsable(screen))
            continue;
        // Draw each output independently. In particular a 100% output is
        // blitted 1:1 instead of upscaling to the workspace's maximum DPR
        // and then downscaling again for the preview.
        painter->setRenderHint(QPainter::SmoothPixmapTransform,
                              screen.scale > paintScale);
        painter->drawImage(screen.logical.translated(-canvasOrigin),
                           screen.image, screen.image.rect());
    }
    painter->restore();
}
