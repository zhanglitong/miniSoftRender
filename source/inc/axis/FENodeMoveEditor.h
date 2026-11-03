#pragma     once

#include    <functional>
#include    "../node/FENodeTree.hpp"
#include    "FEAxisSetting.hpp"
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
        /// 设置要编辑的节点
        /// </summary>
        /// <param name="nodes"></param>
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
            const   bool    editingAxis     =   _axisSetting != nullptr
                                            &&  _axisSetting->editType == AET_Axis
                                            &&  _axisSetting->centerType == ACE_User;
            /// 编辑轴且中心为用户时,抓手跟随自定义轴心;其余情况实时同步到节点包围盒中心
            if (editingAxis)
                applyTransform(_axisSetting->userCenter);
            else
                sync();
        }
    protected:
        /// <summary>
        /// 把抓手摆到指定位置,同时按坐标系类型设置朝向
        /// </summary>
        void    applyTransform(const real3& pos);
        /// <summary>
        /// 按编辑轴配置启用/禁用 XYZ 轴
        /// </summary>
        inline  void    applyAxisEnabled()
        {
            const   bool    x       =   _axisSetting == nullptr || !_axisSetting->axisDisabled(0);
            const   bool    y       =   _axisSetting == nullptr || !_axisSetting->axisDisabled(1);
            const   bool    z       =   _axisSetting == nullptr || !_axisSetting->axisDisabled(2);
            setEnabled(FEEditAxisMove::AXIS_X,x);
            setEnabled(FEEditAxisMove::AXIS_Y,y);
            setEnabled(FEEditAxisMove::AXIS_Z,z);
        }
        void    onMAxis ( EditStatus status
                        , const real3& relativeOffset
                        , const real3& absoluteOffset
                        , FEEditAxisMove& sender);
    protected:
        Objects _objects;
        /// <summary>
        /// 编辑轴配置(不持有,由编辑器 UI 生命周期管理)
        /// </summary>
        FEAxisSetting*  _axisSetting    =   nullptr;
        /// <summary>
        /// 编辑状态通知回调(不持有宿主)
        /// </summary>
        EditNotify      _editNotify;
    };

    using   NodeMoveEditor  =   SharedPtr<FENodeMoveEditor>;
}
