#pragma     once
#include    <QStandardItem>

#include    "animation/FEKeyFrameTrack.hpp"

class   AnimationItem : public QStandardItem
{
public: 
    enum    ItemType
    {
        IT_Track,
        IT_Par
    };
public:
    AnimationItem(const QString& text, ItemType type, FE::FEObject* obj, QIcon icon = QIcon());
public:
    inline FE::FEKeyFrameTrack* animKeyframeTrack() const
    {
        return ItemType::IT_Track == _type ? (FE::FEKeyFrameTrack*)(_obj.get()) : nullptr;
    }

    inline ItemType itemType() const
    {
        return _type;
    }

    inline void     setIsSelected(bool isSelected)
    {
        _isSelcted  =   isSelected;
    }

    inline bool     isSelected() const
    {
        return _isSelcted;
    }

    inline void     setRow(const int& row)
    {
        _row = row;
    }

    inline int      getRow() const
    {
        return _row;
    }

    inline auto     getObject() const
    {
        return _obj;
    }
public:
    bool    isExpanded() const;
    void    setIsExpanded(bool isExpand);
private:
    bool        _isSelcted  =   false;
    int         _row        =   -1;
    ItemType    _type;
    FE::Object  _obj = nullptr;

public:
    std::map<int, bool> _drawKeyDatas;
};


using   AnimationItems  =   std::vector<AnimationItem>;
