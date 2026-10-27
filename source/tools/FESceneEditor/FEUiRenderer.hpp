/// <summary>
/// FEUiRenderer
/// 把 FEUiDocument 的控件树用 ImGui 画出来:
///   - 布局引擎:QHBoxLayout / QVBoxLayout / QGridLayout,按 sizePolicy 的 stretch 分配空间,
///     固定尺寸控件按文本尺寸预留,剩余空间按权重分给 Expanding 控件;
///   - 控件映射:按钮 / 复选框 / 下拉框 / 输入框 / 标签 / 分割器 / 状态栏;
///   - 未识别的提升控件画占位框,并调用按类名注册的回调。
/// 注意:ImGui 1.65 没有 DockSpace / TabBar / Table / Splitter,相关效果均为自绘。
/// </summary>

#pragma     once

#include    "FEUiDocument.hpp"

#include    <functional>
#include    <map>
#include    <string>
#include    <vector>

namespace   FE
{
    class   FEUiDocument;
    class   FEUiWidget;
    struct  FEUiRect;

    /// <summary>
    /// 自定义提升控件回调:在给定屏幕矩形内绘制控件
    /// </summary>
    using   FEUiHandler    =   std::function<void(FEUiWidget&,const FEUiRect&)>;

    class   FEUiRenderer
    {
    public:
        /// <summary>
        /// 在调用方已 Begin 的窗口内绘制整棵控件树
        /// </summary>
        /// <param name="doc">解析后的 .ui 文档</param>
        void    draw(FEUiDocument& doc);
        /// <summary>
        /// 按控件类名注册自定义控件回调
        /// </summary>
        /// <param name="cls">控件类名</param>
        /// <param name="handler">绘制回调</param>
        void    setHandler(const std::string& cls,FEUiHandler handler);
    protected:
        void    drawWidget(FEUiWidget* widget,const FEUiRect& rect);
        void    drawLayoutNode(FEUiLayout* layout,const FEUiRect& rect);
        void    drawLayout(FEUiWidget* widget,const FEUiRect& rect);
        void    drawSplitter(FEUiWidget* widget,const FEUiRect& rect);
        void    drawGroupBox(FEUiWidget* widget,const FEUiRect& rect);
        void    drawControl(FEUiWidget* widget,const FEUiRect& rect);
        void    drawPlaceholder(FEUiWidget* widget,const FEUiRect& rect);
        void    layoutLinear(FEUiLayout* layout,const FEUiRect& rect,bool horizontal);
        void    layoutGrid(FEUiLayout* layout,const FEUiRect& rect);
        /// <summary>
        /// 控件的固有尺寸(横向返回宽度,纵向返回高度)
        /// </summary>
        /// <param name="widget">控件</param>
        /// <param name="horizontal">true 取宽度 / false 取高度</param>
        /// <returns>固有尺寸</returns>
        float   widgetHint(FEUiWidget* widget,bool horizontal) const;
        /// <summary>
        /// 控件显示文本(按钮文字 / 控件名 / 类名兜底)
        /// </summary>
        /// <param name="widget">控件</param>
        /// <returns>显示文本</returns>
        std::string widgetLabel(FEUiWidget* widget) const;
        /// <summary>
        /// 稳定的 ImGui ID(部分控件没有 name)
        /// </summary>
        /// <param name="widget">控件</param>
        /// <returns>ID 文本</returns>
        std::string widgetId(FEUiWidget* widget) const;
    protected:
        std::map<std::string,FEUiHandler>   _handlers;
    };
}
