/// <summary>
/// FEEditorUi 内部共享定义:版式常量与 Ribbon 绘制工具。
/// 拆分为 FEEditorUi.cpp / FEEditorUiRibbon.cpp / FEEditorUiActions.cpp 后由三者共用;
/// 仍放在匿名命名空间内,保持内部链接,不导出符号。
/// </summary>

#pragma     once

#include    "FEEditorUi.hpp"
#include    "FEUiDocument.hpp"
#include    "FEIconGlyphs.hpp"
#include    "FEUiStyle.hpp"

#include    <cfloat>
#include    <filesystem>
#include    <map>
#include    <string>
#include    <vector>

#include    <imgui.h>

namespace   FE
{
namespace   ui
{
namespace   internal
{
namespace
{
    /// <summary>
    /// Ribbon 外观常量(暗色主题):强调色/文字色取自 FEUiStyle
    /// </summary>
    const   ImU32   kIconColor  =   IM_COL32(205,210,220,255);
    const   ImU32   kTabBarBg   =   IM_COL32(30,33,40,255);
    const   ImU32   kRibbonBg   =   IM_COL32(41,44,52,255);

    /// <summary>
    /// 标签条起始 y
    /// </summary>
    const   float   kTabRowY    =   3.0f;
    /// <summary>
    /// 功能区起始 y
    /// </summary>
    const   float   kGroupRowY  =   36.0f;
    /// <summary>
    /// 分组内容高度(按钮行的总高度)
    /// </summary>
    const   float   kContentH   =   64.0f;
    /// <summary>
    /// 按钮最小宽(同一分组内所有按钮等宽)
    /// </summary>
    const   float   kMinButtonW =   64.0f;
    /// <summary>
    /// 按钮最大宽
    /// </summary>
    const   float   kButtonMaxW =   200.0f;
    /// <summary>
    /// 分组内左右留白
    /// </summary>
    const   float   kGroupPadX  =   6.0f;
    /// <summary>
    /// 分组之间的间距(竖线居中)
    /// </summary>
    const   float   kGroupGap   =   10.0f;
    /// <summary>
    /// 功能区内容的左起始位置
    /// </summary>
    const   float   kGroupStartX    =   6.0f;
    /// <summary>
    /// 内容放不下时,左右滚动箭头的宽度
    /// </summary>
    const   float   kArrowW     =   18.0f;
    /// <summary>
    /// 点击滚动箭头时每次滚动的像素数
    /// </summary>
    const   float   kScrollStep =   160.0f;
    /// <summary>
    /// 按钮/标签/箭头的圆角
    /// </summary>
    const   float   kRibbonRounding  =   3.0f;
    /// <summary>
    /// 按钮在文字宽度之外额外留出的宽度
    /// </summary>
    const   float   kRibbonButtonPadW  =   12.0f;
    /// <summary>
    /// 文字超过按钮宽度减该值后折成两行
    /// </summary>
    const   float   kRibbonWrapPad     =   8.0f;
    /// <summary>
    /// 文字贴左边时的最小左边距
    /// </summary>
    const   float   kRibbonTextMinX    =   2.0f;
    /// <summary>
    /// 分组标题与按钮底边的间距
    /// </summary>
    const   float   kGroupTitleGapY    =   3.0f;
    /// <summary>
    /// 标签页水平/垂直留白与底部强调线高度
    /// </summary>
    const   float   kTabHPad           =   26.0f;
    const   float   kTabHeight         =   28.0f;
    const   float   kTabUnderlineH     =   2.0f;
    /// <summary>
    /// 标签条右边界/信息文字边距与纵向偏移
    /// </summary>
    const   float   kInfoMarginX       =   8.0f;
    const   float   kTabStartX         =   6.0f;
    const   float   kTabSpacing        =   2.0f;
    const   float   kInfoOffsetY       =   5.0f;
    /// <summary>
    /// 标签条底边相对功能区的上移量
    /// </summary>
    const   float   kTabBarBottomGap   =   4.0f;
    /// <summary>
    /// 滚动箭头绘制参数:线宽、X/Y 方向半长与线厚
    /// </summary>
    const   float   kArrowDx           =   3.5f;
    const   float   kArrowDy           =   4.5f;
    const   float   kArrowThickness    =   1.6f;
    /// <summary>
    /// Ribbon 交互态底色/描边
    /// </summary>
    const   ImU32   kRibbonCheckedFill  =   IM_COL32(74,144,226,70);
    const   ImU32   kRibbonHoverFill    =   IM_COL32(255,255,255,20);
    const   ImU32   kRibbonHoverBorder  =   IM_COL32(255,255,255,40);
    const   ImU32   kTabHoverFill       =   IM_COL32(255,255,255,18);
    const   ImU32   kRibbonDivider      =   IM_COL32(255,255,255,26);
    const   ImU32   kRibbonTextDisabled =   IM_COL32(120,124,134,255);
    const   ImU32   kArrowDisabled      =   IM_COL32(92,96,106,255);
    const   ImU32   kShortcutGrid       =   IM_COL32(66,70,78,255);

    /// <summary>
    /// 配置窗口默认位置与尺寸
    /// </summary>
    const   float   kSettingWindowW     =   560.0f;
    const   float   kSettingWindowH     =   600.0f;
    const   float   kSettingWindowX     =   32.0f;
    const   float   kSettingWindowYOff  =   16.0f;
    /// <summary>
    /// 快捷键对话框尺寸上限/下限、屏幕边距与位置偏移
    /// </summary>
    const   float   kShortcutDlgW       =   640.0f;
    const   float   kShortcutDlgH       =   520.0f;
    const   float   kShortcutDlgMinH    =   240.0f;
    const   float   kShortcutScreenPad  =   16.0f;
    const   float   kShortcutPosPad     =   8.0f;
    const   float   kShortcutBottomPad  =   24.0f;
    const   float   kShortcutYOff       =   12.0f;
    /// <summary>
    /// 快捷键列表版式:列宽比例、行高增量、单元格/底线/底部留白与按钮宽度
    /// </summary>
    const   float   kShortcutSplitRatio =   0.48f;
    const   float   kShortcutRowPadH    =   12.0f;
    const   float   kShortcutCellPadX   =   6.0f;
    const   float   kShortcutGridLineW  =   1.0f;
    const   float   kShortcutFooterGap  =   6.0f;
    const   float   kShortcutBtnPadW    =   28.0f;
    /// <summary>
    /// 关于窗口尺寸、品牌区与信息行版式
    /// </summary>
    const   float   kAboutWindowW       =   420.0f;
    const   float   kAboutYOff          =   16.0f;
    const   float   kAboutPad           =   16.0f;
    const   float   kAboutHeroH         =   92.0f;
    const   float   kAboutAccentW       =   3.0f;
    const   float   kAboutIconCx        =   46.0f;
    const   float   kAboutIconR         =   24.0f;
    const   float   kAboutTextX         =   84.0f;
    const   float   kAboutTitleY        =   22.0f;
    const   float   kAboutSubY          =   50.0f;
    const   float   kAboutTitleFontSize =   22.0f;
    const   float   kAboutChipPadX      =   8.0f;
    const   float   kAboutChipH         =   20.0f;
    const   float   kAboutChipY         =   12.0f;
    const   float   kAboutSectionGap    =   14.0f;
    const   float   kAboutSectionBarW   =   3.0f;
    const   float   kAboutRowH          =   22.0f;
    const   float   kAboutRowPadX       =   8.0f;
    const   float   kAboutRowRounding   =   3.0f;
    const   float   kAboutFooterGap     =   8.0f;
    const   float   kAboutBtnPadX       =   18.0f;
    /// <summary>
    /// 关于窗口配色
    /// </summary>
    const   ImU32   kAboutHeroBg       =   IM_COL32(31,35,43,255);
    const   ImU32   kAboutChipBg       =   IM_COL32(74,144,226,55);
    const   ImU32   kAboutChipBorder   =   IM_COL32(74,144,226,170);
    const   ImU32   kAboutChipText     =   IM_COL32(120,190,255,255);
    const   ImU32   kAboutDivider      =   IM_COL32(255,255,255,26);
    const   ImU32   kAboutRowAlt       =   IM_COL32(255,255,255,7);
    /// <summary>
    /// 模型树/属性/UI 预览窗口的默认位置与尺寸
    /// </summary>
    const   float   kModelTreeX         =   8.0f;
    const   float   kModelTreeYOff      =   8.0f;
    const   float   kModelTreeW         =   300.0f;
    const   float   kModelTreeH         =   420.0f;
    const   float   kPropertyX          =   316.0f;
    const   float   kPropertyYOff       =   8.0f;
    const   float   kPropertyW          =   300.0f;
    const   float   kPropertyH          =   360.0f;
    const   float   kUiPreviewX         =   80.0f;
    const   float   kUiPreviewYOff      =   16.0f;
    const   float   kUiPreviewW         =   900.0f;
    const   float   kUiPreviewH         =   560.0f;
    /// <summary>
    /// 默认中文字体字号
    /// </summary>
    const   float   kDefaultFontSize    =   16.0f;
    /// <summary>
    /// 右上角信息栏缓冲长度
    /// </summary>
    const   int     kInfoBufSize        =   1200;
    /// <summary>
    /// 脚本截图等待超时(毫秒)
    /// </summary>
    const   double  kShotTimeoutMs      =   3000.0;

    /// <summary>
    /// Windows 虚拟键码(只用到这几种;不把 windows.h 引进 UI 层)
    /// </summary>
    enum    FEVirtualKey : int
    {
        /// <summary>
        /// Backspace
        /// </summary>
        kVkBack         =   0x08,
        kVkTab          =   0x09,
        kVkEnter        =   0x0D,
        kVkShift        =   0x10,
        kVkControl      =   0x11,
        kVkAlt          =   0x12,
        kVkEscape       =   0x1B,
        kVkSpace        =   0x20,
        kVkPageUp       =   0x21,
        kVkPageDown     =   0x22,
        kVkEnd          =   0x23,
        kVkHome         =   0x24,
        kVkLeft         =   0x25,
        kVkUp           =   0x26,
        kVkRight        =   0x27,
        kVkDown         =   0x28,
        kVkInsert       =   0x2D,
        kVkDelete       =   0x2E,
        kVkA            =   0x41,
        kVkC            =   0x43,
        kVkV            =   0x56,
        kVkX            =   0x58,
        kVkY            =   0x59,
        kVkZ            =   0x5A,
        kVkLShift       =   0xA0,
        kVkRShift       =   0xA1,
        kVkLControl     =   0xA2,
        kVkRControl     =   0xA3,
        kVkLAlt         =   0xA4,
        kVkRAlt         =   0xA5,
    };

    /// <summary>
    /// ImGui 1.65 通过 io.KeyMap 把 ImGuiKey 映射到 io.KeysDown 的下标,
    /// 这里直接以 Windows 虚拟键码作下标,消息转发时无需再转换
    /// </summary>
    /// <param name="io">ImGui IO</param>
    void    setupKeyMap(ImGuiIO& io)
    {
        io.KeyMap[ImGuiKey_Tab]         =   kVkTab;
        io.KeyMap[ImGuiKey_LeftArrow]   =   kVkLeft;
        io.KeyMap[ImGuiKey_RightArrow]  =   kVkRight;
        io.KeyMap[ImGuiKey_UpArrow]     =   kVkUp;
        io.KeyMap[ImGuiKey_DownArrow]   =   kVkDown;
        io.KeyMap[ImGuiKey_PageUp]      =   kVkPageUp;
        io.KeyMap[ImGuiKey_PageDown]    =   kVkPageDown;
        io.KeyMap[ImGuiKey_Home]        =   kVkHome;
        io.KeyMap[ImGuiKey_End]         =   kVkEnd;
        io.KeyMap[ImGuiKey_Insert]      =   kVkInsert;
        io.KeyMap[ImGuiKey_Delete]      =   kVkDelete;
        io.KeyMap[ImGuiKey_Backspace]   =   kVkBack;
        io.KeyMap[ImGuiKey_Space]       =   kVkSpace;
        io.KeyMap[ImGuiKey_Enter]       =   kVkEnter;
        io.KeyMap[ImGuiKey_Escape]      =   kVkEscape;
        io.KeyMap[ImGuiKey_A]           =   kVkA;
        io.KeyMap[ImGuiKey_C]           =   kVkC;
        io.KeyMap[ImGuiKey_V]           =   kVkV;
        io.KeyMap[ImGuiKey_X]           =   kVkX;
        io.KeyMap[ImGuiKey_Y]           =   kVkY;
        io.KeyMap[ImGuiKey_Z]           =   kVkZ;
    }

    /// <summary>
    /// 修饰键状态由左右两个虚拟键码组合推导
    /// </summary>
    /// <param name="io">ImGui IO</param>
    void    syncModifierKeys(ImGuiIO& io)
    {
        io.KeyCtrl  =   io.KeysDown[kVkControl]  || io.KeysDown[kVkLControl] || io.KeysDown[kVkRControl];
        io.KeyShift =   io.KeysDown[kVkShift]    || io.KeysDown[kVkLShift]   || io.KeysDown[kVkRShift];
        io.KeyAlt   =   io.KeysDown[kVkAlt]      || io.KeysDown[kVkLAlt]     || io.KeysDown[kVkRAlt];
    }

    /// <summary>
    /// 建立 动作名 → .ui 菜单项 的索引(递归)
    /// </summary>
    /// <param name="node">当前节点</param>
    /// <param name="map">输出索引</param>
    void    collectActionMap(const FE::FEUiMenuItem* node,
                             std::map<std::string,const FE::FEUiMenuItem*>& map)
    {
        if (node == nullptr)
            return;
        if (node->_type == FE::kUiAction && !node->_name.empty())
            map.emplace(node->_name,node);
        for (const FE::FEUiMenuItem* child : node->_children)
            collectActionMap(child,map);
    }

    /// <summary>
    /// 按动作名在菜单树中查找(可写版本,用于把抓手可见状态回写到勾选标志)
    /// </summary>
    /// <param name="node">当前节点</param>
    /// <param name="name">动作名</param>
    /// <returns>找到的动作;不存在返回 nullptr</returns>
    FE::FEUiMenuItem*  findActionItem(FE::FEUiMenuItem* node,const std::string& name)
    {
        if (node == nullptr)
            return  nullptr;
        if (node->_type == FE::kUiAction && node->_name == name)
            return  node;
        for (FE::FEUiMenuItem* child : node->_children)
        {
            FE::FEUiMenuItem*   found   =   findActionItem(child,name);
            if (found != nullptr)
                return  found;
        }
        return  nullptr;
    }

    /// <summary>
    /// 两个对象列表是否一致(按对象指针比较,顺序敏感)
    /// </summary>
    bool    sameObjects(const FE::Objects& left,const FE::Objects& right)
    {
        if (left.size() != right.size())
            return  false;
        for (size_t i = 0; i < left.size(); ++i)
        {
            if (left[i].get() != right[i].get())
                return  false;
        }
        return  true;
    }

    /// <summary>
    /// 录制态下视为占位/解绑的键
    /// </summary>
    bool    isModifierKey(int vk)
    {
        return  vk == kVkShift || vk == kVkControl || vk == kVkAlt
             || vk == kVkLShift || vk == kVkRShift
             || vk == kVkLControl || vk == kVkRControl
             || vk == kVkLAlt || vk == kVkRAlt;
    }



    /// <summary>
    /// 按钮里的图标字号(像素)
    /// </summary>
    const   float   kIconFontSize   =   24.0f;
    /// <summary>
    /// 图标与文字之间的间距
    /// </summary>
    const   float   kIconTextGap    =   4.0f;
    /// <summary>
    /// 从图标路径中取出文件名(去掉目录与扩展名)
    /// </summary>
    /// <param name="path">.ui 中的图标路径,例如 :/EditorRes/res/cube.png</param>
    /// <returns>文件名,例如 cube</returns>
    std::string iconBaseName(const std::string& path)
    {
        const   size_t  slash   =   path.find_last_of("/\\");
        const   size_t  begin   =   (slash == std::string::npos) ? 0 : (slash + 1);
        size_t          end     =   path.find_last_of('.');
        if (end == std::string::npos || end < begin)
            end =   path.size();
        return  path.substr(begin,end - begin);
    }

    /// <summary>
    /// 图标路径 -> Font Awesome 码位;未登记时回退到默认字形
    /// </summary>
    /// <param name="path">.ui 中的图标路径</param>
    /// <returns>码位</returns>
    unsigned int    iconCodepoint(const std::string& path)
    {
        const   std::string name    =   iconBaseName(path);
        for (const auto& glyph : kIconGlyphs)
        {
            if (name == glyph._name)
                return  glyph._code;
        }
        return  kIconDefault;
    }

    /// <summary>
    /// 码位转 UTF-8 文本(最多 4 字节,含结束符共 5 字节)
    /// </summary>
    /// <param name="code">Unicode 码位</param>
    /// <param name="out">输出缓冲,至少 5 字节</param>
    void    codepointToUtf8(unsigned int code,char* out)
    {
        if (code < 0x80)
        {
            out[0]  =   char(code);
            out[1]  =   '\0';
        }
        else if (code < 0x800)
        {
            out[0]  =   char(0xC0 | (code >> 6));
            out[1]  =   char(0x80 | (code & 0x3F));
            out[2]  =   '\0';
        }
        else if (code < 0x10000)
        {
            out[0]  =   char(0xE0 | (code >> 12));
            out[1]  =   char(0x80 | ((code >> 6) & 0x3F));
            out[2]  =   char(0x80 | (code & 0x3F));
            out[3]  =   '\0';
        }
        else
        {
            out[0]  =   char(0xF0 | (code >> 18));
            out[1]  =   char(0x80 | ((code >> 12) & 0x3F));
            out[2]  =   char(0x80 | ((code >> 6) & 0x3F));
            out[3]  =   char(0x80 | (code & 0x3F));
            out[4]  =   '\0';
        }
    }

    /// <summary>
    /// UTF-8 路径转本地(native)窄字符串。
    /// 引擎的文件接口按本机窄编码解释文件名(Windows 为 ANSI,类 Unix 即 UTF-8),
    /// 与标准库 path 的本地编码一致,故交给 std::filesystem 完成转换
    /// </summary>
    /// <param name="utf8Path">UTF-8 路径</param>
    /// <returns>本地窄编码路径</returns>
    std::string toNativePath(const std::string& utf8Path)
    {
        return  std::filesystem::u8path(utf8Path).string();
    }

    /// <summary>
    /// 本地(native)窄字符串路径转 UTF-8(ImGui 与对话框内部统一 UTF-8)
    /// </summary>
    /// <param name="nativePath">本地窄编码路径</param>
    /// <returns>UTF-8 路径</returns>
    std::string toUtf8Path(const std::string& nativePath)
    {
        return  std::filesystem::path(nativePath).u8string();
    }

    /// <summary>
    /// Ribbon 里的动作按钮集合(来自 .ui 菜单树的叶子)
    /// </summary>
    using   RibbonActions   =   std::vector<const FEUiMenuItem*>;

    /// <summary>
    /// 一个分组的按钮排布参数
    /// </summary>
    struct  FERibbonMetrics
    {
        /// <summary>
        /// 按钮列数
        /// </summary>
        int     _cols   =   1;
        /// <summary>
        /// 按钮宽
        /// </summary>
        float   _btnW   =   0.0f;
        /// <summary>
        /// 按钮高
        /// </summary>
        float   _btnH   =   0.0f;
        /// <summary>
        /// 分组占用宽度(含左右留白)
        /// </summary>
        float   _groupW =   0.0f;
    };

    /// <summary>
    /// 计算分组内的按钮排布:所有按钮等宽、单行排列(图标在上、文字在下)
    /// </summary>
    /// <param name="actions">分组内的动作</param>
    /// <returns>排布参数</returns>
    FERibbonMetrics ribbonMetrics(const RibbonActions& actions)
    {
        FERibbonMetrics     metrics;
        const   int         count   =   int(actions.size());
        metrics._cols   =   (count > 0) ? count : 1;
        float   textW   =   0.0f;
        for (const auto* action : actions)
            textW   =   std::max(textW,ImGui::CalcTextSize(action->_text.c_str()).x);
        metrics._btnW   =   std::min(std::max(kMinButtonW,std::max(textW,kIconFontSize) + kRibbonButtonPadW),kButtonMaxW);
        metrics._btnH   =   kContentH;
        metrics._groupW =   float(metrics._cols) * metrics._btnW + kGroupPadX * 2.0f;
        return  metrics;
    }

    /// <summary>
    /// 把文本按 UTF-8 字符边界均分成两行(大按钮最多显示两行)
    /// </summary>
    /// <param name="text">原文本</param>
    /// <param name="first">第一行</param>
    /// <param name="second">第二行</param>
    void    splitTwoLines(const std::string& text,std::string& first,std::string& second)
    {
        std::vector<size_t> offsets;
        offsets.reserve(text.size());
        for (size_t i = 0; i < text.size(); ++i)
        {
            if ((static_cast<unsigned char>(text[i]) & 0xC0) != 0x80)
                offsets.push_back(i);
        }
        if (offsets.size() <= 1)
        {
            first   =   text;
            return;
        }
        const   size_t  mid =   offsets.size() / 2;
        first   =   text.substr(0,offsets[mid]);
        second  =   text.substr(offsets[mid]);
    }

    /// <summary>
    /// 动作按钮:hover 有底色,图标在上、文字居中在下方(过长折两行);
    /// 图标来自 .ui,未登记的用默认字形
    /// </summary>
    /// <param name="action">动作(提供文本、图标与快捷键)</param>
    /// <param name="pos">按钮左上角(屏幕坐标)</param>
    /// <param name="size">按钮尺寸</param>
    /// <param name="iconFont">Font Awesome 图标字体;为空时不画图标</param>
    /// <returns>true 表示本帧被点击</returns>
    bool    ribbonActionButton(const FEUiMenuItem* action,const ImVec2& pos,const ImVec2& size,
                               ImFont* iconFont)
    {
        /// <summary>
        /// 用动作名做 ImGui ID,菜单里动作名唯一
        /// </summary>
        ImGui::SetCursorScreenPos(pos);
        const   bool    pressed =   ImGui::InvisibleButton(action->_name.c_str(),size);
        const   bool    hovered =   ImGui::IsItemHovered();
        ImDrawList*     dl      =   ImGui::GetWindowDrawList();
        const   ImVec2  end     =   ImVec2(pos.x + size.x,pos.y + size.y);
        /// <summary>
        /// 可勾选动作的选中态:主题色底与描边(如移动/旋转/缩放手柄开关)
        /// </summary>
        if (action->_checkable && action->_checked)
        {
            dl->AddRectFilled(pos,end,kRibbonCheckedFill,kRibbonRounding);
            dl->AddRect(pos,end,kAccent,kRibbonRounding);
        }
        if (hovered)
        {
            dl->AddRectFilled(pos,end,kRibbonHoverFill,kRibbonRounding);
            dl->AddRect(pos,end,kRibbonHoverBorder,kRibbonRounding);
        }
        const   ImU32   fg      =   !action->_enabled ? kRibbonTextDisabled
                                    : (hovered ? IM_COL32(255,255,255,255) : kTextColor);
        /// <summary>
        /// 图标通路:所有动作都画图标,.ui 未登记时回退默认字形
        /// </summary>
        char    glyph[5]    =   {};
        if (iconFont != nullptr)
            codepointToUtf8(iconCodepoint(action->_icon),glyph);
        std::string     first   =   action->_text;
        std::string     second;
        if (ImGui::CalcTextSize(action->_text.c_str()).x > size.x - kRibbonWrapPad)
            splitTwoLines(action->_text,first,second);
        const   float   lineH   =   ImGui::GetTextLineHeight();
        const   float   textH   =   second.empty() ? lineH : lineH * 2.0f;
        const   bool    hasIcon =   (glyph[0] != '\0');
        const   float   gap     =   hasIcon ? kIconTextGap : 0.0f;
        const   float   iconH   =   hasIcon ? kIconFontSize : 0.0f;
        float           ty      =   pos.y + (size.y - (iconH + gap + textH)) * 0.5f;
        /// <summary>
        /// 图标与文字均水平居中;文字过长时从左侧起画,由裁剪矩形收边
        /// </summary>
        dl->PushClipRect(pos,end,true);
        if (hasIcon)
        {
            const   ImVec2  glyphSize   =   iconFont->CalcTextSizeA(kIconFontSize,FLT_MAX,0.0f,glyph);
            dl->AddText(iconFont,kIconFontSize,
                        ImVec2(pos.x + (size.x - glyphSize.x) * 0.5f,ty),fg,glyph);
            ty  +=  kIconFontSize + gap;
        }
        const   ImVec2  firstSize   =   ImGui::CalcTextSize(first.c_str());
        dl->AddText(ImVec2(std::max(pos.x + kRibbonTextMinX,pos.x + (size.x - firstSize.x) * 0.5f),ty),
                    fg,first.c_str());
        if (!second.empty())
        {
            const   ImVec2  secondSize  =   ImGui::CalcTextSize(second.c_str());
            dl->AddText(ImVec2(std::max(pos.x + kRibbonTextMinX,pos.x + (size.x - secondSize.x) * 0.5f),ty + lineH),
                        fg,second.c_str());
        }
        dl->PopClipRect();
        if (hovered && !action->_shortcut.empty())
            ImGui::SetTooltip("%s  (%s)",action->_text.c_str(),action->_shortcut.c_str());
        return  pressed;
    }

    /// <summary>
    /// 递归收集分组内的叶子动作(子菜单更深时直接展平到当前分组)
    /// </summary>
    /// <param name="menu">分组菜单</param>
    /// <param name="actions">输出的动作列表</param>
    void    collectGroupActions(const FEUiMenuItem* menu,RibbonActions& actions)
    {
        for (const auto* child : menu->_children)
        {
            if (child->_type == kUiAction)
                actions.push_back(child);
            else if (child->_type == kUiMenu)
                collectGroupActions(child,actions);
        }
    }

    /// <summary>
    /// 绘制一个 Ribbon 分组:按钮 + 分组标题 + 右侧分隔竖线
    /// </summary>
    /// <param name="actions">分组内的动作</param>
    /// <param name="title">分组标题</param>
    /// <param name="x">分组左边界(窗口内坐标)</param>
    /// <param name="iconFont">Font Awesome 图标字体</param>
    /// <param name="triggered">本帧被点击的动作(输出);已有值时不被覆盖</param>
    /// <returns>分组占用宽度(含组间间距)</returns>
    float   drawRibbonGroup(const RibbonActions& actions,const char* title,float x,ImFont* iconFont,
                            const FEUiMenuItem** triggered)
    {
        ImDrawList*         dl      =   ImGui::GetWindowDrawList();
        const   ImVec2      wp      =   ImGui::GetWindowPos();
        const   FERibbonMetrics metrics   =   ribbonMetrics(actions);
        int     index   =   0;
        for (const auto* action : actions)
        {
            const   ImVec2  pos     =   ImVec2(wp.x + x + kGroupPadX + float(index) * metrics._btnW,
                                               wp.y + kGroupRowY);
            if (ribbonActionButton(action,pos,ImVec2(metrics._btnW,metrics._btnH),iconFont)
                && triggered != nullptr && *triggered == nullptr)
                *triggered  =   action;
            ++index;
        }
        /// <summary>
        /// 分组标题居中于按钮区下方(标题行占功能区剩余高度)
        /// </summary>
        const   ImVec2  titleSize   =   ImGui::CalcTextSize(title);
        dl->AddText(ImVec2(wp.x + x + (metrics._groupW - titleSize.x) * 0.5f,
                           wp.y + kGroupRowY + kContentH + kGroupTitleGapY),
                    kTextMuted,title);
        /// <summary>
        /// 组间分隔竖线
        /// </summary>
        const   float   lineX   =   wp.x + x + metrics._groupW + kGroupGap * 0.5f;
        dl->AddLine(ImVec2(lineX,wp.y + kGroupRowY),
                    ImVec2(lineX,wp.y + kGroupRowY + kContentH + kGroupTitleGapY + titleSize.y),
                    kRibbonDivider);
        return  metrics._groupW + kGroupGap;
    }
    /// <summary>
    /// 标签页按钮:选中项与功能区同色,并在底部画强调线
    /// </summary>
    /// <param name="label">标签文本</param>
    /// <param name="selected">是否选中</param>
    /// <returns>true 表示本帧被点击</returns>
    bool    ribbonTab(const char* label,bool selected)
    {
        const   ImVec2  textSize    =   ImGui::CalcTextSize(label);
        const   ImVec2  size        =   ImVec2(textSize.x + kTabHPad,kTabHeight);
        const   ImVec2  pos         =   ImGui::GetCursorScreenPos();
        const   bool    pressed     =   ImGui::InvisibleButton(label,size);
        const   bool    hovered     =   ImGui::IsItemHovered();
        ImDrawList*     dl          =   ImGui::GetWindowDrawList();
        if (selected)
        {
            dl->AddRectFilled(pos,ImVec2(pos.x + size.x,pos.y + size.y),kRibbonBg);
            dl->AddRectFilled(ImVec2(pos.x,pos.y + size.y - kTabUnderlineH),
                              ImVec2(pos.x + size.x,pos.y + size.y),kAccent);
        }
        else if (hovered)
        {
            dl->AddRectFilled(pos,ImVec2(pos.x + size.x,pos.y + size.y),kTabHoverFill);
        }
        dl->AddText(ImVec2(pos.x + (size.x - textSize.x) * 0.5f,pos.y + (size.y - textSize.y) * 0.5f),
                    selected ? IM_COL32(255,255,255,255) : kTextColor,label);
        return  pressed;
    }

    /// <summary>
    /// 内容放不下时左右两侧的滚动箭头;已在端点时画灰色且不可点
    /// </summary>
    /// <param name="pos">箭头左上角(屏幕坐标)</param>
    /// <param name="size">箭头边长</param>
    /// <param name="enabled">是否可点</param>
    /// <param name="right">true 画右箭头,false 画左箭头</param>
    /// <returns>true 表示本帧被点击</returns>
    bool    scrollArrow(const ImVec2& pos,float size,bool enabled,bool right)
    {
        ImGui::SetCursorScreenPos(pos);
        const   bool    pressed =   ImGui::InvisibleButton(right ? "##fesr_scroll_right" : "##fesr_scroll_left",
                                                           ImVec2(size,size));
        const   bool    hovered =   ImGui::IsItemHovered();
        ImDrawList*     dl      =   ImGui::GetWindowDrawList();
        const   ImVec2  end     =   ImVec2(pos.x + size,pos.y + size);
        if (hovered && enabled)
        {
            dl->AddRectFilled(pos,end,kRibbonHoverFill,kRibbonRounding);
            dl->AddRect(pos,end,kRibbonHoverBorder,kRibbonRounding);
        }
        const   ImU32   color   =   enabled ? (hovered ? IM_COL32(255,255,255,255) : kTextColor)
                                            : kArrowDisabled;
        const   float   cx      =   pos.x + size * 0.5f;
        const   float   cy      =   pos.y + size * 0.5f;
        const   float   dx      =   right ? kArrowDx : -kArrowDx;
        dl->AddLine(ImVec2(cx - dx,cy - kArrowDy),ImVec2(cx + dx,cy),color,kArrowThickness);
        dl->AddLine(ImVec2(cx + dx,cy),ImVec2(cx - dx,cy + kArrowDy),color,kArrowThickness);
        return  pressed && enabled;
    }
}
}
}
}
