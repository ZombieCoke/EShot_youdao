#pragma once

#include <QByteArray>
#include <QImage>
#include <QRect>
#include <QString>
#include <QVector>

namespace YoudaoProtocol {

struct Region {
    QRect box;
    QString lang;
    QString source;
    QString target;
};

struct Result {
    QVector<Region> regions;
    QImage rendered;
    QString rawJson;
    qint64 elapsedMs = 0;
    // 上传图相对原图的缩放比；regions 里的 box 是上传图像素坐标，
    // 乘 1/uploadScale 才是原图像素坐标。
    double uploadScale = 1.0;
};

extern const char *const kEndpoint;      // https://ocrtran.youdao.com/ocr/imgtranocr
extern const char *const kClientele;     // deskdict
extern const char *const kDefaultSecret; // VPaHE3kX_vl4BhgYiu2n

QString genSign(const QByteArray &uploadBytes, const QString &salt, const QString &secret);
QString randomSalt();                                   // "0." + 9 位零填充随机
QString mapLanguage(const QString &eshotTag, bool asSource);
bool parseResponse(const QByteArray &json, Result &out, QString &errorText);
QRect scaledBox(const QRect &box, double factor);       // 乘 factor 后夹紧到 >=0 且宽高 >=0
QByteArray encodeUpload(const QImage &image, double &outScale, QString &outMime);

} // namespace YoudaoProtocol
