#pragma     once

#include    "../../../inc/FEUndoCmd.hpp"
#include    "../../../inc/graphic/FEScene.h"

namespace   FE
{
    /// <summary>
    /// 创建节点命令(几何体与空节点共用):
    /// redo: 节点加入节点树(模型树显示)并分发到渲染工厂与各系统;
    /// undo: 从节点树摘除,并遍历所有工厂移除该节点的渲染引用
    ///       (FEScene 不订阅节点树的移除事件,必须手动清理,否则工厂内仍会绘制)。
    /// 命令持有 Node,undo/redo 往返期间对象保持存活;节点名需在构造前唯一化,
    /// 这样 redo 不会因场景中已有同名节点而改名。
    /// </summary>
    class   FEEditorCreateNodeCmd :public FEUndoCmd
    {
    public:
        FEEditorCreateNodeCmd(FEScene& scene,Node node,const String& text)
            :FEUndoCmd(text)
            ,_scene(&scene)
            ,_node(node)
        {}
    public:
        virtual void    redo()  override
        {
            if (_scene == nullptr || _node == nullptr)
                return;
            _scene->addNodesToTree({_node});
            _scene->dispatchNodesToSystem({_node});
        }
        virtual void    undo()  override
        {
            if (_scene == nullptr || _node == nullptr)
                return;
            _scene->nodeTree().removeNode(_node);
            for (auto factory : _scene->factoryMgr().objects())
                factory->removeNode(_node);
        }
        /// <summary>
        /// 命令创建的节点(push 之后立即有效)
        /// </summary>
        const   Node&   node() const    {   return  _node; }
    protected:
        FEScene*    _scene  =   nullptr;
        Node        _node;
    };
}
