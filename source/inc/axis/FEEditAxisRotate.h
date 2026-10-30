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
        /// <summary>
        ///旋转轴顶点(6个: X/Y/Z 起止)
        /// </summary>
        const   float3 (&rotateAxis())[6];
        /// <summary>
        ///屏幕圆顶点(62个: 球面31 + 大圆31)
        /// </summary>
        const   float3 (&rotateScreenCircle())[62];
        /// <summary>
        ///三轴圆弧顶点
        /// </summary>
        const   float3s    (&rotateArcs())[3];
        /// <summary>
        ///切线顶点(8个)
        /// </summary>
        const   float3 (&tangent())[8];
        ///旋转扇形顶点
        const   float3s&    fan() const;
        /// <summary>
        ///是否正在旋转(绘制扇形/切线用)
        /// </summary>
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
        /// <summary>
        ///高亮轴
        /// </summary>
        AXIS    hoverAxis(const int2& pos);
        /// <summary>
        ///拾取轴
        /// </summary>
        AXIS    pickAxis(const int2& pos);
        /// <summary>
        ///拾取球
        /// </summary>
        AXIS    pickBall(const int2& pos);
        /// <summary>
        ///拾取圆
        /// </summary>
        AXIS    pickCricle(const int2& pos);
        /// <summary>
        ///更新此次的旋转轴方向以及该旋转方向的权重
        /// </summary>
        void    updateRotationAxisAndWeight();
        /// <summary>
        ///旋转计算
        /// </summary>
        void    calcRotate(const int2& start, const int2& end, real& rAngle);
        /// <summary>
        /// 旋转平面上的真实夹角(度,相对拖拽起点),按上一次累计角做 ±360 展开
        /// 说明:旧实现用切线投影长度近似角度,拖拽超过 180 度后投影方向反转会回退
        /// </summary>
        real    rotateOnPlane(const int2& start, const int2& end, const real3& axis);
        /// <summary>
        /// 切线投影方式(环面侧对视线时的退化处理),像素长度近似角度
        /// </summary>
        real    rotateByTangent(const int2& start, const int2& end, const real3& axis, real radius);
        /// <summary>
        ///计算切线与扇形
        /// </summary>
        void    calcTangentAndFan(const real3& rotateAxis, real radius, real3& vtStart, real3& vtEnd);
    protected:
        virtual bool mouseButtonPress(const int2& pos)  ;
        virtual bool mouseButtonRelease(const int2& pos);
        virtual bool mouseMove(const int2& pos)         ;
        virtual bool touchDown(const int2& pos)         ;
        virtual bool touchUp(const int2& pos)           ;
        virtual bool touchMove(const int2& pos)         ;
    private:
        /// <summary>
        ///旋转轴
        /// </summary>
        float3      _rotateAxis[6];
        /// <summary>
        ///始终与屏幕平行的圆(球面31 + 大圆31)
        /// </summary>
        float3      _rotateScreenCricle[62];
        /// <summary>
        ///x, y, z 摄像机方向的圆弧
        /// </summary>
        float3s     _vRotateAxis[3];
        /// <summary>
        ///切线
        /// </summary>
        float3      _tangent[8];
        /// <summary>
        ///旋转中扇形
        /// </summary>
        float3s     _fan;

        /// <summary>
        ///球半径
        /// </summary>
        real        _radiusBall;
        /// <summary>
        ///大圆半径
        /// </summary>
        real        _radiusCircle;

        /// <summary>
        ///选中的轴
        /// </summary>
        AXIS        _selectedAxis;
        /// <summary>
        ///高亮的轴
        /// </summary>
        AXIS        _hoveredAxis;

        /// <summary>
        ///是否正在旋转
        /// </summary>
        bool        _bRotatting;
        /// <summary>
        ///此次的旋转轴
        /// </summary>
        real3       _rotationAxis;
        /// <summary>
        ///此次的旋转角权重
        /// </summary>
        real        _rotationAngleWeight;
        /// <summary>
        ///旋转过的角度
        /// </summary>
        real        _offAngle;

        /// <summary>
        ///鼠标按钮按下
        /// </summary>
        bool        _bMouseDown;
        /// <summary>
        ///鼠标按下的最后位置
        /// </summary>
        int2        _downPos;
        /// <summary>
        ///开始时的鼠标位置
        /// </summary>
        int2        _startPos;

        /// <summary>
        ///触屏按下
        /// </summary>
        bool        _bTouchDown;
        /// <summary>
        ///触屏拾取轴
        /// </summary>
        bool        _bTouchPickup;
        /// <summary>
        ///触屏按下的最后位置
        /// </summary>
        int2        _touchDownPos;
        /// <summary>
        ///触屏开始时按下的位置
        /// </summary>
        int2        _touchStartPos;

        /// <summary>
        ///旋转吸附是否启用
        /// </summary>
        bool                _adsorptionEnabled;
        /// <summary>
        ///旋转吸附标志
        /// </summary>
        AdsorptionFlags     _adsorptionFlags;
        /// <summary>
        ///旋转吸附角度
        /// </summary>
        real                _adsorptionAngle;

        /// <summary>
        ///回调
        /// </summary>
        MDelegate           _delegate;
    };

    using   EditAxisRotate    =   SharedPtr<FEEditAxisRotate>;
}
