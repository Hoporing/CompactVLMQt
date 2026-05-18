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

// Replaced cv::Mat dependency (from MFC) with QImage
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

    // VLC smem callbacks (static → instance dispatch)
    void VideoPrerender(uint8_t **pp_pixel_buffer, int size);
    void VideoPostrender(uint8_t *p_pixel_buffer);

private:
    bool Init();
    void PlayMedia();
    void StopMedia();
    void ReleaseVLCInstances();

    double GetFPSfromMedia(libvlc_media_t *pMedia); // Read FPS from media header
    void ProcFrame(); // Frame pacing thread (outputs at native FPS)
    void CheckFPS();  // FPS calculation thread

    static void StaticVideoPrerender(void *p_video_data,
                                     uint8_t **pp_pixel_buffer, int size);
    static void StaticVideoPostrender(void *p_video_data,
                                      uint8_t *p_pixel_buffer);

    // ── VLC instance (shared) ───────────────────────────────────
    static std::shared_ptr<libvlc_instance_t> g_sharedInstance;

    ModeVLC                  m_mode;
    libvlc_instance_t       *m_pInstance      = nullptr;
    libvlc_media_t          *m_pMedia         = nullptr;
    libvlc_media_player_t   *m_pMediaPlayer   = nullptr;
    libvlc_event_manager_t  *m_pEventManager  = nullptr;

    // ── Buffer (raw buffer + QImage) ────────────────────────────
    std::mutex               m_bufferMutex;
    int                      m_iWidth          = 0;
    int                      m_iHeight         = 0;
    size_t                   m_videoBufferSize  = 0;
    int                      m_lastFrameSize   = 0;   // Actual frame size passed from prerender to postrender
    std::unique_ptr<uint8_t[]> m_pVideoBuffer;
    QImage                   m_frame;          // Latest frame (RGB888)

    // ── Frame queue (PostRender → ProcFrame) ──────────────────
    std::mutex               m_mutexQueue;
    std::queue<QImage>       m_bufferQueue;
    bool                     m_bLifeQueue      = false;
    std::thread             *m_pThreadQueue    = nullptr;

    // ── FPS calculation thread ────────────────────────────────
    mutable std::mutex       m_mutexFPS;
    bool                     m_bLifeFPS        = false;
    std::thread             *m_pThreadFPS      = nullptr;
    std::atomic<int>         m_frameCount      {0};  // VLC delivery count
    std::atomic<uint64_t>    m_frameSeq        {0};  // Screen refresh trigger
    double                   m_mediaFPS        = 30.0; // Native video FPS
    double                   m_fps             = 0.0;
};
