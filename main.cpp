#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QDir>
#include <QCoreApplication>
#include <QObject>
#include <QByteArray>
#include <QIcon>

#include "src/VLMBridge.h"
#include "src/CaptureImageProvider.h"
#include "src/ImageUtils.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <cstdio>
#endif

// ── CaptureController ──────────────────────────────────────────
// Controller for QML captureProvider.setImageBase64(b64) / captureProvider.version() calls
// Inherits QObject only (separated from QQuickImageProvider)
class CaptureController : public QObject
{
    Q_OBJECT
public:
    explicit CaptureController(CaptureImageProvider *provider, QObject *parent = nullptr)
        : QObject(parent), m_provider(provider) {}

    Q_INVOKABLE void setImageBase64(const QString &base64) {
        QByteArray ba = QByteArray::fromBase64(base64.toUtf8());
        m_provider->updateImage(ba);
        ++m_version;
    }
    Q_INVOKABLE int version() const { return m_version; }

private:
    CaptureImageProvider *m_provider;
    int m_version = 0;
};

#include "main.moc"

// ──────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
#if defined(Q_OS_WIN) && defined(QT_DEBUG)
    // Allocate console window only in Debug builds
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
#endif
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QGuiApplication app(argc, argv);
    app.setApplicationName("CompactVLM");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("CompactVLM");
    app.setWindowIcon(QIcon(":/resources/app.ico"));

    QQuickStyle::setStyle("Basic");

    // ── VLC plugin path ────────────────────────────────────────────
    QString exeDir = QCoreApplication::applicationDirPath();
    QString pluginPath = exeDir + "/plugins";
    if (!QDir(pluginPath).exists()) {
        pluginPath = "C:/libVLC/bin/X64/plugins";
    }
    qputenv("VLC_PLUGIN_PATH", pluginPath.toLocal8Bit());

    // ── QML engine ─────────────────────────────────────────────────
    QQmlApplicationEngine engine;

    // vlmBridge: exposed to QML as a context property
    VLMBridge vlmBridge;
    engine.rootContext()->setContextProperty("vlmBridge", &vlmBridge);

    // captureProvider: image provider for image://capture/frame
    // Ownership transferred to engine via addImageProvider — no manual delete needed
    auto *imgProvider = new CaptureImageProvider();
    engine.addImageProvider("capture", imgProvider);

    // captureProvider (QML context): QObject wrapper exposing setImageBase64 / version()
    CaptureController captureCtrl(imgProvider);
    engine.rootContext()->setContextProperty("captureProvider", &captureCtrl);

    // imageUtils: image file loader (file → PNG base64)
    ImageUtils imageUtils;
    engine.rootContext()->setContextProperty("imageUtils", &imageUtils);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("CompactVLMQt", "Main");
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
