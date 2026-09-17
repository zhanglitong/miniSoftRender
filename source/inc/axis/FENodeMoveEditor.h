#pragma     once

#include    "../node/FENodeTree.hpp"
#include    "FEEditAxisMove.h"
#include    "FEInputSystem.hpp"

namespace   FE
{
    /// <summary>
    /// 节点编辑工具
    /// </summary>
    
    DEFINE_CLASS_UUID(FENodeMoveEditor, "{3105288C-0B6A-49D8-864C-05A195F6C28E}");
    class   FE_API  FENodeMoveEditor: public FEEditAxisMove
    {
        IMPLEMENT_CLASS_REFLECT(FENodeMoveEditor)
    public:
        FENodeMoveEditor(FEContext& ctx);
        FENodeMoveEditor(const FENodeMoveEditor& other);
        virtual ~FENodeMoveEditor();
    public:
        /// <summary>
        /// 设置要编辑的节点
        /// </summary>
        /// <param name="nodes"></param>
        void    setNodes(const Nodes& nodes);
        /// <summary>
        /// 同步更新编辑器到节点的包围盒中心
        /// </summary>
        void    sync();
    protected:
        void    onMAxis ( EditStatus status
                        , const real3& relativeOffset
                        , const real3& absoluteOffset
                        , FEEditAxisMove& sender);
    protected:
        Nodes   _nodes;
    };

    using   NodeMoveEditor  =   SharedPtr<FENodeMoveEditor>;
}
