#pragma     once

#include    "../FEMath.hpp"

namespace   FE
{
    /// <summary>
    /// 轴位置类型:抓手摆放位置
    /// </summary>
    enum    AxisPosType
    {
        /// 包围盒中心
        APT_AabbCenter  =   0,
        /// 位移中心
        APT_Translation,
    };

    /// <summary>
    /// 编辑类型
    /// </summary>
    enum    AxisEditType
    {
        /// 对象
        AET_Object  =   0,
        /// 编辑轴
        AET_Axis,
    };

    /// <summary>
    /// 坐标系类型
    /// </summary>
    enum    AxisCoordType
    {
        ACT_World   =   0,
        ACT_Parent,
        ACT_Local,
        ACT_Screen,
    };

    /// <summary>
    /// 变换中心类型:决定绕哪个点旋转/缩放
    /// </summary>
    enum    AxisCenterType
    {
        /// 包围盒中心(选中对象整体)
        ACE_AabbCenter  =   0,
        /// 位移中心(各对象自身位置)
        ACE_Translation,
        /// 各自原点
        ACE_SelfOrigin,
        /// 用户自定义轴心
        ACE_User,
    };

    /// <summary>
    /// 编辑轴配置(对应 Qt 版 AxisEditor.ui),由轴编辑器读取
    /// </summary>
    class   FEAxisSetting
    {
    public:
        /// <summary>
        /// 轴位置类型
        /// </summary>
        AxisPosType     posType     =   APT_AabbCenter;
        /// <summary>
        /// 编辑类型
        /// </summary>
        AxisEditType    editType    =   AET_Object;
        /// <summary>
        /// 坐标系类型
        /// </summary>
        AxisCoordType   coordType   =   ACT_World;
        /// <summary>
        /// 中心类型
        /// </summary>
        AxisCenterType  centerType  =   ACE_AabbCenter;
        /// <summary>
        /// 用户自定义轴心(世界空间)
        /// </summary>
        real3           userCenter  =   real3(0);
        /// <summary>
        /// 自定义轴向(未归一化)
        /// </summary>
        real3           customAxis  =   real3(0,1,0);
        /// <summary>
        /// 禁用编辑轴
        /// </summary>
        bool            disableX    =   false;
        bool            disableY    =   false;
        bool            disableZ    =   false;
    public:
        /// <summary>
        /// 按索引判断某个轴是否被禁用(0=X,1=Y,2=Z)
        /// </summary>
        inline  bool    axisDisabled(int index) const
        {
            switch(index)
            {
            case    0:  return  disableX;
            case    1:  return  disableY;
            case    2:  return  disableZ;
            default:    return  false;
            }
        }
        /// <summary>
        /// 按世界方向判断是否被禁用:只有与某个主轴几乎对齐时才按该轴处理
        /// (球面/屏幕圆等任意方向不参与禁用)
        /// </summary>
        inline  bool    axisDisabled(const real3& axis) const
        {
            const   real    kEps    =   real(0.999);
            real3   a               =   glm::abs(axis);
            if  (a.x >= kEps)   return  disableX;
            if  (a.y >= kEps)   return  disableY;
            if  (a.z >= kEps)   return  disableZ;
            return  false;
        }
        /// <summary>
        /// 计算某对象的旋转中心(世界空间)
        /// </summary>
        /// <param name="editorPos">抓手中心(整个选区)</param>
        /// <param name="objectPos">对象的世界位置</param>
        inline  real3   rotationCenter(const real3& editorPos,const real3& objectPos) const
        {
            switch(centerType)
            {
            case    ACE_AabbCenter: return  editorPos;
            case    ACE_Translation:return  objectPos;
            case    ACE_SelfOrigin: return  objectPos;
            case    ACE_User:       return  userCenter;
            default:                return  editorPos;
            }
        }
    };
}
