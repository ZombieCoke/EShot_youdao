#ifndef OCRDIALOG_H
#define OCRDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QString>
#include <QTextEdit>
#include <QVBoxLayout>

#include "core/YoudaoProtocol.h"

class OcrEngine;
class YoudaoTranslate;

class OcrDialog : public QDialog {
    Q_OBJECT

public:
    explicit OcrDialog(const QPixmap &pixmap, QWidget *parent = nullptr);
    ~OcrDialog() override;

    void setLanguageTag(const QString &tag);
    bool hasPinnedImage() const;
    QImage pinnedImage() const;

private slots:
    void onTextReady(const QString &text);
    void onOcrFailed(const QString &reason);
    void onYoudaoFinished(const YoudaoProtocol::Result &result);
    void onYoudaoFailed(const QString &reason, int errorCode);
    void onCopyClicked();
    void onTranslateClicked();
    void onRetryClicked();
    void onLanguageChanged(int index);
    void onEngineChanged(int index);
    void onCopyAllSourceClicked();
    void onCopyAllTargetClicked();
    void onPinInPlaceClicked();

private:
    void setBusy(bool busy);
    void translateUi();
    void updateEngineUi();
    // 有道要上传选区，第一次用之前必须让用户明确同意。
    bool ensureYoudaoConsent();
    // 当前选中的翻译目标语言（有道代码）；未选择时跟随界面语言。
    QString youdaoTargetCode() const;
    void runOcr();
    void runTesseract();
    void runYoudao();
    void requestOcr();
    void finishOcrRun();
    void populateLanguages();
    bool isLanguageInstalled(const QString &tag) const;
    int firstInstalledLanguageIndex() const;
    void renderYoudaoCards(const YoudaoProtocol::Result &result);

    QPixmap m_pixmap;
    OcrEngine *m_engine = nullptr;
    YoudaoTranslate *m_youdao = nullptr;
    QString m_languageTag = "auto";
    QString m_preferredLanguageTag = "en";
    QString m_engineType = "tesseract";
    bool m_initialRunPending = true;
    bool m_ocrRunning = false;
    bool m_rerunPending = false;
    QImage m_pinnedImage;
    YoudaoProtocol::Result m_lastYoudaoResult;

    QLabel *m_langLabel;
    QComboBox *m_langCombo;
    QLabel *m_engineLabel;
    QComboBox *m_engineCombo;
    QLabel *m_targetLabel = nullptr;
    QComboBox *m_targetCombo = nullptr;
    QLabel *m_statusLabel;
    QStackedWidget *m_stack;
    QTextEdit *m_textEdit;
    QScrollArea *m_scrollArea;
    QWidget *m_cardContainer;
    QVBoxLayout *m_cardLayout;

    // Tesseract 按钮
    QPushButton *m_copyBtn;
    QPushButton *m_translateBtn;
    QPushButton *m_retryBtn;

    // 有道按钮
    QPushButton *m_copyAllSourceBtn;
    QPushButton *m_copyAllTargetBtn;
    QPushButton *m_pinBtn;
    QPushButton *m_retranslateBtn;

    QPushButton *m_closeBtn;
};

#endif
