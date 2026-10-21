#include "OcrDialog.h"
#include "core/OcrEngine.h"
#include "core/OcrLanguageSelector.h"
#include "core/YoudaoProtocol.h"
#include "core/YoudaoTranslate.h"
#include "core/TranslationManager.h"
#include <QClipboard>
#include <QColor>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>

namespace {
constexpr int LanguageInstalledRole = Qt::UserRole + 1;
}

OcrDialog::OcrDialog(const QPixmap &pixmap, QWidget *parent)
    : QDialog(parent), m_pixmap(pixmap)
{
    setWindowTitle(TranslationManager::ocrTitle());
    setWindowIcon(QIcon(":/icons/pen.svg"));
    resize(560, 460);

    m_engine = new OcrEngine(this);
    connect(m_engine, &OcrEngine::textReady, this, &OcrDialog::onTextReady);
    connect(m_engine, &OcrEngine::failed, this, &OcrDialog::onOcrFailed);

    m_youdao = new YoudaoTranslate(this);
    connect(m_youdao, &YoudaoTranslate::finished, this, &OcrDialog::onYoudaoFinished);
    connect(m_youdao, &YoudaoTranslate::failed, this, &OcrDialog::onYoudaoFailed);

    QSettings settings(QStringLiteral("EShot"), QStringLiteral("EShot"));
    m_languageTag = settings.value(QStringLiteral("ocrLanguage"), m_languageTag).toString();
    m_preferredLanguageTag = settings.value(
        QStringLiteral("ocrPreferredLanguage"), TranslationManager::langCode()).toString();
    m_engineType = settings.value(QStringLiteral("ocrEngine"), QStringLiteral("tesseract")).toString();
    const QString customSecret = settings.value(QStringLiteral("youdaoSecret")).toString();
    if (!customSecret.trimmed().isEmpty()) {
        m_youdao->setSecret(customSecret);
    }

    connect(m_engine, &OcrEngine::languageResolved, this, [this](const QString &languages) {
        if (m_languageTag != QStringLiteral("auto"))
            return;
        const int automaticIndex = m_langCombo ? m_langCombo->findData(QStringLiteral("auto")) : -1;
        if (automaticIndex >= 0) {
            m_langCombo->setItemText(
                automaticIndex,
                QStringLiteral("%1 (%2)").arg(TranslationManager::ocrAutomatic(), languages));
        }
    });

    auto *layout = new QVBoxLayout(this);

    // 语言行
    auto *langRow = new QHBoxLayout();
    m_langLabel = new QLabel(QStringLiteral("%1:").arg(TranslationManager::language()), this);
    m_langCombo = new QComboBox(this);
    populateLanguages();
    langRow->addWidget(m_langLabel);
    langRow->addWidget(m_langCombo);
    langRow->addStretch();
    layout->addLayout(langRow);

    int idx = m_langCombo->findData(m_languageTag);
    if (idx < 0 || !m_langCombo->itemData(idx, LanguageInstalledRole).toBool()) {
        idx = firstInstalledLanguageIndex();
    }
    if (idx >= 0) {
        m_langCombo->setCurrentIndex(idx);
        m_languageTag = m_langCombo->currentData().toString();
    }
    connect(m_langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OcrDialog::onLanguageChanged);

    // 引擎行
    auto *engineRow = new QHBoxLayout();
    m_engineLabel = new QLabel(QStringLiteral("%1:").arg(TranslationManager::ocrEngine()), this);
    m_engineCombo = new QComboBox(this);
    m_engineCombo->addItem(TranslationManager::ocrEngineLocal(), QStringLiteral("tesseract"));
    m_engineCombo->addItem(TranslationManager::ocrEngineYoudao(), QStringLiteral("youdao"));
    const int initialEngineIdx = m_engineCombo->findData(m_engineType);
    if (initialEngineIdx >= 0) {
        m_engineCombo->setCurrentIndex(initialEngineIdx);
    }
    connect(m_engineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OcrDialog::onEngineChanged);
    engineRow->addWidget(m_engineLabel);
    engineRow->addWidget(m_engineCombo);
    engineRow->addStretch();
    layout->addLayout(engineRow);

    m_statusLabel = new QLabel(TranslationManager::ocrProcessing(), this);
    layout->addWidget(m_statusLabel);

    // 结果区堆叠窗口
    m_stack = new QStackedWidget(this);

    // 页面 0: Tesseract (QTextEdit)
    m_textEdit = new QTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setPlaceholderText(TranslationManager::ocrEmpty());
    m_stack->addWidget(m_textEdit);

    // 页面 1: 有道卡片列表 (QScrollArea)
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_cardContainer = new QWidget(m_scrollArea);
    m_cardLayout = new QVBoxLayout(m_cardContainer);
    m_cardLayout->setContentsMargins(4, 4, 4, 4);
    m_cardLayout->setSpacing(8);
    m_cardContainer->setLayout(m_cardLayout);
    m_scrollArea->setWidget(m_cardContainer);
    m_stack->addWidget(m_scrollArea);

    layout->addWidget(m_stack, 1);

    // 底部按钮区
    auto *btnRow = new QHBoxLayout();

    // Tesseract 按钮
    m_copyBtn = new QPushButton(TranslationManager::ocrCopy(), this);
    m_copyBtn->setEnabled(false);
    m_translateBtn = new QPushButton(TranslationManager::tr("ocrTranslate"), this);
    m_translateBtn->setIcon(QIcon(QStringLiteral(":/icons/external_link.svg")));
    m_translateBtn->setIconSize(QSize(16, 16));
    m_translateBtn->setToolTip(TranslationManager::tr("ocrTranslate"));
    m_translateBtn->setEnabled(false);
    m_retryBtn = new QPushButton(TranslationManager::ocrRetry(), this);
    m_retryBtn->setEnabled(false);

    // 有道按钮
    m_copyAllSourceBtn = new QPushButton(TranslationManager::ocrCopyAllSource(), this);
    m_copyAllSourceBtn->setEnabled(false);
    m_copyAllTargetBtn = new QPushButton(TranslationManager::ocrCopyAllTarget(), this);
    m_copyAllTargetBtn->setEnabled(false);
    m_pinBtn = new QPushButton(TranslationManager::ocrPinInPlace(), this);
    m_pinBtn->setEnabled(false);
    m_retranslateBtn = new QPushButton(TranslationManager::ocrRetry(), this);
    m_retranslateBtn->setEnabled(false);

    m_closeBtn = new QPushButton(TranslationManager::ocrClose(), this);

    const QList<QPushButton *> actionButtons = {
        m_copyBtn, m_translateBtn, m_retryBtn,
        m_copyAllSourceBtn, m_copyAllTargetBtn, m_pinBtn, m_retranslateBtn,
        m_closeBtn
    };
    for (QPushButton *button : actionButtons) {
        button->setFixedHeight(30);
        button->setStyleSheet(QStringLiteral("QPushButton { padding: 0 10px; }"));
    }

    // 将按钮按顺序加入布局
    btnRow->addWidget(m_copyBtn);
    btnRow->addWidget(m_translateBtn);
    btnRow->addWidget(m_retryBtn);

    btnRow->addWidget(m_copyAllSourceBtn);
    btnRow->addWidget(m_copyAllTargetBtn);
    btnRow->addWidget(m_pinBtn);
    btnRow->addWidget(m_retranslateBtn);

    btnRow->addStretch();
    btnRow->addWidget(m_closeBtn);
    layout->addLayout(btnRow);

    connect(m_copyBtn, &QPushButton::clicked, this, &OcrDialog::onCopyClicked);
    connect(m_translateBtn, &QPushButton::clicked, this, &OcrDialog::onTranslateClicked);
    connect(m_retryBtn, &QPushButton::clicked, this, &OcrDialog::onRetryClicked);

    connect(m_copyAllSourceBtn, &QPushButton::clicked, this, &OcrDialog::onCopyAllSourceClicked);
    connect(m_copyAllTargetBtn, &QPushButton::clicked, this, &OcrDialog::onCopyAllTargetClicked);
    connect(m_pinBtn, &QPushButton::clicked, this, &OcrDialog::onPinInPlaceClicked);
    connect(m_retranslateBtn, &QPushButton::clicked, this, &OcrDialog::onRetryClicked);

    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    updateEngineUi();

    // Defer the first run to the event loop so a setLanguageTag() call made
    // right after construction (before exec()) decides the language, and OCR
    // runs exactly once with it.
    setBusy(true);
    QTimer::singleShot(0, this, [this]() {
        m_initialRunPending = false;
        runOcr();
    });
}

OcrDialog::~OcrDialog()
{
    if (m_youdao) {
        m_youdao->cancel();
    }
}

bool OcrDialog::hasPinnedImage() const
{
    return !m_pinnedImage.isNull();
}

QImage OcrDialog::pinnedImage() const
{
    return m_pinnedImage;
}

void OcrDialog::setLanguageTag(const QString &tag)
{
    if (tag.isEmpty()) return;
    if (m_langCombo) {
        const int idx = m_langCombo->findData(tag);
        if (idx >= 0 && m_langCombo->itemData(idx, LanguageInstalledRole).toBool()
            && tag != m_languageTag) {
            m_languageTag = tag;
            {
                const QSignalBlocker blocker(m_langCombo);
                m_langCombo->setCurrentIndex(idx);
            }
            // Before the deferred first run the new tag is simply picked up.
            if (!m_initialRunPending)
                requestOcr();
        }
    }
}

void OcrDialog::populateLanguages()
{
    struct LanguageItem {
        const char *label;
        const char *tag;
    };

    const LanguageItem languages[] = {
        {"English", "en-US"},
        {"T\303\274rk\303\247e", "tr-TR"},
        {"\320\240\321\203\321\201\321\201\320\272\320\270\320\271", "ru-RU"},
        {"Deutsch", "de-DE"},
        {"Fran\303\247ais", "fr-FR"},
        {"Espa\303\261ol", "es-ES"},
        {"Italiano", "it-IT"},
        {"Portugu\303\252s", "pt-BR"},
        {"Polski", "pl-PL"},
        {"Nederlands", "nl-NL"},
        {"\346\227\245\346\234\254\350\252\236", "ja-JP"},
        {"\355\225\234\352\265\255\354\226\264", "ko-KR"},
        {"\347\256\200\344\275\223\344\270\255\346\226\207", "zh-CN"},
    };

    const QString missingTip = TranslationManager::ocrLanguagePackMissing();

    const bool automaticAvailable = !installedOcrLanguageCodes(OcrEngine::tessdataDir()).isEmpty();
    m_langCombo->addItem(TranslationManager::ocrAutomatic(), QStringLiteral("auto"));
    m_langCombo->setItemData(0, automaticAvailable, LanguageInstalledRole);
    if (!automaticAvailable) {
        auto *model = qobject_cast<QStandardItemModel *>(m_langCombo->model());
        if (QStandardItem *item = model ? model->item(0) : nullptr) {
            item->setEnabled(false);
            item->setForeground(QColor(145, 145, 145));
            item->setToolTip(missingTip);
        }
    }

    for (const auto &language : languages) {
        const QString tag = QString::fromLatin1(language.tag);
        const bool installed = isLanguageInstalled(tag);
        m_langCombo->addItem(QString::fromUtf8(language.label), tag);
        const int row = m_langCombo->count() - 1;
        m_langCombo->setItemData(row, installed, LanguageInstalledRole);

        auto *model = qobject_cast<QStandardItemModel *>(m_langCombo->model());
        QStandardItem *item = model ? model->item(row) : nullptr;
        if (!installed && item) {
            item->setEnabled(false);
            item->setForeground(QColor(145, 145, 145));
            item->setToolTip(missingTip);
        }
    }
}

bool OcrDialog::isLanguageInstalled(const QString &tag) const
{
    const QString mapped = OcrEngine::mapLanguageTag(tag);
    if (mapped.isEmpty()) {
        return false;
    }
    return QFileInfo::exists(QDir(OcrEngine::tessdataDir()).filePath(mapped + QStringLiteral(".traineddata")));
}

int OcrDialog::firstInstalledLanguageIndex() const
{
    for (int i = 0; i < m_langCombo->count(); ++i) {
        if (m_langCombo->itemData(i, LanguageInstalledRole).toBool()) {
            return i;
        }
    }
    return -1;
}

void OcrDialog::translateUi()
{
    setWindowTitle(TranslationManager::ocrTitle());
    m_langLabel->setText(QStringLiteral("%1:").arg(TranslationManager::language()));
    m_engineLabel->setText(QStringLiteral("%1:").arg(TranslationManager::ocrEngine()));
    m_copyBtn->setText(TranslationManager::ocrCopy());
    m_translateBtn->setText(TranslationManager::tr("ocrTranslate"));
    m_retryBtn->setText(TranslationManager::ocrRetry());
    m_copyAllSourceBtn->setText(TranslationManager::ocrCopyAllSource());
    m_copyAllTargetBtn->setText(TranslationManager::ocrCopyAllTarget());
    m_pinBtn->setText(TranslationManager::ocrPinInPlace());
    m_retranslateBtn->setText(TranslationManager::ocrRetry());
    m_closeBtn->setText(TranslationManager::ocrClose());
}

void OcrDialog::updateEngineUi()
{
    const bool isYoudao = (m_engineType == QStringLiteral("youdao"));
    m_stack->setCurrentIndex(isYoudao ? 1 : 0);

    m_copyBtn->setVisible(!isYoudao);
    m_translateBtn->setVisible(!isYoudao);
    m_retryBtn->setVisible(!isYoudao);

    m_copyAllSourceBtn->setVisible(isYoudao);
    m_copyAllTargetBtn->setVisible(isYoudao);
    m_pinBtn->setVisible(isYoudao);
    m_retranslateBtn->setVisible(isYoudao);
}

void OcrDialog::setBusy(bool busy)
{
    m_retryBtn->setEnabled(!busy);
    m_retranslateBtn->setEnabled(!busy);
    m_langCombo->setEnabled(!busy);
    m_engineCombo->setEnabled(!busy);
    if (busy) {
        if (m_engineType == QStringLiteral("youdao")) {
            m_statusLabel->setText(TranslationManager::ocrYoudaoBusy());
        } else {
            m_statusLabel->setText(TranslationManager::ocrProcessing());
        }
        m_copyBtn->setEnabled(false);
        m_translateBtn->setEnabled(false);
        m_copyAllSourceBtn->setEnabled(false);
        m_copyAllTargetBtn->setEnabled(false);
        m_pinBtn->setEnabled(false);
    }
}

void OcrDialog::requestOcr()
{
    if (m_ocrRunning) {
        // Recognition cannot be restarted mid-run; redo it once it finishes.
        m_rerunPending = true;
        return;
    }
    runOcr();
}

void OcrDialog::finishOcrRun()
{
    m_ocrRunning = false;
    if (m_rerunPending) {
        m_rerunPending = false;
        runOcr();
    }
}

void OcrDialog::runOcr()
{
    m_ocrRunning = true;
    setBusy(true);
    if (m_engineType == QStringLiteral("youdao")) {
        runYoudao();
    } else {
        runTesseract();
    }
}

void OcrDialog::runTesseract()
{
    m_textEdit->clear();
    m_copyBtn->setEnabled(false);
    m_translateBtn->setEnabled(false);
    m_engine->recognize(m_pixmap, m_languageTag, m_preferredLanguageTag);
}

void OcrDialog::runYoudao()
{
    // 清理卡片区域
    while (QLayoutItem *item = m_cardLayout->takeAt(0)) {
        if (QWidget *w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    m_lastYoudaoResult = YoudaoProtocol::Result();
    m_pinnedImage = QImage();

    QString eshotTag = m_languageTag;
    if (eshotTag != QStringLiteral("auto")) {
        eshotTag = OcrEngine::mapLanguageTag(m_languageTag);
    }
    const QString from = YoudaoProtocol::mapLanguage(eshotTag, true);

    const QString code = TranslationManager::langCode();
    const QString to = YoudaoProtocol::mapLanguage(code, false);

    m_youdao->translate(m_pixmap.toImage(), from, to, true);
}

void OcrDialog::onLanguageChanged(int index)
{
    if (index < 0 || !m_langCombo->itemData(index, LanguageInstalledRole).toBool()) {
        return;
    }
    QString tag = m_langCombo->currentData().toString();
    if (!tag.isEmpty() && tag != m_languageTag) {
        m_languageTag = tag;
        QSettings settings(QStringLiteral("EShot"), QStringLiteral("EShot"));
        settings.setValue(QStringLiteral("ocrLanguage"), tag);
        if (tag != QStringLiteral("auto")) {
            m_preferredLanguageTag = tag;
            settings.setValue(QStringLiteral("ocrPreferredLanguage"), tag);
        }
        if (!m_initialRunPending)
            requestOcr();
    }
}

void OcrDialog::onEngineChanged(int index)
{
    if (index < 0) return;
    const QString newEngine = m_engineCombo->itemData(index).toString();
    if (newEngine == m_engineType) return;

    if (newEngine == QStringLiteral("youdao")) {
        QSettings settings(QStringLiteral("EShot"), QStringLiteral("EShot"));
        const bool accepted = settings.value(QStringLiteral("youdaoNoticeAccepted"), false).toBool();
        if (!accepted) {
            const auto reply = QMessageBox::question(
                this,
                TranslationManager::ocrYoudaoNoticeTitle(),
                TranslationManager::ocrYoudaoNoticeBody(),
                QMessageBox::Ok | QMessageBox::Cancel
            );
            if (reply != QMessageBox::Ok) {
                const QSignalBlocker blocker(m_engineCombo);
                const int tessIdx = m_engineCombo->findData(QStringLiteral("tesseract"));
                if (tessIdx >= 0) {
                    m_engineCombo->setCurrentIndex(tessIdx);
                }
                return;
            }
            settings.setValue(QStringLiteral("youdaoNoticeAccepted"), true);
        }
    }

    m_engineType = newEngine;
    QSettings settings(QStringLiteral("EShot"), QStringLiteral("EShot"));
    settings.setValue(QStringLiteral("ocrEngine"), m_engineType);

    updateEngineUi();
    if (!m_initialRunPending) {
        requestOcr();
    }
}

void OcrDialog::onTextReady(const QString &text)
{
    if (m_rerunPending) {
        finishOcrRun();
        return;
    }
    m_ocrRunning = false;
    setBusy(false);
    if (text.trimmed().isEmpty()) {
        m_statusLabel->setText(TranslationManager::ocrEmpty());
        m_textEdit->clear();
        m_copyBtn->setEnabled(false);
        m_translateBtn->setEnabled(false);
    } else {
        m_statusLabel->setText(TranslationManager::ocrTitle());
        m_textEdit->setPlainText(text);
        m_copyBtn->setEnabled(true);
        m_translateBtn->setEnabled(true);
    }
}

void OcrDialog::onOcrFailed(const QString &reason)
{
    if (m_rerunPending) {
        finishOcrRun();
        return;
    }
    m_ocrRunning = false;
    setBusy(false);
    m_statusLabel->setText(TranslationManager::ocrFailed() + QStringLiteral(" - ") + reason);
    m_textEdit->clear();
    m_copyBtn->setEnabled(false);
    m_translateBtn->setEnabled(false);
    m_retryBtn->setEnabled(true);
    qWarning() << "[EShot] OCR failed:" << reason;
}

void OcrDialog::onYoudaoFinished(const YoudaoProtocol::Result &result)
{
    if (m_rerunPending) {
        finishOcrRun();
        return;
    }
    m_ocrRunning = false;
    setBusy(false);
    m_lastYoudaoResult = result;
    // 服务端 bbox 用的是上传图像素坐标，换算回对话框里这张原图的像素坐标，
    // 这样后续任何按框绘制的用法都不用再关心缩放。
    if (result.uploadScale > 0.0 && result.uploadScale < 0.999) {
        const double inv = 1.0 / result.uploadScale;
        for (auto &reg : m_lastYoudaoResult.regions) {
            reg.box = YoudaoProtocol::scaledBox(reg.box, inv);
        }
    }
    m_statusLabel->setText(QStringLiteral("%1 (%2 ms)").arg(
        TranslationManager::ocrTitle(),
        QString::number(result.elapsedMs)));
    renderYoudaoCards(result);
}

void OcrDialog::onYoudaoFailed(const QString &reason, int errorCode)
{
    if (m_rerunPending) {
        finishOcrRun();
        return;
    }
    m_ocrRunning = false;
    setBusy(false);

    QString errorMsg;
    if (errorCode == 10) {
        errorMsg = TranslationManager::ocrYoudaoNoText();
    } else if (errorCode == 20) {
        errorMsg = TranslationManager::ocrYoudaoTranslateFailed();
    } else if (errorCode == 30) {
        errorMsg = TranslationManager::ocrYoudaoAuthFailed();
    } else if (errorCode == 40) {
        errorMsg = TranslationManager::ocrYoudaoLangUnsupported();
    } else if (errorCode == 60) {
        errorMsg = TranslationManager::ocrYoudaoImageFailed();
    } else if (errorCode == -1) {
        errorMsg = QStringLiteral("%1: %2").arg(TranslationManager::ocrYoudaoNetworkFailed(), reason);
    } else {
        errorMsg = QStringLiteral("%1: %2").arg(TranslationManager::ocrFailed(), reason);
    }
    m_statusLabel->setText(errorMsg);
    qWarning() << "[YoudaoTranslate] failed:" << errorCode << reason;

    YoudaoProtocol::Result emptyRes;
    renderYoudaoCards(emptyRes);
    m_retranslateBtn->setEnabled(true);
}

void OcrDialog::renderYoudaoCards(const YoudaoProtocol::Result &result)
{
    while (QLayoutItem *item = m_cardLayout->takeAt(0)) {
        if (QWidget *w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }

    if (result.regions.isEmpty()) {
        QString tip;
        if (!result.rendered.isNull()) {
            tip = TranslationManager::ocrYoudaoRenderedOnly();
        } else {
            tip = TranslationManager::ocrYoudaoNoText();
        }
        auto *emptyLabel = new QLabel(tip, m_cardContainer);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet(QStringLiteral("color: #888888; padding: 20px;"));
        m_cardLayout->addWidget(emptyLabel);
        m_cardLayout->addStretch();

        m_copyAllSourceBtn->setEnabled(false);
        m_copyAllTargetBtn->setEnabled(false);
        m_pinBtn->setEnabled(!result.rendered.isNull());
        m_retranslateBtn->setEnabled(true);
        return;
    }

    for (int i = 0; i < result.regions.size(); ++i) {
        const auto &reg = result.regions.at(i);
        auto *card = new QFrame(m_cardContainer);
        card->setFrameShape(QFrame::StyledPanel);
        card->setStyleSheet(QStringLiteral(
            "QFrame { background-color: rgba(128, 128, 128, 0.08); border: 1px solid rgba(128, 128, 128, 0.2); border-radius: 6px; }"));

        auto *cLayout = new QVBoxLayout(card);
        cLayout->setContentsMargins(10, 8, 10, 8);
        cLayout->setSpacing(6);

        // 头部行：序号与语言
        auto *headLabel = new QLabel(QStringLiteral("#%1  %2").arg(i + 1).arg(reg.lang), card);
        headLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #888888;"));
        cLayout->addWidget(headLabel);

        // 原文行
        auto *srcRow = new QHBoxLayout();
        auto *srcTitle = new QLabel(QStringLiteral("%1:").arg(TranslationManager::ocrSourceText()), card);
        srcTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
        auto *srcContent = new QLabel(reg.source, card);
        srcContent->setWordWrap(true);
        srcContent->setTextInteractionFlags(Qt::TextSelectableByMouse);
        auto *copySrcBtn = new QPushButton(TranslationManager::ocrCopySource(), card);
        copySrcBtn->setFixedHeight(24);
        copySrcBtn->setStyleSheet(QStringLiteral("QPushButton { padding: 0 6px; font-size: 11px; }"));
        connect(copySrcBtn, &QPushButton::clicked, this, [this, src = reg.source]() {
            QGuiApplication::clipboard()->setText(src);
            m_statusLabel->setText(TranslationManager::ocrCopied());
        });
        srcRow->addWidget(srcTitle);
        srcRow->addWidget(srcContent, 1);
        srcRow->addWidget(copySrcBtn);
        cLayout->addLayout(srcRow);

        // 译文行
        auto *tgtRow = new QHBoxLayout();
        auto *tgtTitle = new QLabel(QStringLiteral("%1:").arg(TranslationManager::ocrTargetText()), card);
        tgtTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
        auto *tgtContent = new QLabel(reg.target, card);
        tgtContent->setWordWrap(true);
        tgtContent->setTextInteractionFlags(Qt::TextSelectableByMouse);
        auto *copyTgtBtn = new QPushButton(TranslationManager::ocrCopyTarget(), card);
        copyTgtBtn->setFixedHeight(24);
        copyTgtBtn->setStyleSheet(QStringLiteral("QPushButton { padding: 0 6px; font-size: 11px; }"));
        connect(copyTgtBtn, &QPushButton::clicked, this, [this, tgt = reg.target]() {
            QGuiApplication::clipboard()->setText(tgt);
            m_statusLabel->setText(TranslationManager::ocrCopied());
        });
        tgtRow->addWidget(tgtTitle);
        tgtRow->addWidget(tgtContent, 1);
        tgtRow->addWidget(copyTgtBtn);
        cLayout->addLayout(tgtRow);

        m_cardLayout->addWidget(card);
    }

    m_cardLayout->addStretch();
    m_copyAllSourceBtn->setEnabled(true);
    m_copyAllTargetBtn->setEnabled(true);
    m_pinBtn->setEnabled(!result.rendered.isNull());
    m_retranslateBtn->setEnabled(true);
}

void OcrDialog::onCopyClicked()
{
    QGuiApplication::clipboard()->setText(m_textEdit->toPlainText());
    m_statusLabel->setText(TranslationManager::ocrCopied());
}

void OcrDialog::onCopyAllSourceClicked()
{
    QStringList lines;
    for (const auto &reg : m_lastYoudaoResult.regions) {
        if (!reg.source.isEmpty())
            lines.append(reg.source);
    }
    if (!lines.isEmpty()) {
        QGuiApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
        m_statusLabel->setText(TranslationManager::ocrCopied());
    }
}

void OcrDialog::onCopyAllTargetClicked()
{
    QStringList lines;
    for (const auto &reg : m_lastYoudaoResult.regions) {
        if (!reg.target.isEmpty())
            lines.append(reg.target);
    }
    if (!lines.isEmpty()) {
        QGuiApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
        m_statusLabel->setText(TranslationManager::ocrCopied());
    }
}

void OcrDialog::onPinInPlaceClicked()
{
    if (!m_lastYoudaoResult.rendered.isNull()) {
        // 上传前可能缩过图，服务端渲染图是上传尺寸；贴回原位必须和选区同尺寸，
        // 否则大选区贴出来会明显变小。
        QImage rendered = m_lastYoudaoResult.rendered;
        if (!m_pixmap.isNull() && rendered.size() != m_pixmap.size()) {
            rendered = rendered.scaled(m_pixmap.size(), Qt::IgnoreAspectRatio,
                                       Qt::SmoothTransformation);
        }
        m_pinnedImage = rendered;
        accept();
    }
}

void OcrDialog::onTranslateClicked()
{
    const QString text = m_textEdit->toPlainText().trimmed();
    if (text.isEmpty())
        return;

    QUrl url(QStringLiteral("https://translate.google.com/"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("sl"), QStringLiteral("auto"));
    query.addQueryItem(QStringLiteral("tl"), TranslationManager::langCode());
    // QUrlQuery leaves '+' (and similar) literal, which the server reads as a
    // space; percent-encode the free-form text explicitly.
    query.addQueryItem(QStringLiteral("text"),
                       QString::fromLatin1(QUrl::toPercentEncoding(text)));
    query.addQueryItem(QStringLiteral("op"), QStringLiteral("translate"));
    url.setQuery(query.query(QUrl::FullyEncoded), QUrl::StrictMode);
    if (!QDesktopServices::openUrl(url)) {
        QMessageBox::warning(this, TranslationManager::tr("visualSearchBrowserLaunchTitle"),
                             TranslationManager::tr("ocrTranslateBrowserError"));
    }
}

void OcrDialog::onRetryClicked()
{
    runOcr();
}
