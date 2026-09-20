#pragma once
#include    <QUndoCommand>
#include    <functional>
#include    <vector>

#include    "node/FENode.hpp"
#include    "animation/FEKeyFrameTrack.hpp"
#include    "animation/FEAnimClip.hpp"
#include    "animation/FEAnimation.hpp"

namespace   FE
{
    class RenameNodeCmd : public QUndoCommand
    {
    public:
        RenameNodeCmd(Node node, const char* redoName, const char* undoName, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
    public:
        Node        _pNode;
        String      _redoName;
        String      _undoName;
    };

    /// <summary>
    /// 添加关键帧命令,支持 undo/redo
    /// 每个 AnimState 记录一个 (node, anim) 对的操作前快照
    /// </summary>
    class AddKeyframeCmd : public QUndoCommand
    {
    public:
        struct TrackSnapshot
        {
            KeyFrameTrack       track       =   nullptr;
            KFValue             oldVal      =   {};
            bool                existed     =   false;
        };
        using   TrackSnapshots   =   std::vector<TrackSnapshot>;
        struct AnimState
        {
            Node                node;
            Animation           anim;
            real                time        =   0;
            TrackSnapshots      snapshots;
            bool                createdNew  =   false;
        };
        using   AnimStates      =   std::vector<AnimState>;
    public:
        AddKeyframeCmd(AnimStates&& states, std::function<void()> refreshCb, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
        /// <summary>
        /// 对指定动画的所有轨道做时间点快照(供调用方在 push 前捕获状态)
        /// </summary>
        static  TrackSnapshots  snapshotTracks(Animation anim, real time);
    private:
        std::vector<AnimState>      _states;
        std::function<void()>       _refreshCb;
    };
}
