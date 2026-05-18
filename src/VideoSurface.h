#pragma once

#include <QQuickPaintedItem>
#include <QImage>
#include <QTimer>
#include <QMutex>
#include <memory>

class VideoPlayer;

class VideoSurface : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool   playing  READ isPlaying  NOTIFY playingChanged)
    Q_PROPERTY(double fps      READ fps        NOTIFY fpsChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    explicit VideoSurface(QQuickItem *parent = nullptr);
    ~VideoSurface() override;

    // ── QML invokable ──────────────────────────────────────
    Q_INVOKABLE bool   open(const QString &url, int w = 960, int h = 540);
    Q_INVOKABLE void   stop();
    Q_INVOKABLE QString grabFrameBase64();   // implemented in STEP 5

    // ── properties ─────────────────────────────────────────
    bool    isPlaying()  const { return m_playing; }
    double  fps()        const { return m_fps; }
    QString statusText() const { return m_statusText; }

    // ── QQuickPaintedItem override ──────────────────────────
    void paint(QPainter *painter) override;

signals:
    void playingChanged();
    void fpsChanged();
    void statusTextChanged();
    void frameReady();

private slots:
    void onTimerTick();

private:
    void setPlaying(bool v);
    void setStatusText(const QString &s);

    std::unique_ptr<VideoPlayer> m_player;
    QTimer   *m_timer        = nullptr;
    QMutex    m_mutex;
    QImage    m_frame;
    bool      m_playing      = false;
    double    m_fps          = 0.0;
    uint64_t  m_lastFrameSeq = 0;
    QString   m_statusText;
};
