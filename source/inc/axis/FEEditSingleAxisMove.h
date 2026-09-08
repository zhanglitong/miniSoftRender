#pragma     once

/// <summary>
/// FEEditSingleAxisMove.h
/// 定义单轴移动编辑
/// </summary>
#include    "FEAxis.h"

namespace FE
{
    class FEEditSingleAxisMovePrivate;
    /// <summary>
    /// 单轴移动编辑坐标轴
    /// </summary>
    class FE_API FEEditSingleAxisMove: public FEEditAxis
    {
    public:
        enum AxisStyle
        {
            ///箭头
            AxisStyleArrow = 0,
            ///圆锥体
            AxisStyleCone = 1,
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
            , FEEditSingleAxisMove& sender)>;
    private:
        FEEditSingleAxisMovePrivate* _p;
        friend class FEEditSingleAxisMovePrivate;
    public:
        FEEditSingleAxisMove(FEContext& context);
        virtual ~FEEditSingleAxisMove();
    public: 
        /// <summary>
        /// 移动操作回调,当轴发生移动操作时，触发该回调
        /// </summary>
        MDelegate& mDelegate();
        /// <summary>
        /// 设置轴显示风格(默认为箭头)
        /// </summary>
        /// <param name="style">轴显示风格</param>
        void setAxisStyle(AxisStyle style);
        /// <summary>
        /// 获取轴显示风格(默认为箭头)
        /// </summary>
        /// <returns>轴显示风格</returns>
        AxisStyle axisStyle() const;
        /// <summary>
        /// 设置轴心位置
        /// </summary>
        /// <param name="pos">轴心位置</param>
        inline void setPosition(const real3& pos)
        {
            if (this->position() == pos)
                return;
            _transform = mat4r(1.0);
            _transform[3] = real4(pos, 1.0);
            _internalFlags.addFlag(InteralFlag_Update);
            _ctx.requireNextFrame();
        }
        /// <summary>
        /// 设置轴朝向
        /// </summary>
        /// <param name="dir">轴朝向(单位向量)</param>
        void setDir(const real3& dir);
        /// <summary>
        /// 获取轴朝向
        /// </summary>
        /// <returns>轴朝向(单位向量)</returns>
        const real3& dir();
        /// <summary>
        /// 设置正向移动最大距离限定
        /// </summary>
        /// <param name="maxDis">正向移动最大限定距离</param>
        void setForwardDistanceMaxLimit(real maxDis);
        /// <summary>
        /// 获取正向移动最大距离限定
        /// </summary>
        /// <returns>正向移动最大限定距离</returns>
        real forwardDistanceMaxLimit() const;
        /// <summary>
        /// 设置反向移动最大距离限定
        /// </summary>
        /// <param name="maxDis">反向移动最大限定距离</param>
        void setReverseDistanceMaxLimit(real maxDis);
        /// <summary>
        /// 获取反向移动最大距离限定
        /// </summary>
        /// <returns>反向移动最大限定距离</returns>
        real reverseDistanceMaxLimit() const;
    public:
        /// <summary>
        /// 轴是否高亮
        /// </summary>
        /// <returns>高亮返回true，否则返回false</returns>
        virtual bool isAxisHovered() const override;
        /// <summary>
        /// 轴是被选中
        /// </summary>
        /// <returns>选中返回true，否则返回false</returns>
        virtual bool isAxisSelected() const override;
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
            return false;
        }
        /// <summary>
        /// 判断某个值是否是X轴的枚举值,子类实现
        /// </summary>
        virtual bool isEnumAxisY(int value) const override
        {
            return false;
        }
        /// <summary>
        /// 判断某个值是否是X轴的枚举值,子类实现
        /// </summary>
        virtual bool isEnumAxisZ(int value) const override
        {
            return false;
        }
    public:
        virtual bool mouseButtonPress(FEContext& context, const int2& pos) override;
        virtual bool mouseButtonRelease(FEContext& context, const int2& pos) override;
        virtual bool mouseMove(FEContext& context, const int2& pos) override;
        virtual bool touchDown(FEContext& context, const int2& pos) override;
        virtual bool touchUp(FEContext& context, const int2& pos) override;
        virtual bool touchMove(FEContext& context, const int2& pos)override;
    public:
        /// <summary>
        /// 这里不再使用矩阵来变换轴向，而是直接通过接口 setDir() 来指定
        /// </summary>
        inline void setTransform(const mat4r&) = delete;
        /// <summary>
        /// 这里不再使用矩阵来变换轴向
        /// </summary>
        inline const mat4r& transform() const = delete;
        /// <summary>
        /// 这里X轴向不会生效,而是使用 dir() 获取轴向
        /// </summary>
        inline real3 axisX() const = delete;
        /// <summary>
        /// 这里Y轴向不会生效,而是使用 dir() 获取轴向
        /// </summary>
        inline real3 axisY() const = delete;
        /// <summary>
        /// 这里Z轴向不会生效,而是使用 dir() 获取轴向
        /// </summary>
        inline real3 axisZ() const = delete;
    };
}

