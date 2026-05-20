#include "VLCFrameGrabber.h"

#include <QUrl>
#include <QByteArray>
#include <QString>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>

// ── Shared libvlc instance ───────────────────────────────────────
std::shared_ptr<libvlc_instance_t> VLCFrameGrabber::g_sharedInstance = nullptr;

// ────────────────────────────────────────────────────────────────
// Static vmem callbacks → instance method dispatch
// ────────────────────────────────────────────────────────────────
unsigned VLCFrameGrabber::StaticVideoSetup(void **opaque, char *chroma,
                                            unsigned *width, unsigned *height,
                                            unsigned *pitches, unsigned *lines)
{
    return static_cast<VLCFrameGrabber*>(*opaque)->VideoSetup(chroma, width, height, pitches, lines);
}

void VLCFrameGrabber::StaticVideoCleanup(void * /*opaque*/)
{
}

void* VLCFrameGrabber::StaticVideoLock(void *opaque, void **planes)
{
    return static_cast<VLCFrameGrabber*>(opaque)->VideoLock(planes);
}

void VLCFrameGrabber::StaticVideoUnlock(void *opaque, void *picture, void * const *planes)
{
    static_cast<VLCFrameGrabber*>(opaque)->VideoUnlock(picture, planes);
}

void VLCFrameGrabber::StaticVideoDisplay(void *opaque, void *picture)
{
    static_cast<VLCFrameGrabber*>(opaque)->VideoDisplay(picture);
}

// ────────────────────────────────────────────────────────────────
// RTSP / file mode detection
// ────────────────────────────────────────────────────────────────
ModeVLC VLCFrameGrabber::GetMode(const std::string &url)
{
    if (url.rfind("rtsp://", 0) == 0) return ModeVLC::RTSPSTREAM;
    return ModeVLC::VIDEOFILE;
}

// ────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ────────────────────────────────────────────────────────────────
VLCFrameGrabber::VLCFrameGrabber(ModeVLC mode)
    : m_mode(mode)
{
    m_videoBufferSize = 0;
    m_pThreadQueue    = nullptr;
    m_bLifeQueue      = false;
    m_pThreadFPS      = nullptr;
    m_bLifeFPS        = false;
    m_frameCount      = 0;
    m_fps             = 0.0;

    if (!Init()) std::cerr << "[VLC] libvlc init failed\n";
}

VLCFrameGrabber::~VLCFrameGrabber()
{
    StopMedia();
    ReleaseVLCInstances();
}

// ────────────────────────────────────────────────────────────────
// libvlc initialization
// ────────────────────────────────────────────────────────────────
bool VLCFrameGrabber::Init()
{
    if (!g_sharedInstance) {
        const char *args[] = {
            "--ignore-config",
            "--no-audio",          // Audio not needed for frame capture
            "--drop-late-frames",
            "--skip-frames",
            "--network-caching=300",
            "--live-caching=300",
            "--file-caching=300",
        };
        const int argc = static_cast<int>(sizeof(args) / sizeof(args[0]));
        g_sharedInstance = std::shared_ptr<libvlc_instance_t>(
            libvlc_new(argc, args),
            [](libvlc_instance_t *i) { if (i) libvlc_release(i); });

        if (g_sharedInstance) {
            libvlc_log_set(g_sharedInstance.get(),
                [](void *, int level, const libvlc_log_t *, const char *fmt, va_list args) {
                    if (level < LIBVLC_WARNING) return;
                    char buf[512];
                    vsnprintf(buf, sizeof(buf), fmt, args);
                    std::cerr << (level == LIBVLC_ERROR ? "[VLC-ERR] " : "[VLC-WARN]")
                              << " " << buf << "\n";
                }, nullptr);
        }
    }
    m_pInstance = g_sharedInstance.get();
    return m_pInstance != nullptr;
}

// ────────────────────────────────────────────────────────────────
// OpenVideo — vmem approach (no transcode+smem)
// ────────────────────────────────────────────────────────────────
bool VLCFrameGrabber::OpenVideo(const QString &url, int width, int height)
{
    if (!m_pInstance) { std::cerr << "[VLC] FAIL: pInstance is null\n"; return false; }

    m_iWidth  = width;
    m_iHeight = height;

    // ── Create media ──────────────────────────────────────────────
    std::string mrl;
    if (m_mode == ModeVLC::VIDEOFILE) {
        QUrl fileUrl = QUrl::fromLocalFile(url);
        mrl = fileUrl.toEncoded().toStdString();
    } else {
        mrl = url.toUtf8().toStdString();
    }

    m_pMedia = libvlc_media_new_location(m_pInstance, mrl.c_str());
    if (!m_pMedia) { std::cerr << "[VLC] FAIL: libvlc_media_new_location\n"; return false; }

    if (m_mode == ModeVLC::RTSPSTREAM) {
        libvlc_media_add_option(m_pMedia, ":rtsp-tcp");
    }

    // ── Create media player ────────────────────────────────────────
    if (m_pMediaPlayer) libvlc_media_player_release(m_pMediaPlayer);
    m_pMediaPlayer = libvlc_media_player_new_from_media(m_pMedia);
    if (!m_pMediaPlayer) { std::cerr << "[VLC] FAIL: libvlc_media_player_new_from_media\n"; return false; }

    // ── vmem video output ─────────────────────────────────────────
    // format_callbacks: VLC asks us for the output format → we always
    // return I420 at (width x height), forcing ONE swscale pass
    // (I420 full-res → I420 target-res, no chroma change = faster).
    // Using format_callbacks (not set_format) prevents VLC's resize
    // events from overwriting our format and causing buffer overflow.
    libvlc_video_set_format_callbacks(m_pMediaPlayer,
                                      StaticVideoSetup,
                                      StaticVideoCleanup);
    libvlc_video_set_callbacks(m_pMediaPlayer,
                               StaticVideoLock,
                               StaticVideoUnlock,
                               StaticVideoDisplay,
                               this);

    // ── Read FPS (VIDEOFILE only; RTSP unavailable before connection) ──
    if (m_mode == ModeVLC::VIDEOFILE) {
        m_mediaFPS = GetFPSfromMedia(m_pMedia);
    } else {
        m_mediaFPS = 30.0;
    }

    PlayMedia();

    m_frameCount = 0;
    m_frameSeq   = 0;
    if (m_pThreadQueue == nullptr)
        m_pThreadQueue = new std::thread([this]() { ProcFrame(); });

    if (m_pThreadFPS == nullptr)
        m_pThreadFPS = new std::thread([this]() { CheckFPS(); });

    return true;
}

// ────────────────────────────────────────────────────────────────
// Read FPS from media header
// ────────────────────────────────────────────────────────────────
double VLCFrameGrabber::GetFPSfromMedia(libvlc_media_t *pMedia)
{
    libvlc_media_parse_with_options(pMedia, libvlc_media_parse_local, 3000);

    for (int i = 0; i < 300; ++i) {
        libvlc_media_parsed_status_t st = libvlc_media_get_parsed_status(pMedia);
        if (st == libvlc_media_parsed_status_done)    break;
        if (st == libvlc_media_parsed_status_timeout ||
            st == libvlc_media_parsed_status_failed) {
            std::cerr << "[VLC] Media parse failed/timeout\n";
            return 30.0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    double fps = 0.0;
    libvlc_media_track_t **tracks = nullptr;
    unsigned count = libvlc_media_tracks_get(pMedia, &tracks);
    for (unsigned i = 0; i < count; ++i) {
        if (tracks[i]->i_type == libvlc_track_video) {
            double num = static_cast<double>(tracks[i]->video->i_frame_rate_num);
            double den = static_cast<double>(tracks[i]->video->i_frame_rate_den);
            if (den > 0.0) { fps = num / den; break; }
        }
    }
    if (tracks) libvlc_media_tracks_release(tracks, count);

    return (fps > 0.0) ? fps : 30.0;
}

// ────────────────────────────────────────────────────────────────
// vmem callback implementation
// ────────────────────────────────────────────────────────────────
unsigned VLCFrameGrabber::VideoSetup(char *chroma, unsigned *width, unsigned *height,
                                     unsigned *pitches, unsigned *lines)
{
    // Use source native resolution for capture quality (VLM needs full detail).
    // Cap at 1920x1080 to avoid 4K performance issues.
    // VideoSurface::paint handles display scaling via letterbox — no resize needed here.
    unsigned srcW = *width  ? *width  : 1920u;
    unsigned srcH = *height ? *height : 1080u;

    // Cap the longer dimension at 1920 — works for both landscape and portrait.
    // e.g. 3840×2160 → 1920×1080, 2160×3840 → 1080×1920
    const unsigned maxLong = 1920u;
    unsigned longSide = std::max(srcW, srcH);
    double scale = (longSide > maxLong) ? static_cast<double>(maxLong) / longSide : 1.0;

    unsigned outW = (static_cast<unsigned>(srcW * scale) + 1) & ~1u;
    unsigned outH = (static_cast<unsigned>(srcH * scale) + 1) & ~1u;
    if (outW == 0) outW = 2;
    if (outH == 0) outH = 2;

    m_iWidth  = static_cast<int>(outW);
    m_iHeight = static_cast<int>(outH);

    memcpy(chroma, "I420", 4);
    *width  = outW;
    *height = outH;

    pitches[0] = static_cast<unsigned>(m_iWidth);      lines[0] = static_cast<unsigned>(m_iHeight);
    pitches[1] = static_cast<unsigned>(m_iWidth / 2);  lines[1] = static_cast<unsigned>(m_iHeight / 2);
    pitches[2] = static_cast<unsigned>(m_iWidth / 2);  lines[2] = static_cast<unsigned>(m_iHeight / 2);
    pitches[3] = lines[3] = 0;
    pitches[4] = lines[4] = 0;

    size_t bufSize = static_cast<size_t>(m_iWidth) * m_iHeight * 3 / 2;
    if (bufSize > m_videoBufferSize || !m_pVideoBuffer) {
        m_pVideoBuffer.reset(new uint8_t[bufSize]());
        m_videoBufferSize = bufSize;
    }
    return 1;
}

void* VLCFrameGrabber::VideoLock(void **planes)
{
    // I420: 3 planes — Y, U, V
    uint8_t *buf = m_pVideoBuffer.get();
    planes[0] = buf;                                                   // Y: w*h
    planes[1] = buf + m_iWidth * m_iHeight;                            // U: (w/2)*(h/2)
    planes[2] = buf + m_iWidth * m_iHeight + (m_iWidth/2)*(m_iHeight/2); // V
    return nullptr;
}

void VLCFrameGrabber::VideoUnlock(void * /*picture*/, void * const * /*planes*/)
{
}

void VLCFrameGrabber::VideoDisplay(void * /*picture*/)
{
    ++m_frameCount;

    // Convert I420 → RGB888 at native source resolution (capped at 1920x1080)
    const uint8_t *Y = m_pVideoBuffer.get();
    const uint8_t *U = Y + m_iWidth * m_iHeight;
    const uint8_t *V = U + (m_iWidth/2) * (m_iHeight/2);

    QImage img(m_iWidth, m_iHeight, QImage::Format_RGB888);
    for (int row = 0; row < m_iHeight; ++row) {
        uchar *dst = img.scanLine(row);
        for (int col = 0; col < m_iWidth; ++col) {
            int y = Y[row * m_iWidth + col];
            int u = U[(row/2) * (m_iWidth/2) + col/2] - 128;
            int v = V[(row/2) * (m_iWidth/2) + col/2] - 128;
            dst[col*3]   = static_cast<uchar>(qBound(0, (y*298 + v*409         + 128) >> 8, 255));
            dst[col*3+1] = static_cast<uchar>(qBound(0, (y*298 - u*100 - v*208 + 128) >> 8, 255));
            dst[col*3+2] = static_cast<uchar>(qBound(0, (y*298 + u*516         + 128) >> 8, 255));
        }
    }

    std::lock_guard<std::mutex> lk(m_mutexQueue);
    while (m_bufferQueue.size() >= 30) m_bufferQueue.pop();
    m_bufferQueue.emplace(std::move(img));
}

// ────────────────────────────────────────────────────────────────
// Frame pacing thread — outputs at native FPS
// ────────────────────────────────────────────────────────────────
void VLCFrameGrabber::ProcFrame()
{
    m_bLifeQueue = true;

    double fps = (m_mediaFPS > 0.0) ? m_mediaFPS : 30.0;
    auto frameInterval = std::chrono::microseconds(
        static_cast<int64_t>(1'000'000.0 / fps));

    auto nextTime = std::chrono::steady_clock::now();

    while (m_bLifeQueue) {
        nextTime += frameInterval;

        QImage frame;
        {
            std::lock_guard<std::mutex> lk(m_mutexQueue);
            if (!m_bufferQueue.empty()) {
                frame = std::move(m_bufferQueue.front());
                m_bufferQueue.pop();
            }
        }

        if (!frame.isNull()) {
            SetFrame(frame);
            ++m_frameSeq;
        }

        std::this_thread::sleep_until(nextTime);
    }
}

// ────────────────────────────────────────────────────────────────
// FPS calculation thread
// ────────────────────────────────────────────────────────────────
void VLCFrameGrabber::CheckFPS()
{
    m_bLifeFPS = true;
    auto start = std::chrono::steady_clock::now();

    while (m_bLifeFPS) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - start;
        if (elapsed.count() >= 1.0) {
            int count = m_frameCount.exchange(0);
            SetFPS(count / elapsed.count());
            start = now;
        }
    }
}

// ────────────────────────────────────────────────────────────────
// Frame access (thread-safe)
// ────────────────────────────────────────────────────────────────
QImage VLCFrameGrabber::GetFrame()
{
    std::lock_guard<std::mutex> lk(m_bufferMutex);
    return m_frame;
}

void VLCFrameGrabber::SetFrame(const QImage &frame)
{
    std::lock_guard<std::mutex> lk(m_bufferMutex);
    m_frame = frame;
}

double VLCFrameGrabber::GetFPS() const
{
    std::lock_guard<std::mutex> lk(m_mutexFPS);
    return m_fps;
}

void VLCFrameGrabber::SetFPS(double fps)
{
    std::lock_guard<std::mutex> lk(m_mutexFPS);
    m_fps = fps;
}

// ────────────────────────────────────────────────────────────────
// Stop / Release
// ────────────────────────────────────────────────────────────────
void VLCFrameGrabber::PlayMedia()
{
    if (m_pMediaPlayer) libvlc_media_player_play(m_pMediaPlayer);
}

void VLCFrameGrabber::StopMedia()
{
    if (m_pMediaPlayer) libvlc_media_player_stop(m_pMediaPlayer);

    if (m_pThreadQueue) {
        m_bLifeQueue = false;
        if (m_pThreadQueue->joinable()) m_pThreadQueue->join();
        delete m_pThreadQueue;
        m_pThreadQueue = nullptr;
    }
    if (m_pThreadFPS) {
        m_bLifeFPS = false;
        if (m_pThreadFPS->joinable()) m_pThreadFPS->join();
        delete m_pThreadFPS;
        m_pThreadFPS = nullptr;
    }

    std::lock_guard<std::mutex> lk(m_mutexQueue);
    while (!m_bufferQueue.empty()) m_bufferQueue.pop();
}

void VLCFrameGrabber::ReleaseVLCInstances()
{
    if (m_pMediaPlayer) {
        libvlc_media_player_stop(m_pMediaPlayer);
        libvlc_media_player_release(m_pMediaPlayer);
        m_pMediaPlayer = nullptr;
    }
    if (m_pMedia) {
        libvlc_media_release(m_pMedia);
        m_pMedia = nullptr;
    }
}
