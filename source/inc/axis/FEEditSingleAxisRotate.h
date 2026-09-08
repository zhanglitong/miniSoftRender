#pragma     once

/// <summary>
/// FEEditSingleAxisRotate.h
/// 定义单轴旋转编辑
/// </summary>
#include    "FEAxis.h"

namespace   FE
{
    class FEEditSingleAxisRotatePrivate;
    /// <summary>
    /// 单轴移动编辑坐标轴
    /// </summary>
    class FE_API FEEditSingleAxisRotate : public FEEditAxis
    {
    public:
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
            , FEEditSingleAxisRotate& sender)>;
    private:
        FEEditSingleAxisRotatePrivate * _p;
        friend class FEEditSingleAxisRotatePrivate;
    public:
        FEEditSingleAxisRotate(FEContext& context);
        virtual ~FEEditSingleAxisRotate();
    public: 
        /// <summary>
        /// 移动操作回调,当轴发生移动操作时，触发该回调
        /// </summary>
        MDelegate& mDelegate();
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
        /// 设置旋转轴朝向
        /// </summary>
        /// <param name="dir">旋转轴朝向(单位向量)</param>
        void setAxis(const real3& axis);
        /// <summary>
        /// 获取旋转轴朝向
        /// </summary>
        /// <returns>旋转轴朝向(单位向量)</returns>
        const real3& axis();
        /// <summary>
        /// 设置正向旋转最大角度限定
        /// </summary>
        /// <param name="maxDis">正向旋转最大角度</param>
        void setForwardAngleMaxLimit(real maxAngle);
        /// <summary>
        /// 获取正向移动最大距离限定
        /// </summary>
        /// <returns>正向移动最大限定距离</returns>
        real forwardAngleMaxLimit() const;
        /// <summary>
        /// 设置反向旋转最大角度限定
        /// </summary>
        /// <param name="maxDis">反向旋转最大角度</param>
        void setReverseAngleMaxLimit(real maxAngle);
        /// <summary>
        /// 获取反向旋转最大角度限定
        /// </summary>
        /// <returns>反向旋转最大角度</returns>
        real reverseAngleMaxLimit() const;
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
        virtual  bool isAxisSelected() const override;
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
        virtual bool mouseButtonPress(FEContext& context, const int2& pos) ;
        virtual bool mouseButtonRelease(FEContext& context, const int2& pos) ;
        virtual bool mouseMove(FEContext& context, const int2& pos) ;
        virtual bool touchDown(FEContext& context, const int2& pos) ;
        virtual bool touchUp(FEContext& context, const int2& pos) ;
        virtual bool touchMove(FEContext& context, const int2& pos) ;
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

