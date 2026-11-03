#pragma     once

#include    <functional>
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
        /// 编辑状态通知(开始/进行/结束),宿主据此记录撤销起点与终点
        /// </summary>
        using   EditNotify  =   std::function<void(EditStatus status)>;
        /// <summary>
        /// 设置编辑状态通知回调
        /// </summary>
        inline  void    setEditNotify(EditNotify notify)
        {
            _editNotify =   std::move(notify);
        }
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
            applyAxisEnabled();
            const   bool    userCenter      =   _axisSetting != nullptr
                                            &&  _axisSetting->centerType == ACE_User;
            /// 用户轴心模式下抓手跟随自定义轴心;其余情况在非拖拽时重新同步
            if (userCenter)
                applyTransform(_axisSetting->userCenter);
            else if (!isRotatting())
                sync();
        }
        /// <summary>
        /// 获取编辑轴配置
        /// </summary>
        inline  FEAxisSetting*  axisSetting() const
        {
            return  _axisSetting;
        }
    protected:
        /// <summary>
        /// 把抓手摆到指定位置,同时按坐标系类型设置朝向
        /// </summary>
        void    applyTransform(const real3& pos);
        /// <summary>
        /// 拖拽开始时把本次旋转的轴心写入节点属性(父空间),供动画回放重建圆弧
        /// </summary>
        void    recordRotationCenter();
        /// <summary>
        /// 按编辑轴配置启用/禁用 XYZ 轴
        /// </summary>
        inline  void    applyAxisEnabled()
        {
            const   bool    x       =   _axisSetting == nullptr || !_axisSetting->axisDisabled(0);
            const   bool    y       =   _axisSetting == nullptr || !_axisSetting->axisDisabled(1);
            const   bool    z       =   _axisSetting == nullptr || !_axisSetting->axisDisabled(2);
            setEnabled(FEEditAxisRotate::AXIS_X,x);
            setEnabled(FEEditAxisRotate::AXIS_Y,y);
            setEnabled(FEEditAxisRotate::AXIS_Z,z);
        }
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
        /// <summary>
        /// 编辑状态通知回调(不持有宿主)
        /// </summary>
        EditNotify      _editNotify;
    };

    using   NodeRotateEditor  =   SharedPtr<FENodeRotateEditor>;
}
