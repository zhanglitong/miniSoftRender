#pragma     once

#include    "../node/FENodeTree.hpp"
#include    "FEEditAxisMove.h"
#include    "FEInputSystem.hpp"

namespace   FE
{
    /// <summary>
    /// 节点编辑工具
    /// </summary>
    class   FE_API  FENodeMoveEditor: public FEEditAxisMove
    {
    public:
        FENodeMoveEditor(FEContext& ctx);
        FENodeMoveEditor(const FENodeMoveEditor& other);
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
