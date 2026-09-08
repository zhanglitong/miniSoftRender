#pragma     once

#include    "FEContext.hpp"
#include    "FEDelegate.hpp"
#include    "FEFlags.hpp"
#include    "FEMath.hpp"
#include    "FEInputCom.hpp"

namespace   FE
{
    static  const float4    DisableColor = {0.3f, 0.3f, 0.3f, 1.0f};

    class   FECamera;
    /// <summary>
    /// 坐标轴基类
    /// </summary>
    class   FE_API  FEAxis :public FEInputCom
    {
    public:
        enum AxisType
        {
            /// 基本轴
            AxisTypeBase    =   0,
            /// 编辑轴
            AxisTypeEdit    =   1,
            /// 浏览轴
            AxisTypeBrowse  =   2,
        };
    protected:
        mat4r           _transform;
        AxisType        _axisType;
    public:
        static const real3 AxisX()
        {
            return real3(1.0, 0.0, 0.0);
        }
        static const real3 AxisY()
        {
            return real3(0.0, 1.0, 0.0);
        }
        static const real3 AxisZ()
        {
            return real3(0.0, 0.0, 1.0);
        }
    public:
        FEAxis(AxisType type, FEContext& ctx);

        virtual ~FEAxis();
    public:
        /// <summary>
        /// 获取轴类型
        /// </summary>
        /// <returns>具体请查看 AxisType 类型</returns>
        inline AxisType axisType()const
        {
            return _axisType;
        }
        /// <summary>
        /// 设置轴变换矩阵
        /// 理论来说，这个矩阵应该包含
        /// TMat(位置,决定轴显示的位置)
        /// RMat(旋转,决定轴的朝向)
        /// SHMat(切变,会导致轴两两不垂直,但对于包含切变矩阵的模型来说，这是正常现象)
        /// SMat(缩放,但不会因为缩放而改变轴的大小,也就是说缩放是不会生效的)
        /// </summary>
        /// <param name="transform">轴变换矩阵</param>
        inline void     setTransform(const mat4r& transform)
        {
            if (_transform == transform)
                return;
            _transform = transform;
            _internalFlags.addFlag(InteralFlag_Update);
            _ctx.requireNextFrame();
        }
        /// <summary>
        /// 获取轴变换矩阵
        /// </summary>
        /// <returns>轴变换矩阵</returns>
        const mat4r&    transform() const
        {
            return _transform;
        }
        /// <summary>
        /// 获取轴的位置
        /// </summary>
        /// <returns>轴的位置</returns>
        inline real3    position() const
        {
            return real3(_transform[3][0], _transform[3][1], _transform[3][2]);
        }
        /// <summary>
        /// 获取X轴方向
        /// </summary>
        /// <returns>X轴方向</returns>
        inline real3    axisX() const
        {
            mat4r t = transform();
            t[3] = real4(real3(0.0), 1.0);
            real3 axis = normalize(real3(t * real4(AxisX(), 0.0)));
            return axis;
        }
        /// <summary>
        /// 获取Y轴方向
        /// </summary>
        /// <returns>Y轴方向</returns>
        inline real3    axisY() const
        {
            mat4r t = transform();
            t[3] = real4(real3(0.0), 1.0);
            real3 axis = normalize(real3(t * real4(AxisY(), 0.0)));
            return axis;
        }
        /// <summary>
        /// 获取Z轴方向
        /// </summary>
        /// <returns>Z轴方向</returns>
        inline real3    axisZ() const
        {
            mat4r t = transform();
            t[3] = real4(real3(0.0), 1.0);
            real3 axis = normalize(real3(t * real4(AxisZ(), 0.0)));
            return axis;
        }
    protected:
        /// <summary>
        /// 获取垂直于X轴的一个任意单位向量
        ///  理论上这里应该返回 axisY() 或 axisZ()
        ///  但是由于当前轴有可能发生切变导致 XYZ俩俩不垂直，因此需要从新计算
        /// </summary>
        /// <returns>垂直于X轴的一个任意单位向量</returns>
        real3   vectorPerpendicularToAxisX() const;
        /// <summary>
        /// 获取垂直于Y轴的一个任意单位向量
        ///  理论上这里应该返回 axisZ() 或 axisX()
        ///  但是由于当前轴有可能发生切变导致 XYZ俩俩不垂直，因此需要从新计算
        /// </summary>
        /// <returns>垂直于Y轴的一个任意单位向量</returns>
        real3   vectorPerpendicularToAxisY() const;
        /// <summary>
        /// 获取垂直于Z轴的一个任意单位向量
        ///  理论上这里应该返回 axisX() 或 axisY()
        ///  但是由于当前轴有可能发生切变导致 XYZ俩俩不垂直，因此需要从新计算
        /// </summary>
        /// <returns>垂直于Z轴的一个任意单位向量</returns>
        real3   vectorPerpendicularToAxisZ() const;
        /// <summary>
        /// 根据当前摄像机参数计算任意一个轴所在平面(最适合使用摄像机射线计算相交)
        /// </summary>
        /// <param name="camera">摄像机对象</param>
        /// <param name="axis">任意轴(单位向量)</param>
        /// <param name="outNor">结果平面的法线</param>
        /// <returns>返回true表示该平面存在，否则不存在(原因是摄像机与该轴共线(接近共线))</returns>
        bool    calcAxisPlaneNormalize(FECamera& camera, const real3& axis, real3& outNor) const;
    protected:
        enum InternalFlag
        {
            InteralFlag_None        =   0,
            InteralFlag_Update      =   1 << 0,
            InteralFlag_Visible     =   1 << 1, 
        };
        using InternalFlags = FEFlags<InternalFlag, uint>;
        InternalFlags _internalFlags;
    };
    /// <summary>
    /// 编辑坐标轴基类,提供编辑接口
    /// </summary>
    class   FE_API  FEEditAxis :public FEAxis
    {
    public:
        /// <summary>
        /// 编辑轴类型
        /// </summary>
        enum    EditAxisType
        {
            ///base
            EditAxisTypeBase = 0,
            ///移动
            EditAxisTypeMove,
            ///旋转
            EditAxisTypeRotate,
            ///缩放
            EditAxisTypeScale,
            ///单轴移动
            EditAxisTypeSingleMove,
            ///单轴旋转
            EditAxisTypeSingleRotate,
            ///面移动
            EditAxisTypeFaceMove,
        };
        /// <summary>
        /// 编辑状态
        /// </summary>
        enum    EditStatus
        {
            ///开始编辑
            EditStart   =   0,
            ///正在编辑
            Editting    =   1,
            ///结束编辑
            EditEnd     =   2,
        };

        /// <summary>
        /// 轴 高亮状态改变 通知
        /// </summary>
        /// <param name="hovered">true表示轴被高亮 false 表示轴取消高亮</param>
        /// <param name="sender">发送者</param>
        using AxisHoveredChangedDelegage = FETMultiDelegate<void(bool hovered, FEEditAxis& sender)>;

        /// <summary>
        /// 轴 选中状态改变 通知
        /// </summary>
        /// <param name="selected">true表示轴被选中 false 表示轴取消选中</param>
        /// <param name="sender">发送者</param>
        using AxisSelectedChangedDelegate = FETMultiDelegate<void(bool selected, FEEditAxis& sender)>; 

        /// <summary>
        /// 轴禁用/启用 通知
        /// </summary>
        /// <param name="selected">true表示轴被启用 false 表示轴被禁用</param>
        /// <param name="axis">轴枚举，具体值需要到子类中查看，比如移动轴的X轴枚举为 FEEditAxisMove::AXIS::AXIS_X;</param>
        /// <param name="sender">发送者</param>
        using AxisEnabledChangedDelegate = FETMultiDelegate<void(bool enabled, int axis, FEEditAxis& sender)>;

    protected:
        EditAxisType                _editAxisType;
        AxisHoveredChangedDelegage  _hoveredDelegate;
        AxisSelectedChangedDelegate _selectedDelegate;
        AxisEnabledChangedDelegate  _enabledDelegate;
        
    private:
        enum InternalAxisFlag
        {
            IAF_None    =   0,
            IAF_Axis_X  =   1 << 0,
            IAF_Axis_Y  =   1 << 1,
            IAF_Axis_Z  =   1 << 2,
        };
        using InternalAxisFlags = FEFlags<InternalAxisFlag, int>;
        InternalAxisFlags _disabledAxices;
    public:
        FEEditAxis(EditAxisType type, FEContext& ctx);
        virtual ~FEEditAxis();
    public:
        /// <summary>
        /// 获取编辑轴类型
        /// </summary>
        inline EditAxisType editAxisType() const
        {
            return _editAxisType;
        }
        /// <summary>
        /// 获取轴 启用/禁用 通知
        /// </summary>
        inline AxisEnabledChangedDelegate& axisEnabledChangedDelegate()
        {
            return _enabledDelegate;
        }
        /// <summary>
        /// 轴禁用，默认情况下，所有轴都是启用状态
        /// </summary>
        /// <param name="axis">轴枚举，具体值需要到子类中查看，比如移动轴的X轴枚举为 FEEditAxisMove::AXIS::AXIS_X;</param>
        /// <param name="enabled">当值为true时，启用该轴，否则禁用该轴。</param>
        inline void     setEnabled(int axis, bool enabled)
        {
            if (enabled == this->enabled(axis))
                return;

            if (this->isEnumAxisX(axis))
            {
                enabled ? _disabledAxices.removeFlag(FEEditAxis::IAF_Axis_X) : _disabledAxices.addFlag(FEEditAxis::IAF_Axis_X);
                _enabledDelegate(enabled, axis, *this);
            }
            else if (this->isEnumAxisY(axis))
            {
                enabled ? _disabledAxices.removeFlag(FEEditAxis::IAF_Axis_Y) : _disabledAxices.addFlag(FEEditAxis::IAF_Axis_Y);
                _enabledDelegate(enabled, axis, *this);
            }
            else if (this->isEnumAxisZ(axis))
            {
                enabled ? _disabledAxices.removeFlag(FEEditAxis::IAF_Axis_Z) : _disabledAxices.addFlag(FEEditAxis::IAF_Axis_Z);
                _enabledDelegate(enabled, axis, *this);
            }
        }
        /// <summary>
        /// 获取某个轴是否被锁定
        /// </summary>
        /// <param name="axis">轴枚举，具体值需要到子类中查看，比如移动轴的X轴枚举为 FEEditAxisMove::AXIS::AXIS_X;</param>
        /// <returns>如果轴被禁用，返回false，否则返回true</returns>
        inline bool     enabled(int axis) const
        {
            if (this->isEnumAxisX(axis))
                return !_disabledAxices.hasFlag(FEEditAxis::IAF_Axis_X);
            else if (this->isEnumAxisY(axis))
                return !_disabledAxices.hasFlag(FEEditAxis::IAF_Axis_Y);
            else if (this->isEnumAxisZ(axis))
                return !_disabledAxices.hasFlag(FEEditAxis::IAF_Axis_Z);
            return false;
        }
    public:
        /// <summary>
        /// 获取轴高亮状态改变通知
        /// </summary>
        inline AxisHoveredChangedDelegage& axisHoveredChangedDelegate() 
        {
            return _hoveredDelegate;
        }
        /// <summary>
        /// 获取轴选中状态改变通知
        /// </summary>
        inline AxisSelectedChangedDelegate& axisSelectedChangedDelegate()
        {
            return _selectedDelegate;
        }
        /// <summary>
        /// 是否有轴被高亮
        /// </summary>
        /// <returns>如果有轴被高亮，则返回true,否则返回false</returns>
        virtual bool    isAxisHovered() const = 0;
        /// <summary>
        /// 是否有轴被选中
        /// </summary>
        /// <returns>如果有轴被选中,则返回true,否则返回false</returns>
        virtual bool    isAxisSelected() const = 0;
        /// <summary>
        /// 取消轴的高亮状态
        /// </summary>
        virtual void    cancelHovered() = 0;
        /// <summary>
        /// 取消轴的选中状态
        /// </summary>
        virtual void    cancelSelected() = 0;
    protected:
        /// <summary>
        /// 判断某个值是否是X轴的枚举值,子类实现
        /// </summary>
        virtual bool    isEnumAxisX(int value) const = 0;
        /// <summary>
        /// 判断某个值是否是X轴的枚举值,子类实现
        /// </summary>
        virtual bool    isEnumAxisY(int value) const = 0;
        /// <summary>
        /// 判断某个值是否是X轴的枚举值,子类实现
        /// </summary>
        virtual bool    isEnumAxisZ(int value) const = 0;
    protected:
        inline void     sendHoveredDelegate() 
        {
            _ctx.requireNextFrame();
            _hoveredDelegate(this->isAxisHovered(), *this);
        }
        inline void     sendSelectedDelegate()
        {
            _ctx.requireNextFrame();
            _selectedDelegate(this->isAxisSelected(), *this);
        }
    };
    /// <summary>
    /// 浏览坐标轴基类,该轴只提供浏览,不包含其他功能
    /// </summary>
    class   FE_API  FEBrowseAxis :public FEAxis
    {
    public:
        /// <summary>
        /// 浏览轴类型
        /// </summary>
        enum    BrowseAxisType
        {
            ///base
            BrowseAxisTypeBase = 0,
            ///移动
            BrowseAxisTypeMove,
        };
    protected:
        BrowseAxisType _browseAxisType;
    public:
        FEBrowseAxis(BrowseAxisType type, FEContext& context);
        virtual ~FEBrowseAxis();
    };
}

