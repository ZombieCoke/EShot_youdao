#include <QtTest/QTest>
#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QRegularExpression>
#include <QSignalSpy>

#include "core/YoudaoProtocol.h"
#include "core/YoudaoTranslate.h"

class YoudaoTranslateTests : public QObject
{
    Q_OBJECT

private slots:
    void verifiesFixedSignVector();
    void verifiesSignBoundarySafety();
    void verifiesRandomSaltShape();
    void verifiesLanguageMappingTable();
    void verifiesParseResponseSuccessAndVariants();
    void verifiesParseResponseErrorCodes();
    void verifiesScaledBox();
    void verifiesEncodeUpload();
    void verifiesYoudaoTranslateEmptyImage();
    void verifiesYoudaoTranslateCancelSafety();
    void verifiesYoudaoTranslateNetworkFailure();
};

void YoudaoTranslateTests::verifiesFixedSignVector()
{
    QFile fixture(QStringLiteral("tests/fixtures/hello.png"));
    if (!fixture.exists()) {
        // 如果测试工作目录不是源码根目录，尝试当前目录或上级目录
        fixture.setFileName(QFINDTESTDATA("fixtures/hello.png"));
    }
    QVERIFY2(fixture.open(QIODevice::ReadOnly), "Could not open fixtures/hello.png");
    const QByteArray uploadBytes = fixture.readAll();
    fixture.close();

    const QByteArray b64 = uploadBytes.toBase64();
    QCOMPARE(b64.size(), 5352);

    const QString salt = QStringLiteral("0.123456789");
    const QString secret = QString::fromLatin1(YoudaoProtocol::kDefaultSecret);
    const QString sign = YoudaoProtocol::genSign(uploadBytes, salt, secret);

    QCOMPARE(sign, QStringLiteral("c2fe005b5ca5bad4b6654ce269a58d10"));
}

void YoudaoTranslateTests::verifiesSignBoundarySafety()
{
    // 测试超短字节（base64 长度 < 20），确保不越界崩溃
    const QByteArray shortBytes = "hello";
    const QString salt = QStringLiteral("0.987654321");
    const QString secret = QStringLiteral("testSecret");
    const QString sign = YoudaoProtocol::genSign(shortBytes, salt, secret);
    QCOMPARE(sign.size(), 32);

    // 空字节
    const QString emptySign = YoudaoProtocol::genSign(QByteArray(), salt, secret);
    QCOMPARE(emptySign.size(), 32);
}

void YoudaoTranslateTests::verifiesRandomSaltShape()
{
    static const QRegularExpression regex(QStringLiteral("^0\\.\\d{9}$"));
    for (int i = 0; i < 20; ++i) {
        const QString salt = YoudaoProtocol::randomSalt();
        QCOMPARE(salt.size(), 11);
        QVERIFY(salt.startsWith(QStringLiteral("0.")));
        QVERIFY2(regex.match(salt).hasMatch(), qPrintable(salt));
    }
}

void YoudaoTranslateTests::verifiesLanguageMappingTable()
{
    // 源语言下的 auto
    QCOMPARE(YoudaoProtocol::mapLanguage(QStringLiteral("auto"), true), QStringLiteral("auto"));
    // 目标语言下 auto 应该回退到 en
    QCOMPARE(YoudaoProtocol::mapLanguage(QStringLiteral("auto"), false), QStringLiteral("en"));

    // 21 条全表映射测试
    struct MappingPair {
        const char *tag;
        const char *expected;
    };
    static constexpr MappingPair table[] = {
        {"eng", "en"},
        {"chi_sim", "zh-CHS"},
        {"chi_tra", "zh-CHT"},
        {"jpn", "ja"},
        {"kor", "ko"},
        {"fra", "fr"},
        {"deu", "de"},
        {"spa", "es"},
        {"por", "pt"},
        {"rus", "ru"},
        {"ita", "it"},
        {"ara", "ar"},
        {"tha", "th"},
        {"vie", "vi"},
        {"ind", "id"},
        {"msa", "ms"},
        {"tur", "tr"},
        {"nld", "nl"},
        {"pol", "pl"},
        {"hin", "hi"}
    };

    for (const auto &item : table) {
        const QString tag = QString::fromLatin1(item.tag);
        const QString expected = QString::fromLatin1(item.expected);
        QCOMPARE(YoudaoProtocol::mapLanguage(tag, true), expected);
        QCOMPARE(YoudaoProtocol::mapLanguage(tag, false), expected);
    }

    // 界面语言给的是 ISO 代码，必须直接命中，不能被回退成 en
    static constexpr MappingPair isoTable[] = {
        {"ja", "ja"},
        {"ko", "ko"},
        {"fr", "fr"},
        {"de", "de"},
        {"es", "es"},
        {"ru", "ru"},
        {"tr", "tr"},
        {"en", "en"},
        {"zh", "zh-CHS"},
        {"zh-CN", "zh-CHS"},
        {"zh-TW", "zh-CHT"}
    };

    for (const auto &item : isoTable) {
        const QString tag = QString::fromLatin1(item.tag);
        const QString expected = QString::fromLatin1(item.expected);
        QCOMPARE(YoudaoProtocol::mapLanguage(tag, false), expected);
    }

    // 未列出的标签回退 en
    QCOMPARE(YoudaoProtocol::mapLanguage(QStringLiteral("ukr"), true), QStringLiteral("en"));
    QCOMPARE(YoudaoProtocol::mapLanguage(QStringLiteral("ukr"), false), QStringLiteral("en"));
    QCOMPARE(YoudaoProtocol::mapLanguage(QStringLiteral("unknown_lang"), true), QStringLiteral("en"));
    QCOMPARE(YoudaoProtocol::mapLanguage(QString(), true), QStringLiteral("en"));
}

void YoudaoTranslateTests::verifiesParseResponseSuccessAndVariants()
{
    // 1. 标准成功形态（errorCode 为字符串 "0"，含 resRegions）
    const QByteArray jsonNormal = R"({
        "errorCode": "0",
        "resRegions": [
            {
                "boundingBox": "18,53,400,34",
                "lang": "en",
                "context": "Hello World",
                "tranContent": "你好世界"
            },
            {
                "boundingBox": "invalid_bbox",
                "lang": "en",
                "context": "Skip me",
                "tranContent": "跳过"
            },
            {
                "boundingBox": "10,20,30,40",
                "lang": "en",
                "context": "Second Line",
                "tranContent": "第二行"
            }
        ]
    })";

    YoudaoProtocol::Result res;
    QString errText;
    QVERIFY(YoudaoProtocol::parseResponse(jsonNormal, res, errText));
    QVERIFY(errText.isEmpty());
    QCOMPARE(res.regions.size(), 2);
    QCOMPARE(res.regions[0].box, QRect(18, 53, 400, 34));
    QCOMPARE(res.regions[0].lang, QStringLiteral("en"));
    QCOMPARE(res.regions[0].source, QStringLiteral("Hello World"));
    QCOMPARE(res.regions[0].target, QStringLiteral("你好世界"));
    QCOMPARE(res.regions[1].box, QRect(10, 20, 30, 40));
    QCOMPARE(res.regions[1].source, QStringLiteral("Second Line"));
    QVERIFY(res.rendered.isNull());

    // 2. errorCode 为数字形态 0
    const QByteArray jsonNumericCode = R"({
        "errorCode": 0,
        "resRegions": []
    })";
    QVERIFY(YoudaoProtocol::parseResponse(jsonNumericCode, res, errText));
    QCOMPARE(res.regions.size(), 0);

    // 3. 缺失 errorCode（视为成功）
    const QByteArray jsonMissingCode = R"({
        "resRegions": []
    })";
    QVERIFY(YoudaoProtocol::parseResponse(jsonMissingCode, res, errText));

    // 4. isRender 返回图片（image 为 base64）
    QImage sample(16, 16, QImage::Format_RGB32);
    sample.fill(Qt::blue);
    QByteArray imgBytes;
    QBuffer buf(&imgBytes);
    buf.open(QIODevice::WriteOnly);
    sample.save(&buf, "JPEG");
    const QString b64Img = QString::fromLatin1(imgBytes.toBase64());

    const QByteArray jsonWithImage = QStringLiteral(R"({
        "errorCode": "0",
        "image": "%1"
    })").arg(b64Img).toUtf8();

    QVERIFY(YoudaoProtocol::parseResponse(jsonWithImage, res, errText));
    QVERIFY(!res.rendered.isNull());
    QCOMPARE(res.rendered.size(), QSize(16, 16));

    // 5. 缺失 resRegions
    const QByteArray jsonNoRegions = R"({"errorCode": "0"})";
    QVERIFY(YoudaoProtocol::parseResponse(jsonNoRegions, res, errText));
    QCOMPARE(res.regions.size(), 0);
}

void YoudaoTranslateTests::verifiesParseResponseErrorCodes()
{
    YoudaoProtocol::Result res;
    QString errText;

    // errorCode 10 (无有效文字)
    QVERIFY(!YoudaoProtocol::parseResponse(R"({"errorCode": "10", "msg": "No text detected"})", res, errText));
    QVERIFY(errText.contains(QStringLiteral("errorCode=10")));
    QVERIFY(errText.contains(QStringLiteral("No text detected")));

    // errorCode 20 (翻译失败)
    QVERIFY(!YoudaoProtocol::parseResponse(R"({"errorCode": 20, "message": "Translation error"})", res, errText));
    QVERIFY(errText.contains(QStringLiteral("errorCode=20")));
    QVERIFY(errText.contains(QStringLiteral("Translation error")));

    // errorCode 30 (签名或鉴权失败)
    QVERIFY(!YoudaoProtocol::parseResponse(R"({"errorCode": "30"})", res, errText));
    QCOMPARE(errText, QStringLiteral("errorCode=30"));

    // errorCode 40 (语言不支持)
    QVERIFY(!YoudaoProtocol::parseResponse(R"({"errorCode": "40"})", res, errText));
    QCOMPARE(errText, QStringLiteral("errorCode=40"));

    // errorCode 60 (图片为空或压缩失败)
    QVERIFY(!YoudaoProtocol::parseResponse(R"({"errorCode": "60"})", res, errText));
    QCOMPARE(errText, QStringLiteral("errorCode=60"));

    // 畸形 JSON
    QVERIFY(!YoudaoProtocol::parseResponse("not a valid json", res, errText));
    QVERIFY(errText.contains(QStringLiteral("JSON 解析失败")));
}

void YoudaoTranslateTests::verifiesScaledBox()
{
    // 整数倍
    QCOMPARE(YoudaoProtocol::scaledBox(QRect(10, 20, 100, 200), 2.0), QRect(20, 40, 200, 400));
    QCOMPARE(YoudaoProtocol::scaledBox(QRect(20, 40, 200, 400), 0.5), QRect(10, 20, 100, 200));

    // 非整数倍四舍五入: 10 * 1.33 = 13.3 -> 13; 15 * 1.35 = 20.25 -> 20; 15 * 1.37 = 20.55 -> 21
    QCOMPARE(YoudaoProtocol::scaledBox(QRect(10, 15, 30, 45), 1.37), QRect(14, 21, 41, 62));

    // 越界与负数夹紧
    QCOMPARE(YoudaoProtocol::scaledBox(QRect(-10, -5, 50, 60), 1.0), QRect(0, 0, 50, 60));
    QCOMPARE(YoudaoProtocol::scaledBox(QRect(10, 20, -30, -40), 1.0), QRect(10, 20, 0, 0));
    QCOMPARE(YoudaoProtocol::scaledBox(QRect(-50, -50, -10, -10), 2.0), QRect(0, 0, 0, 0));

    // 空 box
    QCOMPARE(YoudaoProtocol::scaledBox(QRect(), 1.5), QRect(0, 0, 0, 0));
}

void YoudaoTranslateTests::verifiesEncodeUpload()
{
    // 1. 空图安全检查
    double scale = 0.0;
    QString mime;
    QByteArray bytes = YoudaoProtocol::encodeUpload(QImage(), scale, mime);
    QVERIFY(bytes.isEmpty());
    QCOMPARE(scale, 1.0);
    QCOMPARE(mime, QStringLiteral("image/png"));

    // 2. 超大图（4000x3000）缩放到最长边 2000
    QImage large(4000, 3000, QImage::Format_RGB32);
    large.fill(Qt::white);
    bytes = YoudaoProtocol::encodeUpload(large, scale, mime);
    QVERIFY(!bytes.isEmpty());
    QCOMPARE(scale, 0.5); // 2000 / 4000 = 0.5
    QCOMPARE(mime, QStringLiteral("image/jpeg"));
    QImage decodedLarge;
    QVERIFY(decodedLarge.loadFromData(bytes, "JPEG"));
    QCOMPARE(decodedLarge.width(), 2000);
    QCOMPARE(decodedLarge.height(), 1500);

    // 3. 小图无 alpha 通道（Format_RGB32）：不缩放，JPEG
    QImage smallRgb(800, 600, QImage::Format_RGB32);
    smallRgb.fill(Qt::red);
    bytes = YoudaoProtocol::encodeUpload(smallRgb, scale, mime);
    QVERIFY(!bytes.isEmpty());
    QCOMPARE(scale, 1.0);
    QCOMPARE(mime, QStringLiteral("image/jpeg"));
    QImage decodedRgb;
    QVERIFY(decodedRgb.loadFromData(bytes, "JPEG"));
    QCOMPARE(decodedRgb.size(), QSize(800, 600));

    // 4. 小图有 alpha 通道（Format_ARGB32 且未缩放）：PNG
    QImage smallArgb(400, 300, QImage::Format_ARGB32_Premultiplied);
    smallArgb.fill(Qt::transparent);
    bytes = YoudaoProtocol::encodeUpload(smallArgb, scale, mime);
    QVERIFY(!bytes.isEmpty());
    QCOMPARE(scale, 1.0);
    QCOMPARE(mime, QStringLiteral("image/png"));
    QImage decodedArgb;
    QVERIFY(decodedArgb.loadFromData(bytes, "PNG"));
    QCOMPARE(decodedArgb.size(), QSize(400, 300));
}

void YoudaoTranslateTests::verifiesYoudaoTranslateEmptyImage()
{
    YoudaoTranslate yt;
    QSignalSpy failedSpy(&yt, &YoudaoTranslate::failed);
    QSignalSpy finishedSpy(&yt, &YoudaoTranslate::finished);

    yt.translate(QImage(), QStringLiteral("auto"), QStringLiteral("zh-CHS"), false);

    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 0);
    const QList<QVariant> args = failedSpy.takeFirst();
    QCOMPARE(args.at(1).toInt(), 60);
}

void YoudaoTranslateTests::verifiesYoudaoTranslateCancelSafety()
{
    YoudaoTranslate yt;
    // 指向必然拒绝连接的本地端口，保证这个用例不发真实请求
    yt.setEndpoint(QStringLiteral("http://127.0.0.1:1/"));
    // 多次安全 cancel
    yt.cancel();
    yt.cancel();

    QImage img(100, 100, QImage::Format_RGB32);
    img.fill(Qt::black);
    yt.translate(img, QStringLiteral("auto"), QStringLiteral("zh-CHS"), false);
    yt.cancel();
    yt.cancel();
}

void YoudaoTranslateTests::verifiesYoudaoTranslateNetworkFailure()
{
    YoudaoTranslate yt;
    // 用例必须与网络环境无关：指向本机必然拒绝连接的端口，
    // 无论宿主机是否联网都会得到确定性的连接失败。
    yt.setEndpoint(QStringLiteral("http://127.0.0.1:1/"));
    QSignalSpy failedSpy(&yt, &YoudaoTranslate::failed);
    QSignalSpy finishedSpy(&yt, &YoudaoTranslate::finished);

    QImage img(100, 100, QImage::Format_RGB32);
    img.fill(Qt::white);
    yt.translate(img, QStringLiteral("en"), QStringLiteral("zh-CHS"), false);

    QVERIFY(failedSpy.wait(5000));
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 0);

    const QList<QVariant> args = failedSpy.first();
    // 断网或网络错误时 errorCode 为 -1
    QCOMPARE(args.at(1).toInt(), -1);
    QVERIFY(!args.at(0).toString().isEmpty());
}

QTEST_MAIN(YoudaoTranslateTests)

#include "YoudaoTranslateTests.moc"
