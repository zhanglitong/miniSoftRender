/// summary
/// FEUiMetrics
/// 各窗口共用的版式尺寸与缓冲容量,避免同一数值在多个文件里各自硬编码。
/// 与颜色/行高相关的度量仍放在 FEUiStyle.hpp。
/// </summary>

#pragma     once

namespace   FE
{
namespace   ui
{
    /// <summary>
    /// 功能区高度(像素):撤销窗口等浮层据此定位
    /// </summary>
    constexpr   float   kRibbonHeight  =   128.0f;

    /// <summary>
    /// 时间线:动画树名称列宽度(可由分割条拖动)
    /// </summary>
    constexpr   float   kTimelineNameWidth      =   340.0f;
    /// <summary>
    /// 时间线:名称列与画布之间的分割条宽度
    /// </summary>
    constexpr   float   kTimelineSplitterWidth  =   6.0f;
    /// <summary>
    /// 时间线:标尺高度
    /// </summary>
    constexpr   float   kTimelineRulerHeight    =   24.0f;
    /// <summary>
    /// 时间线:行高(与模型树一致)
    /// </summary>
    constexpr   float   kTimelineRowHeight      =   24.0f;
    /// <summary>
    /// 时间线:行步进 = 行高 + 行间距
    /// </summary>
    constexpr   float   kTimelineRowStep        =   28.0f;

    /// <summary>
    /// 搜索输入缓冲长度
    /// </summary>
    constexpr   int     kSearchBufSize =   64;
    /// <summary>
    /// 重命名输入缓冲长度
    /// </summary>
    constexpr   int     kRenameBufSize =   128;
    /// <summary>
    /// 文件对话框路径缓冲长度
    /// </summary>
    constexpr   int     kPathBufSize   =   1024;
    /// <summary>
    /// 文件名缓冲长度(Windows MAX_PATH)
    /// </summary>
    constexpr   int     kFileNameBufSize    =   260;
}
}
