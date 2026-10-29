#pragma     once

#include    "../node/FENodeTree.hpp"
#include    "FEAxisSetting.hpp"
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
        /// 获取当前正在编辑的对象列表
        /// </summary>
        const   Objects&   objects()   const
        {
            return  _objects;
        }
        /// <summary>
        /// 同步更新编辑器到节点的包围盒中心
        /// </summary>
        void    sync();
        /// <summary>
        /// 设置编辑轴配置(坐标轴编辑窗口修改后由编辑器每帧推送)
        /// </summary>
        inline  void    setAxisSetting(FEAxisSetting* setting)
        {
            _axisSetting    =   setting;
        }
        /// <summary>
        /// 获取编辑轴配置
        /// </summary>
        inline  FEAxisSetting*  axisSetting() const
        {
            return  _axisSetting;
        }
    protected:
        void    onRAxis ( EditStatus status
                        , const real3& axis
                        , real relativeOffsetAngle
                        , real absoluteOffsetAngle
                        , FEEditAxisRotate& sender);
    protected:
        Objects         _objects;
        /// <summary>
        /// 编辑轴配置(不持有,由编辑器 UI 生命周期管理)
        /// </summary>
        FEAxisSetting*  _axisSetting    =   nullptr;
    };

    using   NodeRotateEditor  =   SharedPtr<FENodeRotateEditor>;
}
