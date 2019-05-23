#ifndef QIMAGETOQML_H
#define QIMAGETOQML_H

#include <QObject>
#include <QQuickPaintedItem>
#include <QQuickItem>
#include <QPainter>
#include <QImage>

#define PUT_IMG_TO_CENTER_INSTEAD_OF_0_0 1
// LOL exactly what I was asking myself: https://stackoverflow.com/questions/50567000/updating-an-image-from-c-to-qml

class QImageToQml : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QImage image READ image WRITE setImage NOTIFY imageChanged)


public:
    QImageToQml();

    Q_INVOKABLE void setImage(const QImage &image);
    QImage image() const;

    // reimplemented function
    void paint(QPainter *painter);

signals:
    void imageChanged();
private:
    QImage current_image;

};

void ImageItem::setImage(const QImage &image)
{
    this->current_image = image;
    update();
}

void QImageToQml::paint(QPainter *painter)
{
#ifdef PUT_IMG_TO_CENTER_INSTEAD_OF_0_0
    QRectF bounding_rect = boundingRect();
    QImage scaled = this->current_image.scaledToHeight(bounding_rect.height());
    QPointF center = bounding_rect.center() - scaled.rect().center();

    if(center.x() < 0)
        center.setX(0);
    if(center.y() < 0)
        center.setY(0);
   painter->drawImage(center, scaled);
#else
   painter->drawImage(QPoint(0,0),this->image);
#endif
}

#endif // QIMAGETOQML_H
