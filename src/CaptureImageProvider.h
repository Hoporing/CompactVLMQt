#pragma once

#include <QQuickImageProvider>
#include <QImage>
#include <QMutex>

// QML: image://capture/frame?v=N
class CaptureImageProvider : public QQuickImageProvider
{
public:
    CaptureImageProvider();
    void   updateImage(const QByteArray &pngData);
    QImage requestImage(const QString &id, QSize *size,
                        const QSize &requestedSize) override;

private:
    mutable QMutex m_mutex;
    QImage         m_image;
};
