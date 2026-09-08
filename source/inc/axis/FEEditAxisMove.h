#pragma     once

#include    "FEAxis.h"
/// <summary>
/// FEEditAxisMove.h
/// 定义移动编辑坐标轴
/// </summary>
namespace   FE
{
    class   FE_API  FEEditAxisMove: public FEEditAxis
    {
    public:
        /// <summary>
        /// 轴标志
        /// </summary>
        enum AXIS
        {
            AXIS_NULL = 0,
            AXIS_X,
            AXIS_Y,
            AXIS_Z,
            AXIS_XY,//xoy平面
            AXIS_YZ,//yoz平面
            AXIS_XZ,//xoz平面
        };
        /// <summary>
        /// 移动通知
        /// </summary>
        /// <param name="status">编辑状态</param>
        /// <param name="relativeOffset">相对偏移量(世界坐标)</param>
        /// <param name="absoluteOffset">绝对偏移量(世界坐标)</param>
        /// <param name="sender">调用通知的轴对象</param>
        using MDelegate = FETMultiDelegate<void(EditStatus status
            , const real3& relativeOffset
            , const real3& absoluteOffset
            , FEEditAxisMove& sender)>;

    public:
        FEEditAxisMove(FEContext& ctx);

        FEEditAxisMove(const FEEditAxisMove& other);

        ~FEEditAxisMove();
    public:
        /// <summary>
        /// 移动操作回调,当轴发生移动操作时，触发该回调
        /// </summary>
        MDelegate&  mDelegate();
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
        /// 给定一个单位向量，计算当沿着着该单位向量移动时，移动偏移量相对于X轴的移动偏移量的权重
        /// </summary>
        inline real weightX(const real3& vec) const
        {
            return dot(vec, this->axisX());
        }
        /// <summary>
        /// 给定一个单位向量，计算当沿着着该单位向量移动时，移动偏移量相对于Y轴的移动偏移量的权重
        /// </summary>
        inline real weightY(const real3& vec) const
        {
            return dot(vec, this->axisY());
        }
        /// <summary>
        /// 给定一个单位向量，计算当沿着着该单位向量移动时，移动偏移量相对于Z轴的移动偏移量的权重
        /// </summary>
        inline real weightZ(const real3& vec) const
        {
            return dot(vec, this->axisZ());
        }
        /// <summary>
        /// 每一帧调用该函数,会自动生成 moveAxis,axisArray,indexs 数据
        /// </summary>
        /// <param name="camera"></param>
        void    update(FECamera& camera);
        ///轴顶点
        const   float3s&    moveAxis() const;
        ///箭头顶点
        const   float3s&    axisArray() const;
        /// <summary>
        /// 索引数据
        /// </summary>
        const   uint16s&    indexs() const;
    public:
        /// <summary>
        /// 是否有轴被高亮
        /// </summary>
        /// <returns>如果有轴被高亮，则返回true,否则返回false</returns>
        virtual bool isAxisHovered() const override
        {
            return hoveredAxis() != FEEditAxisMove::AXIS::AXIS_NULL;
        }
        /// <summary>
        /// 是否有轴被选中
        /// </summary>
        /// <returns>如果有轴被选中,则返回true,否则返回false</returns>
        virtual bool isAxisSelected() const override
        {
            return selectedAxis() != FEEditAxisMove::AXIS::AXIS_NULL;
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
            return value == FEEditAxisMove::AXIS::AXIS_X;
        }
        /// <summary>
        /// 判断某个值是否是X轴的枚举值,子类实现
        /// </summary>
        virtual bool isEnumAxisY(int value) const override
        {
            return value == FEEditAxisMove::AXIS::AXIS_Y;
        }
        /// <summary>
        /// 判断某个值是否是X轴的枚举值,子类实现
        /// </summary>
        virtual bool isEnumAxisZ(int value) const override
        {
            return value == FEEditAxisMove::AXIS::AXIS_Z;
        }
    public:
        /// <summary>
        /// 需要被其他子类重写，处理消息
        /// </summary>
        /// <param name=""></param>
        virtual void    onMessage(const FEMessage& ) override;
    private:
        /// <summary>
        /// 高亮轴
        /// </summary>
        AXIS    hoverAxis(FEContext& context, const int2& pos);
        /// <summary>
        /// 拾取轴
        /// </summary>
        AXIS    pickAxis(FEContext& context, const int2& screen);
        /// <summary>
        /// 拾取面
        /// </summary>
        AXIS    pickFace(FEContext& context, const int2& screen);
        real3   moveX(FEContext& context, const int2& start, const int2& end);
        real3   moveY(FEContext& context, const int2& start, const int2& end);
        real3   moveZ(FEContext& context, const int2& start, const int2& end);
        real3   moveXY(FEContext& context, const int2& start, const int2& end);
        real3   moveXZ(FEContext& context, const int2& start, const int2& end);
        real3   moveYZ(FEContext& context, const int2& start, const int2& end);
        /// <summary>
        /// 根据选中的轴计算移动偏移量
        /// </summary>
        real3   calcMove(FEContext& context, const int2& start, const int2& end);
    protected:
        virtual bool mouseButtonPress(const int2& pos)  ;
        virtual bool mouseButtonRelease(const int2& pos);
        virtual bool mouseMove(const int2& pos)         ;
        virtual bool touchDown(const int2& pos)         ;
        virtual bool touchUp(const int2& pos)           ;
        virtual bool touchMove(const int2& pos)         ;
    private:
        ///轴顶点
        float3s             _moveAxis;
        ///箭头顶点
        float3s             _axisAr;
        /// <summary>
        /// 索引数据
        /// </summary>
        uint16s             _indexs;
        ///高亮的轴
        AXIS                _hoveredAxis;
        ///选择的轴
        AXIS                _selectedAxis;
        ///
        real3               _offMove;
        ///开始移动时轴的位置
        real3               _downPosWorld;
        ///鼠标按下的最后位置
        int2                _downPos;
        ///开始时的鼠标位置
        int2                _startPos;

        ///触屏按下的最后位置
        int2                _touchDownPos;
        ///触屏开始时按下的位置
        int2                _touchStartPos;
        ///移动通知
        MDelegate           _delegate;
        ///鼠标按钮按下
        bool                _bMouseDown;
        ///触屏按下
        bool                _bTouchDown;
        ///触屏拾取轴
        bool                _bTouchPickup;
    };

    using   EditAxisMove    =   SharedPtr<FEEditAxisMove>;
}
