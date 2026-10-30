/// <summary>
/// FEUiStyle
/// 模型树 / 动画树 / 时间线共用的行高、缩进、行底色与文字颜色。
/// 三个控件都引用这里的常量,保证行高计算与选中/悬停视觉效果完全一致。
/// </summary>

#pragma     once

#include    <imgui.h>

namespace   FE
{
namespace   ui
{
    /// <summary>
    /// 行距相对字体大小的倍数:行距 = 字体大小 × 2.4
    /// </summary>
    constexpr   float   kRowLineScale   =   2.4f;
    /// <summary>
    /// 行与行之间的留白(像素)
    /// </summary>
    constexpr   float   kRowGap         =   4.0f;
    /// <summary>
    /// 每级缩进宽度(像素)
    /// </summary>
    constexpr   float   kIndent         =   14.0f;
    /// <summary>
    /// 滚动条厚度(像素)
    /// </summary>
    constexpr   float   kScrollBarSize  =   10.0f;

    /// <summary>
    /// 行距(含行间距)
    /// </summary>
    inline  float   rowStep()
    {
        return  ImGui::GetFontSize() * kRowLineScale;
    }
    /// <summary>
    /// 行高(不含行间距)
    /// </summary>
    inline  float   rowHeight()
    {
        const   float   step    =   rowStep();
        return  (step > kRowGap) ? (step - kRowGap) : 1.0f;
    }

    /// <summary>
    /// 选中行填充色
    /// </summary>
    constexpr   ImU32   kRowSelected   =   IM_COL32(74,144,226,72);
    /// <summary>
    /// 选中行线框色
    /// </summary>
    constexpr   ImU32   kRowSelBorder  =   IM_COL32(74,144,226,220);
    /// <summary>
    /// 悬停行底色
    /// </summary>
    constexpr   ImU32   kRowHover      =   IM_COL32(255,255,255,14);
    /// <summary>
    /// 隔行底色
    /// </summary>
    constexpr   ImU32   kRowAlt        =   IM_COL32(255,255,255,10);
    /// <summary>
    /// 普通文字色
    /// </summary>
    constexpr   ImU32   kTextColor     =   IM_COL32(214,219,229,255);
    /// <summary>
    /// 选中行文字色
    /// </summary>
    constexpr   ImU32   kTextSelected  =   IM_COL32(235,245,255,255);
    /// <summary>
    /// 折叠箭头色
    /// </summary>
    constexpr   ImU32   kArrowColor    =   IM_COL32(150,156,168,255);
    /// <summary>
    /// 强调色(拖放提示/高亮线)
    /// </summary>
    constexpr   ImU32   kAccent        =   IM_COL32(74,144,226,255);

    /// <summary>
    /// 次要文字色(禁用/隐藏项)
    /// </summary>
    constexpr   ImU32   kTextMuted     =   IM_COL32(140,146,158,255);

    /// <summary>
    /// 替换颜色的 alpha 通道(ImU32 高 8 位为 alpha)
    /// </summary>
    constexpr   ImU32   withAlpha(ImU32 color,unsigned char alpha)
    {
        return  (color & 0x00FFFFFFu) | (ImU32(alpha) << 24);
    }
}
}
