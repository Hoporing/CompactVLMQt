#include "VLMBridge.h"
#include "VLMEngine.h"

#include <QImage>
#include <QByteArray>
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include <QDebug>

VLMBridge::VLMBridge(QObject *parent)
    : QObject(parent)
{
    m_engine = std::make_unique<VLMEngine>();
}

VLMBridge::~VLMBridge() = default;

QString VLMBridge::modelName() const
{
    return m_engine ? QString::fromStdString(m_engine->GetModelName()) : QString();
}

void VLMBridge::initialize()
{
    if (m_processing) return;
    setProcessing(true);

    auto *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher]() {
                bool ok = m_engine->IsOllamaRunning();
                setOllamaReady(ok);
                setProcessing(false);
                emit initializationDone(ok);
                watcher->deleteLater();
            });

    VLMEngine *engine = m_engine.get();
    watcher->setFuture(QtConcurrent::run([engine]() -> bool {
        return engine->Initialize();
    }));
}

void VLMBridge::sendChat(const QString &prompt, const QString &imageBase64)
{
    if (m_processing || prompt.isEmpty()) return;
    setProcessing(true);

    QImage image;
    if (!imageBase64.isEmpty()) {
        QByteArray ba = QByteArray::fromBase64(imageBase64.toUtf8());
        image.loadFromData(ba);
    }

    struct TaskResult {
        bool    success;
        QString responseText;
        QString errorMessage;
    };

    auto *watcher = new QFutureWatcher<TaskResult>(this);
    connect(watcher, &QFutureWatcher<TaskResult>::finished, this,
            [this, watcher]() {
                auto res = watcher->result();
                setProcessing(false);
                setOllamaReady(m_engine->IsOllamaRunning());
                if (res.success) emit responseReceived(res.responseText);
                else             emit errorOccurred(res.errorMessage);
                watcher->deleteLater();
            });

    VLMEngine *engine = m_engine.get();
    watcher->setFuture(
        QtConcurrent::run([engine, prompt, image]() -> TaskResult {
            VLMResult r = engine->SendChat(prompt, image);
            return { r.success, r.responseText, r.errorMessage };
        })
    );
}

void VLMBridge::clearHistory()
{
    if (m_engine) m_engine->ClearHistory();
}

void VLMBridge::setProcessing(bool v)
{
    if (m_processing == v) return;
    m_processing = v;
    emit processingChanged();
}

void VLMBridge::setOllamaReady(bool v)
{
    if (m_ollamaReady == v) return;
    m_ollamaReady = v;
    emit ollamaReadyChanged();
}
