#include    "../../inc/FECamera.hpp"
#include    "../../inc/axis/FEAxisMath.hpp"
#include    "../../inc/axis/FEGraphicsStub.hpp"
#include    "../../inc/axis/FEEditAxisGroup.h"
#include    "../../inc/FEContext.hpp"


namespace   FE
{
    using AxisMap = std::map<FEEditAxis*, FEEditAxisGroup::AxisAttribute>;
    class FEEditAxisGroupPrivate
    {
    public:
        FEEditAxisGroup & _d;
        AxisMap _axisMap;
    public:
        FEEditAxisGroupPrivate(FEEditAxisGroup& d) :_d(d) {}
    public:
    };

    FEEditAxisGroup::FEEditAxisGroup():_p(*(new FEEditAxisGroupPrivate(*this)))
    {
    }
    FEEditAxisGroup::~FEEditAxisGroup()
    {
        delete &_p;
    }

    void FEEditAxisGroup::addAxis(FEEditAxis* pAxis)
    {
        if (pAxis == nullptr)
            return;
        AxisMap::iterator itr = _p._axisMap.find(pAxis);
        if (itr == _p._axisMap.end())
        {
            _p._axisMap[pAxis] = AxisAttribute();
        }
    }
    bool FEEditAxisGroup::removeAxis(FEEditAxis* pAxis)
    {
        if (pAxis == nullptr)
            return false;
        AxisMap::iterator itr = _p._axisMap.find(pAxis);
        if (itr != _p._axisMap.end())
        {
            itr = _p._axisMap.erase(itr);
            return true;
        }
        return false;
    }

    void FEEditAxisGroup::clearAxies()
    {
        _p._axisMap.clear();
    }

    bool FEEditAxisGroup::containsAxis(FEEditAxis* pAxis) const
    {
        return _p._axisMap.find(pAxis) != _p._axisMap.end();
    }
    FEEditAxisGroup::AxisAttribute* FEEditAxisGroup::attribute(FEEditAxis* pAxis)
    {
        AxisMap::iterator itr = _p._axisMap.find(pAxis);
        if (itr != _p._axisMap.end())
            return &itr->second;
        return nullptr;
    }
    const FEEditAxisGroup::AxisAttribute* FEEditAxisGroup::attribute(FEEditAxis* pAxis) const
    {
        AxisMap::iterator itr = _p._axisMap.find(pAxis);
        if (itr != _p._axisMap.end())
            return &itr->second;
        return nullptr;
    }
    bool FEEditAxisGroup::empty() const
    {
        return _p._axisMap.empty();
    }
    FEEditAxisGroup::Axies FEEditAxisGroup::axies()const
    {
        Axies ret;
        for (AxisMap::const_iterator itr = _p._axisMap.begin(); itr != _p._axisMap.end(); ++itr)
        {
            if (itr->first == nullptr)
                continue;
            ret.push_back(itr->first);
        }
        return ret;
    }
    

    void FEEditAxisGroup::mouseButtonPress(FEContext& context, const int2& pos)
    {
        for (AxisMap::iterator itr = _p._axisMap.begin(); itr != _p._axisMap.end(); ++itr)
        {
            if (itr->first == nullptr)
                continue;
            if (!itr->second.visible())
                continue;
            itr->first->mouseButtonPress(context, pos);
        }
    }
    void FEEditAxisGroup::mouseButtonRelease(FEContext& context, const int2& pos)
    {
        for (AxisMap::iterator itr = _p._axisMap.begin(); itr != _p._axisMap.end(); ++itr)
        {
            if (itr->first == nullptr)
                continue;
            if (!itr->second.visible())
                continue;
            itr->first->mouseButtonRelease(context, pos);
        }
    }
    void FEEditAxisGroup::mouseMove(FEContext& context, const int2& pos)
    {
        ///如果有轴被选中，则直接调用Move
        FEEditAxis* pSelectedAxis = this->selectedAxis();
        if (pSelectedAxis != nullptr)
        {
            pSelectedAxis->mouseMove(context, pos);
            return;
        }
        ///首先根据距离排序
        ///然后拿到最近的 hovered 状态的轴,并且将其他轴的 hovered状态重置
        Axies axies = this->sortAxies(context);
        FEEditAxis* pFirstHovered = nullptr;
        for (Axies::iterator itr = axies.begin(); itr != axies.end(); ++itr)
        {
            if ((*itr) == nullptr)
                continue;
            (*itr)->cancelHovered();
            auto fItr = _p._axisMap.find((*itr));
            if (fItr != _p._axisMap.end() && !fItr->second.visible())
            {
                continue;
            }
            (*itr)->mouseMove(context, pos);
            if ((*itr)->isAxisHovered() && pFirstHovered == nullptr)
            {
                pFirstHovered = (*itr);
            }
            else
            {
                (*itr)->cancelHovered();
            }
        }

        if (this->hasHoveredAxis())
        {
        }
        else
        {
        }
    }
    void FEEditAxisGroup::touchDown(FEContext& context, const int2& pos)
    {
        for (AxisMap::iterator itr = _p._axisMap.begin(); itr != _p._axisMap.end(); ++itr)
        {
            if (itr->first == nullptr)
                continue;
            if (!itr->second.visible())
                continue;
            itr->first->touchDown(context, pos);
        }
    }
    void FEEditAxisGroup::touchUp(FEContext& context, const int2& pos)
    {
        ///如果有轴被选中，则直接调用touchUp
        FEEditAxis* pSelectedAxis = this->selectedAxis();
        if (pSelectedAxis != nullptr)
        {
            pSelectedAxis->touchUp(context, pos);
            return;
        }
        ///首先根据距离排序
        ///然后拿到最近的 hovered 状态的轴,并且将其他轴的 hovered状态重置
        Axies axies = this->sortAxies(context);
        FEEditAxis* pFirstSelected = nullptr;
        for (Axies::iterator itr = axies.begin(); itr != axies.end(); ++itr)
        {
            if ((*itr) == nullptr)
                continue;
            auto fItr = _p._axisMap.find((*itr));
            if (fItr != _p._axisMap.end() && !fItr->second.visible())
                continue;
            (*itr)->touchUp(context, pos);
            if ((*itr)->isAxisSelected() && pFirstSelected == nullptr)
            {
                pFirstSelected = (*itr);
            }
            else
            {
                (*itr)->cancelSelected();
                (*itr)->cancelHovered();
            }
        }

        if (this->hasSelcetedAxis())
        {
        }
        else
        {
        }
    }
    void FEEditAxisGroup::touchMove(FEContext& context, const int2& pos)
    {
        for (AxisMap::iterator itr = _p._axisMap.begin(); itr != _p._axisMap.end(); ++itr)
        {
            if (itr->first == nullptr)
                continue;
            if (!itr->second.visible())
                continue;
            itr->first->touchMove(context, pos);
        }
    }
    
    void FEEditAxisGroup::update(FEContext& context)
    {
        for (AxisMap::iterator itr = _p._axisMap.begin(); itr != _p._axisMap.end(); ++itr)
        {
            if (itr->first == nullptr)
                continue;
            itr->first->update(context);
        }
    }
    void FEEditAxisGroup::render(FEContext& context)
    {
        Axies axies = this->sortAxies(context);
        for (auto itr = axies.rbegin(); itr != axies.rend(); ++itr)
        {
            if ((*itr) == nullptr)
                continue;
            auto fItr = _p._axisMap.find((*itr));
            if (fItr != _p._axisMap.end() && !fItr->second.visible())
                continue;
            (*itr)->render(context);
        }
    }

    FEEditAxisGroup::Axies FEEditAxisGroup::sortAxies(FEContext& context) const
    {
        FECamera& camera = context.activeCamera();
        Axies       ret;
        for (AxisMap::const_iterator itr = _p._axisMap.begin(); itr != _p._axisMap.end(); ++itr)
        {
            if (itr->first == nullptr)
                continue;
            if (!itr->second.visible())
                continue;
            if (ret.empty())
            {
                ret.push_back(itr->first);
            }
            else
            {//与已有的轴做排序
                real dis = distance(camera.getEye(), itr->first->position());
                real frontDis = distance(camera.getEye(), ret.front()->position());
                real backDis = distance(camera.getEye(), ret.back()->position());
                if (dis >= backDis)
                {
                    ret.push_back(itr->first);
                    continue;
                }
                else if (dis <= frontDis)
                {
                    ret.push_front(itr->first);
                    continue;
                }
                else
                {
                    Axies::iterator rItr = ret.begin();
                    while (rItr != ret.end())
                    {
                        real tDis = distance(camera.getEye(), (*rItr)->position());
                        if (dis <= tDis)
                        {
                            ret.insert(rItr, itr->first);
                            break;
                        }
                        else
                        {
                            rItr++;
                        }
                    }
                }
            }
        }
        return ret;
    }
    
    FEEditAxis* FEEditAxisGroup::hoveredAxis() const
    {
        for (AxisMap::const_iterator itr = _p._axisMap.begin(); itr != _p._axisMap.end(); ++itr)
        {
            if (itr->first == nullptr)
                continue;
            if (itr->first->isAxisHovered())
                return itr->first;
        }
        return nullptr;
    }
    FEEditAxis* FEEditAxisGroup::selectedAxis() const
    {
        for (AxisMap::const_iterator itr = _p._axisMap.begin(); itr != _p._axisMap.end(); ++itr)
        {
            if (itr->first == nullptr)
                continue;
            if (itr->first->isAxisSelected())
                return itr->first;
        }
        return nullptr;
    }
}
