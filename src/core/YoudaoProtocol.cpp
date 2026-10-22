#include "YoudaoProtocol.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRandomGenerator>
#include <QStringList>
#include <algorithm>

namespace YoudaoProtocol {

const char *const kEndpoint = "https://ocrtran.youdao.com/ocr/imgtranocr";
const char *const kClientele = "deskdict";
const char *const kDefaultSecret = "VPaHE3kX_vl4BhgYiu2n";

QString genSign(const QByteArray &uploadBytes, const QString &salt, const QString &secret)
{
    const QByteArray b64 = uploadBytes.toBase64();
    const int len = b64.size();
    QString mid;
    if (len < 20) {
        mid = QString::fromLatin1(b64.left(std::min(10, len)))
            + QString::number(len)
            + QString::fromLatin1(b64.right(std::min(10, len)));
    } else {
        mid = QString::fromLatin1(b64.left(10))
            + QString::number(len)
            + QString::fromLatin1(b64.right(10));
    }
    const QString raw = QString::fromLatin1(kClientele) + mid + salt + secret;
    return QString::fromLatin1(QCryptographicHash::hash(raw.toUtf8(), QCryptographicHash::Md5).toHex());
}

QString randomSalt()
{
    const quint32 n = QRandomGenerator::global()->bounded(1000000000);
    return QStringLiteral("0.%1").arg(n, 9, 10, QLatin1Char('0'));
}

QString mapLanguage(const QString &eshotTag, bool asSource)
{
    if (asSource && eshotTag == QStringLiteral("auto")) {
        return QStringLiteral("auto");
    }
    // 界面语言给出的是 ISO 代码（ja/de/fr/zh 等），有道要的是自己的代码。
    // 先认 ISO 与已是目标代码的写法，再走 tesseract 标签表，避免 UI 语言被误回退成 en。
    if (eshotTag == QStringLiteral("zh") || eshotTag == QStringLiteral("zh-CN")
        || eshotTag == QStringLiteral("zh-Hans") || eshotTag == QStringLiteral("zh-CHS")) {
        return QStringLiteral("zh-CHS");
    }
    if (eshotTag == QStringLiteral("zh-TW") || eshotTag == QStringLiteral("zh-Hant")
        || eshotTag == QStringLiteral("zh-CHT")) {
        return QStringLiteral("zh-CHT");
    }
    static const QStringList youdaoCodes = {
        QStringLiteral("en"), QStringLiteral("ja"), QStringLiteral("ko"), QStringLiteral("fr"),
        QStringLiteral("de"), QStringLiteral("es"), QStringLiteral("pt"), QStringLiteral("ru"),
        QStringLiteral("it"), QStringLiteral("ar"), QStringLiteral("th"), QStringLiteral("vi"),
        QStringLiteral("id"), QStringLiteral("ms"), QStringLiteral("tr"), QStringLiteral("nl"),
        QStringLiteral("pl"), QStringLiteral("hi")
    };
    if (youdaoCodes.contains(eshotTag)) {
        return eshotTag;
    }
    if (eshotTag == QStringLiteral("eng")) return QStringLiteral("en");
    if (eshotTag == QStringLiteral("chi_sim")) return QStringLiteral("zh-CHS");
    if (eshotTag == QStringLiteral("chi_tra")) return QStringLiteral("zh-CHT");
    if (eshotTag == QStringLiteral("jpn")) return QStringLiteral("ja");
    if (eshotTag == QStringLiteral("kor")) return QStringLiteral("ko");
    if (eshotTag == QStringLiteral("fra")) return QStringLiteral("fr");
    if (eshotTag == QStringLiteral("deu")) return QStringLiteral("de");
    if (eshotTag == QStringLiteral("spa")) return QStringLiteral("es");
    if (eshotTag == QStringLiteral("por")) return QStringLiteral("pt");
    if (eshotTag == QStringLiteral("rus")) return QStringLiteral("ru");
    if (eshotTag == QStringLiteral("ita")) return QStringLiteral("it");
    if (eshotTag == QStringLiteral("ara")) return QStringLiteral("ar");
    if (eshotTag == QStringLiteral("tha")) return QStringLiteral("th");
    if (eshotTag == QStringLiteral("vie")) return QStringLiteral("vi");
    if (eshotTag == QStringLiteral("ind")) return QStringLiteral("id");
    if (eshotTag == QStringLiteral("msa")) return QStringLiteral("ms");
    if (eshotTag == QStringLiteral("tur")) return QStringLiteral("tr");
    if (eshotTag == QStringLiteral("nld")) return QStringLiteral("nl");
    if (eshotTag == QStringLiteral("pol")) return QStringLiteral("pl");
    if (eshotTag == QStringLiteral("hin")) return QStringLiteral("hi");
    return QStringLiteral("en");
}

QVector<LanguageOption> targetLanguageOptions()
{
    // 名称一律用该语言自己的写法：既不用为 8 个界面语言各维护一份翻译，
    // 用户也总能认出自己的母语。
    return {
        {QStringLiteral("zh-CHS"), QStringLiteral("简体中文")},
        {QStringLiteral("zh-CHT"), QStringLiteral("繁體中文")},
        {QStringLiteral("en"), QStringLiteral("English")},
        {QStringLiteral("ja"), QStringLiteral("日本語")},
        {QStringLiteral("ko"), QStringLiteral("한국어")},
        {QStringLiteral("fr"), QStringLiteral("Français")},
        {QStringLiteral("de"), QStringLiteral("Deutsch")},
        {QStringLiteral("es"), QStringLiteral("Español")},
        {QStringLiteral("pt"), QStringLiteral("Português")},
        {QStringLiteral("ru"), QStringLiteral("Русский")},
        {QStringLiteral("it"), QStringLiteral("Italiano")},
        {QStringLiteral("ar"), QStringLiteral("العربية")},
        {QStringLiteral("th"), QStringLiteral("ไทย")},
        {QStringLiteral("vi"), QStringLiteral("Tiếng Việt")},
        {QStringLiteral("id"), QStringLiteral("Bahasa Indonesia")},
        {QStringLiteral("ms"), QStringLiteral("Bahasa Melayu")},
        {QStringLiteral("tr"), QStringLiteral("Türkçe")},
        {QStringLiteral("nl"), QStringLiteral("Nederlands")},
        {QStringLiteral("pl"), QStringLiteral("Polski")},
        {QStringLiteral("hi"), QStringLiteral("हिन्दी")}
    };
}

bool parseResponse(const QByteArray &json, Result &out, QString &errorText)
{
    out = Result();
    out.rawJson = QString::fromUtf8(json);

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        errorText = QStringLiteral("JSON 解析失败: %1").arg(parseErr.errorString());
        return false;
    }

    const QJsonObject root = doc.object();

    QString errCodeStr;
    if (root.contains(QStringLiteral("errorCode"))) {
        const QJsonValue errVal = root.value(QStringLiteral("errorCode"));
        if (errVal.isString()) {
            errCodeStr = errVal.toString().trimmed();
        } else if (errVal.isDouble()) {
            errCodeStr = QString::number(errVal.toInteger());
        }
    }

    if (!errCodeStr.isEmpty() && errCodeStr != QStringLiteral("0")) {
        QString msg;
        if (root.contains(QStringLiteral("msg")) && root.value(QStringLiteral("msg")).isString()) {
            msg = root.value(QStringLiteral("msg")).toString();
        } else if (root.contains(QStringLiteral("message")) && root.value(QStringLiteral("message")).isString()) {
            msg = root.value(QStringLiteral("message")).toString();
        }
        if (!msg.isEmpty()) {
            errorText = QStringLiteral("errorCode=%1 message=%2").arg(errCodeStr, msg);
        } else {
            errorText = QStringLiteral("errorCode=%1").arg(errCodeStr);
        }
        return false;
    }

    if (root.contains(QStringLiteral("resRegions")) && root.value(QStringLiteral("resRegions")).isArray()) {
        const QJsonArray arr = root.value(QStringLiteral("resRegions")).toArray();
        for (const QJsonValue &itemVal : arr) {
            if (!itemVal.isObject())
                continue;
            const QJsonObject item = itemVal.toObject();
            const QString bboxStr = item.value(QStringLiteral("boundingBox")).toString();
            const QStringList parts = bboxStr.split(QLatin1Char(','), Qt::SkipEmptyParts);
            if (parts.size() != 4)
                continue;
            bool okX = false, okY = false, okW = false, okH = false;
            const int x = parts[0].trimmed().toInt(&okX);
            const int y = parts[1].trimmed().toInt(&okY);
            const int w = parts[2].trimmed().toInt(&okW);
            const int h = parts[3].trimmed().toInt(&okH);
            if (!okX || !okY || !okW || !okH)
                continue;

            Region reg;
            reg.box = QRect(x, y, w, h);
            reg.lang = item.value(QStringLiteral("lang")).toString();
            reg.source = item.value(QStringLiteral("context")).toString();
            reg.target = item.value(QStringLiteral("tranContent")).toString();
            out.regions.append(reg);
        }
    }

    if (root.contains(QStringLiteral("image"))) {
        const QString imgB64 = root.value(QStringLiteral("image")).toString();
        if (!imgB64.isEmpty()) {
            const QByteArray imgData = QByteArray::fromBase64(imgB64.toLatin1());
            out.rendered.loadFromData(imgData);
        }
    }

    return true;
}

QRect scaledBox(const QRect &box, double factor)
{
    int x = qRound(box.x() * factor);
    int y = qRound(box.y() * factor);
    int w = qRound(box.width() * factor);
    int h = qRound(box.height() * factor);
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (w < 0) w = 0;
    if (h < 0) h = 0;
    return QRect(x, y, w, h);
}

QByteArray encodeUpload(const QImage &image, double &outScale, QString &outMime)
{
    if (image.isNull()) {
        outScale = 1.0;
        outMime = QStringLiteral("image/png");
        return QByteArray();
    }

    QImage toUpload = image;
    const int maxDim = std::max(image.width(), image.height());
    if (maxDim > 2000) {
        toUpload = image.scaled(2000, 2000, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        outScale = static_cast<double>(toUpload.width()) / static_cast<double>(image.width());
    } else {
        outScale = 1.0;
    }

    const bool useJpeg = (outScale < 1.0) || (!image.hasAlphaChannel());
    const char *format = useJpeg ? "JPEG" : "PNG";
    outMime = useJpeg ? QStringLiteral("image/jpeg") : QStringLiteral("image/png");

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    toUpload.save(&buffer, format, useJpeg ? 85 : -1);
    return bytes;
}

} // namespace YoudaoProtocol
