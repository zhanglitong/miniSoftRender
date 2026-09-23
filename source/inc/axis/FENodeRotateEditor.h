#pragma     once

#include    "../node/FENodeTree.hpp"
#include    "FEEditAxisRotate.h"
#include    "FEInputSystem.hpp"

namespace   FE
{
    /// <summary>
    /// 节点旋转编辑工具
    /// </summary>
    DEFINE_CLASS_UUID(FENodeRotateEditor, "{C3A18540-5CD3-4A69-9A36-5BD6C6250602}");

    class   FE_API  FENodeRotateEditor: public FEEditAxisRotate
    {
        IMPLEMENT_CLASS_REFLECT(FENodeRotateEditor)
    public:
        FENodeRotateEditor(FEContext& ctx);
        FENodeRotateEditor(const FENodeRotateEditor& other);
        virtual ~FENodeRotateEditor();
    public:
        /// <summary>
        /// 设置要编辑的对象
        /// </summary>
        /// <param name="objects"></param>
        void    setObjects(const Objects& objects);
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
        Objects _objects;
    };

    using   NodeRotateEditor  =   SharedPtr<FENodeRotateEditor>;
}
