/// <summary>
/// FESceneEditor 主界面实现:
///   - ImGui 上下文 / 渲染后端 / 字体图集 / Overlay 工厂的一次性挂接;
///   - 以 bin/data/ui/MainWindow.ui 作为界面定义,Ribbon 功能区由其中的
///     菜单栏生成:主菜单为 Page,子菜单为分组,动作(QAction)为按钮;
///   - 鼠标消息转发,功能区被操作时宿主跳过场景处理。
/// </summary>

#include    "FEEditorUi.hpp"

#include    "FEEditorUndoCmds.hpp"
#include    "FEPropertyView.hpp"
#include    "FESettingPropertyModel.hpp"

#include    "../../../inc/FEFileFormatHelper.hpp"
#include    "../../../inc/FEImage.h"
#include    "../../../inc/FEMessage.hpp"
#include    "../../../inc/animation/FEAnimationSys.hpp"
#include    "../../core/FEImageHelper.hpp"
#include    "../../../inc/FEApp.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/node/FENode.hpp"
#include    "../../../inc/FEInputSystem.hpp"
#include    "../../../inc/axis/FENodeMoveEditor.h"
#include    "../../../inc/axis/FENodeRotateEditor.h"
#include    "../../../inc/axis/FENodeScaleEditor.h"
#include    "../../../inc/material/FEMaterialPBR.hpp"
#include    "../../../inc/geometry/FEGeometryBox.hpp"
#include    "../../../inc/geometry/FEGeometrySphere.hpp"
#include    "../../../inc/geometry/FEGeometryCylinder.hpp"
#include    "../../../inc/geometry/FEGeometryPyramid.hpp"
#include    "../../../inc/geometry/FEGeometryTorus.hpp"
#include    "../../../inc/geometry/FEGeometryTorusKnot.hpp"
#include    "../../../inc/geometry/FEGeometryPRCP.hpp"
#include    "../../../inc/geometry/FEGeometryPRCN.hpp"
#include    "../../../inc/geometry/FEGeometryPRPY.hpp"
#include    "../../../inc/geometry/FEGeometryPRRT.hpp"
#include    "../../../inc/geometry/FEGeometryPRDH.hpp"

#include    <algorithm>
#include    <cctype>
#include    <chrono>
#include    <cmath>
#include    <cstdio>
#include    <cstdlib>
#include    <cstring>
#include    <fstream>
#include    <imgui.h>
#include    <iostream>
#include    <map>
#include    <sstream>
#include    <string>
#include    <vector>

namespace
{
    /// <summary>
    /// Ribbon 外观常量(暗色主题)
    /// </summary>
    const   ImU32   kAccent     =   IM_COL32(74,144,226,255);
    const   ImU32   kIconColor  =   IM_COL32(205,210,220,255);
    const   ImU32   kTextColor  =   IM_COL32(214,219,229,255);
    const   ImU32   kTextMuted  =   IM_COL32(140,146,158,255);
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
    /// 功能区总高(分组内容 64 + 标题行,留出上下边距)
    /// </summary>
    const   float   kRibbonH    =   128.0f;
    /// <summary>
    /// 内容放不下时,左右滚动箭头的宽度
    /// </summary>
    const   float   kArrowW     =   18.0f;
    /// <summary>
    /// 点击滚动箭头时每次滚动的像素数
    /// </summary>
    const   float   kScrollStep =   160.0f;

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
    /// .ui 里未登记的图标统一回退到该字形(shapes)
    /// </summary>
    const   unsigned int    kIconDefault    =   0xF61F;

    /// <summary>
    /// 图标文件名(不含目录与扩展名)到 Font Awesome 码位的映射。
    /// 键取自 bin/data/ui/res 下的图标文件名,新增图标时在此登记即可
    /// </summary>
    struct  FEIconGlyph
    {
        /// <summary>
        /// 图标文件名(不含 .png),对应 .ui 里 :/EditorRes/res/&lt;name&gt;.png
        /// </summary>
        const   char*           _name;
        /// <summary>
        /// Font Awesome 私有区码位
        /// </summary>
        unsigned int            _code;
    };

    const   FEIconGlyph     kIconGlyphs[]   =
    {
        { "about",              0xF05A },   ///< info-circle
        { "align_back",         0xF070 },   ///< eye-slash
        { "align_bottom",       0xF063 },   ///< arrow-down
        { "align_front",        0xF06E },   ///< eye
        { "align_left",         0xF036 },   ///< align-left
        { "align_right",        0xF038 },   ///< align-right
        { "align_rotate",       0xF2F1 },   ///< sync-alt
        { "align_scale",        0xF31E },   ///< expand-arrows-alt
        { "align_top",          0xF062 },   ///< arrow-up
        { "align_x",            0xF337 },   ///< arrows-alt-h
        { "align_y",            0xF338 },   ///< arrows-alt-v
        { "align_z",            0xF14E },   ///< compass
        { "bbox_center",        0xF140 },   ///< bullseye
        { "cone_frustum",       0xF1CE },   ///< circle-notch
        { "cover.reset",        0xF2EA },   ///< undo-alt
        { "cube",               0xF1B2 },   ///< cube
        { "cylinder",           0xF1C0 },   ///< database
        { "hyperboloid",        0xF5FD },   ///< layer-group
        { "ico_FirstFrame",     0xF048 },   ///< step-backward
        { "ico_LastFrame",      0xF051 },   ///< step-forward
        { "ico_Move",           0xF0B2 },   ///< arrows-alt
        { "ico_NextFrame",      0xF04E },   ///< forward
        { "ico_Pause",          0xF04C },   ///< pause
        { "ico_PreFrame",       0xF04A },   ///< backward
        { "ico_Redo",           0xF01E },   ///< redo
        { "ico_Rotate",         0xF021 },   ///< refresh
        { "ico_RunBack",        0xF049 },   ///< fast-backward
        { "ico_RunFront",       0xF050 },   ///< fast-forward
        { "ico_Scale",          0xF065 },   ///< expand
        { "ico_Undo",           0xF0E2 },   ///< undo
        { "mark",               0xF3C5 },   ///< map-marker-alt
        { "maximum",            0xF102 },   ///< angle-double-up
        { "minimum",            0xF103 },   ///< angle-double-down
        { "model_load",         0xF56F },   ///< file-import
        { "node_close",         0xF00D },   ///< times
        { "pyramid_frustum",    0xF61F },   ///< shapes
        { "scene_open",         0xF07C },   ///< folder-open
        { "scene_save",         0xF0C7 },   ///< save
        { "sphere",             0xF111 },   ///< circle
        { "spherical_cap",      0xF192 },   ///< dot-circle
        { "square_pyramid",     0xF0D8 },   ///< caret-up
        { "torus",              0xF1CE },   ///< circle-notch
        { "torus_knot",         0xF126 },   ///< code-branch
        { "torus_sector",       0xF200 },   ///< chart-pie
        { "torus_solid",        0xF111 },   ///< circle
    };

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
        metrics._btnW   =   std::min(std::max(kMinButtonW,std::max(textW,kIconFontSize) + 12.0f),kButtonMaxW);
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
            dl->AddRectFilled(pos,end,IM_COL32(74,144,226,70),3.0f);
            dl->AddRect(pos,end,kAccent,3.0f);
        }
        if (hovered)
        {
            dl->AddRectFilled(pos,end,IM_COL32(255,255,255,20),3.0f);
            dl->AddRect(pos,end,IM_COL32(255,255,255,40),3.0f);
        }
        const   ImU32   fg      =   !action->_enabled ? IM_COL32(120,124,134,255)
                                    : (hovered ? IM_COL32(255,255,255,255) : kTextColor);
        /// <summary>
        /// 图标通路:所有动作都画图标,.ui 未登记时回退默认字形
        /// </summary>
        char    glyph[5]    =   {};
        if (iconFont != nullptr)
            codepointToUtf8(iconCodepoint(action->_icon),glyph);
        std::string     first   =   action->_text;
        std::string     second;
        if (ImGui::CalcTextSize(action->_text.c_str()).x > size.x - 8.0f)
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
        dl->AddText(ImVec2(std::max(pos.x + 2.0f,pos.x + (size.x - firstSize.x) * 0.5f),ty),
                    fg,first.c_str());
        if (!second.empty())
        {
            const   ImVec2  secondSize  =   ImGui::CalcTextSize(second.c_str());
            dl->AddText(ImVec2(std::max(pos.x + 2.0f,pos.x + (size.x - secondSize.x) * 0.5f),ty + lineH),
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
        dl->AddText(ImVec2(wp.x + x + (metrics._groupW - titleSize.x) * 0.5f,wp.y + kGroupRowY + kContentH + 3.0f),
                    kTextMuted,title);
        /// <summary>
        /// 组间分隔竖线
        /// </summary>
        const   float   lineX   =   wp.x + x + metrics._groupW + kGroupGap * 0.5f;
        dl->AddLine(ImVec2(lineX,wp.y + kGroupRowY),
                    ImVec2(lineX,wp.y + kGroupRowY + kContentH + 3.0f + titleSize.y),IM_COL32(255,255,255,26));
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
        const   ImVec2  size        =   ImVec2(textSize.x + 26.0f,28.0f);
        const   ImVec2  pos         =   ImGui::GetCursorScreenPos();
        const   bool    pressed     =   ImGui::InvisibleButton(label,size);
        const   bool    hovered     =   ImGui::IsItemHovered();
        ImDrawList*     dl          =   ImGui::GetWindowDrawList();
        if (selected)
        {
            dl->AddRectFilled(pos,ImVec2(pos.x + size.x,pos.y + size.y),kRibbonBg);
            dl->AddRectFilled(ImVec2(pos.x,pos.y + size.y - 2.0f),ImVec2(pos.x + size.x,pos.y + size.y),kAccent);
        }
        else if (hovered)
        {
            dl->AddRectFilled(pos,ImVec2(pos.x + size.x,pos.y + size.y),IM_COL32(255,255,255,18));
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
            dl->AddRectFilled(pos,end,IM_COL32(255,255,255,20),3.0f);
            dl->AddRect(pos,end,IM_COL32(255,255,255,40),3.0f);
        }
        const   ImU32   color   =   enabled ? (hovered ? IM_COL32(255,255,255,255) : kTextColor)
                                            : IM_COL32(92,96,106,255);
        const   float   cx      =   pos.x + size * 0.5f;
        const   float   cy      =   pos.y + size * 0.5f;
        const   float   dx      =   right ? 3.5f : -3.5f;
        dl->AddLine(ImVec2(cx - dx,cy - 4.5f),ImVec2(cx + dx,cy),color,1.6f);
        dl->AddLine(ImVec2(cx + dx,cy),ImVec2(cx - dx,cy + 4.5f),color,1.6f);
        return  pressed && enabled;
    }
}

namespace   FE
{
    FEEditorImGuiFactory::FEEditorImGuiFactory(FEContext& ctx,FEImGuiBackend* backend)
        :FEFactoryRender(ctx)
        ,_backend(backend)
    {
        /// <summary>
        /// 在最后一层(Overlay)绘制:场景工厂默认归属 USAGE_Scene,本工厂放到
        /// 其后的 Overlay 层,保证菜单绝对最后绘制、不被场景覆盖
        /// </summary>
        _usages =   ViewerUsage::USAGE_Overlay;
        priority(FEFactory::PT_Render).setPriority(EP_Last).setOrder(int16_t(32767));
        priority(FEFactory::PT_Update).setPriority(EP_Last).setOrder(int16_t(32767));
        /// <summary>
        /// 常驻工厂:场景 clear 时不销毁
        /// </summary>
        setResident(true);
    }

    void    FEEditorImGuiFactory::update(CMDPtr cmd)
    {
        UNUSED(cmd);
        if (_backend == nullptr)
            return;
        ImGuiIO&    io  =   ImGui::GetIO();
        io.DisplaySize  =   ImVec2(float(_ctx.windowsWidth()),float(_ctx.windowsHeight()));
        io.DeltaTime    =   float(_ctx.deltaTime());
        ImGui::NewFrame();
        if (_buildUI)
            _buildUI();
        ImGui::Render();
        extractDrawData();
    }

    void    FEEditorImGuiFactory::render(CMDPtr cmd)
    {
        if (_backend == nullptr || cmd == nullptr)
            return;
        _backend->render(_drawData,cmd.get());
    }

    void    FEEditorImGuiFactory::extractDrawData()
    {
        ImDrawData* dd  =   ImGui::GetDrawData();
        _cmds.clear();
        _vtx.clear();
        _idx.clear();
        if (dd == nullptr || dd->CmdListsCount == 0)
        {
            _drawData   =   FEImGuiDrawData();
            return;
        }

        _vtx.reserve(size_t(dd->TotalVtxCount) * 20);
        _idx.reserve(size_t(dd->TotalIdxCount) * sizeof(ImDrawIdx));
        _cmds.reserve(size_t(dd->CmdListsCount) * 4);

        uint32  vtxBase =   0;
        uint32  idxBase =   0;
        for (int n = 0; n < dd->CmdListsCount; ++n)
        {
            const ImDrawList*   list    =   dd->CmdLists[n];
            /// <summary>
            /// ImGui 1.65:ImDrawCmd 无 VtxOffset/IdxOffset 字段,同一 list 内各命令
            /// 按 ElemCount 顺序消费该 list 的索引,故需自行累加命令内的索引偏移
            /// </summary>
            uint32  runningIdx  =   0;
            for (int c = 0; c < list->CmdBuffer.Size; ++c)
            {
                const ImDrawCmd&    pcmd    =   list->CmdBuffer[c];
                if (pcmd.ElemCount == 0)
                    continue;
                FEImGuiDrawCmd  out;
                out._indexCount  =   pcmd.ElemCount;
                out._indexOffset =   idxBase + runningIdx;
                out._vertexOffset=   int32(vtxBase);
                out._clipX       =   pcmd.ClipRect.x;
                out._clipY       =   pcmd.ClipRect.y;
                out._clipW       =   pcmd.ClipRect.z;
                out._clipH       =   pcmd.ClipRect.w;
                _cmds.push_back(out);
                runningIdx  +=  pcmd.ElemCount;
            }
            const char* vData   =   (const char*)list->VtxBuffer.Data;
            _vtx.insert(_vtx.end(),vData,vData + size_t(list->VtxBuffer.Size) * 20);
            const char* iData   =   (const char*)list->IdxBuffer.Data;
            _idx.insert(_idx.end(),iData,iData + size_t(list->IdxBuffer.Size) * sizeof(ImDrawIdx));
            vtxBase +=  uint32(list->VtxBuffer.Size);
            idxBase +=  uint32(list->IdxBuffer.Size);
        }

        _drawData._vertexData   =   _vtx.empty()  ? nullptr : _vtx.data();
        _drawData._vertexCount  =   uint32(_vtx.size() / 20);
        _drawData._indexData    =   _idx.empty()  ? nullptr : _idx.data();
        _drawData._indexCount   =   uint32(_idx.size() / sizeof(ImDrawIdx));
        _drawData._cmds         =   _cmds.empty() ? nullptr : _cmds.data();
        _drawData._cmdCount     =   uint32(_cmds.size());
        _drawData._displayPosX  =   dd->DisplayPos.x;
        _drawData._displayPosY  =   dd->DisplayPos.y;
        _drawData._displaySizeX =   dd->DisplaySize.x;
        _drawData._displaySizeY =   dd->DisplaySize.y;
        /// <summary>
        /// ImGui 1.65 的 ImDrawData 无 FramebufferScale 字段,直接按 1:1 渲染
        /// </summary>
        _drawData._scaleX       =   1.0f;
        _drawData._scaleY       =   1.0f;
    }

    bool    FEEditorUi::attach(FEContext& ctx,FEScene* scene)
    {
        detach();
        if (scene == nullptr)
            return  false;

        ImGui::CreateContext();
        _hasContext =   true;
        ImGui::StyleColorsDark();
        ImGuiIO&    io  =   ImGui::GetIO();
        /// <summary>
        /// 不落盘 imgui.ini,避免编辑器产物污染工作目录
        /// </summary>
        io.IniFilename  =   nullptr;
        /// <summary>
        /// 键盘映射:让文本输入框等控件能读取按键状态
        /// </summary>
        setupKeyMap(io);

        /// <summary>
        /// 输入法候选窗口按输入框位置显示:ImGui 默认 IME 处理需要原生窗口句柄
        /// </summary>
        if (FEWindow* window = ctx.window())
        {
            if (FEApp* app = window->as<FEApp>())
                io.ImeWindowHandle  =   app->cInfo()._window;
        }

        _backend    =   scene->device()->createImGuiBackend();
        if (_backend == nullptr || !_backend->init(scene->swapchainColorFormat()))
        {
            std::cerr << "create ImGui backend failed" << std::endl;
            detach();
            return  false;
        }

        /// <summary>
        /// 上传字体图集(RGBA8);TexID 仅作占位,后端始终绑定该纹理。
        /// 先加载宋体作为默认字体以支持中文,失败时回退 ImGui 内置字体
        /// </summary>
        ImFont* cnFont  =   io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\simsun.ttc",16.0f,nullptr,
                                                          io.Fonts->GetGlyphRangesChineseFull());
        if (cnFont != nullptr)
            io.FontDefault  =   cnFont;

        /// <summary>
        /// Ribbon 图标:Font Awesome 实心图标字体,只加载 kIconGlyphs 里用到的码位,
        /// 避免字体图集被整套图标(上千个字形)撑大
        /// </summary>
        {
            static  const   ImWchar kIconRanges[]   =
                {
                    0xF00D,0xF00D,  0xF01E,0xF021,  0xF031,0xF031,  0xF036,0xF038,
                    0xF048,0xF04C,  0xF04E,0xF04E,  0xF050,0xF051,  0xF05A,0xF05A,
                    0xF062,0xF063,  0xF065,0xF067,  0xF06E,0xF06E,  0xF070,0xF070,
                    0xF07C,0xF07C,  0xF0B2,0xF0B2,  0xF0C7,0xF0C7,  0xF0D8,0xF0D8,
                    0xF0E2,0xF0E2,  0xF102,0xF103,  0xF111,0xF111,  0xF126,0xF126,
                    0xF140,0xF140,  0xF14E,0xF14E,  0xF192,0xF192,  0xF1B2,0xF1B2,
                    0xF1C0,0xF1C0,  0xF1CE,0xF1CE,  0xF200,0xF200,  0xF2EA,0xF2EA,
                    0xF2F1,0xF2F1,  0xF31E,0xF31E,  0xF337,0xF338,  0xF3C5,0xF3C5,
                    0xF52C,0xF52C,  0xF56F,0xF56F,  0xF5FD,0xF5FD,  0xF61F,0xF61F,  0
                };
            ImFontConfig    iconCfg =   {};
            iconCfg.PixelSnapH      =   true;
            const   String  iconPath=   ctx.resourcePath() + "/data/fonts/fa-solid-900.ttf";
            _iconFont   =   io.Fonts->AddFontFromFileTTF(iconPath.c_str(),kIconFontSize,&iconCfg,kIconRanges);
            if (_iconFont == nullptr)
                std::cerr << "load font awesome failed: " << iconPath << std::endl;
        }

        unsigned char*  pixels  =   nullptr;
        int             texW    =   0;
        int             texH    =   0;
        io.Fonts->GetTexDataAsRGBA32(&pixels,&texW,&texH);
        FEImGuiFontData font;
        font._pixels    =   pixels;
        font._width     =   uint32(texW);
        font._height    =   uint32(texH);
        _backend->uploadFont(font);
        io.Fonts->TexID =   (void*)1;

        _scene      =   scene;
        _ctx        =   &ctx;
        _showGrid   =   true;
        _activeTab  =   0;
        _guiDrag[0] =   _guiDrag[1] = _guiDrag[2] = false;
        /// <summary>
        /// 工程相关状态:文件对话框默认从程序目录开始(与 Qt 版一致)
        /// </summary>
        _projectName.clear();
        _statusText.clear();
        _pendingActions.clear();
        _startDir   =   toUtf8Path(ctx.workPath());
        /// <summary>
        /// 预加载 .ui 预览文档(bin/data/ui/MainWindow.ui)
        /// </summary>
        {
            const   String  uiPath  =   ctx.resourcePath() + "/data/ui/MainWindow.ui";
            if (!_uiDoc.load(uiPath))
            {
                _uiError    =   _uiDoc.error();
                std::cerr << "load ui failed: " << _uiError << std::endl;
            }
        }
        /// <summary>
        /// 快捷键表:命令来自 .ui,绑定来自 exe 同名 XML(不存在时用默认值)
        /// </summary>
        reloadShortcuts();
        /// <summary>
        /// 首次运行(设置树里还没有快捷键节点):把默认绑定写入设置并落盘,
        /// 生成 exe 同目录的 FESceneEditor.xml,避免配置窗口一直是空的
        /// </summary>
        if (_ctx->setting().find("shortcut") == nullptr)
        {
            _shortcuts.save(_ctx->setting());
            _ctx->setting().save(FEShortcutConfig::exeXmlPath().c_str());
        }
        _shortcutOpen       =   false;
        _shortcutDirty      =   false;
        _shortcutSelected   =   -1;
        _shortcutRecord     =   -1;
        _shortcutError.clear();
        /// <summary>
        /// 渲染设置:天空盒开关默认开启;读取后立刻同步到场景,
        /// 关闭时天空不绘制且 PBR 的解析天空 IBL 一并停止计入
        /// </summary>
        _skyShown   =   _ctx->setting().get("render").get("showSkyBox").setIfUnset(true).getOr(true);
        scene->showSky(_skyShown);
        _factory    =   new FEEditorImGuiFactory(ctx,_backend);
        _factory->_buildUI  =   [this]() {   this->drawRibbon();  };
        scene->factoryMgr().addObject(_factory);
        /// <summary>
        /// 时间线面板的数据来源:场景动画系统 / 撤销栈 / 模型树选中项
        /// </summary>
        _timeLine.bind(scene,&_undoStack,&_modelTree);
        /// <summary>
        /// 模型树的层级/顺序调整入撤销栈,支持撤销/重做
        /// </summary>
        _modelTree.setUndoStack(&_undoStack);
        /// <summary>
        /// 模型树右键菜单增删节点/动画/约束需要访问场景的节点树与各组件系统
        /// </summary>
        _modelTree.setScene(scene);
        /// <summary>
        /// 时间线窗口默认集成显示在编辑器底部
        /// </summary>
        _timeLine.setOpen(true);
        _attached   =   true;
        return  true;
    }

    void    FEEditorUi::detach()
    {
        /// <summary>
        /// 先断开回调与后端链接,避免工厂在后续帧继续访问已释放对象
        /// </summary>
        _modelTree.unbind();
        _modelTree.setScene(nullptr);
        _timeLine.unbind();
        if (_factory != nullptr)
        {
            _factory->_buildUI  =   nullptr;
            _factory->invalidateBackend();
        }
        _factory    =   nullptr;
        /// <summary>
        /// 在场景销毁之前释放命令持有的节点引用,避免析构晚于场景
        /// </summary>
        _undoStack.clear();
        _scene      =   nullptr;
        _ctx        =   nullptr;
        _iconFont   =   nullptr;
        _fileDialog.close();
        /// <summary>
        /// 动作索引指向 _uiDoc 里的菜单项,必须在 clear 之前丢弃
        /// </summary>
        _actionById.clear();
        _shortcutOpen   =   false;
        _shortcutRecord =   -1;
        _shortcutError.clear();
        _settingOpen    =   false;
        _uiDoc.clear();
        _uiError.clear();
        _uiPreviewOpen  =   false;
        if (_backend != nullptr)
        {
            _backend->destroy();
            delete  _backend;
            _backend    =   nullptr;
        }
        if (_hasContext)
        {
            ImGui::DestroyContext();
            _hasContext =   false;
        }
        _attached   =   false;
    }

    void    FEEditorUi::processShortcuts()
    {
        ImGuiIO&    io  =   ImGui::GetIO();
        /// <summary>
        /// 文本输入框获得焦点时按键属于输入(InputText 内部已有自己的撤销);
        /// 设置窗口打开或正在录制时,按键属于设置操作
        /// </summary>
        if (io.WantTextInput || _scene == nullptr)
            return;
        if (_shortcutOpen || _shortcutRecord >= 0)
            return;

        /// <summary>
        /// 按键用 Windows 虚拟键码作 io.KeysDown 的下标,与快捷键表里保存的 _key 一致。
        /// 修饰键必须与绑定完全相等(避免 Ctrl+S 被裸 S 触发)
        /// </summary>
        for (const FEShortcutItem& item : _shortcuts._items)
        {
            if (item._key <= 0 || item._key >= 512)
                continue;
            if (item._ctrl != io.KeyCtrl || item._shift != io.KeyShift || item._alt != io.KeyAlt)
                continue;
            if (!ImGui::IsKeyPressed(item._key,false))
                continue;
            triggerAction(item._id);
            break;
        }
    }

    void    FEEditorUi::reloadShortcuts()
    {
        _actionById.clear();
        collectActionMap(_uiDoc.menuBar(),_actionById);
        _shortcuts.build(_uiDoc,nullptr);
        /// 设置已在启动时加载,这里只从设置树取快捷键
        _shortcuts.load(_ctx->setting());
        _shortcutDirty  =   false;
    }

    void    FEEditorUi::discardShortcutChanges()
    {
        reloadShortcuts();
        _shortcutRecord =   -1;
        _shortcutError.clear();
    }

    void    FEEditorUi::triggerAction(const std::string& name)
    {
        auto    it  =   _actionById.find(name);
        if (it == _actionById.end() || it->second == nullptr)
            return;
        onAction(it->second);
    }

    namespace
    {
        /// <summary>
        /// 脚本自动化计时基准(steady_clock,单位毫秒)
        /// </summary>
        double  scriptClockMs()
        {
            static  const   auto    base    =   std::chrono::steady_clock::now();
            return  std::chrono::duration<double,std::milli>(
                        std::chrono::steady_clock::now() - base).count();
        }
        /// <summary>
        /// 去掉首尾空白
        /// </summary>
        std::string trimText(const std::string& text)
        {
            const   size_t  begin   =   text.find_first_not_of(" \t\r\n");
            if (begin == std::string::npos)
                return  std::string();
            const   size_t  end     =   text.find_last_not_of(" \t\r\n");
            return  text.substr(begin,end - begin + 1);
        }
        /// <summary>
        /// 属性名 → 标量属性索引(仅支持平移/缩放分量)
        /// </summary>
        int     scalarPropOf(const std::string& key)
        {
            if (key == "x")     return  PROP_TRANSFORM_X;
            if (key == "y")     return  PROP_TRANSFORM_Y;
            if (key == "z")     return  PROP_TRANSFORM_Z;
            if (key == "sx")    return  PROP_SCALE_X;
            if (key == "sy")    return  PROP_SCALE_Y;
            if (key == "sz")    return  PROP_SCALE_Z;
            return  -1;
        }
    }

    bool    FEEditorUi::loadScript(const std::string& path)
    {
        std::ifstream   file(std::filesystem::u8path(path),std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "load script failed: " << path << std::endl;
            return  false;
        }
        _scriptCommands.clear();
        std::string line;
        bool        first   =   true;
        while (std::getline(file,line))
        {
            /// 首行去掉 UTF-8 BOM,避免第一条命令带不可见前缀
            if (first && line.size() >= 3 &&
                (unsigned char)line[0] == 0xEF && (unsigned char)line[1] == 0xBB &&
                (unsigned char)line[2] == 0xBF)
                line.erase(0,3);
            first   =   false;
            /// '#' 之后为注释;空行直接跳过
            const   size_t  sharp   =   line.find('#');
            if (sharp != std::string::npos)
                line.erase(sharp);
            line    =   trimText(line);
            if (!line.empty())
                _scriptCommands.push_back(line);
        }
        _scriptPath     =   path;
        _scriptIndex    =   0;
        _scriptWaitUntil=   0.0;
        _scriptFailed   =   false;
        _scriptReport.clear();
        _shotPath.clear();
        _shotDeadline   =   0.0;
        _scriptDone     =   _scriptCommands.empty();
        std::cout << "script loaded: " << path << " (" << _scriptCommands.size()
                  << " commands)" << std::endl;
        return  true;
    }

    void    FEEditorUi::scriptLog(const std::string& text)
    {
        std::cout << "[script] " << text << std::endl;
        _scriptReport.push_back(text);
    }

    void    FEEditorUi::runScript()
    {
        if (_scriptDone)
            return;
        /// 截帧未落盘前不推进后续命令,保证画面状态与脚本顺序一致
        if (!_shotPath.empty())
        {
            finishShot();
            return;
        }
        /// wait 未到期时保持挂起,不消耗命令
        if (scriptClockMs() < _scriptWaitUntil)
            return;
        if (_scriptIndex < _scriptCommands.size())
        {
            const   std::string line    =   _scriptCommands[_scriptIndex++];
            execScriptCommand(line);
            /// 每条命令占一帧,给渲染与动画系统留出传导时间
            return;
        }
        /// 全部执行完毕:写汇总日志并自动关闭程序(便于 CI 判定)
        finishScript();
    }

    void    FEEditorUi::finishScript()
    {
        if (_scriptDone)
            return;
        _scriptDone =   true;
        {
            std::ofstream   log(std::filesystem::u8path(_scriptPath + ".log"),std::ios::binary);
            if (log.is_open())
            {
                for (auto& var : _scriptReport)
                    log << var << '\n';
                log << (_scriptFailed ? "RESULT: FAIL" : "RESULT: PASS") << '\n';
            }
        }
        std::cout << "script finished: " << (_scriptFailed ? "FAIL" : "PASS") << std::endl;
        if (_onQuit)
            _onQuit();
    }

    void    FEEditorUi::execScriptCommand(const std::string& line)
    {
        std::istringstream  in(line);
        std::string         cmd;
        in  >>  cmd;
        if (cmd.empty())
            return;

        if (cmd == "wait")
        {
            int ms  =   0;
            in  >>  ms;
            _scriptWaitUntil    =   scriptClockMs() + (ms > 0 ? ms : 0);
            return;
        }
        if (cmd == "log")
        {
            std::string text;
            std::getline(in,text);
            scriptLog(trimText(text));
            return;
        }
        if (cmd == "action")
        {
            std::string name;
            in  >>  name;
            if (_actionById.find(name) == _actionById.end())
            {
                _scriptFailed   =   true;
                scriptLog("FAIL action not found: " + name);
                return;
            }
            triggerAction(name);
            scriptLog("action " + name);
            return;
        }
        if (cmd == "select")
        {
            std::string name;
            in  >>  name;
            if (!_modelTree.selectByName(name))
            {
                _scriptFailed   =   true;
                scriptLog("FAIL select: node not found: " + name);
                return;
            }
            scriptLog("select " + name);
            return;
        }
        if (cmd == "time")
        {
            double  sec =   0.0;
            in  >>  sec;
            const   int frame   =   int(std::lround(sec * double(_timeLine.fps())));
            _timeLine.setCurFrame(frame,true);
            scriptLog("time " + std::to_string(sec) + "s (frame " + std::to_string(frame) + ")");
            return;
        }
        if (cmd == "setprop")
        {
            std::string nodeName;
            std::string propKey;
            in  >>  nodeName >> propKey;
            if (!_modelTree.selectByName(nodeName))
            {
                _scriptFailed   =   true;
                scriptLog("FAIL setprop: node not found: " + nodeName);
                return;
            }
            Object  node    =   _modelTree.selected();
            if (node == nullptr || _scene == nullptr)
            {
                _scriptFailed   =   true;
                scriptLog("FAIL setprop: no selected node");
                return;
            }
            KFValue     value   =   {};
            if (propKey == "pos")
            {
                real    x = 0,y = 0,z = 0;
                in  >>  x >> y >> z;
                value   =   real3(x,y,z);
            }
            else
            {
                const   int prop    =   scalarPropOf(propKey);
                if (prop < 0)
                {
                    _scriptFailed   =   true;
                    scriptLog("FAIL setprop: unknown property: " + propKey);
                    return;
                }
                real    v   =   0;
                in  >>  v;
                value   =   v;
            }
            const   int prop    =   (propKey == "pos") ? PROP_TRANSFORM_XYZ : scalarPropOf(propKey);
            node->beginSetProp();
            bool    ok  =   false;
            if (auto* sys = _scene->animationSystem())
                ok      =   sys->setNodeProperty(node,prop,value);
            else
                ok      =   node->setProperty(prop,value);
            node->endSetProp(_ctx != nullptr ? _ctx->deltaTime() : real(0),ok);
            scriptLog("setprop " + nodeName + " " + propKey + " = " + trimText(line.substr(line.find(propKey) + propKey.size())));
            return;
        }
        if (cmd == "check")
        {
            std::string nodeName;
            std::string propKey;
            double      expect  =   0.0;
            double      eps     =   1e-3;
            in  >>  nodeName >> propKey >> expect;
            if (!(in >> eps))
            {
                in.clear();
                eps     =   1e-3;
            }
            if (!_modelTree.selectByName(nodeName))
            {
                _scriptFailed   =   true;
                scriptLog("FAIL check: node not found: " + nodeName);
                return;
            }
            Node    node    =   _modelTree.selected() != nullptr
                                    ? _modelTree.selected()->cast<FENode>()
                                    : Node();
            if (node == nullptr)
            {
                _scriptFailed   =   true;
                scriptLog("FAIL check: no selected node");
                return;
            }
            double  actual  =   0.0;
            bool    known   =   true;
            const   real3   pos =   node->localTranslation();
            const   real3   scl =   real3(node->localScaling());
            if (propKey == "x")         actual = double(pos.x);
            else if (propKey == "y")    actual = double(pos.y);
            else if (propKey == "z")    actual = double(pos.z);
            else if (propKey == "sx")   actual = double(scl.x);
            else if (propKey == "sy")   actual = double(scl.y);
            else if (propKey == "sz")   actual = double(scl.z);
            else                        known = false;
            if (!known)
            {
                _scriptFailed   =   true;
                scriptLog("FAIL check: unknown property: " + propKey);
                return;
            }
            const   bool    pass    =   std::abs(actual - expect) <= eps;
            if (!pass)
                _scriptFailed   =   true;
            std::ostringstream  os;
            os  << (pass ? "PASS" : "FAIL") << " check " << nodeName << " " << propKey
                << " = " << actual << " (expect " << expect << " eps " << eps << ")";
            scriptLog(os.str());
            return;
        }
        if (cmd == "shot")
        {
            std::string path;
            in  >>  path;
            std::filesystem::path   out =   std::filesystem::u8path(path);
            if (out.is_relative())
                out =   std::filesystem::u8path(_scriptPath).parent_path() / out;
            std::filesystem::create_directories(out.parent_path());
            if (_scene == nullptr)
            {
                _scriptFailed   =   true;
                scriptLog("FAIL shot: no scene");
                return;
            }
            /// 窗口模式不能直接回读颜色附件:请求引擎在下一帧开头(acquire 之后、
            /// 记录本帧命令之前)抓一帧,由 runScript 每帧轮询取回后再写文件
            _shotPath       =   out.u8string();
            _shotDeadline   =   scriptClockMs() + 3000.0;
            _scene->requestCapture();
            scriptLog("shot request " + _shotPath);
            return;
        }
        if (cmd == "quit")
        {
            scriptLog("quit");
            finishScript();
            return;
        }
        _scriptFailed   =   true;
        scriptLog("FAIL unknown command: " + line);
    }

    bool    FEEditorUi::finishShot()
    {
        if (_shotPath.empty())
            return  true;
        std::vector<unsigned char>  pixels;
        if (!_scene->takeCapture(pixels))
        {
            /// 抓帧还没发生(等在下一帧 onFrameStart);超时视为失败,避免脚本卡死
            if (scriptClockMs() >= _shotDeadline)
            {
                _scriptFailed   =   true;
                scriptLog("FAIL shot: capture timeout " + _shotPath);
                _shotPath.clear();
                return  true;
            }
            return  false;
        }

        const   uint2   size    =   _scene->captureSize();
        const   size_t  need    =   size_t(size.x) * size_t(size.y) * 4;
        if (_ctx == nullptr || need == 0 || pixels.size() < need)
        {
            _scriptFailed   =   true;
            scriptLog("FAIL shot: bad capture size " + _shotPath);
            _shotPath.clear();
            return  true;
        }

        FEImage image(*_ctx,size.x,size.y,FMT_R8G8B8A8_UNORM);
        auto    plane   =   image.plane(0,0);
        if (plane.data() == nullptr)
        {
            _scriptFailed   =   true;
            scriptLog("FAIL shot: image plane empty " + _shotPath);
            _shotPath.clear();
            return  true;
        }
        const   size_t  pitch   =   size_t(size.x) * 4;
        for (uint y = 0; y < size.y; ++y)
            std::memcpy(plane.rowAt(y),pixels.data() + size_t(y) * pitch,pitch);

        if (FEImageHelper::save(image,_shotPath.c_str()))
            scriptLog("shot " + _shotPath);
        else
        {
            _scriptFailed   =   true;
            scriptLog("FAIL shot: save failed " + _shotPath);
        }
        _shotPath.clear();
        return  true;
    }

    void    FEEditorUi::captureShortcutKeys()
    {
        if (_shortcutRecord < 0 || size_t(_shortcutRecord) >= _shortcuts._items.size())
            return;
        ImGuiIO&    io  =   ImGui::GetIO();
        /// <summary>
        /// Esc 放弃本次录制,保留原绑定
        /// </summary>
        if (ImGui::IsKeyPressed(kVkEscape,false))
        {
            _shortcutRecord =   -1;
            _shortcutError.clear();
            return;
        }
        /// <summary>
        /// Enter 提交候选组合;候选为空时忽略
        /// </summary>
        if (ImGui::IsKeyPressed(kVkEnter,false))
        {
            if (!_recordPressed || _recordKey == 0)
                return;
            const   int other   =   _shortcuts.conflictWith(_recordKey,_recordCtrl,_recordShift,
                                                            _recordAlt,_shortcutRecord);
            if (other >= 0)
            {
                _shortcutError  =   "该快捷键已被占用: " + _shortcuts._items[other]._text;
                return;
            }
            _shortcuts.assign(size_t(_shortcutRecord),_recordKey,_recordCtrl,_recordShift,_recordAlt);
            _shortcutDirty  =   true;
            _shortcutRecord =   -1;
            _shortcutError.clear();
            return;
        }

        /// <summary>
        /// 修饰键状态实时跟随,便于预览组合文本
        /// </summary>
        _recordCtrl     =   io.KeyCtrl;
        _recordShift    =   io.KeyShift;
        _recordAlt      =   io.KeyAlt;

        /// <summary>
        /// 扫描全部虚拟键码,跳开修饰键与 Enter/Esc(它们有固定含义)。
        /// ImGui 1.65 的 ImGuiKey 枚举覆盖不了字母/数字/F 键,只能直接查 KeysDown
        /// </summary>
        for (int vk = 1; vk < 256; ++vk)
        {
            if (isModifierKey(vk) || vk == kVkEnter || vk == kVkEscape)
                continue;
            if (!ImGui::IsKeyPressed(vk,false))
                continue;
            /// <summary>
            /// Backspace / Delete 视为解绑
            /// </summary>
            if (vk == kVkBack || vk == kVkDelete)
            {
                _shortcuts.assign(size_t(_shortcutRecord),0,false,false,false);
                _shortcutDirty  =   true;
                _shortcutRecord =   -1;
                _shortcutError.clear();
                return;
            }
            _recordKey      =   vk;
            _recordPressed  =   true;
            return;
        }
    }

    void    FEEditorUi::drawSettingViewer()
    {
        if (!_settingOpen || _ctx == nullptr)
            return;

        bool    open    =   true;
        ImGui::SetNextWindowSize(ImVec2(560.0f,600.0f),ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(32.0f,kRibbonH + 16.0f),ImGuiCond_FirstUseEver);
        if (ImGui::Begin("配置",&open,ImGuiWindowFlags_NoCollapse))
        {
            const   std::string path    =   FEShortcutConfig::exeXmlPath();
            ImGui::TextDisabled("运行时设置树:分组可折叠,值可直接编辑");
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)",path.c_str());
            ImGui::Separator();

            /// <summary>
            /// 通用 key/value 控件:数据来自 FESetting 适配器
            /// </summary>
            FESettingPropertyModel  model(_ctx->setting());
            ImGui::BeginChild("##setting_rows",ImVec2(0.0f,-ImGui::GetFrameHeightWithSpacing()),false);
            FEPropertyView::draw(model,"##setting");
            ImGui::EndChild();

            if (ImGui::Button("保存"))
            {
                _settingError   =   _ctx->setting().save(path.c_str())
                                  ?   std::string()
                                  :   std::string("保存失败");
            }
            ImGui::SameLine();
            if (ImGui::Button("重新加载"))
            {
                _ctx->setting().load(path.c_str());
                _settingError.clear();
            }
            if (!_settingError.empty())
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.92f,0.45f,0.42f,1.0f),"%s",_settingError.c_str());
            }
        }
        ImGui::End();
        _settingOpen    =   open;
    }

    void    FEEditorUi::drawShortcutSettings()
    {
        if (!_shortcutOpen)
            return;
        if (_shortcutRecord >= 0)
            captureShortcutKeys();

        bool    open    =   true;
        /// <summary>
        /// 首次打开居中显示,尺寸不超过可用区域(小窗口下自动收缩,避免按钮被裁掉)
        /// </summary>
        const   ImVec2  display =   ImGui::GetIO().DisplaySize;
        const   ImVec2  dlgSize =   ImVec2(std::min(640.0f,display.x - 16.0f),
                                           std::min(520.0f,std::max(240.0f,display.y - kRibbonH - 24.0f)));
        const   float   posX    =   std::max(8.0f,(display.x - dlgSize.x) * 0.5f);
        ImGui::SetNextWindowSize(dlgSize,ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(posX,kRibbonH + 12.0f),ImGuiCond_FirstUseEver);
        if (ImGui::Begin("快捷键设置",&open,ImGuiWindowFlags_NoCollapse))
        {
            ImGui::TextColored(ImVec4(0.55f,0.57f,0.62f,1.0f),
                               "双击\"快捷键\"列进入录制态。按下要绑定的键组合(如 Ctrl+S / F5)。"
                               "Enter 确认 / Esc 取消 / Backspace 解绑。");
            ImGui::Spacing();

            /// <summary>
            /// 底部预留:一行提示 + 一行按钮
            /// </summary>
            const   float   footerH =   ImGui::GetTextLineHeightWithSpacing()
                                    +   ImGui::GetFrameHeight()
                                    +   ImGui::GetStyle().ItemSpacing.y + 6.0f;
            ImGui::BeginChild("##shortcut_list",ImVec2(0.0f,-footerH),true);
            {
                /// <summary>
                /// 表头:动作 / 快捷键(两列居中,与参考图一致)
                /// </summary>
                const   float   split   =   ImGui::GetWindowContentRegionWidth() * 0.48f;
                const   float   rowH    =   ImGui::GetTextLineHeight() + 12.0f;
                const   float   listW   =   ImGui::GetWindowContentRegionWidth();
                const   ImU32   gridCol =   IM_COL32(66,70,78,255);
                const   ImVec2  header  =   ImGui::GetCursorScreenPos();
                const   float   head1X  =   (split - ImGui::CalcTextSize("动作").x) * 0.5f;
                const   float   head2X  =   split + (listW - split - ImGui::CalcTextSize("快捷键").x) * 0.5f;
                ImDrawList*     dl      =   ImGui::GetWindowDrawList();
                dl->AddText(ImVec2(header.x + head1X,header.y),kTextMuted,"动作");
                dl->AddText(ImVec2(header.x + head2X,header.y),kTextMuted,"快捷键");
                ImGui::Dummy(ImVec2(0.0f,ImGui::GetTextLineHeight()));
                ImGui::Separator();

                if (_shortcuts._items.empty())
                {
                    ImGui::TextDisabled("没有可配置的命令(请检查 .ui 菜单栏)");
                }
                for (size_t i = 0; i < _shortcuts._items.size(); ++i)
                {
                    const   FEShortcutItem& item    =   _shortcuts._items[i];
                    const   bool            recording   =   (int(i) == _shortcutRecord);
                    const   bool            selected    =   (int(i) == _shortcutSelected);
                    ImGui::PushID(int(i));

                    const   ImVec2  pos =   ImGui::GetCursorScreenPos();
                    if (ImGui::Selectable("##row",selected,0,ImVec2(0.0f,rowH)))
                        _shortcutSelected   =   int(i);
                    /// <summary>
                    /// 双击快捷键列进入录制
                    /// </summary>
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                    {
                        _shortcutSelected   =   int(i);
                        _shortcutRecord     =   int(i);
                        _recordKey          =   0;
                        _recordCtrl         =   false;
                        _recordShift        =   false;
                        _recordAlt          =   false;
                        _recordPressed      =   false;
                        _shortcutError.clear();
                    }

                    const   float   textY   =   pos.y + (rowH - ImGui::GetTextLineHeight()) * 0.5f;
                    const   ImU32   color   =   recording ? kAccent : kTextColor;
                    dl  =   ImGui::GetWindowDrawList();
                    /// <summary>
                    /// 列分隔线与行底线,做出表格的观感
                    /// </summary>
                    dl->AddLine(ImVec2(pos.x + split,pos.y),ImVec2(pos.x + split,pos.y + rowH),gridCol);
                    dl->AddLine(ImVec2(pos.x,pos.y + rowH - 1.0f),ImVec2(pos.x + listW,pos.y + rowH - 1.0f),gridCol);
                    dl->AddText(ImVec2(pos.x + 6.0f,textY),color,item._text.c_str());
                    if (recording)
                    {
                        std::string preview =   "按下按键…";
                        if (_recordPressed && _recordKey != 0)
                            preview =   FEShortcutConfig::comboText(_recordKey,_recordCtrl,
                                                                    _recordShift,_recordAlt);
                        dl->AddText(ImVec2(pos.x + split + 6.0f,textY),kAccent,preview.c_str());
                    }
                    else
                    {
                        const   std::string combo   =   _shortcuts.text(i);
                        dl->AddText(ImVec2(pos.x + split + 6.0f,textY),
                                    combo.empty() ? kTextMuted : kTextColor,
                                    combo.empty() ? "" : combo.c_str());
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndChild();

            /// <summary>
            /// 错误提示(重复绑定 / 保存失败)
            /// </summary>
            if (!_shortcutError.empty())
                ImGui::TextColored(ImVec4(0.92f,0.45f,0.42f,1.0f),"%s",_shortcutError.c_str());
            else
                ImGui::TextDisabled("配置文件: %s",FEShortcutConfig::exeXmlPath().c_str());

            if (ImGui::Button("编辑快捷键"))
            {
                if (_shortcutSelected >= 0)
                {
                    _shortcutRecord =   _shortcutSelected;
                    _recordKey      =   0;
                    _recordCtrl     =   false;
                    _recordShift    =   false;
                    _recordAlt      =   false;
                    _recordPressed  =   false;
                    _shortcutError.clear();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("恢复默认"))
            {
                if (_shortcutSelected >= 0)
                {
                    _shortcuts.reset(size_t(_shortcutSelected));
                    _shortcutDirty  =   true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("全部恢复默认"))
            {
                _shortcuts.resetAll();
                _shortcutDirty  =   true;
            }

            /// <summary>
            /// 右下角:关闭(丢弃改动) / 确定(写入 XML)
            /// </summary>
            const   float   closeW  =   ImGui::CalcTextSize("关闭").x + 28.0f;
            const   float   okW     =   ImGui::CalcTextSize("确定").x + 28.0f;
            const   float   rightX  =   ImGui::GetWindowContentRegionMax().x - closeW - okW
                                     -   ImGui::GetStyle().ItemSpacing.x;
            ImGui::SameLine();
            ImGui::SetCursorPosX(rightX);
            if (ImGui::Button("关闭",ImVec2(closeW,0.0f)))
            {
                discardShortcutChanges();
                _shortcutOpen   =   false;
                open            =   false;
            }
            ImGui::SameLine();
            if (ImGui::Button("确定",ImVec2(okW,0.0f)))
            {
                bool    saved   =   false;
                if (_ctx != nullptr)
                {
                    _shortcuts.save(_ctx->setting());
                    saved   =   _ctx->setting().save(FEShortcutConfig::exeXmlPath().c_str());
                }
                if (saved)
                {
                    _shortcutDirty  =   false;
                    _shortcutOpen   =   false;
                    open            =   false;
                    _shortcutRecord =   -1;
                }
                else
                {
                    _shortcutError  =   "保存失败: " + FEShortcutConfig::exeXmlPath();
                }
            }
        }
        ImGui::End();

        /// <summary>
        /// 标题栏的 X 与"关闭"等价:丢弃未保存的改动
        /// </summary>
        if (!open && _shortcutOpen)
        {
            discardShortcutChanges();
            _shortcutOpen   =   false;
        }
    }

    void    FEEditorUi::drawRibbon()
    {
        ImGuiIO&    io      =   ImGui::GetIO();
        const float width   =   io.DisplaySize.x;
        if (width < 1.0f)
            return;

        processShortcuts();
        /// <summary>
        /// 先同步抓手:按钮绘制之前更新勾选态,选区变化也在此推给已显示的抓手
        /// </summary>
        syncTransformTools();

        ImGui::SetNextWindowPos(ImVec2(0.0f,0.0f),ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width,kRibbonH),ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0.0f,0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.0f);

        const   ImGuiWindowFlags    flags   =   ImGuiWindowFlags_NoTitleBar         |   ImGuiWindowFlags_NoResize
                                            |   ImGuiWindowFlags_NoMove             |   ImGuiWindowFlags_NoScrollbar
                                            |   ImGuiWindowFlags_NoScrollWithMouse  |   ImGuiWindowFlags_NoCollapse
                                            |   ImGuiWindowFlags_NoSavedSettings    |   ImGuiWindowFlags_NoBringToFrontOnFocus;
        /// <summary>
        /// 本帧被点击的动作:窗口结束后统一分发
        /// </summary>
        const   FEUiMenuItem*   triggered   =   nullptr;
        if (ImGui::Begin("##fesr_ribbon",nullptr,flags))
        {
            ImDrawList*     dl          =   ImGui::GetWindowDrawList();
            const   ImVec2  wp          =   ImGui::GetWindowPos();
            const   float   barBottom   =   kGroupRowY - 4.0f;
            /// <summary>
            /// 标签条与功能区两层底色;选中标签与功能区同色,视觉上连成一体
            /// </summary>
            dl->AddRectFilled(wp,ImVec2(wp.x + width,wp.y + barBottom),kTabBarBg);
            dl->AddRectFilled(ImVec2(wp.x,wp.y + barBottom),ImVec2(wp.x + width,wp.y + kRibbonH),kRibbonBg);

            /// <summary>
            /// 标签页行:MainWindow.ui 的主菜单即 Ribbon 的 Page
            /// </summary>
            std::vector<const FEUiMenuItem*>    pages;
            if (_uiDoc.menuBar() != nullptr)
            {
                for (const auto* page : _uiDoc.menuBar()->_children)
                {
                    if (page->_type == kUiMenu)
                        pages.push_back(page);
                }
            }
            if (_activeTab < 0 || _activeTab >= int(pages.size()))
                _activeTab  =   0;
            ImGui::SetCursorPos(ImVec2(6.0f,kTabRowY));
            for (int i = 0; i < int(pages.size()); ++i)
            {
                if (ribbonTab(pages[size_t(i)]->_text.c_str(),_activeTab == i))
                {
                    _activeTab      =   i;
                    /// <summary>
                    /// 换页后内容整体变化,滚动位置回到最左
                    /// </summary>
                    _ribbonScrollX  =   0.0f;
                }
                ImGui::SameLine(0.0f,2.0f);
            }
            /// <summary>
            /// 标签行右边界:右上角信息从这里开始,避免与标签重叠
            /// </summary>
            const   float   tabRight    =   ImGui::GetCursorPosX();

            /// <summary>
            /// 右上角:当前工程名、状态提示与帧率(右对齐,超出部分裁剪)
            /// </summary>
            {
                const   std::string projectName =   _projectName.empty() ? std::string("未命名工程")
                                                                         : _projectName;
                char    info[1200]  =   {};
                if (_statusText.empty())
                    snprintf(info,sizeof(info),"%s   %.1f FPS",projectName.c_str(),io.Framerate);
                else
                    snprintf(info,sizeof(info),"%s   %.1f FPS   |   %s",projectName.c_str(),io.Framerate,
                             _statusText.c_str());
                const   ImVec2  infoSize    =   ImGui::CalcTextSize(info);
                const   float   infoRight   =   width - 8.0f;
                const   float   infoLeft    =   std::max(tabRight,8.0f);
                dl->PushClipRect(ImVec2(wp.x + infoLeft,wp.y),ImVec2(wp.x + infoRight,wp.y + barBottom),true);
                ImGui::SetCursorPos(ImVec2(std::max(infoLeft,infoRight - infoSize.x),kTabRowY + 5.0f));
                ImGui::TextDisabled("%s",info);
                dl->PopClipRect();
            }

            /// <summary>
            /// 功能区:Page 下的子菜单为分组,分组内的动作为按钮;
            /// Page 下直接挂的动作归入以 Page 标题命名的分组
            /// </summary>
            if (!pages.empty())
            {
                const   FEUiMenuItem*   page    =   pages[size_t(_activeTab)];

                /// <summary>
                /// 先按 .ui 中 addaction 的原始顺序组装分组列表:
                /// Page 下直接挂的动作按出现顺序成组,遇到子菜单则另起一个分组
                /// </summary>
                struct  FERibbonGroup
                {
                    RibbonActions           _actions;
                    /// <summary>
                    /// 分组菜单;为空表示这组来自 Page 下直接挂的动作
                    /// </summary>
                    const   FEUiMenuItem*   _menu   =   nullptr;
                };
                std::vector<FERibbonGroup>  groups;
                RibbonActions   direct;
                auto    flushDirect =   [&]()
                {
                    if (direct.empty())
                        return;
                    FERibbonGroup   group;
                    group._actions  =   direct;
                    groups.push_back(group);
                    direct.clear();
                };
                for (const auto* child : page->_children)
                {
                    if (child->_type == kUiAction)
                    {
                        direct.push_back(child);
                    }
                    else if (child->_type == kUiMenu)
                    {
                        flushDirect();
                        FERibbonGroup   group;
                        group._menu =   child;
                        collectGroupActions(child,group._actions);
                        if (!group._actions.empty())
                            groups.push_back(group);
                    }
                }
                flushDirect();

                /// <summary>
                /// 内容总宽 = 各分组宽度之和(含组间间距)
                /// </summary>
                float   contentW    =   0.0f;
                for (const auto& group : groups)
                    contentW    +=  ribbonMetrics(group._actions)._groupW + kGroupGap;

                /// <summary>
                /// 放不下时左右各留出箭头宽度,可视区间相应收窄
                /// </summary>
                const   bool    overflow    =   (contentW > width - kGroupStartX * 2.0f);
                const   float   viewL       =   overflow ? kArrowW : kGroupStartX;
                const   float   viewR       =   overflow ? (width - kArrowW) : (width - kGroupStartX);
                const   float   maxScroll   =   std::max(0.0f,contentW - (viewR - viewL));
                _ribbonScrollX  =   std::max(0.0f,std::min(_ribbonScrollX,maxScroll));

                /// <summary>
                /// 按钮内容整体左移 scrollX,并裁剪到可视区间
                /// </summary>
                dl->PushClipRect(ImVec2(wp.x + viewL,wp.y + barBottom),
                                 ImVec2(wp.x + viewR,wp.y + kRibbonH),true);
                float   x   =   viewL - _ribbonScrollX;
                for (const auto& group : groups)
                {
                    const   char*   title   =   (group._menu != nullptr) ? group._menu->_text.c_str()
                                                                        : page->_text.c_str();
                    x   +=  drawRibbonGroup(group._actions,title,x,_iconFont,&triggered);
                }
                dl->PopClipRect();

                /// <summary>
                /// 放不下时在左右两侧画滚动箭头,点击按 kScrollStep 步进
                /// </summary>
                if (overflow)
                {
                    const   float   arrowY  =   wp.y + kGroupRowY + (kContentH - kArrowW) * 0.5f;
                    if (scrollArrow(ImVec2(wp.x,arrowY),kArrowW,_ribbonScrollX > 0.0f,false))
                        _ribbonScrollX  =   std::max(0.0f,_ribbonScrollX - kScrollStep);
                    if (scrollArrow(ImVec2(wp.x + width - kArrowW,arrowY),kArrowW,
                                    _ribbonScrollX < maxScroll,true))
                        _ribbonScrollX  =   std::min(maxScroll,_ribbonScrollX + kScrollStep);
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(3);

        /// <summary>
        /// Ribbon 按钮动作分发。文件对话框是普通的 ImGui 窗口(非系统模态),
        /// 在帧内处理不会引起消息重入
        /// </summary>
        if (triggered != nullptr)
            onAction(triggered);

        /// <summary>
        /// 关于窗口
        /// </summary>
        if (_aboutOpen)
        {
            /// <summary>
            /// 首次打开落在功能区下方居中,避免盖住标签与按钮
            /// </summary>
            ImGui::SetNextWindowPos(ImVec2((width - 360.0f) * 0.5f,kRibbonH + 16.0f),ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(360.0f,0.0f),ImGuiCond_FirstUseEver);
            if (ImGui::Begin("关于 FESceneEditor",&_aboutOpen,ImGuiWindowFlags_NoCollapse))
            {
                ImGui::Text("miniSoftRender 场景编辑器");
                ImGui::Separator();
                ImGui::Text("Dear ImGui %s",ImGui::GetVersion());
                ImGui::Text("帧率 %.1f FPS",ImGui::GetIO().Framerate);
            }
            ImGui::End();
        }

        drawFileDialog();
        drawModelTree();
        drawProperty();
        drawAxisSetting();
        drawTimeLine();
        /// <summary>
        /// 撤销历史窗口:先把它自己的开关状态回写到动作勾选标志,再绘制
        /// </summary>
        FEUiMenuItem*   undoViewItem    =   findActionItem(_uiDoc.menuBar(),"action_undoView");
        if (undoViewItem != nullptr)
            undoViewItem->_checked  =   _undoView.isOpen();
        _undoView.draw(_undoStack,_pendingActions);
        drawUiPreview();
        drawShortcutSettings();
        /// <summary>
        /// 天空盒开关:设置窗口里改动 render/showSkyBox 后立即同步到场景,
        /// 无需等保存或重新加载
        /// </summary>
        if (_scene != nullptr && _ctx != nullptr)
        {
            const   bool    shown   =   _ctx->setting().get("render").get("showSkyBox").getOr(true);
            if  (shown != _skyShown)
            {
                _skyShown   =   shown;
                _scene->showSky(shown);
            }
        }
        drawSettingViewer();
    }

    void    FEEditorUi::drawFileDialog()
    {
        if (!_fileDialog.isOpen())
            return;
        std::string file;
        if (!_fileDialog.draw(_iconFont,file))
            return;
        if (file.empty())
            return;

        /// <summary>
        /// 另存为模式走保存,导入模式走导入模型,其余走打开。
        /// 三者都会改场景(open 会新建渲染工厂),必须推迟到渲染趟之外执行
        /// </summary>
        const   bool    saveMode    =   _fileDialogSaveMode;
        const   bool    importMode  =   _fileDialogImportMode;
        _fileDialogSaveMode     =   false;
        _fileDialogImportMode   =   false;
        if (saveMode)
            _pendingActions.push_back([this,file]() {   saveProjectTo(file);    });
        else if (importMode)
            _pendingActions.push_back([this,file]() {   importModel(file);  });
        else
            _pendingActions.push_back([this,file]() {   openProject(file);  });
    }

    void    FEEditorUi::onAction(const FEUiMenuItem* action)
    {
        if (action == nullptr)
            return;
        const   std::string&    name    =   action->_name;
        if (name == "action_openPrj")
        {
            /// <summary>
            /// 打开工程:与 Qt 版一致,默认从程序目录开始
            /// </summary>
            _fileDialogSaveMode =   false;
            _fileDialog.open(FEFileDialog::kOpen,"打开工程",{{"工程文件 (*.fepj)",".fepj"}},_startDir,"");
        }
        else if (name == "action_savePrj")
        {
            _pendingActions.push_back([this]() {    saveProject();  });
        }
        else if (name == "actionReset")
        {
            _pendingActions.push_back([this]() {    resetScene();   });
        }
        else if (name == "action_undo")
        {
            /// <summary>
            /// 撤销会移除场景节点(改渲染工厂),与创建一样必须推迟到 MSG_UPDATE
            /// </summary>
            _pendingActions.push_back([this]() {
                _undoStack.undo();
                /// <summary>
                /// 撤销可能增删节点上的动画等组件,组件变更不触发节点树事件,需手动刷新行列表
                /// </summary>
                _modelTree.refresh();
            });
        }
        else if (name == "action_redo")
        {
            _pendingActions.push_back([this]() {
                _undoStack.redo();
                /// <summary>
                /// 重做可能增删节点上的动画等组件,组件变更不触发节点树事件,需手动刷新行列表
                /// </summary>
                _modelTree.refresh();
            });
        }
        else if (name == "action_ShortcutSettings")
        {
            /// <summary>
            /// 打开快捷键设置窗口;不改场景,无需推迟到 MSG_UPDATE
            /// </summary>
            _shortcutOpen   =   true;
            _shortcutError.clear();
            if (_shortcutSelected < 0 && !_shortcuts._items.empty())
                _shortcutSelected   =   0;
        }
        else if (name == "action_SettingViewer")
        {
            /// <summary>
            /// 打开运行时配置窗口(通用 key/value 控件),不改场景
            /// </summary>
            _settingOpen    =   true;
        }
        else if (name == "action_importModel")
        {
            /// <summary>
            /// 导入模型:默认选 .gltf/.glb,类型下拉框可切到所有文件;
            /// 解析与入场景推迟到下一帧 MSG_UPDATE
            /// (与创建节点同理,避免渲染趟内新建渲染工厂)
            /// </summary>
            _fileDialogSaveMode     =   false;
            _fileDialogImportMode   =   true;
            _fileDialog.open(FEFileDialog::kOpen,"导入模型",
                             {{"3D 模型 (*.gltf;*.glb)",".gltf;.glb"},
                              {"所有文件 (*.*)",""}},
                             _startDir,"");
        }
        else if (name == "action_TimeLine")
        {
            /// <summary>
            /// 时间线面板开关;只改 UI 状态,不改场景
            /// </summary>
            _timeLine.toggle();
        }
        else if (name == "action_undoView")
        {
            /// <summary>
            /// 撤销历史窗口开关;只改 UI 状态,不改场景
            /// </summary>
            _undoView.toggle();
        }
        else if (name == "action_axisEditor")
        {
            /// <summary>
            /// 坐标轴编辑窗口开关;只改 UI 状态,不改场景
            /// </summary>
            _axisOpen   =   !_axisOpen;
        }
        else if (name == "action_move" || name == "action_rotation" || name == "action_scale")
        {
            /// <summary>
            /// 三维变换抓手开关(参考 Qt 版 slotMoveEditor/slotRotEditor/slotScaleEditor):
            /// 三个动作各自独立切换,不互斥;只改抓手的可见标志与编辑对象,
            /// 不修改场景数据,因此不产生撤销记录
            /// </summary>
            toggleTransformTool(name);
        }
        else if (name.rfind("action_add",0) == 0)
        {
            /// <summary>
            /// "模型"分组下的几何体/空节点创建按钮(action_addBox … action_addEmpty);
            /// 相机节点名为 action_AddCameraNode,不会被此前缀匹配。
            /// 这里只登记动作,真正的创建推迟到下一帧 MSG_UPDATE(见 onMessage),
            /// 因为本函数由 Ribbon 绘制调用,而绘制发生在渲染趟内部:此刻新建渲染工厂
            /// 会赶在同一帧被渲染,而工厂的 update 尚未执行、索引缓冲尚未生成
            /// </summary>
            _pendingActions.push_back([this,name]() {   createModelNode(name);  });
        }
    }

    void    FEEditorUi::openProject(const std::string& path)
    {
        if (_scene == nullptr)
            return;
        if (_scene->open(toNativePath(path).c_str()))
        {
            /// <summary>
            /// 场景已被整体替换,旧命令引用的节点不再属于当前工程
            /// </summary>
            _undoStack.clear();
            _timeLine.reset();
            _projectName    =   path;
            _startDir       =   std::filesystem::u8path(path).parent_path().u8string();
            _statusText     =   "打开工程成功";
        }
        else
        {
            _statusText     =   "打开工程失败:" + path;
        }
    }

    void    FEEditorUi::saveProject()
    {
        if (_scene == nullptr)
            return;
        /// <summary>
        /// 与 Qt 版一致:节点树为空时不做任何处理
        /// </summary>
        if (_scene->nodeTree().topLevelNodes().empty())
        {
            _statusText =   "场景为空,没有可保存的内容";
            return;
        }
        if (_projectName.empty())
        {
            _fileDialogSaveMode =   true;
            _fileDialog.open(FEFileDialog::kSave,"另存为",{{"工程文件 (*.fepj)",".fepj"}},_startDir,"unname");
            return;
        }
        saveProjectTo(_projectName);
    }

    void    FEEditorUi::saveProjectTo(const std::string& path)
    {
        if (_scene == nullptr)
            return;
        if (_scene->save(toNativePath(path).c_str()))
        {
            _projectName    =   path;
            _startDir       =   std::filesystem::u8path(path).parent_path().u8string();
            _statusText     =   "保存工程成功";
        }
        else
        {
            _statusText     =   "保存工程失败:" + path;
        }
    }

    void    FEEditorUi::resetScene()
    {
        if (_scene == nullptr)
            return;
        /// <summary>
        /// 与 Qt 版 slotReset 一致:清空场景与工程名,不退出程序。
        /// 模型树订阅了节点树的清空事件,下一帧自动刷新
        /// </summary>
        _scene->clear();
        _undoStack.clear();
        _timeLine.reset();
        _projectName.clear();
        _statusText =   "已重置";
    }

    namespace
    {
        /// <summary>
        /// 编辑器创建几何体时统一使用的属性流(位置/法线/UV),
        /// 与脚本绑定及 MCP 工具的默认流一致
        /// </summary>
        Attrs   editorGeoAttrs()
        {
            return  Attrs({{IS_VERTEX_POS,         FMT_R32G32B32_FLOAT},
                           {IS_VERTEX_NOR,         FMT_R32G32B32_FLOAT},
                           {IS_VERTEX_TEXCOORD0,   FMT_R32G32_FLOAT}});
        }

        /// <summary>
        /// 生成不与现有顶层节点重名的新名字:重名时追加 " 2"、" 3" 依次递增
        /// </summary>
        /// <param name="scene">场景(取其顶层节点名)</param>
        /// <param name="base">期望的基础名字</param>
        /// <returns>可用的节点名</returns>
        std::string editorUniqueName(FEScene& scene,const std::string& base)
        {
            std::string name    =   base;
            for (int index = 2; ; ++index)
            {
                bool    used    =   false;
                for (auto&  n   :   scene.nodeTree().topLevelNodes())
                {
                    if (n != nullptr && name == n->name())
                    {
                        used    =   true;
                        break;
                    }
                }
                if (!used)
                    return  name;
                name    =   base + " " + std::to_string(index);
            }
        }
    }

    void    FEEditorUi::createModelNode(const std::string& actionName)
    {
        if (_scene == nullptr || _ctx == nullptr)
            return;

        /// <summary>
        /// 统一入口:节点名先唯一化(命令重做时沿用它,避免因场景已有同名节点而改名),
        /// 再压入撤销栈;push() 会立即执行一次 redo(),把节点加入节点树并分发到
        /// 渲染工厂与各系统(模型树显示与真正进入渲染)。
        /// 本函数运行在 MSG_UPDATE(见 onAction),此时新建工厂能在同一帧的 update
        /// 中生成 VBO/IBO,不会出现空索引缓冲
        /// </summary>
        auto    addWithUndo =   [this](Node node,const std::string& name)
        {
            if (node == nullptr)
                return;
            node->setName(editorUniqueName(*_scene,name));
            _undoStack.push(new FEEditorCreateNodeCmd(*_scene,{node},"创建" + name));
        };

        /// <summary>
        /// 按动作名生成网格;每种几何体用默认参数即可获得合理的初始形状
        /// </summary>
        Mesh        mesh;
        std::string label;
        if (actionName == "action_addBox")
        {
            FEGeometryBox   geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "正方体";
        }
        else if (actionName == "action_addSphere")
        {
            FEGeometrySphere    geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "球体";
        }
        else if (actionName == "action_addCylinder")
        {
            FEGeometryCylinder  geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "圆柱";
        }
        else if (actionName == "action_addSphericalCap")
        {
            FEGeometryPRCP  geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "球冠体";
        }
        else if (actionName == "action_addConeFrustum")
        {
            FEGeometryPRCN  geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "圆锥台";
        }
        else if (actionName == "action_addPyramidFrustum")
        {
            FEGeometryPRPY  geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "棱锥台";
        }
        else if (actionName == "action_addTorusSector")
        {
            /// <summary>
            /// 圆环扇形:取四分之一圆的弧段
            /// </summary>
            FEGeometryTorus geo(*_ctx);
            geo.param().arc    =   float(TWO_PI) * 0.25f;
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "圆环扇形";
        }
        else if (actionName == "action_addTorus")
        {
            FEGeometryTorus geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "圆环";
        }
        else if (actionName == "action_addHyperboloid")
        {
            /// <summary>
            /// 引擎暂无独立的双曲面实现,用椭球体(PRDH)代替
            /// </summary>
            FEGeometryPRDH  geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "双曲面";
        }
        else if (actionName == "action_addPyramid")
        {
            /// <summary>
            /// 四棱锥:FEGeometryPyramid 默认即为 radiusTop=0、radialSegments=4
            /// </summary>
            FEGeometryPyramid   geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "四棱锥";
        }
        else if (actionName == "action_addTorusSolid")
        {
            FEGeometryPRRT  geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "圆环体";
        }
        else if (actionName == "action_addTorusKnot")
        {
            FEGeometryTorusKnot geo(*_ctx);
            mesh    =   geo.triangular(editorGeoAttrs());
            label   =   "圆环扭结";
        }
        else if (actionName == "action_addEmpty")
        {
            /// <summary>
            /// 空节点:只有名字,没有网格与材质
            /// </summary>
            Node    empty   =   new FENode(*_ctx);
            addWithUndo(empty,"空节点");
            _statusText =   "已创建空节点";
            return;
        }
        else
        {
            return;
        }

        if (mesh == nullptr)
        {
            _statusText =   "几何体生成失败:" + label;
            return;
        }

        Node    node    =   new FENode(*_ctx);
        node->setMesh(mesh);

        /// <summary>
        /// 普通 PBR 材质:明亮的漫反射与自发光,使新对象在默认光照下清晰可见
        /// </summary>
        FEMaterialPBR*  mat =   new FEMaterialPBR(*_ctx);
        mat->data()._value._diffuse     =   float4(0.35f,0.55f,0.85f,1.0f);
        mat->data()._value._emissive    =   float4(0.08f,0.16f,0.28f,1.0f);
        mat->data().update();
        node->setMaterial(mat);

        addWithUndo(node,label);
        _statusText =   "已创建" + label;
    }

    void    FEEditorUi::importModel(const std::string& path)
    {
        if (_scene == nullptr || _ctx == nullptr || path.empty())
            return;

        const   std::filesystem::path   fsPath  =   std::filesystem::u8path(path);
        std::string ext =   fsPath.extension().u8string();
        std::transform(ext.begin(),ext.end(),ext.begin(),
                       [](unsigned char c) {   return  char(std::tolower(c));   });

        /// <summary>
        /// 格式必须与 FEContext 注册读取器时使用的完全一致(见 FEContext.cpp 的
        /// DEFINE_FORMAT_READER(FEFormatGLTFReader)),否则 queryReader 查不到
        /// </summary>
        FEFileFormat    fmt(".gltf","1.0.0.0","GLTF text Format!");
        if (ext == ".glb")
            fmt =   FEFileFormat(".glb","1.0.0.0","GLTF binary Format!");
        else if (ext != ".gltf")
        {
            _statusText =   "不支持导入该格式:" + fsPath.filename().u8string();
            return;
        }

        auto    reader  =   FEFileFormatHelper::queryReader(*_ctx,fmt);
        if (reader == nullptr)
        {
            _statusText =   "导入失败:没有可用的模型读取器";
            return;
        }

        /// <summary>
        /// 解析与入场景都在 MSG_UPDATE 内执行(由 _pendingActions 推迟):
        /// 此时新建的渲染工厂能在同一帧 update 里生成顶点/索引缓冲
        /// </summary>
        const   Objects objects =   reader->readFiles({toNativePath(path)});
        Nodes   nodes;
        for (auto object : objects)
        {
            Node    node    =   object->cast<FENode>();
            if (node != nullptr)
                nodes.push_back(node);
        }
        if (nodes.empty())
        {
            _statusText =   "导入失败,未解析到节点:" + fsPath.filename().u8string();
            return;
        }

        /// <summary>
        /// 顶层节点名唯一化(重做时沿用该名字,避免与场景已有节点重名);
        /// 文件内节点无名字时用文件名兜底
        /// </summary>
        const   std::string stem    =   fsPath.stem().u8string();
        for (Node& node : nodes)
        {
            const   std::string current =   node->name() != nullptr ? node->name() : "";
            const   std::string base    =   current.empty() ? stem : current;
            node->setName(editorUniqueName(*_scene,base));
        }

        _undoStack.push(new FEEditorCreateNodeCmd(*_scene,nodes,"导入" + stem));
        _statusText =   "已导入 " + std::to_string(nodes.size()) + " 个节点:"
                    +   fsPath.filename().u8string();
    }

    void    FEEditorUi::drawModelTree()
    {
        if (!_treeOpen)
            return;
        ImGui::SetNextWindowPos(ImVec2(8.0f,kRibbonH + 8.0f),ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300.0f,420.0f),ImGuiCond_FirstUseEver);
        if (ImGui::Begin("模型树",&_treeOpen))
        {
            /// <summary>
            /// 模型树的数据来源是场景的 FENodeTree
            /// </summary>
            _modelTree.draw(_scene ? &_scene->nodeTree() : nullptr,_iconFont);
        }
        ImGui::End();
    }

    void    FEEditorUi::drawTimeLine()
    {
        _timeLine.draw(_iconFont);
    }

    void    FEEditorUi::drawProperty()
    {
        if (!_propertyOpen)
            return;
        /// <summary>
        /// 首次打开落在模型树右侧,避免与模型树重叠
        /// </summary>
        ImGui::SetNextWindowPos(ImVec2(316.0f,kRibbonH + 8.0f),ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300.0f,360.0f),ImGuiCond_FirstUseEver);
        /// <summary>
        /// 数据来源:模型树当前选中节点;未选中时面板显示提示文字。
        /// 约束目标拾取期间改为固定显示约束所有者对象(拾取点击只更新候选目标)
        /// </summary>
        FENode* node    =   nullptr;
        Object  selected    =   _propertyPanel.pickingOwner();
        if (!selected)
            selected        =   _modelTree.selected();
        if (selected)
            node            =   selected->cast<FENode>();
        /// <summary>
        /// 标题栏显示当前节点/对象名称;"###属性"保证窗口 ID 不随名称变化,
        /// 改名或切换选中时窗口位置/停靠状态保持不变
        /// </summary>
        String  title   =   "属性";
        if (selected)
        {
            const   char*   name    =   (node != nullptr && node->name() != nullptr)
                                      ? node->name() : selected->className();
            title  +=  " - ";
            title  +=  (name != nullptr && name[0] != '\0') ? name : "(未命名)";
        }
        title  +=  "###属性";
        if (ImGui::Begin(title.c_str(),&_propertyOpen))
        {
            /// <summary>
            /// 时间线提供当前时间的关键帧状态(check)与添加/移除关键帧。
            /// 旋转值由节点自己的累计欧拉角账本提供,面板不再需要额外参数
            /// </summary>
            _propertyPanel.draw(node,&_timeLine,&_modelTree.selection());
        }
        ImGui::End();
        /// <summary>
        /// 拾取结束后把模型树选中恢复回约束所有者节点,避免属性面板跳到被拾取对象
        /// </summary>
        const   Object  restore =   _propertyPanel.takeRestoreSelection();
        if (restore)
            _modelTree.select(restore);
    }

    void    FEEditorUi::drawAxisSetting()
    {
        if (!_axisOpen)
            return;
        /// <summary>
        /// 轴编辑窗口:配置编辑轴的轴位置/编辑类型/坐标系/中心/自定义轴心轴向/禁用轴
        /// </summary>
        drawAxisSettingWindow(_axisSetting,_axisOpen,_modelTree.selection());
    }

    void    FEEditorUi::drawUiPreview()
    {
        if (!_uiPreviewOpen)
            return;
        /// <summary>
        /// 首次打开落在功能区下方,避免遮挡标签页
        /// </summary>
        ImGui::SetNextWindowPos(ImVec2(80.0f,kRibbonH + 16.0f),ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(900.0f,560.0f),ImGuiCond_FirstUseEver);
        if (ImGui::Begin("UI 预览 - MainWindow.ui",&_uiPreviewOpen))
        {
            if (_uiDoc.root() == nullptr)
            {
                ImGui::TextDisabled("解析 .ui 失败:%s",_uiError.c_str());
                ImGui::TextDisabled("请确认 bin/data/ui/MainWindow.ui 存在");
            }
            else
            {
                _uiRenderer.draw(_uiDoc);
            }
        }
        ImGui::End();
    }

    void    FEEditorUi::toggleGrid()
    {
        _showGrid   =   !_showGrid;
        if (_scene != nullptr)
            _scene->showGrid(_showGrid);
    }

    void    FEEditorUi::resetCamera()
    {
        if (_scene == nullptr)
            return;
        Camera  cam =   _scene->camera();
        if (cam == nullptr)
            return;
        cam->setEye(real3(0.0f,-2.5f,0.0f));
        cam->setTarget(real3(0.0f,0.0f,0.0f));
        cam->setUp(real3(0.0f,0.0f,1.0f));
        cam->update();
    }

    void    FEEditorUi::toggleTransformTool(const std::string& actionName)
    {
        if (_scene == nullptr)
            return;
        FEInputSystem*  inputSys    =   _scene->inputSystem();
        if (inputSys == nullptr)
            return;

        /// <summary>
        /// 与 Qt 版一致:已显示则隐藏并清空编辑对象,否则显示并把模型树当前选区
        /// 交给抓手。抓手是常驻的渲染/输入工厂,切换只改可见标志,不产生撤销记录
        /// </summary>
        const   Objects&    selection   =   _modelTree.selection();
        if (actionName == "action_move")
        {
            auto    com =   inputSys->query(UUIDOF(FENodeMoveEditor));
            if (com == nullptr)
                return;
            FENodeMoveEditor*   editor  =   com->as<FENodeMoveEditor>();
            if (com->flags().hasFlag(FLAG_VISIBLE))
            {
                com->flags().removeFlag(FLAG_VISIBLE);
                editor->setObjects({});
            }
            else
            {
                com->flags().addFlag(FLAG_VISIBLE);
                editor->setObjects(selection);
            }
        }
        else if (actionName == "action_rotation")
        {
            auto    com =   inputSys->query(UUIDOF(FENodeRotateEditor));
            if (com == nullptr)
                return;
            FENodeRotateEditor* editor  =   com->as<FENodeRotateEditor>();
            if (com->flags().hasFlag(FLAG_VISIBLE))
            {
                com->flags().removeFlag(FLAG_VISIBLE);
                editor->setObjects({});
            }
            else
            {
                com->flags().addFlag(FLAG_VISIBLE);
                editor->setObjects(selection);
            }
        }
        else if (actionName == "action_scale")
        {
            auto    com =   inputSys->query(UUIDOF(FENodeScaleEditor));
            if (com == nullptr)
                return;
            FENodeScaleEditor*  editor  =   com->as<FENodeScaleEditor>();
            if (com->flags().hasFlag(FLAG_VISIBLE))
            {
                com->flags().removeFlag(FLAG_VISIBLE);
                editor->setObjects({});
            }
            else
            {
                com->flags().addFlag(FLAG_VISIBLE);
                editor->setObjects(selection);
            }
        }
        else
            return;
        /// <summary>
        /// 本次已把选区交给抓手,记录后避免同一选区再触发一次 setObjects
        /// </summary>
        _toolSelection  =   selection;
    }

    void    FEEditorUi::syncTransformTools()
    {
        if (_scene == nullptr)
            return;
        FEInputSystem*  inputSys    =   _scene->inputSystem();
        if (inputSys == nullptr)
            return;

        Component       move    =   inputSys->query(UUIDOF(FENodeMoveEditor));
        Component       rotate  =   inputSys->query(UUIDOF(FENodeRotateEditor));
        Component       scale   =   inputSys->query(UUIDOF(FENodeScaleEditor));
        const   bool    moveOn  =   move    != nullptr && move->flags().hasFlag(FLAG_VISIBLE);
        const   bool    rotateOn=   rotate  != nullptr && rotate->flags().hasFlag(FLAG_VISIBLE);
        const   bool    scaleOn =   scale   != nullptr && scale->flags().hasFlag(FLAG_VISIBLE);
        /// <summary>
        /// 轴配置每帧推送给旋转抓手(轴编辑窗口可能刚被修改)
        /// </summary>
        if (rotateOn)
            rotate->as<FENodeRotateEditor>()->setAxisSetting(&_axisSetting);

        /// <summary>
        /// 把抓手的实际可见状态回写到动作的勾选标志,
        /// 这样工程被重新打开(抓手回到默认隐藏)后 Ribbon 不会残留高亮
        /// </summary>
        FEUiMenuItem*   bar =   _uiDoc.menuBar();
        if (bar != nullptr)
        {
            FEUiMenuItem*   item    =   findActionItem(bar,"action_move");
            if (item != nullptr)
                item->_checked  =   moveOn;
            item    =   findActionItem(bar,"action_rotation");
            if (item != nullptr)
                item->_checked  =   rotateOn;
            item    =   findActionItem(bar,"action_scale");
            if (item != nullptr)
                item->_checked  =   scaleOn;
        }

        /// <summary>
        /// 播放中禁止编辑:清空抓手的编辑对象,暂停后选区变化会自动重新推给抓手
        /// </summary>
        if  (_timeLine.isPlaying())
        {
            if  (!_toolSelection.empty())
            {
                if (moveOn)
                    move->as<FENodeMoveEditor>()->setObjects({});
                if (rotateOn)
                    rotate->as<FENodeRotateEditor>()->setObjects({});
                if (scaleOn)
                    scale->as<FENodeScaleEditor>()->setObjects({});
                _toolSelection.clear();
            }
            return;
        }
        /// <summary>
        /// 模型树选区变化后,把新选区推给已显示的抓手(与 Qt 版选中事件一致)
        /// </summary>
        const   Objects&    selection   =   _modelTree.selection();
        if (sameObjects(selection,_toolSelection))
            return;
        _toolSelection  =   selection;
        if (moveOn)
            move->as<FENodeMoveEditor>()->setObjects(selection);
        if (rotateOn)
            rotate->as<FENodeRotateEditor>()->setObjects(selection);
        if (scaleOn)
            scale->as<FENodeScaleEditor>()->setObjects(selection);
    }

    bool    FEEditorUi::onMessage(const FEMessage& msg)
    {
        if (!_attached || !_hasContext)
            return  false;

        ImGuiIO&        io      =   ImGui::GetIO();
        const   auto    msgId   =   msg.msgId();
        /// <summary>
        /// 按下时刻的捕获状态决定整段拖拽归 GUI 还是归场景
        /// </summary>
        const   bool    captured=   io.WantCaptureMouse;
        bool            block   =   false;
        switch (msgId)
        {
        case MSG_MOUSE_MOVE:
            {
                const auto& m   =   static_cast<const MsgMouseMove&>(msg)._info;
                io.MousePos     =   ImVec2(float(m._mouse.x),float(m._mouse.y));
            }
            break;
        case MSG_LBUTTON_DOWN:
            {
                const auto& m   =   static_cast<const MsgLButtonDown&>(msg)._info;
                io.MousePos     =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                io.MouseDown[0] =   true;
                _guiDrag[0]     =   captured;
            }
            break;
        case MSG_LBUTTON_UP:
            {
                const auto& m   =   static_cast<const MsgLButtonUp&>(msg)._info;
                io.MousePos     =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                io.MouseDown[0] =   false;
            }
            break;
        case MSG_RBUTTON_DOWN:
            {
                const auto& m   =   static_cast<const MsgRButtonDown&>(msg)._info;
                io.MousePos     =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                io.MouseDown[1] =   true;
                _guiDrag[1]     =   captured;
            }
            break;
        case MSG_RBUTTON_UP:
            {
                const auto& m   =   static_cast<const MsgRButtonUp&>(msg)._info;
                io.MousePos     =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                io.MouseDown[1] =   false;
            }
            break;
        case MSG_MBUTTON_DOWN:
            {
                const auto& m   =   static_cast<const MsgMButtonDown&>(msg)._info;
                io.MousePos     =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                io.MouseDown[2] =   true;
                _guiDrag[2]     =   captured;
            }
            break;
        case MSG_MBUTTON_UP:
            {
                const auto& m   =   static_cast<const MsgMButtonUp&>(msg)._info;
                io.MousePos     =   ImVec2(float(m._mouse.x),float(m._mouse.y));
                io.MouseDown[2] =   false;
            }
            break;
        case MSG_MOUSE_WHEEL:
            {
                const auto& w   =   static_cast<const MsgMouseWheel&>(msg)._info;
                io.MousePos     =   ImVec2(float(w._mouse.x),float(w._mouse.y));
                io.MouseWheel   =   float(w._zDelta) / 120.0f;
            }
            break;
        case MSG_KEYDOWN:
            {
                const auto& k   =   static_cast<const MsgKeyDown&>(msg)._info;
                if (k._key < 512)
                    io.KeysDown[k._key] =   true;
                syncModifierKeys(io);
                block   =   io.WantCaptureKeyboard;
            }
            break;
        case MSG_KEYUP:
            {
                const auto& k   =   static_cast<const MsgKeyUp&>(msg)._info;
                if (k._key < 512)
                    io.KeysDown[k._key] =   false;
                syncModifierKeys(io);
                block   =   io.WantCaptureKeyboard;
            }
            break;
        case MSG_CHAR:
            {
                const auto& c   =   static_cast<const MsgChar&>(msg)._info;
                /// <summary>
                /// 退格/回车/制表等控制字符由按键状态处理,不能当可见字符插入
                /// </summary>
                if (c._char >= 32)
                    io.AddInputCharacter(ImWchar(c._char));
                block   =   io.WantCaptureKeyboard;
            }
            break;
        case MSG_UPDATE:
            /// <summary>
            /// 本消息先于场景到达(宿主 main 的 notify 先调 ui.onMessage 再转发场景),
            /// 而场景在此消息中才执行 onFrameStart/onFrameUpdate:
            /// 在此刻执行排队的场景操作,新建的渲染工厂能在同一帧的 update 中生成
            /// VBO/IBO,避免其赶在 update 之前被渲染而导致空索引缓冲崩溃。
            /// 先整体取出再执行,避免执行过程中新压入的动作在同一次循环里被重复处理
            /// </summary>
            if (!_pendingActions.empty())
            {
                std::vector<std::function<void()>>  actions;
                actions.swap(_pendingActions);
                for (auto& fn : actions)
                    fn();
            }
            /// 脚本自动化:每条命令占一帧,排在排队的场景操作之后执行
            runScript();
            break;
        default:
            break;
        }
        /// <summary>
        /// 在 GUI 上按下的拖拽整段归 GUI(直到对应按键抬起);
        /// 在场景里按下的拖拽即使经过功能区,抬起事件仍交回场景,避免按键状态卡死
        /// </summary>
        switch (msgId)
        {
        case MSG_LBUTTON_DOWN:
            block   =   _guiDrag[0];
            break;
        case MSG_LBUTTON_UP:
            block   =   _guiDrag[0];
            _guiDrag[0]  =   false;
            break;
        case MSG_RBUTTON_DOWN:
            block   =   _guiDrag[1];
            break;
        case MSG_RBUTTON_UP:
            block   =   _guiDrag[1];
            _guiDrag[1]  =   false;
            break;
        case MSG_MBUTTON_DOWN:
            block   =   _guiDrag[2];
            break;
        case MSG_MBUTTON_UP:
            block   =   _guiDrag[2];
            _guiDrag[2]  =   false;
            break;
        case MSG_MOUSE_MOVE:
        case MSG_MOUSE_WHEEL:
            block   =   captured || _guiDrag[0] || _guiDrag[1] || _guiDrag[2];
            break;
        default:
            break;
        }
        return  block;
    }
}
