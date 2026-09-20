#pragma     once
#include    <QStandardItem>

#include    "animation/FEKeyFrameTrack.hpp"
#include    "animation/FEAnimClip.hpp"
#include    "animation/FEAnimation.hpp"

class   AnimationItem : public QStandardItem
{
public: 
    enum    ItemType:uint8_t
    {
        IT_Track,
        IT_Par
    };
public:
    AnimationItem(FE::Animation anim,const QString& text, ItemType type, FE::FEObject* obj, QIcon icon = QIcon());
public:
    inline FE::FEKeyFrameTrack* animKeyframeTrack() const
    {
        return ItemType::IT_Track == _type ? (FE::FEKeyFrameTrack*)(_object.get()) : nullptr;
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

    inline  auto    object() const
    {
        return _object;
    }
    inline  auto    animation() const
    {
        return  _animation;
    }
public:
    bool    isExpanded() const;
    void    setIsExpanded(bool isExpand);
private:
    ItemType        _type;
    bool            _isSelcted  =   false;
    int             _row        =   -1;
    FE::Animation   _animation  =   nullptr;
    FE::Object      _object     =   nullptr;
};


using   AnimationItems  =   std::vector<AnimationItem*>;
