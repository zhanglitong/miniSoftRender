#include    "TimeSlider.h"

TimeSlider::TimeSlider()
{
    setSize(QSize(12, 5));
}

TimeSlider::~TimeSlider()
{
}

void TimeSlider::setPos(const QPoint& pos)
{
    _pos    =   pos;

    QSize   halfSize    =   _size / 2;
    QPoint  lt          =   _pos - QPoint(halfSize.width(), halfSize.height());
    QPoint  rb          =   _pos + QPoint(halfSize.width(), halfSize.height());
            _rect       =   QRect(lt, rb);
}

void TimeSlider::setSize(const QSize& size)
{
    _size   =   size;
    QSize   halfSize    =   _size / 2;
    QPoint  lt          =   _pos - QPoint(halfSize.width(), halfSize.height());
    QPoint  rb          =   _pos + QPoint(halfSize.width(), halfSize.height());
            _rect       =   QRect(lt, rb);
}

void TimeSlider::paint(QPainter& painter, const uint& len, const QString& text)
{
    QPen pen(_color, 4);
    painter.setPen(pen);
    painter.setBrush(_color);

    painter.drawLine(_pos.x(), _pos.y(), _pos.x(), _pos.y() + len);
    painter.drawRoundedRect(_rect, 2, 2);;
    //painter.fillRect(_rect, _color);
    painter.setPen(QColor(255,255,255,255));
    painter.drawText(_rect, Qt::AlignCenter, text);
}

bool TimeSlider::isInRect(const QPoint& p)
{
    return _rect.contains(p);
}
