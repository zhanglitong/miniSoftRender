#pragma     once

#include    "../node/FENodeTree.hpp"
#include    "FEEditAxisScale.h"
#include    "FEInputSystem.hpp"

namespace   FE
{
    /// <summary>
    /// 节点缩放编辑工具
    /// </summary>
    
    DEFINE_CLASS_UUID(FENodeScaleEditor, "{36459D8D-180D-4F8B-A6FE-6604B3EF05B1}");

    class   FE_API  FENodeScaleEditor: public FEEditAxisScale
    {
        IMPLEMENT_CLASS_REFLECT(FENodeScaleEditor)
    public:
        FENodeScaleEditor(FEContext& ctx);
        FENodeScaleEditor(const FENodeScaleEditor& other);
        virtual~FENodeScaleEditor();
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
        void    onSAxis ( EditStatus status
                        , const real3& relativeOffset
                        , const real3& absoluteOffset
                        , FEEditAxisScale& sender);
    protected:
        Nodes   _nodes;
    };

    using   NodeScaleEditor  =   SharedPtr<FENodeScaleEditor>;
}
