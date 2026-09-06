#pragma     once

/// <summary>
/// FEEditSingleAxisMove.h
/// 定义单轴移动编辑
/// </summary>
#include    "FEAxis.h"
#include    <array>

namespace FE
{
    class FEEditFaceAxisMovePrivate;
    /// <summary>
    /// 单轴移动编辑坐标轴
    /// </summary>
    class FE_API FEEditFaceAxisMove: public FEEditAxis
    {
    public:
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
            , FEEditFaceAxisMove& sender)>;
    private:
        FEEditFaceAxisMovePrivate* _p;
        friend class FEEditFaceAxisMovePrivate;
    public:
        FEEditFaceAxisMove(FEContext& context);
        virtual ~FEEditFaceAxisMove();
    public: 
        /// <summary>
        /// 移动操作回调,当轴发生移动操作时，触发该回调
        /// </summary>
        MDelegate& mDelegate();
        /// <summary>
        /// 设置矩形顶点
        /// </summary>
        void setRectPoints(const std::array<real3, 4>& pts);
        /// <summary>
        /// 获取矩形顶点
        /// </summary>
        const std::array<real3, 4>& rectPoints() const;
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
    public:
        virtual bool mouseButtonPress(FEContext& context, const int2& pos) override;
        virtual bool mouseButtonRelease(FEContext& context, const int2& pos) override;
        virtual bool mouseMove(FEContext& context, const int2& pos) override;
        virtual bool touchDown(FEContext& context, const int2& pos) override;
        virtual bool touchUp(FEContext& context, const int2& pos) override;
        virtual bool touchMove(FEContext& context, const int2& pos)override;
        virtual void update(FEContext& context) override;
        virtual void render(FEContext& context) override;
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
    private:
        void setPosition(const real3& pos)
        {
            _transform = mat4r(1.0);
            _transform[3] = real4(pos, 1.0);
        }
    };
}

