#include "YoudaoTranslate.h"

#include <QDebug>
#include <QHttpMultiPart>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

YoudaoTranslate::YoudaoTranslate(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_secret(QString::fromLatin1(YoudaoProtocol::kDefaultSecret))
    , m_endpoint(QString::fromLatin1(YoudaoProtocol::kEndpoint))
{
}

YoudaoTranslate::~YoudaoTranslate()
{
    cancel();
}

void YoudaoTranslate::setSecret(const QString &secret)
{
    if (secret.trimmed().isEmpty()) {
        m_secret = QString::fromLatin1(YoudaoProtocol::kDefaultSecret);
    } else {
        m_secret = secret.trimmed();
    }
}

void YoudaoTranslate::setEndpoint(const QString &endpoint)
{
    m_endpoint = endpoint.trimmed().isEmpty()
        ? QString::fromLatin1(YoudaoProtocol::kEndpoint)
        : endpoint.trimmed();
}

void YoudaoTranslate::cancel()
{
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void YoudaoTranslate::translate(const QImage &image, const QString &from, const QString &to, bool render)
{
    cancel();

    if (image.isNull()) {
        emit failed(QStringLiteral("图片为空"), 60);
        return;
    }

    double scale = 1.0;
    QString mime;
    const QByteArray uploadBytes = YoudaoProtocol::encodeUpload(image, scale, mime);
    if (uploadBytes.isEmpty()) {
        emit failed(QStringLiteral("图片压缩或处理失败"), 60);
        return;
    }
    m_uploadScale = (scale > 0.0) ? scale : 1.0;

    const QString salt = YoudaoProtocol::randomSalt();
    const QString secret = m_secret.isEmpty()
        ? QString::fromLatin1(YoudaoProtocol::kDefaultSecret)
        : m_secret;
    const QString sign = YoudaoProtocol::genSign(uploadBytes, salt, secret);

    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    auto appendTextField = [multiPart](const QString &name, const QString &val) {
        QHttpPart part;
        part.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QStringLiteral("form-data; name=\"%1\"").arg(name));
        part.setBody(val.toUtf8());
        multiPart->append(part);
    };

    appendTextField(QStringLiteral("clientele"), QString::fromLatin1(YoudaoProtocol::kClientele));
    appendTextField(QStringLiteral("salt"), salt);
    appendTextField(QStringLiteral("sign"), sign);
    appendTextField(QStringLiteral("from"), from);
    appendTextField(QStringLiteral("to"), to);
    appendTextField(QStringLiteral("isRender"), render ? QStringLiteral("true") : QStringLiteral("false"));
    appendTextField(QStringLiteral("funDesc"), QStringLiteral("screenshot"));

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QStringLiteral("form-data; name=\"multipartFile\"; filename=\"capture.png\""));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, mime);
    filePart.setBody(uploadBytes);
    multiPart->append(filePart);

    QNetworkRequest request{QUrl(m_endpoint)};
    request.setRawHeader("User-Agent", "YoudaoDictHelper/11.3.24.0");
    request.setTransferTimeout(60000);

    m_startTime = QDateTime::currentMSecsSinceEpoch();
    m_reply = m_nam->post(request, multiPart);
    multiPart->setParent(m_reply);

    connect(m_reply, &QNetworkReply::finished, this, &YoudaoTranslate::onReplyFinished);
}

void YoudaoTranslate::onReplyFinished()
{
    if (!m_reply)
        return;

    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    reply->deleteLater();

    const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_startTime;

    if (reply->error() != QNetworkReply::NoError) {
        const QString reason = reply->errorString();
        qWarning() << "[YoudaoTranslate] 网络请求失败:" << reason;
        emit failed(reason, -1);
        return;
    }

    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (httpStatus != 200) {
        const QByteArray body = reply->readAll();
        const QByteArray preview = body.left(300);
        const QString reason = QStringLiteral("HTTP %1: %2").arg(httpStatus).arg(QString::fromUtf8(preview));
        qWarning() << "[YoudaoTranslate]" << reason;
        emit failed(reason, httpStatus);
        return;
    }

    const QByteArray respBody = reply->readAll();
    YoudaoProtocol::Result result;
    QString errorText;
    if (!YoudaoProtocol::parseResponse(respBody, result, errorText)) {
        qWarning() << "[YoudaoTranslate] 解析失败:" << errorText;
        int code = -1;
        static const QRegularExpression codeRe(QStringLiteral("errorCode=(\\d+)"));
        const auto match = codeRe.match(errorText);
        if (match.hasMatch()) {
            code = match.captured(1).toInt();
        }
        emit failed(errorText, code);
        return;
    }

    result.elapsedMs = elapsed;
    result.uploadScale = m_uploadScale;
    emit finished(result);
}
