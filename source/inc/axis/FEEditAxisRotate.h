#pragma     once
#include    "FEAxis.h"

/// <summary>
/// FEEditAxisRotate.h
/// 定义旋转编辑坐标轴
/// </summary>
namespace   FE
{
    class FEEditAxisRotatePrivate;
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
    private:
        FEEditAxisRotatePrivate* _p;
        friend class FEEditAxisRotatePrivate;
    public:
        FEEditAxisRotate(FEContext& context);
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
        FEEditAxisRotate::AXIS hoveredAxis() const;

        /// <summary>
        /// 获取当前已经被选中的轴
        /// </summary>
        /// <returns>返回选中的轴索引</returns>
        FEEditAxisRotate::AXIS selectedAxis() const;
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
    public:
        /// <summary>
        /// 是否有轴被高亮
        /// </summary>
        /// <returns>如果有轴被高亮，则返回true,否则返回false</returns>
        virtual bool isAxisHovered() const override
        {
            return hoveredAxis() != FEEditAxisRotate::AXIS::AXIS_NULL;
        }
        /// <summary>
        /// 是否有轴被选中
        /// </summary>
        /// <returns>如果有轴被选中,则返回true,否则返回false</returns>
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
        /// 判断某个值是否是X轴的枚举值,子类实现
        /// </summary>
        virtual bool isEnumAxisY(int value) const override
        {
            return value == FEEditAxisRotate::AXIS::AXIS_Y;
        }
        /// <summary>
        /// 判断某个值是否是X轴的枚举值,子类实现
        /// </summary>
        virtual bool isEnumAxisZ(int value) const override
        {
            return value == FEEditAxisRotate::AXIS::AXIS_Z;
        }
    public:
        virtual bool mouseButtonPress(FEContext& context, const int2& pos) override;
        virtual bool mouseButtonRelease(FEContext& context, const int2& pos) override;
        virtual bool mouseMove(FEContext& context, const int2& pos) override;
        virtual bool touchDown(FEContext& context, const int2& pos) override;
        virtual bool touchUp(FEContext& context, const int2& pos) override;
        virtual bool touchMove(FEContext& context, const int2& pos) override;
        virtual void update(FEContext& context) override;
        virtual void render(FEContext& context) override;
    };
}


