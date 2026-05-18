#pragma once

#include <QObject>
#include <QImage>
#include <QString>
#include <QFuture>
#include <QtQml/qqmlregistration.h>
#include <memory>

class VLMEngine;

// QObject wrapper that exposes VLMEngine to QML
// QML usage: vlmBridge.sendChat("prompt", imageBase64)
//            handle vlmBridge.responseReceived signal
class VLMBridge : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool processing     READ isProcessing     NOTIFY processingChanged)
    Q_PROPERTY(bool ollamaReady    READ isOllamaReady    NOTIFY ollamaReadyChanged)
    Q_PROPERTY(bool translateReady READ isTranslateReady NOTIFY translateReadyChanged)
    Q_PROPERTY(bool useOllamaTranslate READ useOllamaTranslate NOTIFY useOllamaTranslateChanged)
    Q_PROPERTY(QString modelName   READ modelName         CONSTANT)

public:
    explicit VLMBridge(QObject *parent = nullptr);
    ~VLMBridge() override;

    bool    isProcessing()       const { return m_processing; }
    bool    isOllamaReady()      const { return m_ollamaReady; }
    bool    isTranslateReady()   const { return m_translateReady; }
    bool    useOllamaTranslate() const;
    QString modelName()          const;

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void sendChat(const QString &prompt,
                              const QString &imageBase64 = {});
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE void setUseOllamaTranslate(bool useOllama);

signals:
    void processingChanged();
    void ollamaReadyChanged();
    void translateReadyChanged();
    void useOllamaTranslateChanged();

    void responseReceived(const QString &text);
    void errorOccurred(const QString &error);
    void initializationDone(bool success);

private:
    void setProcessing(bool v);
    void setOllamaReady(bool v);
    void setTranslateReady(bool v);

    std::unique_ptr<VLMEngine> m_engine;
    bool m_processing     = false;
    bool m_ollamaReady    = false;
    bool m_translateReady = false;
};
