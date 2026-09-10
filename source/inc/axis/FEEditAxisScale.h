#pragma     once
#include    "FEAxis.h"

/// <summary>
/// FEEditAxisScale.h
/// 定义缩放编辑坐标轴
/// </summary>
namespace   FE
{
    class FE_API FEEditAxisScale: public FEEditAxis
    {
    public:
        enum AXIS
        {
            AXIS_NULL = 0,
            AXIS_X,
            AXIS_Y,
            AXIS_Z,
            AXIS_XY,//XY缩放
            AXIS_YZ,//YZ缩放
            AXIS_XZ,//XZ缩放
            AXIS_XYZ,//XYZ缩放
        };
        /// <summary>
        /// 缩放通知
        /// </summary>
        /// <param name="status">编辑状态</param>
        /// <param name="relativeOffset">轴向缩放偏移(相对偏移)</param>
        /// <param name="absoluteOffset">轴向缩放偏移(绝对偏移)</param>
        /// <param name="sender">调用通知的轴对象</param>
        using MDelegate = FETMultiDelegate<void(EditStatus status
            , const real3& relativeOffset
            , const real3& absoluteOffset
            , FEEditAxisScale& sender)>;
    public:
        FEEditAxisScale(FEContext& context);
        FEEditAxisScale(const FEEditAxisScale& other);
        ~FEEditAxisScale();
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
        /// 给定一个单位向量，计算当沿着该单位向量缩放时，缩放偏移量对于当前X轴缩放偏移量的权重
        /// </summary>
        inline real weightX(const real3& vec) const
        {
            return dot(vec, this->axisX());
        }
        /// <summary>
        /// 给定一个单位向量，计算当沿着该单位向量缩放时，缩放偏移量对于当前Y轴缩放偏移量的权重
        /// </summary>
        inline real weightY(const real3& vec) const
        {
            return dot(vec, this->axisY());
        }
        /// <summary>
        /// 给定一个单位向量，计算当沿着该单位向量缩放时，缩放偏移量对于当前Z轴缩放偏移量的权重
        /// </summary>
        inline real weightZ(const real3& vec) const
        {
            return dot(vec, this->axisZ());
        }
    public:
        /// <summary>
        /// 每一帧调用,自动生成缩放轴顶点数据
        /// </summary>
        /// <param name="camera"></param>
        void    update(FECamera& camera);
        ///缩放轴顶点(16个)
        const   float3 (&scaleAxis())[16];
        ///箭头顶点(15个)
        const   float3 (&scaleAxisArr())[15];
        ///是否正在进行缩放
        inline  bool    isScaleing() const
        {
            return  _bScaleing;
        }
    public:
        /// <summary>
        /// 是否有轴被高亮
        /// </summary>
        virtual bool isAxisHovered() const override
        {
            return hoveredAxis() != FEEditAxisScale::AXIS::AXIS_NULL;
        }
        /// <summary>
        /// 是否有轴被选中
        /// </summary>
        virtual bool isAxisSelected() const override
        {
            return selectedAxis() != FEEditAxisScale::AXIS::AXIS_NULL;
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
            return value == FEEditAxisScale::AXIS::AXIS_X;
        }
        /// <summary>
        /// 判断某个值是否是Y轴的枚举值,子类实现
        /// </summary>
        virtual bool isEnumAxisY(int value) const override
        {
            return value == FEEditAxisScale::AXIS::AXIS_Y;
        }
        /// <summary>
        /// 判断某个值是否是Z轴的枚举值,子类实现
        /// </summary>
        virtual bool isEnumAxisZ(int value) const override
        {
            return value == FEEditAxisScale::AXIS::AXIS_Z;
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
        ///缩放计算
        real3   calcScale(const int2& start, const int2& end);
        ///计算轴朝向
        real3   calcAxisDir(FECamera& camera);
        ///拾取三轴汇聚三角面
        AXIS    pickTriMin(const int2& screen);
        ///拾取三轴外三角面
        AXIS    pickTriMax(const int2& screen);
        ///拾取坐标轴
        AXIS    pickAxis(const int2& screen);
        real    scaleX(const int2& start, const int2& end);
        real    scaleY(const int2& start, const int2& end);
        real    scaleZ(const int2& start, const int2& end);
        real    scaleXY(const int2& start, const int2& end);
        real    scaleYZ(const int2& start, const int2& end);
        real    scaleXZ(const int2& start, const int2& end);
        real    scaleXYZ(const int2& start, const int2& end);
        bool    calcScaleValue(const int2& start, const int2& end
                    , const real3& faceNor, const real3& axis
                    , real& retS, real limit = 1.0);
    protected:
        virtual bool mouseButtonPress(const int2& pos)  ;
        virtual bool mouseButtonRelease(const int2& pos);
        virtual bool mouseMove(const int2& pos)         ;
        virtual bool touchDown(const int2& pos)         ;
        virtual bool touchUp(const int2& pos)           ;
        virtual bool touchMove(const int2& pos)         ;
    private:
        ///缩放轴
        float3      _scaleAxis[16];
        ///箭头
        float3      _scaleAxisArr[15];
        ///当前选中轴
        AXIS        _selectedAxis;
        ///当前高亮的轴
        AXIS        _hoveredAxis;

        ///是否正在进行缩放
        bool        _bScaleing;
        ///缩放偏移
        real3       _offScale;

        ///保持把坐标轴的屁股不朝向屏幕
        real3       _dir;

        ///轴长度
        real        _axisSize;

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

        ///回调
        MDelegate   _delegate;
    };

    using   EditAxisScale    =   SharedPtr<FEEditAxisScale>;
}
