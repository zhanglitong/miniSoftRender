#pragma     once
#include    "FEAxis.h"

/// <summary>
/// FEEditAxisRotate.h
/// 定义旋转编辑坐标轴
/// </summary>
namespace   FE
{
    class FE_API FEEditAxisRotate:public FEEditAxis
    {
    public:
        enum AXIS
        {
            AXIS_NULL = 0,
            AXIS_X,
            AXIS_Y,
            AXIS_Z,
            ///球面旋转,三个坐标轴所在的球
            AXIS_BL,
            ///与屏幕平行的圆
            AXIS_SC,
        };
        /// <summary>
        /// 旋转吸附标志
        /// </summary>
        enum AdsorptionFlag
        {
            ///无
            AF_None = 0,
            ///旋转角度吸附标志
            AF_Angle = 1 << 0,
        };
        using AdsorptionFlags = FEFlags<AdsorptionFlag, uint>;
        /// <summary>
        /// 旋转通知
        /// </summary>
        /// <param name="status">编辑状态</param>
        /// <param name="axis">旋转的轴(世界坐标)</param>
        /// <param name="relativeOffsetAngle">对应旋转轴旋转过的角度(相对偏移)</param>
        /// <param name="absoluteOffsetAngle">对应旋转轴旋转过的角度(绝对偏移)</param>
        /// <param name="sender">调用通知的轴对象</param>
        using MDelegate = FETMultiDelegate<void(EditStatus status
            , const real3& axis
            , real relativeOffsetAngle
            , real absoluteOffsetAngle
            , FEEditAxisRotate& sender)>;
    public:
        FEEditAxisRotate(FEContext& context);
        FEEditAxisRotate(const FEEditAxisRotate& other);
        ~FEEditAxisRotate();
    public:
        /// <summary>
        /// 操作回调,当轴发生操作时，触发该回调
        /// </summary>
        MDelegate& mDelegate();
        /// <summary>
        /// 获取当前已经被高亮的轴
        /// </summary>
        /// <returns>返回高亮的轴索引</returns>
        AXIS        hoveredAxis() const;
        /// <summary>
        /// 获取当前已经被选中的轴
        /// </summary>
        /// <returns>返回选中的轴索引</returns>
        AXIS        selectedAxis() const;
        /// <summary>
        /// 给定一个单位向量，计算当绕着该单位向量旋转时，旋转角度偏移量对于当前X轴旋转角度的权重
        /// </summary>
        inline real weightX(const real3& vec) const
        {
            return dot(vec, this->axisX());
        }
        /// <summary>
        /// 给定一个单位向量，计算当绕着该单位向量旋转时，旋转角度偏移量对于当前Y轴旋转角度的权重
        /// </summary>
        inline real weightY(const real3& vec) const
        {
            return dot(vec, this->axisY());
        }
        /// <summary>
        /// 给定一个单位向量，计算当绕着该单位向量旋转时，旋转角度偏移量对于当前Z轴旋转角度的权重
        /// </summary>
        inline real weightZ(const real3& vec) const
        {
            return dot(vec, this->axisZ());
        }
        /// <summary>
        /// 启用/禁用旋转吸附
        /// </summary>
        void setAdsorptionEnabled(bool enabled);
        /// <summary>
        /// 获取旋转吸附是否启用
        /// </summary>
        bool adsorptionEnabled() const;
        /// <summary>
        /// 获取旋转吸附标志
        /// </summary>
        AdsorptionFlags adsorptionFlags() const;
        /// <summary>
        /// 设置旋转吸附标志
        /// </summary>
        void setAdsorptionFlags(AdsorptionFlags flags) ;
        /// <summary>
        /// 获取旋转吸附角度
        /// </summary>
        real adsorptionAngle() const;
        /// <summary>
        /// 设置旋转吸附角度
        /// </summary>
        void setAdsorptionAngle(real angle) ;
        /// <summary>
        /// 获取当前累计旋转角度(度)
        /// </summary>
        inline  real    offAngle() const
        {
            return  _offAngle;
        }
    public:
        /// <summary>
        /// 每一帧调用,自动生成旋转轴顶点数据
        /// </summary>
        /// <param name="camera"></param>
        void    update(FECamera& camera);
        ///旋转轴顶点(6个: X/Y/Z 起止)
        const   float3 (&rotateAxis())[6];
        ///屏幕圆顶点(62个: 球面31 + 大圆31)
        const   float3 (&rotateScreenCircle())[62];
        ///三轴圆弧顶点
        const   float3s    (&rotateArcs())[3];
        ///切线顶点(8个)
        const   float3 (&tangent())[8];
        ///旋转扇形顶点
        const   float3s&    fan() const;
        ///是否正在旋转(绘制扇形/切线用)
        inline  bool    isRotatting() const
        {
            return  _bRotatting;
        }
    public:
        /// <summary>
        /// 是否有轴被高亮
        /// </summary>
        virtual bool isAxisHovered() const override
        {
            return hoveredAxis() != FEEditAxisRotate::AXIS::AXIS_NULL;
        }
        /// <summary>
        /// 是否有轴被选中
        /// </summary>
        virtual bool isAxisSelected() const override
        {
            return selectedAxis() != FEEditAxisRotate::AXIS::AXIS_NULL;
        }
        /// <summary>
        /// 取消轴的高亮状态
        /// </summary>
        virtual void cancelHovered() override;
        /// <summary>
        /// 取消轴的选中状态
        /// </summary>
        virtual void cancelSelected()override;
    protected:
        /// <summary>
        /// 判断某个值是否是X轴的枚举值,子类实现
        /// </summary>
        virtual bool isEnumAxisX(int value) const override
        {
            return value == FEEditAxisRotate::AXIS::AXIS_X;
        }
        /// <summary>
        /// 判断某个值是否是Y轴的枚举值,子类实现
        /// </summary>
        virtual bool isEnumAxisY(int value) const override
        {
            return value == FEEditAxisRotate::AXIS::AXIS_Y;
        }
        /// <summary>
        /// 判断某个值是否是Z轴的枚举值,子类实现
        /// </summary>
        virtual bool isEnumAxisZ(int value) const override
        {
            return value == FEEditAxisRotate::AXIS::AXIS_Z;
        }
    public:
        /// <summary>
        /// 需要被其他子类重写，处理消息
        /// </summary>
        /// <param name=""></param>
        virtual void    onMessage(const FEMessage& ) override;
    private:
        ///高亮轴
        AXIS    hoverAxis(const int2& pos);
        ///拾取轴
        AXIS    pickAxis(const int2& pos);
        ///拾取球
        AXIS    pickBall(const int2& pos);
        ///拾取圆
        AXIS    pickCricle(const int2& pos);
        ///更新此次的旋转轴方向以及该旋转方向的权重
        void    updateRotationAxisAndWeight();
        ///旋转计算
        void    calcRotate(const int2& start, const int2& end, real& rAngle);
        real    rotateX(const int2& start, const int2& end, const real3& axis);
        real    rotateY(const int2& start, const int2& end, const real3& axis);
        real    rotateZ(const int2& start, const int2& end, const real3& axis);
        real    rotateSC(const int2& start, const int2& end, const real3& axis);
        ///计算切线与扇形
        void    calcTangentAndFan(const real3& rotateAxis, real radius, real3& vtStart, real3& vtEnd);
    protected:
        virtual bool mouseButtonPress(const int2& pos)  ;
        virtual bool mouseButtonRelease(const int2& pos);
        virtual bool mouseMove(const int2& pos)         ;
        virtual bool touchDown(const int2& pos)         ;
        virtual bool touchUp(const int2& pos)           ;
        virtual bool touchMove(const int2& pos)         ;
    private:
        ///旋转轴
        float3      _rotateAxis[6];
        ///始终与屏幕平行的圆(球面31 + 大圆31)
        float3      _rotateScreenCricle[62];
        ///x, y, z 摄像机方向的圆弧
        float3s     _vRotateAxis[3];
        ///切线
        float3      _tangent[8];
        ///旋转中扇形
        float3s     _fan;

        ///球半径
        real        _radiusBall;
        ///大圆半径
        real        _radiusCircle;

        ///选中的轴
        AXIS        _selectedAxis;
        ///高亮的轴
        AXIS        _hoveredAxis;

        ///是否正在旋转
        bool        _bRotatting;
        ///此次的旋转轴
        real3       _rotationAxis;
        ///此次的旋转角权重
        real        _rotationAngleWeight;
        ///旋转过的角度
        real        _offAngle;

        ///鼠标按钮按下
        bool        _bMouseDown;
        ///鼠标按下的最后位置
        int2        _downPos;
        ///开始时的鼠标位置
        int2        _startPos;

        ///触屏按下
        bool        _bTouchDown;
        ///触屏拾取轴
        bool        _bTouchPickup;
        ///触屏按下的最后位置
        int2        _touchDownPos;
        ///触屏开始时按下的位置
        int2        _touchStartPos;

        ///旋转吸附是否启用
        bool                _adsorptionEnabled;
        ///旋转吸附标志
        AdsorptionFlags     _adsorptionFlags;
        ///旋转吸附角度
        real                _adsorptionAngle;

        ///回调
        MDelegate           _delegate;
    };

    using   EditAxisRotate    =   SharedPtr<FEEditAxisRotate>;
}
