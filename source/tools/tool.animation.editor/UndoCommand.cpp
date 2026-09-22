#include    "UndoCommand.h"
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
}
