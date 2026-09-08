#pragma     once
#include    "FEAxis.h"

/// <summary>
/// FEEditAxisScale.h
/// 定义缩放编辑坐标轴
/// </summary>
namespace   FE
{
    class FEEditAxisScalePrivate;
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
    private:
        FEEditAxisScalePrivate* _p;
        friend class FEEditAxisScalePrivate;
    public:
        FEEditAxisScale(FEContext& context);
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
        FEEditAxisScale::AXIS hoveredAxis() const;
        /// <summary>
        /// 获取当前已经被选中的轴
        /// </summary>
        /// <returns>返回选中的轴索引</returns>
        FEEditAxisScale::AXIS selectedAxis() const;
    public:
        /// <summary>
        /// 是否有轴被高亮
        /// </summary>
        /// <returns>如果有轴被高亮，则返回true,否则返回false</returns>
        virtual bool isAxisHovered() const override
        {
            return hoveredAxis() != FEEditAxisScale::AXIS::AXIS_NULL;
        }        
        /// <summary>
        /// 是否有轴被选中
        /// </summary>
        /// <returns>如果有轴被选中,则返回true,否则返回false</returns>
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
    protected:
        /// <summary>
        /// 判断某个值是否是X轴的枚举值,子类实现
        /// </summary>
        virtual bool isEnumAxisX(int value) const override
        {
            return value == FEEditAxisScale::AXIS::AXIS_X;
        }
        /// <summary>
        /// 判断某个值是否是X轴的枚举值,子类实现
        /// </summary>
        virtual bool isEnumAxisY(int value) const override
        {
            return value == FEEditAxisScale::AXIS::AXIS_Y;
        }
        /// <summary>
        /// 判断某个值是否是X轴的枚举值,子类实现
        /// </summary>
        virtual bool isEnumAxisZ(int value) const override
        {
            return value == FEEditAxisScale::AXIS::AXIS_Z;
        }
    public:
        virtual bool mouseButtonPress(FEContext& context, const int2& pos) override;
        virtual bool mouseButtonRelease(FEContext& context, const int2& pos) override;
        virtual bool mouseMove(FEContext& context, const int2& pos) override;

        virtual bool touchDown(FEContext& context, const int2& pos) override;
        virtual bool touchUp(FEContext& context, const int2& pos) override;
        virtual bool touchMove(FEContext& context, const int2& pos) override;
    };
}

