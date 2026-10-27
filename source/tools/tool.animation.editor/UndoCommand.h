#pragma once
#include    <QUndoCommand>
#include    <functional>
#include    <vector>

#include    "node/FENode.hpp"
#include    "FEPropertyIndex.hpp"
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

    /// <summary>
    /// 节点标量属性编辑命令(属性面板:位置/旋转欧拉角/缩放/显隐)
    /// 旋转属性引擎 setProperty(PROP_ROTATE_X/Y/Z) 暂未实现,
    /// 命令内部按欧拉角(度)重建四元数后写回 localRotation
    /// redo: 写入新值; undo: 恢复旧值
    /// </summary>
    class NodePropsCmd : public QUndoCommand
    {
    public:
        struct PropChange
        {
            PropIndex   prop    =   0;
            real        oldVal  =   0;
            real        newVal  =   0;
        };
        using PropChanges = std::vector<PropChange>;
    public:
        NodePropsCmd(Node node, PropChanges&& changes, std::function<void()> refreshCb, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
        /// 读取节点标量属性(位置/缩放/旋转欧拉角度/显隐 0/1)
        static  real    readProp(Node node, PropIndex prop);
        /// 写入节点标量属性(旋转走四元数重建,显隐走 FLAG_VISIBLE)
        static  void    writeProp(Node node, PropIndex prop, real val);
    private:
        Node                    _node;
        PropChanges             _changes;
        std::function<void()>   _refreshCb;
    };

    /// <summary>
    /// 单个属性轨道关键帧增删命令(属性面板行尾轨道勾选框)
    /// add=true:  redo 勾选加帧 / undo 删帧
    /// add=false: redo 取消勾选删帧(带快照) / undo 恢复
    /// 目标轨道优先精确轴轨道(如 PROP_TRANSFORM_X),不存在时回退 XYZ 组合轨道
    /// 目标轨道整体缺失时按属性组补建轨道(空 clip 同样补建)
    /// </summary>
    class PropertyKeyCmd : public QUndoCommand
    {
    public:
        PropertyKeyCmd(Node node, Animation anim, PropIndex prop, real time, bool add
            , std::function<void()> refreshCb, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
        /// 查找属性对应的目标轨道(精确轴轨道 -> XYZ 组合轨道)
        static  KeyFrameTracks  targetTracks(Animation anim, PropIndex prop);
        /// 时间线时间 time 处目标轨道是否已有关键帧
        static  bool            hasKeyAt(Animation anim, PropIndex prop, real time);
        /// <summary>
        /// addNodeKeyFrame 使用的位标记:存在精确轨道用轴位,否则用 XYZ 组位
        /// </summary>
        static  uint32          helperBit(Animation anim, PropIndex prop);
        /// <summary>
        /// 属性所属轨道组的位标记(用于补建轨道)
        /// </summary>
        static  NodeProperyBit  groupBit(PropIndex prop);
        /// <summary>
        /// 属性是否被该动画的轨道接管(接管后节点本地值不再直接生效)
        /// </summary>
        static  bool            isDriven(Animation anim, PropIndex prop);
        /// <summary>
        /// 读取属性的显示值:被轨道接管时取动画采样值,否则取节点本地值
        /// </summary>
        static  bool            readDriven(Animation anim, PropIndex prop, real& value);
        /// <summary>
        /// 把节点上刚编辑的分量写入动画采样变换
        /// 动画覆盖本地变换后,编辑值必须先落到采样变换再写关键帧,否则记录到的仍是旧值
        /// </summary>
        static  void            syncFromNode(Animation anim, Node node, PropIndex prop);
    private:
        void    doAdd();
        void    doRemoveAdded();
        void    doCaptureRemove();
        void    doRestore();
        struct Snapshot
        {
            KeyFrameTrack   track;
            real            time    =   0;
            KFValue         val;
        };
    private:
        Node                    _node;
        Animation               _anim;
        PropIndex               _prop;
        real                    _time;
        bool                    _add;
        /// 本次勾选是否补建了轨道(undo 时需清理空轨道)
        bool                    _createdTracks      =   false;
        /// 补建的轨道指针,用于精确清理
        KeyFrameTracks          _createdTrackPtrs;
        std::vector<Snapshot>   _snapshots;
        std::function<void()>   _refreshCb;
    };

    /// <summary>
    /// 重挂父节点命令(模型树拖拽调整层级),支持 undo/redo
    /// 层级变更统一走 FENodeTree::setParent:它同时维护父子关系与顶层节点表
    /// (_topLevelNodes),并保持世界变换;FENode::reparent 只是内部实现,不再直接调用。
    /// 顶层节点索引在 undo 时无法精确恢复(FENodeTree 无 insertAt 接口),追加到末尾。
    /// </summary>
    class ReparentNodeCmd : public QUndoCommand
    {
    public:
        ReparentNodeCmd(Node node, FENode* newParent, std::function<void()> cb, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
    private:
        Node                    _node;
        FENode*                 _newParent;
        /// 原父节点(保活,空=原顶层)
        Node                    _oldParent;
        /// 原是否顶层节点
        bool                    _oldTopLevel   =   false;
        /// 原在父/顶层列表中的索引(undo 尽力恢复位置,顶层仅记录)
        size_t                  _oldIndex      =   0;
        /// 旧世界变换(reparent 已保持世界变换,此处冗余备份以应对 redo/undo 间节点被移动)
        real3                   _oldTrans;
        quatr                   _oldRot;
        real3                   _oldScale;
        std::function<void()>   _refreshCb;
    };

    /// <summary>
    /// 同层级排序命令(模型树拖拽调整兄弟顺序),支持 undo/redo
    /// FE 无 nodeIndex/insertChild,同父排序靠手动操作 parent->children() 向量:
    ///   erase 拖拽节点,insert 到目标位置;不调 onAddChild/onRemoveChild,刷新靠 refreshCb 调 modelTree->update()
    /// </summary>
    class ReorderNodeCmd : public QUndoCommand
    {
    public:
        ReorderNodeCmd(Node node, FENode* targetSibling, bool after, std::function<void()> cb, QUndoCommand* parent = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
    private:
        Node                    _node;
        Node                    _parent;
        size_t                  _oldIndex   =   0;
        size_t                  _newIndex   =   0;
        std::function<void()>   _refreshCb;
    };

    /// <summary>
    /// 粘贴节点命令(Ctrl+C 后 Ctrl+V 复制粘贴),支持 undo/redo
    /// redo: 从剪贴板源深拷贝克隆(clone 完整子树+组件,mesh/material 共享),
    ///       清子树 FLAG_SELECTED,挂接到目标父(空=顶层),dispatchNodesToSystem 注册渲染;
    /// undo: nodeTree->removeNode 摘除 + 遍历 factoryMgr 各工厂 removeNode 清渲染(FEScene 未订阅 eventsRemoveNode),
    ///       释放克隆。剪贴板源节点由 MainWindow 持有保活。
    /// </summary>
    class PasteNodeCmd : public QUndoCommand
    {
    public:
        PasteNodeCmd(Node clipSource, FENode* parent, std::function<void(PasteNodeCmd&,bool)> cb, QUndoCommand* parentCmd = nullptr);
    public:
        virtual void    redo() override;
        virtual void    undo() override;
        /// 获取最近一次 redo 克隆出的新节点(undo 后为空;push 同步执行 redo,可用于粘贴后选中)
        Node            node() const { return _node; }
        FENode*         parent() const { return _parent; }
    private:
        Node                    _clipSource;
        FENode*                 _parent;
        Node                    _node;
        std::function<void(PasteNodeCmd&,bool)> _refreshCb;
    };
}
