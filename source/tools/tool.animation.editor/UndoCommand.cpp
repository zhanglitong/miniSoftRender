#include    "UndoCommand.h"
#include    "AnimationTree.h"
#include    "animation/FEAnimationHelper.hpp"
#include    "animation/FEAnimationSys.hpp"
#include    "graphic/FEScene.h"
#include    "animation/FETrackResult.hpp"

namespace   FE
{
    using   TrackSnapshots  =   AddKeyframeCmd::TrackSnapshots;
    using   AnimStates      =   AddKeyframeCmd::AnimStates;

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

    /// <summary>
    /// 对指定动画的所有轨道做时间点快照
    /// </summary>
    TrackSnapshots  AddKeyframeCmd::snapshotTracks(Animation anim, real time)
    {
        TrackSnapshots  snaps;
        if  (!anim || !anim->clip())
            return  snaps;
        /// 时间线时间转内部时间
        real    internalTime    =   time - anim->offset();
        for (auto track : anim->clip()->tracks())
        {
            AddKeyframeCmd::TrackSnapshot  snap;
            snap.track  =   track;
            snap.existed=   false;
            if  (track->_times)
            {
                auto&   times   =   track->_times->values();
                auto    itr     =   std::find(times.begin(), times.end(), internalTime);
                if  (itr != times.end())
                {
                    snap.existed=   true;
                    size_t  idx =   (size_t)std::distance(times.begin(), itr);
                    snap.oldVal =   readTrackValue(track->_values, idx);
                }
            }
            snaps.push_back(std::move(snap));
        }
        return  snaps;
    }

    AddKeyframeCmd::AddKeyframeCmd(AnimStates&& states, std::function<void()> refreshCb, QUndoCommand* parent)
        : QUndoCommand(parent)
        , _states(std::move(states))
        , _refreshCb(std::move(refreshCb))
    {
    }

    void    AddKeyframeCmd::redo()
    {
        for (auto& st : _states)
        {
            /// 新建动画: 先挂回节点和动画系统
            if (st.createdNew)
            {
                st.node->addComponent(st.anim.get());
                auto    scene   =   st.node->ctx().scene();
                if  (scene)
                {
                    auto    sys =   scene->animationSystem();
                    if  (sys)
                        sys->addObject(st.anim.get());
                }
            }
            /// 空 clip: 先创建默认轨道,再添加关键帧
            if  (st.createdDefaultTracks && st.anim && st.anim->clip())
            {
                FEAnimationHelper::addDefaultTracks(st.anim->clip(), st.node->ctx());
            }
            /// 添加/更新关键帧
            if  (!FEAnimationHelper::addNodeKeyFrame(st.anim, st.time, st.node))
                FEAnimationHelper::updateNodeKeyFrame(st.anim, st.time, st.node);
        }
        if  (_refreshCb)
            _refreshCb();
    }

    void    AddKeyframeCmd::undo()
    {
        for (auto& st : _states)
        {
            /// 时间线时间转内部时间
            real    internalTime    =   st.time - st.anim->offset();
            /// 逐轨道恢复: 原有 -> 还原旧值, 新增 -> 移除关键帧
            for (auto& snap : st.snapshots)
            {
                if  (!snap.track)
                    continue;
                if  (snap.existed)
                    snap.track->updateKeyFrame(internalTime, snap.oldVal);
                else
                    snap.track->removeKeyFrame(internalTime);
            }
            /// 空 clip 创建的默认轨道: undo 时清空
            if  (st.createdDefaultTracks && st.anim && st.anim->clip())
            {
                st.anim->clip()->clearObjects();
            }
            /// 新建动画: 从节点和动画系统移除
            if (st.createdNew)
            {
                auto    scene   =   st.node->ctx().scene();
                if  (scene)
                {
                    auto    sys =   scene->animationSystem();
                    if  (sys)
                        sys->removeObject(st.anim.get());
                }
                st.node->removeComponent(st.anim.get());
            }
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
}
