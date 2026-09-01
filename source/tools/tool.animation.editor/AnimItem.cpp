#include    "AnimItem.h"


AnimItem::AnimItem(const QString& text,ItemType type, FE::FEObject* obj, QIcon icon)
    : QStandardItem(text)
{
    _obj    =   obj;
    _type   =   type;
    setIcon(icon);
}

bool AnimItem::isExpanded() const
{
    if (_obj == nullptr)
        return true;
    else
        return  _obj->flags().hasFlag(FE::FLAG_EXPAND);
}

void AnimItem::setIsExpanded(bool isExpand)
{
    if (_obj == nullptr)
    {
        return;
    }
    if (isExpand)
    {
        _obj->flags().addFlag(FE::FLAG_EXPAND);
    }
    else
    {
        _obj->flags().removeFlag(FE::FLAG_EXPAND);
    }
}
