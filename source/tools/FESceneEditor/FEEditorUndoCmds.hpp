#pragma     once

#include    <cstdint>
#include    <functional>
#include    <vector>

#include    "../../../inc/FEUndoCmd.hpp"
#include    "../../../inc/animation/FEAnimation.hpp"
#include    "../../../inc/animation/FEAnimationSys.hpp"
#include    "../../../inc/constraint/FEConstraint.hpp"
#include    "../../../inc/constraint/FEConstraintSys.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/node/FENodeTree.hpp"

namespace   FE
{
    /// <summary>
    /// 创建节点命令(几何体/空节点/导入模型共用;单个节点即只含一个元素的批量):
    /// redo: 节点加入节点树(模型树显示)并分发到渲染工厂与各系统;
    /// undo: 从节点树摘除,并遍历所有工厂移除该节点的渲染引用
    ///       (FEScene 不订阅节点树的移除事件,必须手动清理,否则工厂内仍会绘制)。
    /// 命令持有 Nodes,undo/redo 往返期间对象保持存活;节点名需在构造前唯一化,
    /// 这样 redo 不会因场景中已有同名节点而改名。
    /// </summary>
    class   FEEditorCreateNodeCmd :public FEUndoCmd
    {
    public:
        FEEditorCreateNodeCmd(FEScene& scene,const Nodes& nodes,const String& text)
            :FEUndoCmd(text)
            ,_scene(scene)
            ,_nodes(nodes)
        {}
    public:
        virtual void    redo()  override
        {
            if (_nodes.empty())
                return;
            _scene.addNodesToTree(_nodes);
            _scene.dispatchNodesToSystem(_nodes);
        }
        virtual void    undo()  override
        {
            _scene.removeNodesFromTree(_nodes);
            _scene.removeNodesFromSystem(_nodes);
        }
    protected:
        FEScene&    _scene;
        Nodes       _nodes;
    };

    /// <summary>
    /// 调整节点层级/顺序命令(模型树拖拽,支持批量):
    /// 每条记录保存"被移动节点 + 移动前位置 + 移动后位置",
    /// 位置 = 父节点(为空表示顶层列表) + 同级列表下标。
    /// undo 逆序还原移动前的位置,redo 顺序重新应用移动后的位置;
    /// 下标语义与 FENodeTree::setParent / moveToplevelNode 一致(按移除该节点后的列表计算)。
    /// 模型树是先执行移动再入栈,故构造时传 alreadyApplied=true,
    /// 让 FEUndoStack::push 触发的第一次 redo() 成为空操作。
    /// 命令持有被移动节点的强引用,undo/redo 往返期间对象保持存活
    /// </summary>
    class   FEEditorMoveNodeCmd :public FEUndoCmd
    {
    public:
        /// <summary>
        /// 节点在树中的位置:父节点为空表示顶层列表
        /// </summary>
        struct  Placement
        {
            Node    _parent;
            size_t  _index  =   SIZE_MAX;
        };
    public:
        FEEditorMoveNodeCmd(FENodeTree& tree,const String& text,bool alreadyApplied
                            ,const std::function<void()>& refresh = {})
            :FEUndoCmd(text)
            ,_tree(tree)
            ,_refresh(refresh)
            ,_skipRedo(alreadyApplied)
        {}
        /// <summary>
        /// 追加一条移动记录;位置未发生变化(空操作)的记录会被忽略
        /// </summary>
        /// <param name="node">被移动的节点</param>
        /// <param name="oldPlacement">移动前的位置</param>
        /// <param name="newPlacement">移动后的位置</param>
        void    add(const Node& node,const Placement& oldPlacement,const Placement& newPlacement)
        {
            if (node == nullptr)
                return;
            if (oldPlacement._parent.get() == newPlacement._parent.get()
                && oldPlacement._index == newPlacement._index)
                return;
            _nodes.push_back(node);
            _old.push_back(oldPlacement);
            _new.push_back(newPlacement);
        }
        /// <summary>
        /// 是否一条记录都没有(空操作拖拽,不需要入栈)
        /// </summary>
        bool    empty() const
        {
            return  _nodes.empty();
        }
    public:
        virtual void    redo()  override
        {
            /// 位置已在构造本条命令时由模型树应用,第一次 redo 不重复执行
            if  (_skipRedo)
            {
                _skipRedo   =   false;
                return;
            }
            for (size_t i = 0; i < _nodes.size(); ++i)
                apply(_nodes[i],_new[i]);
            /// 顶层重排不触发节点增删事件,需要宿主手动刷新模型树
            if  (_refresh)
                _refresh();
        }
        virtual void    undo()  override
        {
            /// 逆序还原:后移动的先还原,保证同父重排时的下标仍然有效
            for (size_t i = _nodes.size(); i > 0; --i)
                apply(_nodes[i - 1],_old[i - 1]);
            if  (_refresh)
                _refresh();
        }
    protected:
        /// <summary>
        /// 把节点放回指定位置:父节点为空时先提升为顶层,再按顶层列表重排
        /// </summary>
        /// <param name="node">要移动的节点</param>
        /// <param name="placement">目标位置</param>
        /// <returns>移动成功返回 true</returns>
        bool    apply(const Node& node,Placement& placement)
        {
            if (node == nullptr)
                return  false;
            if (placement._parent != nullptr)
                return  _tree.setParent(node,placement._parent.get(),placement._index);
            /// 已在顶层时只调整列表顺序,避免 setParent 重复重算本地变换
            bool    isTop   =   false;
            for (const Node& top : _tree.topLevelNodes())
            {
                if (top.get() == node.get())
                {
                    isTop   =   true;
                    break;
                }
            }
            if (!isTop && !_tree.setParent(node,nullptr))
                return  false;
            return  _tree.moveToplevelNode(node,placement._index);
        }
    protected:
        FENodeTree&             _tree;
        /// <summary>
        /// 被移动的节点(强引用,与 _old/_new 一一对应)
        /// </summary>
        Nodes                   _nodes;
        std::vector<Placement>  _old;
        std::vector<Placement>  _new;
        /// <summary>
        /// 结构调整后的刷新回调(宿主用来重建模型树行列表)
        /// </summary>
        std::function<void()>   _refresh;
        /// <summary>
        /// 跳过第一次 redo(位置已由模型树提前应用)
        /// </summary>
        bool                    _skipRedo   =   false;
    };

    /// <summary>
    /// 添加节点命令(右键"添加子节点";父节点为空表示加入顶层):
    /// redo: 挂到父节点下(或顶层)并分发到渲染工厂与各组件系统;
    /// undo: 从节点树摘除,并清理工厂与系统中的引用
    /// 命令持有节点与父节点的强引用,undo/redo 往返期间对象保持存活
    /// </summary>
    class   FEEditorAddNodeCmd :public FEUndoCmd
    {
    public:
        FEEditorAddNodeCmd(FEScene& scene,const Node& node,const Node& parent,const String& text)
            :FEUndoCmd(text)
            ,_scene(scene)
            ,_node(node)
            ,_parent(parent)
        {}
    public:
        virtual void    redo()  override
        {
            if (_node == nullptr)
                return;
            if (_parent != nullptr)
            {
                _scene.nodeTree().setParent(_node,_parent.get());
                /// reparent 会保持世界变换,新建节点应落在父节点原点(本地变换归零)
                _node->setLocalTranslation(real3(0));
            }
            else
                _scene.addNodesToTree({_node});
            _scene.dispatchNodesToSystem({_node});
        }
        virtual void    undo()  override
        {
            if (_node == nullptr)
                return;
            _scene.nodeTree().removeNode(_node);
            _scene.removeNodesFromSystem({_node});
        }
    protected:
        FEScene&    _scene;
        Node        _node;
        /// <summary>
        /// 父节点(为空表示顶层)
        /// </summary>
        Node        _parent;
    };

    /// <summary>
    /// 移除节点命令(右键"移除节点"):
    /// 构造时记录每个节点的父节点与同级下标,redo 摘除并清理工厂/系统,
    /// undo 按原位置重新挂回并重新分发,保证撤销后层级与顺序不变
    /// </summary>
    class   FEEditorRemoveNodeCmd :public FEUndoCmd
    {
    public:
        /// <summary>
        /// 节点在树中的位置:父节点为空表示顶层列表
        /// </summary>
        struct  Placement
        {
            Node    _parent;
            size_t  _index  =   SIZE_MAX;
        };
    public:
        FEEditorRemoveNodeCmd(FEScene& scene,const Nodes& nodes,const String& text)
            :FEUndoCmd(text)
            ,_scene(scene)
            ,_nodes(nodes)
        {
            /// 必须在节点摘除前记录位置
            for (const Node& node : _nodes)
            {
                Placement   place;
                place._parent   =   (node != nullptr) ? node->parent() : Node();
                place._index    =   siblingIndex(node);
                _places.push_back(place);
            }
        }
    public:
        virtual void    redo()  override
        {
            for (const Node& node : _nodes)
            {
                if (node != nullptr)
                    _scene.nodeTree().removeNode(node);
            }
            _scene.removeNodesFromSystem(_nodes);
        }
        virtual void    undo()  override
        {
            for (size_t i = 0; i < _nodes.size(); ++i)
                restore(_nodes[i],_places[i]);
            _scene.dispatchNodesToSystem(_nodes);
        }
    protected:
        /// <summary>
        /// 节点在同级列表中的下标
        /// </summary>
        /// <param name="node">目标节点</param>
        /// <returns>下标;不在任何列表里返回 SIZE_MAX</returns>
        size_t  siblingIndex(const Node& node) const
        {
            if (node == nullptr)
                return  SIZE_MAX;
            const   Node    parent  =   node->parent();
            if (parent != nullptr)
            {
                const   auto&   list    =   parent->children();
                for (size_t i = 0; i < list.size(); ++i)
                {
                    if (list[i].get() == node.get())
                        return  i;
                }
                return  SIZE_MAX;
            }
            const   auto&   list    =   _scene.nodeTree().topLevelNodes();
            for (size_t i = 0; i < list.size(); ++i)
            {
                if (list[i].get() == node.get())
                    return  i;
            }
            return  SIZE_MAX;
        }
        /// <summary>
        /// 把节点放回指定位置:父节点为空时先提升为顶层,再按顶层列表重排
        /// </summary>
        /// <param name="node">要恢复的节点</param>
        /// <param name="place">原位置</param>
        void    restore(const Node& node,Placement& place)
        {
            if (node == nullptr)
                return;
            FENodeTree& tree    =   _scene.nodeTree();
            if (place._parent != nullptr)
            {
                tree.setParent(node,place._parent.get(),place._index);
                return;
            }
            /// 已在顶层时只调整顺序,避免 setParent 重复重算本地变换
            bool    isTop   =   false;
            for (const Node& top : tree.topLevelNodes())
            {
                if (top.get() == node.get())
                {
                    isTop   =   true;
                    break;
                }
            }
            if (!isTop && !tree.setParent(node,nullptr))
                return;
            tree.moveToplevelNode(node,place._index);
        }
    protected:
        FEScene&                _scene;
        Nodes                   _nodes;
        std::vector<Placement>  _places;
    };

    /// <summary>
    /// 节点重命名命令(右键"重命名"):
    /// redo/undo 直接切换名字并通知刷新,保证撤销后名称恢复
    /// </summary>
    class   FEEditorRenameNodeCmd :public FEUndoCmd
    {
    public:
        FEEditorRenameNodeCmd(const Node& node,const String& name,const String& text)
            :FEUndoCmd(text)
            ,_node(node)
            ,_name(name)
            ,_old((node != nullptr && node->name() != nullptr) ? String(node->name()) : String())
        {}
    public:
        virtual void    redo()  override
        {
            apply(_name);
        }
        virtual void    undo()  override
        {
            apply(_old);
        }
    protected:
        void    apply(const String& name)
        {
            if (_node == nullptr)
                return;
            _node->setName(name);
            _node->fireChanged();
        }
    protected:
        Node    _node;
        String  _name;
        String  _old;
    };

    /// <summary>
    /// 动画命令(右键"创建动画/删除动画/添加到动画树"):
    /// AM_Attach  添加组件并登记到动画系统,undo 反向摘除;
    /// AM_Detach  从动画系统摘除并删除组件,undo 重新添加;
    /// AM_Register 仅登记到动画系统(动画树),undo 取消登记
    /// 命令持有动画组件的强引用,undo/redo 往返期间对象保持存活
    /// </summary>
    class   FEEditorAnimationCmd :public FEUndoCmd
    {
    public:
        enum    Mode
        {
            /// <summary>
            /// 新建动画:挂到节点并登记到动画系统
            /// </summary>
            AM_Attach,
            /// <summary>
            /// 删除动画:从动画系统与节点上摘除
            /// </summary>
            AM_Detach,
            /// <summary>
            /// 添加到动画树:只登记到动画系统
            /// </summary>
            AM_Register,
        };
    public:
        FEEditorAnimationCmd(FEScene& scene,const Node& node,const Animations& anims,Mode mode,const String& text)
            :FEUndoCmd(text)
            ,_scene(scene)
            ,_node(node)
            ,_anims(anims)
            ,_mode(mode)
        {}
    public:
        virtual void    redo()  override
        {
            switch (_mode)
            {
            case AM_Attach:     attach(true);   registerAnims(true);     break;
            case AM_Detach:     registerAnims(false);   attach(false);  break;
            case AM_Register:   registerAnims(true);                    break;
            }
        }
        virtual void    undo()  override
        {
            switch (_mode)
            {
            case AM_Attach:     registerAnims(false);   attach(false);  break;
            case AM_Detach:     attach(true);   registerAnims(true);    break;
            case AM_Register:   registerAnims(false);                   break;
            }
        }
    protected:
        /// <summary>
        /// 挂载/摘除动画组件
        /// </summary>
        /// <param name="add">true 添加 / false 移除</param>
        void    attach(bool add)
        {
            if (_node == nullptr)
                return;
            for (Animation& anim : _anims)
            {
                if (anim == nullptr)
                    continue;
                if (add)
                    _node->addComponent(anim.get());
                else
                    _node->removeComponent(anim.get());
            }
        }
        /// <summary>
        /// 登记/取消登记到动画系统(即动画树)
        /// </summary>
        /// <param name="add">true 登记 / false 取消登记</param>
        void    registerAnims(bool add)
        {
            FEAnimationSys* sys =   _scene.animationSystem();
            if (sys == nullptr)
                return;
            for (Animation& anim : _anims)
            {
                if (anim == nullptr)
                    continue;
                if (add)
                    sys->addObject(anim.get());
                else
                    sys->removeObject(anim.get());
            }
        }
    protected:
        FEScene&    _scene;
        Node        _node;
        Animations  _anims;
        Mode        _mode;
    };

    /// <summary>
    /// 约束添加/移除命令(右键约束子菜单,勾选状态即是否已添加):
    /// redo 按 add 生效,undo 取反;
    /// 添加时先挂到节点再登记到约束系统,移除时顺序相反
    /// 命令持有约束组件的强引用,undo/redo 往返期间对象保持存活
    /// </summary>
    class   FEEditorConstraintCmd :public FEUndoCmd
    {
    public:
        FEEditorConstraintCmd(FEScene& scene,const Node& node,const Constraint& constraint
                              ,bool add,const String& text)
            :FEUndoCmd(text)
            ,_scene(scene)
            ,_node(node)
            ,_constraint(constraint)
            ,_add(add)
        {}
    public:
        virtual void    redo()  override
        {
            apply(_add);
        }
        virtual void    undo()  override
        {
            apply(!_add);
        }
    protected:
        /// <summary>
        /// 应用添加/移除
        /// </summary>
        /// <param name="add">true 添加 / false 移除</param>
        void    apply(bool add)
        {
            if (_constraint == nullptr)
                return;
            FEConstraintSys* sys    =   _scene.constraintSystem();
            if (add)
            {
                if (_node != nullptr)
                    _node->addComponent(_constraint.get());
                if (sys != nullptr)
                    sys->addObject(_constraint.get());
            }
            else
            {
                if (sys != nullptr)
                    sys->removeObject(_constraint.get());
                if (_node != nullptr)
                    _node->removeComponent(_constraint.get());
            }
        }
    protected:
        FEScene&    _scene;
        Node        _node;
        Constraint  _constraint;
        bool        _add    =   true;
    };
}
