#pragma     once

#include    <functional>
#include    "../node/FENodeTree.hpp"
#include    "FEAxisSetting.hpp"
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
            /// 拖拽期间不重新同步,避免缩放枢轴跟随节点漂移
            if  (!isScaleing())
                sync();
        }
    protected:
        /// <summary>
        /// 按编辑轴配置启用/禁用 XYZ 轴
        /// </summary>
        inline  void    applyAxisEnabled()
        {
            const   bool    x       =   _axisSetting == nullptr || !_axisSetting->axisDisabled(0);
            const   bool    y       =   _axisSetting == nullptr || !_axisSetting->axisDisabled(1);
            const   bool    z       =   _axisSetting == nullptr || !_axisSetting->axisDisabled(2);
            setEnabled(FEEditAxisScale::AXIS_X,x);
            setEnabled(FEEditAxisScale::AXIS_Y,y);
            setEnabled(FEEditAxisScale::AXIS_Z,z);
        }
        void    onSAxis ( EditStatus status
                        , const real3& relativeOffset
                        , const real3& absoluteOffset
                        , FEEditAxisScale& sender);
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

    using   NodeScaleEditor  =   SharedPtr<FENodeScaleEditor>;
}
