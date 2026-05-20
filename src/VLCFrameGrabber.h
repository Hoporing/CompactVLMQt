#pragma once

#include <vlc/vlc.h>
#include <QImage>
#include <mutex>
#include <thread>
#include <memory>
#include <string>
#include <chrono>
#include <queue>
#include <atomic>

enum class ModeVLC
{
    RTSPSTREAM,
    VIDEOFILE,
};

class VLCFrameGrabber
{
public:
    explicit VLCFrameGrabber(ModeVLC mode);
    ~VLCFrameGrabber();

    static ModeVLC GetMode(const std::string &url);

    bool    OpenVideo(const QString &url, int width, int height);

    double   GetFPS() const;
    void     SetFPS(double fps);
    uint64_t GetFrameSeq() const { return m_frameSeq.load(); }
    int     GetWidth()  const { return m_iWidth; }
    int     GetHeight() const { return m_iHeight; }

    QImage  GetFrame();
    void    SetFrame(const QImage &frame);

    // vmem callbacks (static → instance dispatch)
    void* VideoLock(void **planes);
    void  VideoUnlock(void *picture, void * const *planes);
    void  VideoDisplay(void *picture);

private:
    bool Init();
    void PlayMedia();
    void StopMedia();
    void ReleaseVLCInstances();

    double GetFPSfromMedia(libvlc_media_t *pMedia);
    void ProcFrame();
    void CheckFPS();

    static unsigned StaticVideoSetup(void **opaque, char *chroma,
                                     unsigned *width, unsigned *height,
                                     unsigned *pitches, unsigned *lines);
    static void     StaticVideoCleanup(void *opaque);
    static void*    StaticVideoLock(void *opaque, void **planes);
    static void     StaticVideoUnlock(void *opaque, void *picture, void * const *planes);
    static void     StaticVideoDisplay(void *opaque, void *picture);

    unsigned VideoSetup(char *chroma, unsigned *width, unsigned *height,
                        unsigned *pitches, unsigned *lines);

    // ── VLC instance (shared) ───────────────────────────────────
    static std::shared_ptr<libvlc_instance_t> g_sharedInstance;

    ModeVLC                  m_mode;
    libvlc_instance_t       *m_pInstance      = nullptr;
    libvlc_media_t          *m_pMedia         = nullptr;
    libvlc_media_player_t   *m_pMediaPlayer   = nullptr;

    // ── Buffer (raw buffer + QImage) ────────────────────────────
    std::mutex               m_bufferMutex;
    int                      m_iWidth          = 0;
    int                      m_iHeight         = 0;
    size_t                   m_videoBufferSize  = 0;
    std::unique_ptr<uint8_t[]> m_pVideoBuffer;
    QImage                   m_frame;

    // ── Frame queue (VideoDisplay → ProcFrame) ────────────────
    std::mutex               m_mutexQueue;
    std::queue<QImage>       m_bufferQueue;
    bool                     m_bLifeQueue      = false;
    std::thread             *m_pThreadQueue    = nullptr;

    // ── FPS calculation thread ────────────────────────────────
    mutable std::mutex       m_mutexFPS;
    bool                     m_bLifeFPS        = false;
    std::thread             *m_pThreadFPS      = nullptr;
    std::atomic<int>         m_frameCount      {0};
    std::atomic<uint64_t>    m_frameSeq        {0};
    double                   m_mediaFPS        = 30.0;
    double                   m_fps             = 0.0;
};
