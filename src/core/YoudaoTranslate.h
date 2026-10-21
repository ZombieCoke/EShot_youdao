#pragma once

#include <QDateTime>
#include <QImage>
#include <QObject>
#include <QString>

#include "YoudaoProtocol.h"

class QNetworkAccessManager;
class QNetworkReply;

class YoudaoTranslate : public QObject {
    Q_OBJECT
public:
    explicit YoudaoTranslate(QObject *parent = nullptr);
    ~YoudaoTranslate() override;

    void setSecret(const QString &secret);   // 空则用 kDefaultSecret
    // 默认打 ocrtran.youdao.com；单测与私有镜像可替换，避免测试打真实接口。
    void setEndpoint(const QString &endpoint);
    void translate(const QImage &image, const QString &from, const QString &to, bool render);
    void cancel();

signals:
    void finished(const YoudaoProtocol::Result &result);
    void failed(const QString &reason, int errorCode);

private slots:
    void onReplyFinished();

private:
    QNetworkAccessManager *m_nam;
    QNetworkReply *m_reply = nullptr;
    QString m_secret;
    QString m_endpoint;
    double m_uploadScale = 1.0;
    qint64 m_startTime = 0;
};
