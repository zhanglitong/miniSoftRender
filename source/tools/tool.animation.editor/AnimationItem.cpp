#include    "AnimationItem.h"


AnimationItem::AnimationItem(FE::Animation anim,const QString& text,ItemType type, FE::FEObject* obj, QIcon icon)
    : QStandardItem(text)
{
    _animation  =   anim;
    _object     =   obj;
    _type       =   type;
    setIcon(icon);
}

bool    AnimationItem::isExpanded() const
{
    if (_object == nullptr)
        return true;
    else
        return  _object->flags().hasFlag(FE::FLAG_EXPAND);
}

void    AnimationItem::setIsExpanded(bool isExpand)
{
    if (_object == nullptr)
    {
        return;
    }
    if (isExpand)
    {
        _object->flags().addFlag(FE::FLAG_EXPAND);
    }
    else
    {
        _object->flags().removeFlag(FE::FLAG_EXPAND);
    }
}
