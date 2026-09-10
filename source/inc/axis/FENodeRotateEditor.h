#pragma     once

#include    "../node/FENodeTree.hpp"
#include    "FEEditAxisRotate.h"
#include    "FEInputSystem.hpp"

namespace   FE
{
    /// <summary>
    /// 节点旋转编辑工具
    /// </summary>
    class   FE_API  FENodeRotateEditor: public FEEditAxisRotate
    {
    public:
        FENodeRotateEditor(FEContext& ctx);
        FENodeRotateEditor(const FENodeRotateEditor& other);
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
        void    onRAxis ( EditStatus status
                        , const real3& axis
                        , real relativeOffsetAngle
                        , real absoluteOffsetAngle
                        , FEEditAxisRotate& sender);
    protected:
        Nodes   _nodes;
    };

    using   NodeRotateEditor  =   SharedPtr<FENodeRotateEditor>;
}
