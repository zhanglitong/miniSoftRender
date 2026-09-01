#pragma once
#include    <QPainter>

class TimeSlider
{
public:
    TimeSlider();
    ~TimeSlider();
public:
    void    setPos(const QPoint& pos);

    void    setSize(const QSize& size);

    void    paint(QPainter& painter, const uint& len, const QString& text);

    bool    isInRect(const QPoint& p);
private:
    QPoint  _pos;
    QSize   _size;

    QColor  _color  =   QColor(52, 135, 255, 200);

    QRect   _rect;
};
