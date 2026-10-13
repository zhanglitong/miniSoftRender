#include    "UndoCommand.h"
#include    <algorithm>
#include    <fstream>
#include    "AnimationTree.h"
#include    "animation/FEAnimationHelper.hpp"
#include    "animation/FEAnimationSys.hpp"
#include    "graphic/FEScene.h"
#include    "animation/FETrackResult.hpp"
#include    "FETransform.hpp"

namespace   FE
{

    RenameNodeCmd::RenameNodeCmd(Node pNode, const char* redoName, const char* undoName, QUndoCommand* parent)
        : QUndoCommand(parent)
    {
        _pNode      =   pNode;
        _redoName   =   redoName;
        _undoName   =   undoName;
    }

    void RenameNodeCmd::redo()
    {
        _pNode->setName(_redoName);
    }

    void RenameNodeCmd::undo()
    {
        _pNode->setName(_undoName);
    }

    /// <summary>
    /// 从轨道值变体中读取指定索引的关键帧值
    /// </summary>
    static  KFValue  readTrackValue(const ValueObject& values, size_t idx)
    {
        return  std::visit([idx](auto&& obj) -> KFValue
        {
            using   T   =   std::decay_t<decltype(obj)>;
            if  constexpr (std::is_same_v<T, std::monostate>)
                return  KFValue{};
            else
            {
                if  (!obj)
                    return  KFValue{};
                auto&   arr =   obj->values();
                if  (idx < arr.size())
                    return  arr[idx];
                return  KFValue{};
            }
        }, values);
    }


    /// =================== CreateKeyframeCmd ===================

    CreateKeyframeCmd::CreateKeyframeCmd(Node node, Animation anim, real time, bool createdDefaultTracks, std::function<void()> refreshCb, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _node(node)
        , _anim(anim)
        , _time(time)
        , _createdDefaultTracks(createdDefaultTracks)
        , _refreshCb(std::move(refreshCb))
    {
    }

    void    CreateKeyframeCmd::redo()
    {
        /// 空 clip: 先创建默认轨道,再添加关键帧
        if  (_createdDefaultTracks && _anim && _anim->clip())
        {
            FEAnimationHelper::addDefaultTracks(_anim->clip(), _node->ctx());
        }
        FEAnimationHelper::addNodeKeyFrame(_anim, _time, _node);
        if  (_refreshCb)
            _refreshCb();
    }

    void    CreateKeyframeCmd::undo()
    {
        FEAnimationHelper::removeNodeKeyFrame(_anim, _time);
        /// 空 clip 创建的默认轨道: undo 时清空
        if  (_createdDefaultTracks && _anim && _anim->clip())
        {
            _anim->clip()->clearObjects();
        }
        if  (_refreshCb)
            _refreshCb();
    }

    /// =================== UpdateKeyframeCmd ===================

    UpdateKeyframeCmd::TrackSnapshots  UpdateKeyframeCmd::snapshotTracks(Animation anim, real time)
    {
        TrackSnapshots  snaps;
        if  (!anim || !anim->clip())
            return  snaps;
        /// 时间线时间转内部时间
        real    internalTime    =   time - anim->offset();
        for (auto track : anim->clip()->tracks())
        {
            if  (!track->_times)
                continue;
            auto&   times   =   track->_times->values();
            auto    itr     =   std::find(times.begin(), times.end(), internalTime);
            if  (itr == times.end())
                continue;
            TrackSnapshot   snap;
            snap.track  =   track;
            size_t  idx =   (size_t)std::distance(times.begin(), itr);
            snap.oldVal =   readTrackValue(track->_values, idx);
            snaps.push_back(std::move(snap));
        }
        return  snaps;
    }

    UpdateKeyframeCmd::UpdateKeyframeCmd(Node node, Animation anim, real time, TrackSnapshots snapshots, std::function<void()> refreshCb, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _node(node)
        , _anim(anim)
        , _time(time)
        , _snapshots(std::move(snapshots))
        , _refreshCb(std::move(refreshCb))
    {
    }

    void    UpdateKeyframeCmd::redo()
    {
        FEAnimationHelper::updateNodeKeyFrame(_anim, _time, _node);
        if  (_refreshCb)
            _refreshCb();
    }

    void    UpdateKeyframeCmd::undo()
    {
        real    internalTime    =   _time - _anim->offset();
        for (auto& snap : _snapshots)
        {
            if  (snap.track)
                snap.track->updateKeyFrame(internalTime, snap.oldVal);
        }
        if  (_refreshCb)
            _refreshCb();
    }

    /// =================== MoveKeyframesCmd ===================

    static  std::vector<KFValue>    readAllTrackValuesImpl(const ValueObject& values)
    {
        return  std::visit([](auto&& obj) -> std::vector<KFValue>
        {
            using   T   =   std::decay_t<decltype(obj)>;
            if  constexpr (std::is_same_v<T, std::monostate>)
                return  {};
            else
            {
                if  (!obj)
                    return  {};
                std::vector<KFValue>   result;
                for (auto& v : obj->values())
                    result.push_back(v);
                return  result;
            }
        }, values);
    }

    std::vector<KFValue>    MoveKeyframesCmd::readAllValues(const ValueObject& values)
    {
        return  readAllTrackValuesImpl(values);
    }

    /// 用 vals 覆盖 track 的整个值数组(逐索引 updateKeyFrame)
    static  void    writeAllTrackValues(KeyFrameTrack track, const std::vector<KFValue>& vals)
    {
        if  (!track)
            return;
        for (size_t i = 0; i < vals.size(); ++i)
            track->updateKeyFrame(i, vals[i]);
        track->flags().addFlag(FEKeyFrameTrack::TrackChanged);
    }

    /// 用 times 覆盖 track 的整个时间数组
    static  void    writeAllTrackTimes(KeyFrameTrack track, const std::vector<real>& times)
    {
        if  (!track || !track->times())
            return;
        auto&   arr =   track->times()->values();
        arr.assign(times.begin(), times.end());
        track->flags().addFlag(FEKeyFrameTrack::TrackChanged);
    }

    MoveKeyframesCmd::MoveKeyframesCmd(TrackMoves&& moves, std::function<void()> refreshCb, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _moves(std::move(moves))
        , _refreshCb(std::move(refreshCb))
    {
    }

    void    MoveKeyframesCmd::redo()
    {
        for (auto& mv : _moves)
        {
            if  (!mv.track)
                continue;
            writeAllTrackTimes(mv.track, mv.newTimes);
            writeAllTrackValues(mv.track, mv.newValues);
            mv.track->sortKeyFames();
        }
        if  (_refreshCb)
            _refreshCb();
    }

    void    MoveKeyframesCmd::undo()
    {
        for (auto& mv : _moves)
        {
            if  (!mv.track)
                continue;
            writeAllTrackTimes(mv.track, mv.oldTimes);
            writeAllTrackValues(mv.track, mv.oldValues);
            mv.track->sortKeyFames();
        }
        if  (_refreshCb)
            _refreshCb();
    }

    /// =================== MoveOffsetCmd ===================

    MoveOffsetCmd::MoveOffsetCmd(AnimOffsets&& offsets, std::function<void()> refreshCb, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _offsets(std::move(offsets))
        , _refreshCb(std::move(refreshCb))
    {
    }

    void    MoveOffsetCmd::redo()
    {
        for (auto& off : _offsets)
        {
            if  (off.anim)
                off.anim->setOffset(off.newOffset);
        }
        if  (_refreshCb)
            _refreshCb();
    }

    void    MoveOffsetCmd::undo()
    {
        for (auto& off : _offsets)
        {
            if  (off.anim)
                off.anim->setOffset(off.oldOffset);
        }
        if  (_refreshCb)
            _refreshCb();
    }

    /// =================== CreateAnimCmd ===================

    CreateAnimCmd::CreateAnimCmd(Node node, Animation anim, std::function<void()> refreshCb, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _node(node)
        , _anim(anim)
        , _refreshCb(std::move(refreshCb))
    {
    }

    void    CreateAnimCmd::redo()
    {
        if  (!_node || !_anim)
            return;
        _node->addComponent(_anim.get());
        auto    scene   =   _node->ctx().scene();
        if  (scene)
        {
            auto    sys =   scene->animationSystem();
            if  (sys)
                sys->addObject(_anim.get());
        }
        if  (_refreshCb)
            _refreshCb();
    }

    void    CreateAnimCmd::undo()
    {
        if  (!_node || !_anim)
            return;
        auto    scene   =   _node->ctx().scene();
        if  (scene)
        {
            auto    sys =   scene->animationSystem();
            if  (sys)
                sys->removeObject(_anim.get());
        }
        _node->removeComponent(_anim.get());
        if  (_refreshCb)
            _refreshCb();
    }

    /// =================== DeleteAnimCmd ===================

    DeleteAnimCmd::DeleteAnimCmd(AnimInfos&& infos, std::function<void()> refreshCb, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _infos(std::move(infos))
        , _refreshCb(std::move(refreshCb))
    {
    }

    void    DeleteAnimCmd::redo()
    {
        for (auto& info : _infos)
        {
            if  (!info.anim)
                continue;
            auto    scene   =   info.anim->ctx().scene();
            if  (scene)
            {
                auto    sys =   scene->animationSystem();
                if  (sys)
                    sys->removeObject(info.anim.get());
            }
            if  (info.ownerNode)
                info.ownerNode->removeComponent(info.anim.get());
        }
        if  (_refreshCb)
            _refreshCb();
    }

    void    DeleteAnimCmd::undo()
    {
        for (auto& info : _infos)
        {
            if  (!info.anim)
                continue;
            if  (info.ownerNode)
                info.ownerNode->addComponent(info.anim.get());
            auto    scene   =   info.anim->ctx().scene();
            if  (scene)
            {
                auto    sys =   scene->animationSystem();
                if  (sys)
                    sys->addObject(info.anim.get());
            }
        }
        if  (_refreshCb)
            _refreshCb();
    }

    /// =================== ToggleEnableCmd ===================

    ToggleEnableCmd::ToggleEnableCmd(Object obj, std::function<void()> refreshCb, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _obj(obj)
        , _refreshCb(std::move(refreshCb))
    {
    }

    void    ToggleEnableCmd::toggleEnable()
    {
        if  (!_obj)
            return;
        auto    anim    =   _obj->cast<FEAnimation>();
        auto    node    =   _obj->cast<FENode>();
        if  (anim != nullptr)
        {
            anim->setEnable(!anim->isEnable());
        }
        else if (node != nullptr)
        {
            if  (node->flags().hasFlag(FE::FLAG_ENABLE))
                node->flags().removeFlag(FE::FLAG_ENABLE);
            else
                node->flags().addFlag(FE::FLAG_ENABLE);
        }
    }

    void    ToggleEnableCmd::redo()
    {
        toggleEnable();
        if  (_refreshCb)
            _refreshCb();
    }

    void    ToggleEnableCmd::undo()
    {
        toggleEnable();
        if  (_refreshCb)
            _refreshCb();
    }

    /// =================== ToggleTreeObjectCmd ===================

    ToggleTreeObjectCmd::ToggleTreeObjectCmd(AnimationTree* tree, Object item, double curTime, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _tree(tree)
        , _item(item)
        , _curTime(curTime)
    {
    }

    void    ToggleTreeObjectCmd::redo()
    {
        if  (_tree)
            _tree->toggleObject(_item, _curTime);
    }

    void    ToggleTreeObjectCmd::undo()
    {
        if  (_tree)
            _tree->toggleObject(_item, _curTime);
    }

    /// =================== DeleteTrackCmd ===================

    DeleteTrackCmd::DeleteTrackCmd(TrackInfos&& infos, std::function<void()> refreshCb, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _infos(std::move(infos))
        , _refreshCb(std::move(refreshCb))
    {
    }

    void    DeleteTrackCmd::redo()
    {
        for (auto& info : _infos)
        {
            if  (info.clip && info.track)
                info.clip->removeObject(info.track.get());
        }
        if  (_refreshCb)
            _refreshCb();
    }

    void    DeleteTrackCmd::undo()
    {
        for (auto& info : _infos)
        {
            if  (info.clip && info.track)
                info.clip->addTrack(info.track);
        }
        if  (_refreshCb)
            _refreshCb();
    }

    /// =================== ToggleTrackEnableCmd ===================

    ToggleTrackEnableCmd::ToggleTrackEnableCmd(KeyFrameTrack track, std::function<void()> refreshCb, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _track(track)
        , _refreshCb(std::move(refreshCb))
    {
    }

    void    ToggleTrackEnableCmd::redo()
    {
        if  (!_track)
            return;
        if  (_track->flags().hasFlag(FE::FLAG_ENABLE))
            _track->flags().removeFlag(FE::FLAG_ENABLE);
        else
            _track->flags().addFlag(FE::FLAG_ENABLE);
        if  (_refreshCb)
            _refreshCb();
    }

    void    ToggleTrackEnableCmd::undo()
    {
        if  (!_track)
            return;
        if  (_track->flags().hasFlag(FE::FLAG_ENABLE))
            _track->flags().removeFlag(FE::FLAG_ENABLE);
        else
            _track->flags().addFlag(FE::FLAG_ENABLE);
        if  (_refreshCb)
            _refreshCb();
    }

    /// =================== RemoveFromTreeCmd ===================

    RemoveFromTreeCmd::RemoveFromTreeCmd(AnimationTree* tree, Object item, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _tree(tree)
        , _item(item)
    {
    }

    void    RemoveFromTreeCmd::redo()
    {
        if  (_tree)
        {
            _tree->removeObjectFromList(_item);
            _tree->updateUi();
        }
    }

    void    RemoveFromTreeCmd::undo()
    {
        if  (_tree)
        {
            _tree->addObjectToList(_item);
            _tree->updateUi();
        }
    }

    /// =================== ClearTreeCmd ===================

    ClearTreeCmd::ClearTreeCmd(AnimationTree* tree, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _tree(tree)
    {
        if  (_tree)
            _savedObjects =   _tree->snapshotObjectList();
    }

    void    ClearTreeCmd::redo()
    {
        if  (_tree)
        {
            _tree->clearObjectList();
            _tree->updateUi();
        }
    }

    void    ClearTreeCmd::undo()
    {
        if  (_tree)
        {
            _tree->setObjectList(_savedObjects);
            _tree->updateUi();
        }
    }

    /// =================== MoveNodeCmd ===================

    MoveNodeCmd::MoveNodeCmd(ObjectMoves&& moves, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _moves(std::move(moves))
    {
    }

    real3   MoveNodeCmd::getPosition(Object obj)
    {
        if  (!obj)
            return  real3(0.0);
        auto    val =   obj->getProperty(PROP_G_TRANSFORM_XYZ);

        if (std::holds_alternative<real3>(val))
            return  std::get<real3>(val);
        else
            return  real3(0.0);
    }

    void    MoveNodeCmd::setPosition(Object object, const real3& pos)
    {
        if  (!object)
            return;
        auto    val     =   object->getProperty(PROP_TRANSFORM_XYZ);
        if (!std::holds_alternative<real3>(val))
            return;
        object->beginSetProp();
        bool    bModify    =   object->setProperty(PROP_TRANSFORM_XYZ,pos);
        object->endSetProp(0,bModify);
    }

    void    MoveNodeCmd::redo()
    {
        for (auto& m : _moves)
        {
            if  (m.obj)
                setPosition(m.obj, m.newPos);
        }
    }

    void    MoveNodeCmd::undo()
    {
        for (auto& m : _moves)
        {
            if  (m.obj)
                setPosition(m.obj, m.oldPos);
        }
    }

    /// =================== NodePropsCmd ===================

    NodePropsCmd::NodePropsCmd(Node node, PropChanges&& changes, std::function<void()> refreshCb, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _node(node)
        , _changes(std::move(changes))
        , _refreshCb(std::move(refreshCb))
    {
    }

    real    NodePropsCmd::readProp(Node node, PropIndex prop)
    {
        if  (!node)
            return  0;
        if  (prop == PROP_VISIBLE)
            return  node->flags().hasFlag(FLAG_VISIBLE) ? 1.0 : 0.0;

        /// FENode 把属性接口重写为 protected,经基类 FEObject 公有接口调用(虚分发仍到 FENode)
        auto    val =   static_cast<FEObject*>(node.get())->getProperty(prop);
        return  std::visit([](auto&& x) -> real
        {
            using   T   =   std::decay_t<decltype(x)>;
            if  constexpr (std::is_arithmetic_v<T>)
                return  real(x);
            else
                return  0.0;
        }, val);
    }

    void    NodePropsCmd::writeProp(Node node, PropIndex prop, real val)
    {
        if  (!node)
            return;
        switch  (prop)
        {
        /// 位置/缩放:引擎 setProperty 已支持,走 beginSetProp/endSetProp 与移动编辑器一致
        case    PROP_TRANSFORM_X:
        case    PROP_TRANSFORM_Y:
        case    PROP_TRANSFORM_Z:
        case    PROP_SCALE_X:
        case    PROP_SCALE_Y:
        case    PROP_SCALE_Z:
        {
            /// FENode 把属性接口重写为 protected,经基类 FEObject 公有接口调用(虚分发仍到 FENode)
            auto*   obj     =   static_cast<FEObject*>(node.get());
            obj->beginSetProp();
            bool    bModify =   obj->setProperty(prop, KFValue(val));
            obj->endSetProp(0, bModify);
            break;
        }
        /// 旋转:引擎 setProperty(PROP_ROTATE_X/Y/Z) 暂未实现,按欧拉角(度)重建四元数
        case    PROP_ROTATE_X:
        case    PROP_ROTATE_Y:
        case    PROP_ROTATE_Z:
        {
            auto    euler   =   quatToEuler(node->localRotation());
            if  (prop == PROP_ROTATE_X)
                euler.x =   DEG2RAD(val);
            else if (prop == PROP_ROTATE_Y)
                euler.y =   DEG2RAD(val);
            else
                euler.z =   DEG2RAD(val);
            node->setLocalRotation(eulerToQuat(euler));
            node->makeDirty();
            node->update(0);
            node->fireChanged();
            break;
        }
        /// 显隐:与 QtTree 勾选逻辑一致,打 FLAG_PROP_STATE 并 fireChanged 同步实例
        case    PROP_VISIBLE:
        {
            if  (val > 0.5)
                node->flags().addFlag(FLAG_VISIBLE | FLAG_UPDATE | FENode::FLAG_PROP_STATE);
            else
            {
                node->flags().removeFlag(FLAG_VISIBLE);
                node->flags().addFlag(FENode::FLAG_PROP_STATE);
            }
            node->fireChanged();
            break;
        }
        default:
            break;
        }
    }

    void    NodePropsCmd::redo()
    {
        for (auto& c : _changes)
            writeProp(_node, c.prop, c.newVal);
        if  (_refreshCb)
            _refreshCb();
    }

    void    NodePropsCmd::undo()
    {
        for (auto& c : _changes)
            writeProp(_node, c.prop, c.oldVal);
        if  (_refreshCb)
            _refreshCb();
    }

    /// =================== PropertyKeyCmd ===================

    static  PropIndex   groupPropIndex(PropIndex prop)
    {
        switch  (prop)
        {
        case    PROP_TRANSFORM_X:
        case    PROP_TRANSFORM_Y:
        case    PROP_TRANSFORM_Z:
            return  PROP_TRANSFORM_XYZ;
        case    PROP_SCALE_X:
        case    PROP_SCALE_Y:
        case    PROP_SCALE_Z:
            return  PROP_SCALE_XYZ;
        case    PROP_ROTATE_X:
        case    PROP_ROTATE_Y:
        case    PROP_ROTATE_Z:
            return  PROP_ROTATE_XYZ;
        default:
            return  prop;
        }
    }

    static  uint32      exactPropBit(PropIndex prop)
    {
        switch  (prop)
        {
        case    PROP_TRANSFORM_X:    return  NP_TRANSFORM_X;
        case    PROP_TRANSFORM_Y:    return  NP_TRANSFORM_Y;
        case    PROP_TRANSFORM_Z:    return  NP_TRANSFORM_Z;
        case    PROP_TRANSFORM_XYZ:  return  NP_TRANSFORM_XYZ;
        case    PROP_SCALE_X:        return  NP_SCALE_X;
        case    PROP_SCALE_Y:        return  NP_SCALE_Y;
        case    PROP_SCALE_Z:        return  NP_SCALE_Z;
        case    PROP_SCALE_XYZ:      return  NP_SCALE_XYZ;
        case    PROP_ROTATE_X:       return  NP_ROTATE_X;
        case    PROP_ROTATE_Y:       return  NP_ROTATE_Y;
        case    PROP_ROTATE_Z:       return  NP_ROTATE_Z;
        case    PROP_ROTATE_XYZ:     return  NP_ROTATE_XYZ;
        case    PROP_VISIBLE:        return  NP_VISIBLE;
        default:                     return  0;
        }
    }

    PropertyKeyCmd::PropertyKeyCmd(Node node, Animation anim, PropIndex prop, real time, bool add
        , std::function<void()> refreshCb, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _node(node)
        , _anim(anim)
        , _prop(prop)
        , _time(time)
        , _add(add)
        , _refreshCb(std::move(refreshCb))
    {
    }

    NodeProperyBit  PropertyKeyCmd::groupBit(PropIndex prop)
    {
        switch  (prop)
        {
        case    PROP_TRANSFORM_X:
        case    PROP_TRANSFORM_Y:
        case    PROP_TRANSFORM_Z:
        case    PROP_TRANSFORM_XYZ:
            return  NP_TRANSFORM_XYZ;
        case    PROP_SCALE_X:
        case    PROP_SCALE_Y:
        case    PROP_SCALE_Z:
        case    PROP_SCALE_XYZ:
            return  NP_SCALE_XYZ;
        case    PROP_ROTATE_X:
        case    PROP_ROTATE_Y:
        case    PROP_ROTATE_Z:
        case    PROP_ROTATE_XYZ:
            return  NP_ROTATE_XYZ;
        case    PROP_VISIBLE:
            return  NP_VISIBLE;
        default:
            return  (NodeProperyBit)0;
        }
    }

    KeyFrameTracks  PropertyKeyCmd::targetTracks(Animation anim, PropIndex prop)
    {
        KeyFrameTracks  result;
        if  (!anim || !anim->clip())
            return  result;
        auto&   all =   anim->clip()->tracks();
        for (auto& track : all)
        {
            if  (track && track->propertyIndex() == prop)
                result.push_back(track);
        }
        if  (!result.empty())
            return  result;
        /// 回退到 XYZ 组合轨道
        PropIndex   groupProp   =   groupPropIndex(prop);
        if  (groupProp != prop)
        {
            for (auto& track : all)
            {
                if  (track && track->propertyIndex() == groupProp)
                    result.push_back(track);
            }
        }
        return  result;
    }

    bool    PropertyKeyCmd::hasKeyAt(Animation anim, PropIndex prop, real time)
    {
        if  (!anim)
            return  false;
        real    internalTime    =   time - anim->offset();
        for (auto& track : targetTracks(anim, prop))
        {
            if  (!track->_times)
                continue;
            auto&   times   =   track->_times->values();
            if  (std::find(times.begin(), times.end(), internalTime) != times.end())
                return  true;
        }
        return  false;
    }

    uint32  PropertyKeyCmd::helperBit(Animation anim, PropIndex prop)
    {
        /// 存在精确轴轨道时使用轴位,否则使用 XYZ 组位
        if  (anim && anim->clip())
        {
            for (auto& track : anim->clip()->tracks())
            {
                if  (track && track->propertyIndex() == prop)
                    return  exactPropBit(prop);
            }
        }
        return  (uint32)groupBit(prop);
    }

    void    PropertyKeyCmd::doAdd()
    {
        _createdTracks      =   false;
        _createdTrackPtrs.clear();
        auto    clip    =   _anim ? _anim->clip() : nullptr;
        if  (clip && targetTracks(_anim, _prop).empty())
        {
            /// 目标轨道缺失:按属性组补建(空 clip 同样在此补建)
            FEAnimationHelper::addDefaultTracks(clip, _node->ctx(), NodeProperyBits(groupBit(_prop)));
            _createdTracks      =   true;
            _createdTrackPtrs   =   targetTracks(_anim, _prop);
        }
        FEAnimationHelper::addNodeKeyFrame(_anim, _time, _node, NodeProperyBits(helperBit(_anim, _prop)));
    }

    void    PropertyKeyCmd::doRemoveAdded()
    {
        real    internalTime    =   _time - _anim->offset();
        for (auto& track : targetTracks(_anim, _prop))
            track->removeKeyFrame(internalTime);
        /// 仅清理本次补建且已为空的轨道,不影响其他轨道组
        if  (_createdTracks && _anim->clip())
        {
            for (auto& track : _createdTrackPtrs)
            {
                if  (track && !track->isValid())
                    _anim->clip()->removeObject(track);
            }
        }
        _createdTracks      =   false;
        _createdTrackPtrs.clear();
    }

    void    PropertyKeyCmd::doCaptureRemove()
    {
        _snapshots.clear();
        real    internalTime    =   _time - _anim->offset();
        for (auto& track : targetTracks(_anim, _prop))
        {
            if  (!track->_times)
                continue;
            auto&   times   =   track->_times->values();
            auto    itr     =   std::find(times.begin(), times.end(), internalTime);
            if  (itr == times.end())
                continue;
            size_t  idx     =   (size_t)std::distance(times.begin(), itr);
            Snapshot    snap;
            snap.track  =   track;
            snap.time   =   internalTime;
            snap.val    =   readTrackValue(track->_values, idx);
            track->removeKeyFrame(internalTime);
            _snapshots.push_back(std::move(snap));
        }
    }

    void    PropertyKeyCmd::doRestore()
    {
        for (auto& snap : _snapshots)
        {
            if  (snap.track)
                snap.track->addKeyFrame(snap.time, snap.val);
        }
        _snapshots.clear();
    }

    void    PropertyKeyCmd::redo()
    {
        if  (_add)
            doAdd();
        else
            doCaptureRemove();
        if  (_refreshCb)
            _refreshCb();
    }

    void    PropertyKeyCmd::undo()
    {
        if  (_add)
            doRemoveAdded();
        else
            doRestore();
        if  (_refreshCb)
            _refreshCb();
    }

    /// =================== ReparentNodeCmd ===================

    ReparentNodeCmd::ReparentNodeCmd(Node node, FENode* newParent, std::function<void()> cb, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _node(node)
        , _newParent(newParent)
        , _refreshCb(cb)
    {
        _oldParent    =   _node ? _node->parent() : nullptr;
        _oldTopLevel  =   (_oldParent == nullptr);
        auto    scene   =   _node ? _node->ctx().scene() : nullptr;
        if (scene)
        {
            if (_oldTopLevel)
            {
                auto&   tops    =   scene->nodeTree().topLevelNodes();
                auto    it      =   std::find(tops.begin(), tops.end(), _node);
                _oldIndex       =   (it != tops.end()) ? (size_t)std::distance(tops.begin(), it) : 0;
            }
            else
            {
                auto&   chs     =   _oldParent->children();
                auto    it      =   std::find(chs.begin(), chs.end(), _node);
                _oldIndex       =   (it != chs.end()) ? (size_t)std::distance(chs.begin(), it) : 0;
            }
        }
        real3   pos; real3   scale; quatr   rot;
        FE::decompose<real>(_node->globalTransform(), pos, scale, rot);
        _oldTrans   =   pos;
        _oldRot     =   rot;
        _oldScale   =   scale;
    }

    void    ReparentNodeCmd::redo()
    {
        if (!_node)
            return;
        auto    scene   =   _node->ctx().scene();
        if (scene == nullptr)
            return;
        auto&   tree    =   scene->nodeTree();
        if (_newParent != nullptr)
        {
            /// 顶层->子: reparent 不会从 topLevelNodes 摘除,需先手动摘除
            if (_oldTopLevel)
                tree.removeToplevelNode(_node);
            _node->reparent(_newParent);
        }
        else
        {
            /// ->顶层: 仅在原非顶层时执行(顶层->顶层无意义,canDropOn 已拦截)
            if (!_oldTopLevel)
            {
                _node->reparent(nullptr);
                tree.addToplevelNode(_node);
            }
        }
        _node->makeDirty();
        _node->update();
        if (_refreshCb)
            _refreshCb();
    }

    void    ReparentNodeCmd::undo()
    {
        if (!_node)
            return;
        auto    scene   =   _node->ctx().scene();
        if (scene == nullptr)
            return;
        auto&   tree    =   scene->nodeTree();
        if (_oldTopLevel)
        {
            /// 恢复为顶层节点
            if (_newParent != nullptr)
            {
                /// 当前为 _newParent 子节点,先 reparent(nullptr) 摘除再加回顶层
                _node->reparent(nullptr);
                tree.addToplevelNode(_node);
            }
            /// else: 顶层->顶层(无意义场景), 当前已在顶层,无需操作
        }
        else
        {
            /// 恢复为 _oldParent 子节点
            if (_newParent == nullptr)
            {
                /// 当前为顶层,先从顶层摘除再 reparent 到旧父(reparent 对顶层节点不会摘除顶层表)
                tree.removeToplevelNode(_node);
            }
            _node->reparent(_oldParent.get());
        }
        /// 恢复旧世界变换(reparent 已保持世界变换,此处覆盖以应对 redo/undo 间节点被移动;
        /// FE 无 setGlobalScaling,缩放由 reparent 保持的 global 矩阵承载,此处不单独恢复)
        _node->setGlobalTranslation(_oldTrans);
        _node->setGlobalRotation(_oldRot);
        _node->makeDirty();
        _node->update();
        if (_refreshCb)
            _refreshCb();
    }

    /// =================== ReorderNodeCmd ===================

    ReorderNodeCmd::ReorderNodeCmd(Node node, FENode* targetSibling, bool after, std::function<void()> cb, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _node(node)
        , _refreshCb(cb)
    {
        _parent =   _node ? _node->parent() : nullptr;
        if (!_parent)
            return;
        auto&   chs         =   _parent->children();
        auto    nodeIt      =   std::find_if(chs.begin(), chs.end()
            , [this](const Node& n) { return n.get() == _node.get(); });
        _oldIndex           =   (nodeIt != chs.end()) ? (size_t)std::distance(chs.begin(), nodeIt) : 0;
        auto    targetIt    =   std::find_if(chs.begin(), chs.end()
            , [targetSibling](const Node& n) { return n.get() == targetSibling; });
        size_t  targetIdx   =   (targetIt != chs.end()) ? (size_t)std::distance(chs.begin(), targetIt) : 0;
        /// 目标在拖拽节点之后时,移除自身后其索引前移一位(以移除后的列表为基准)
        size_t  art         =   (targetIdx > _oldIndex) ? targetIdx - 1 : targetIdx;
        _newIndex           =   after ? art + 1 : art;
    }

    void    ReorderNodeCmd::redo()
    {
        if (!_node || !_parent)
            return;
        /// 同父排序: reparent 现在支持 index 参数,同父时不 early return,先摘再挂到新位置
        _node->reparent(_parent.get(), _newIndex);
        if (_refreshCb)
            _refreshCb();
    }

    void    ReorderNodeCmd::undo()
    {
        if (!_node || !_parent)
            return;
        _node->reparent(_parent.get(), _oldIndex);
        if (_refreshCb)
            _refreshCb();
    }

    /// =================== PasteNodeCmd ===================

    PasteNodeCmd::PasteNodeCmd(Node clipSource, FENode* parent, std::function<void(PasteNodeCmd&,bool)> cb, QUndoCommand* parentCmd)
        : QUndoCommand(parentCmd)
        , _clipSource(clipSource)
        , _parent(parent)
        , _refreshCb(cb)
    {
    }

    void    PasteNodeCmd::redo()
    {
        _node   =   nullptr;
        {
            std::ofstream f("d:/miniSoftRender/clipdebug.log", std::ios::app);
            if (f.is_open()) f << "[PasteNodeCmd::redo] _clipSource=" << (void*)_clipSource.get() << " _parent=" << (void*)_parent << std::endl;
        }
        if (!_clipSource)
        {
            std::ofstream f("d:/miniSoftRender/clipdebug.log", std::ios::app);
            if (f.is_open()) f << "[PasteNodeCmd::redo] BAIL: _clipSource null" << std::endl;
            if (_refreshCb) _refreshCb(*this, true);
            return;
        }
        /// 深拷贝克隆(子树+组件,mesh/material 共享)
        auto cloned = _clipSource->clone();
        _node   =   cloned->as<FENode>();
        {
            std::ofstream f("d:/miniSoftRender/clipdebug.log", std::ios::app);
            if (f.is_open()) f << "[PasteNodeCmd::redo] clone=" << (void*)cloned.get() << " as<FENode>=" << (void*)_node.get() << std::endl;
        }
        if (!_node)
        {
            std::ofstream f("d:/miniSoftRender/clipdebug.log", std::ios::app);
            if (f.is_open()) f << "[PasteNodeCmd::redo] BAIL: clone cast failed" << std::endl;
            if (_refreshCb) _refreshCb(*this, true);
            return;
        }
        /// 清子树 FLAG_SELECTED(避免假选中)
        _node->flags().removeFlag(FE::FLAG_SELECTED);
        _node->traverseObject([](const FEObject& obj, const FEObject&, const FEObject::FETrvsCtx&, uint)->bool
        {
            const_cast<FEObject&>(obj).flags().removeFlag(FE::FLAG_SELECTED);
            return  true;
        });
        auto    scene   =   _node->ctx().scene();
        if (scene)
        {
            /// 挂接: 父非空挂为子节点,空挂为顶层
            if (_parent)
                _parent->addChild(_node);
            else
                scene->nodeTree().addToplevelNode(_node);
            /// 注册渲染(节点+组件分发到各系统/工厂)
            scene->dispatchNodesToSystem({_node});
        }
        _node->makeDirty();
        _node->update();
        {
            std::ofstream f("d:/miniSoftRender/clipdebug.log", std::ios::app);
            if (f.is_open()) f << "[PasteNodeCmd::redo] DONE childCount=" << (_parent ? _parent->children().size() : 0) << std::endl;
        }
        if (_refreshCb)
            _refreshCb(*this, true);
    }

    void    PasteNodeCmd::undo()
    {
        if (!_node)
        {
            if (_refreshCb) _refreshCb(*this, false);
            return;
        }
        auto    scene   =   _node->ctx().scene();
        if (scene)
        {
            /// 摘除(触发 eventsRemoveNode / _NRemoveNodes,树监听刷新)
            scene->nodeTree().removeNode(_node);
            /// FEScene 未订阅 eventsRemoveNode,需手动遍历各工厂清渲染引用,避免悬挂
            for (auto factory : scene->factoryMgr().objects())
                factory->removeNode(_node);
        }
        _node   =   nullptr;
        if (_refreshCb)
            _refreshCb(*this, false);
    }
}
