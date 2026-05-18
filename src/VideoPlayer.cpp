#include "VideoPlayer.h"

VideoPlayer::VideoPlayer() = default;

VideoPlayer::~VideoPlayer()
{
    CloseVideo();
}

bool VideoPlayer::OpenVideo(const QString &url, int width, int height)
{
    CloseVideo();

    std::string stdUrl = url.toStdString();
    ModeVLC mode = VLCFrameGrabber::GetMode(stdUrl);

    m_grabber = std::make_unique<VLCFrameGrabber>(mode);
    return m_grabber->OpenVideo(url, width, height);
}

void VideoPlayer::CloseVideo()
{
    m_grabber.reset();
}

QImage VideoPlayer::GetCurrentFrame()
{
    if (m_grabber) return m_grabber->GetFrame();
    return {};
}

double VideoPlayer::GetFPS() const
{
    if (m_grabber) return m_grabber->GetFPS();
    return 0.0;
}
