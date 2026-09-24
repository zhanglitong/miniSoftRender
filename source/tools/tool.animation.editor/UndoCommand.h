#pragma once
#include    <QUndoCommand>
#include    <functional>
#include    <vector>

#include    "node/FENode.hpp"
#include    "animation/FEKeyFrameTrack.hpp"
#include    "animation/FEAnimClip.hpp"
#include    "animation/FEAnimation.hpp"

class   AnimationTree;

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
    /// 创建关键帧命令,支持 undo/redo
    /// 当指定时间点不存在关键帧时使用
    /// redo: 创建默认轨道(若空clip) + 添加关键帧; undo: 移除关键帧 + 清空轨道(若空clip)
    /// </summary>
    class CreateKeyframeCmd : public QUndoCommand
    {
    public:
        CreateKeyframeCmd(Node node, Animation anim, real time, bool createdDefaultTracks, std::function<void()> refreshCb, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
    private:
        Node                    _node;
        Animation               _anim;
        real                    _time                    =   0;
        /// 标记此次操作是否为空 clip 创建了默认轨道
        /// redo 时创建,undo 时移除
        bool                    _createdDefaultTracks    =   false;
        std::function<void()>   _refreshCb;
    };

    /// <summary>
    /// 更新关键帧命令,支持 undo/redo
    /// 当指定时间点已存在关键帧时使用
    /// redo: 用节点当前属性覆盖关键帧值; undo: 从快照恢复旧值
    /// </summary>
    class UpdateKeyframeCmd : public QUndoCommand
    {
    public:
        struct TrackSnapshot
        {
            KeyFrameTrack       track       =   nullptr;
            KFValue             oldVal      =   {};
        };
        using   TrackSnapshots   =   std::vector<TrackSnapshot>;
    public:
        UpdateKeyframeCmd(Node node, Animation anim, real time, TrackSnapshots snapshots, std::function<void()> refreshCb, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
        /// <summary>
        /// 对指定动画做时间点快照
        /// 仅返回该时间点已存在关键帧的轨道及其旧值
        /// 返回为空表示该时间点无关键帧(应使用 CreateKeyframeCmd)
        /// </summary>
        static  TrackSnapshots  snapshotTracks(Animation anim, real time);
    private:
        Node                    _node;
        Animation               _anim;
        real                    _time        =   0;
        TrackSnapshots          _snapshots;
        std::function<void()>   _refreshCb;
    };

    /// <summary>
    /// 移动关键帧时间命令(部分拖动),支持 undo/redo
    /// 每个 TrackMove 记录一个 track 的旧/新时间数组和旧/新值数组
    /// </summary>
    class MoveKeyframesCmd : public QUndoCommand
    {
    public:
        struct TrackMove
        {
            KeyFrameTrack           track       =   nullptr;
            std::vector<real>       oldTimes;
            std::vector<real>       newTimes;
            std::vector<KFValue>    oldValues;
            std::vector<KFValue>    newValues;
        };
        using   TrackMoves  =   std::vector<TrackMove>;
    public:
        MoveKeyframesCmd(TrackMoves&& moves, std::function<void()> refreshCb, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
        /// 读取 track 值数组的全部内容(供调用方在 push 前捕获状态)
        static  std::vector<KFValue>    readAllValues(const ValueObject& values);
    private:
        TrackMoves               _moves;
        std::function<void()>    _refreshCb;
    };

    /// <summary>
    /// 修改动画偏移命令(整体拖动/范围块拖动),支持 undo/redo
    /// </summary>
    class MoveOffsetCmd : public QUndoCommand
    {
    public:
        struct AnimOffset
        {
            Animation   anim        =   nullptr;
            real        oldOffset   =   0;
            real        newOffset   =   0;
        };
        using   AnimOffsets =   std::vector<AnimOffset>;
    public:
        MoveOffsetCmd(AnimOffsets&& offsets, std::function<void()> refreshCb, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
    private:
        AnimOffsets              _offsets;
        std::function<void()>    _refreshCb;
    };

    /// <summary>
    /// 创建动画命令,支持 undo/redo
    /// redo: 将动画添加到节点和动画系统; undo: 从节点和动画系统移除
    /// </summary>
    class CreateAnimCmd : public QUndoCommand
    {
    public:
        CreateAnimCmd(Node node, Animation anim, std::function<void()> refreshCb, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
    private:
        Node                    _node;
        Animation               _anim;
        std::function<void()>   _refreshCb;
    };

    /// <summary>
    /// 删除动画命令,支持 undo/redo
    /// 可批量删除多个动画,redo: 从节点和动画系统移除; undo: 重新添加
    /// </summary>
    class DeleteAnimCmd : public QUndoCommand
    {
    public:
        struct AnimInfo
        {
            Node        ownerNode;
            Animation   anim;
        };
        using AnimInfos = std::vector<AnimInfo>;
        DeleteAnimCmd(AnimInfos&& infos, std::function<void()> refreshCb, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
    private:
        AnimInfos               _infos;
        std::function<void()>   _refreshCb;
    };

    /// <summary>
    /// 启用/禁用动画命令,支持 undo/redo
    /// 对动画切换 setEnable, 对节点切换 FLAG_ENABLE
    /// redo 和 undo 都是 toggle 操作
    /// </summary>
    class ToggleEnableCmd : public QUndoCommand
    {
    public:
        ToggleEnableCmd(Object obj, std::function<void()> refreshCb, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
    private:
        void    toggleEnable();
        Object                  _obj;
        std::function<void()>   _refreshCb;
    };

    /// <summary>
    /// 添加到动画树/从动画树移除命令,支持 undo/redo
    /// redo 和 undo 都调用 AnimationTree::toggleObject, toggle 本身是其自身的逆操作
    /// </summary>
    class ToggleTreeObjectCmd : public QUndoCommand
    {
    public:
        ToggleTreeObjectCmd(AnimationTree* tree, Object item, double curTime, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
    private:
        AnimationTree*  _tree;
        Object          _item;
        double          _curTime;
    };

    /// <summary>
    /// 删除轨道命令,支持 undo/redo
    /// 可批量删除多个轨道,redo: 从 clip 移除; undo: 重新添加
    /// </summary>
    class DeleteTrackCmd : public QUndoCommand
    {
    public:
        struct TrackInfo
        {
            AnimClip        clip;
            KeyFrameTrack   track;
        };
        using TrackInfos = std::vector<TrackInfo>;
        DeleteTrackCmd(TrackInfos&& infos, std::function<void()> refreshCb, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
    private:
        TrackInfos              _infos;
        std::function<void()>   _refreshCb;
    };

    /// <summary>
    /// 启用/禁用轨道命令,支持 undo/redo
    /// 切换轨道的 FLAG_ENABLE, redo 和 undo 都是 toggle
    /// </summary>
    class ToggleTrackEnableCmd : public QUndoCommand
    {
    public:
        ToggleTrackEnableCmd(KeyFrameTrack track, std::function<void()> refreshCb, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
    private:
        KeyFrameTrack           _track;
        std::function<void()>   _refreshCb;
    };

    /// <summary>
    /// 从动画树对象列表移除命令,支持 undo/redo
    /// redo: 从 _objects 移除; undo: 添加回 _objects
    /// </summary>
    class RemoveFromTreeCmd : public QUndoCommand
    {
    public:
        RemoveFromTreeCmd(AnimationTree* tree, Object item, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
    private:
        AnimationTree*  _tree;
        Object          _item;
    };

    /// <summary>
    /// 清空动画树对象列表命令,支持 undo/redo
    /// redo: 清空 _objects; undo: 恢复 _objects
    /// </summary>
    class ClearTreeCmd : public QUndoCommand
    {
    public:
        ClearTreeCmd(AnimationTree* tree, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
    private:
        AnimationTree*  _tree;
        FE::Objects     _savedObjects;
    };

    /// <summary>
    /// 节点移动命令,支持 undo/redo
    /// 记录每个对象移动前后的位置
    /// 对象可以是 FENode (localTranslation) 或带 FETransform 接口的对象 (transform.position)
    /// redo: 设置新位置; undo: 恢复旧位置
    /// </summary>
    class MoveNodeCmd : public QUndoCommand
    {
    public:
        struct ObjectMove
        {
            Object  obj;
            real3   oldPos;
            real3   newPos;
        };
        using ObjectMoves = std::vector<ObjectMove>;
    public:
        MoveNodeCmd(ObjectMoves&& moves, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
        /// 读取对象当前位置 (FENode: localTranslation, 其他: transform.position)
        static  real3   getPosition(Object obj);
        /// 设置对象位置, 逻辑与 FENodeMoveEditor::onMAxis 一致
        static  void    setPosition(Object obj, const real3& pos);
    private:
        ObjectMoves _moves;
    };
}
