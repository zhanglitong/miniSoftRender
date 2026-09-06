#pragma     once
#include    "FEAxis.h"
#include    <list>
/// <summary>
/// FEEditAxisGroup.h
/// 定义编辑轴分组，可实现批量使用编辑轴
/// </summary>
namespace   FE
{
    class FEContext;
    /// <summary>
    /// 编辑轴组,一组编辑轴内，每次只响应一个操作
    /// </summary>
    class FEEditAxisGroupPrivate;
    class FE_API FEEditAxisGroup
    {
    public:
        using Axies = std::list<FEEditAxis*>;
        class AxisAttribute
        {
        private:
            bool _bVisible;
        public:
            AxisAttribute()
            {
                _bVisible = true;
            }
        public:
            void setVisible(bool b) { _bVisible = b; }
            inline bool visible() const { return _bVisible; }
        };
    private:
        FEEditAxisGroupPrivate& _p;
    public:
        FEEditAxisGroup();
        ~FEEditAxisGroup();
    public:
        /// <summary>
        /// 添加编辑轴
        /// </summary>
        /// <param name="pAxis">欲添加的编辑轴</param>
        void addAxis(FEEditAxis* pAxis);
        /// <summary>
        /// 是否包含某个编辑轴
        /// </summary>
        /// <param name="pAxis">编辑轴对象</param>
        /// <returns>包含返回true，否则返回false</returns>
        bool containsAxis(FEEditAxis* pAxis) const;
        /// <summary>
        /// 移除编辑轴
        /// </summary>
        /// <param name="pAxis">欲移除的编辑轴</param>
        /// <returns>移除成功返回true,移除失败返回false</returns>
        bool removeAxis(FEEditAxis* pAxis);
        /// <summary>
        /// 清除所有编辑轴
        /// </summary>
        void clearAxies();
        /// <summary>
        /// 获取某个编辑轴的属性
        /// </summary>
        /// <param name="pAxis">欲获取属性的编辑轴对象</param>
        /// <returns>如果编辑轴存在，则返回对应属性，否则返回nullptr</returns>
        AxisAttribute* attribute(FEEditAxis* pAxis);
        /// <summary>
        /// 获取某个编辑轴的属性
        /// </summary>
        /// <param name="pAxis">欲获取属性的编辑轴对象</param>
        /// <returns>如果编辑轴存在，则返回对应属性，否则返回nullptr</returns>
        const AxisAttribute* attribute(FEEditAxis* pAxis) const;
        /// <summary>
        /// 是否为空
        /// </summary>
        bool empty() const;
        /// <summary>
        /// 获取所有已包含的编辑轴列表
        /// </summary>
        /// <returns>编辑轴列表</returns>
        Axies axies()const;
        /// <summary>
        /// 是否有轴高亮
        /// </summary>
        inline bool hasHoveredAxis()const
        {
            return this->hoveredAxis() != nullptr;
        }
        /// <summary>
        /// 是否有轴选中
        /// </summary>
        bool hasSelcetedAxis() const
        {
            return this->selectedAxis() != nullptr;
        }
    public:
        /// <summary>
        /// 鼠标按下事件
        /// </summary>
        /// <param name="context">上下文对象</param>
        /// <param name="pos">鼠标按下时的屏幕坐标位置</param>
        void mouseButtonPress(FEContext& context, const int2& pos);
        /// <summary>
        /// 鼠标抬起事件
        /// </summary>
        /// <param name="context">上下文对象</param>
        /// <param name="pos">鼠标抬起时的屏幕坐标位置</param>
        void mouseButtonRelease(FEContext& context, const int2& pos);
        /// <summary>
        /// 鼠标移动事件
        /// </summary>
        /// <param name="context">上下文对象</param>
        /// <param name="pos">鼠标移动的当前屏幕坐标位置</param>
        void mouseMove(FEContext& context, const int2& pos);
        /// <summary>
        /// 触屏按下事件
        /// </summary>
        /// <param name="context">上下文对象</param>
        /// <param name="pos">触屏按下的当前屏幕坐标位置</param>
        void touchDown(FEContext& context, const int2& pos);
        /// <summary>
        /// 触屏抬起事件
        /// </summary>
        /// <param name="context">上下文对象</param>
        /// <param name="pos">触屏抬起的当前屏幕坐标位置</param>
        void touchUp(FEContext& context, const int2& pos);
        /// <summary>
        /// 触屏移动事件
        /// </summary>
        /// <param name="context">上下文对象</param>
        /// <param name="pos">触屏移动的当前屏幕坐标位置</param>
        void touchMove(FEContext& context, const int2& pos);
    public:
        /// <summary>
        /// 轴更新，将更新所有已加入的轴对象
        /// </summary>
        /// <param name="context">上下文对象</param>
        void update(FEContext& context);
        /// <summary>
        /// 轴绘制，将绘制所有已加入且未被隐藏的轴对象
        /// </summary>
        /// <param name="context">上下文对象</param>
        void render(FEContext& context);
    private:
        ///获取到所有可绘制的轴，并根据与摄像机眼睛的距离做一个由近到远的排序
        Axies sortAxies(FEContext& context) const;
        ///获取当前被高亮的轴
        FEEditAxis* hoveredAxis() const;
        ///获取当前被选中的轴
        FEEditAxis* selectedAxis() const;
    };
}

