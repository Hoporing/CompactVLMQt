#include "CaptureImageProvider.h"
#include <QMutexLocker>

CaptureImageProvider::CaptureImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

void CaptureImageProvider::updateImage(const QByteArray &pngData)
{
    QMutexLocker lk(&m_mutex);
    m_image.loadFromData(pngData);
}

QImage CaptureImageProvider::requestImage(const QString &, QSize *size,
                                           const QSize &)
{
    QMutexLocker lk(&m_mutex);
    if (size) *size = m_image.size();
    return m_image;
}
