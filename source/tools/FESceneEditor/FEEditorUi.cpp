/// <summary>
/// FESceneEditor 主界面实现:
///   - ImGui 上下文 / 渲染后端 / 字体图集 / Overlay 工厂的一次性挂接;
///   - Ribbon 风格功能区(标签页 + 分组大按钮)、关于窗口与 .ui 预览窗口;
///   - 鼠标消息转发,功能区被操作时宿主跳过场景处理。
/// </summary>

#include    "FEEditorUi.hpp"

#include    "../../../inc/FEMessage.hpp"
#include    "../../../inc/graphic/FEScene.h"

#include    <imgui.h>
#include    <iostream>

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
    /// 大按钮宽
    /// </summary>
    const   float   kButtonW    =   72.0f;
    /// <summary>
    /// 大按钮高
    /// </summary>
    const   float   kButtonH    =   46.0f;
    /// <summary>
    /// 功能区总高
    /// </summary>
    const   float   kRibbonH    =   110.0f;

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
    /// Font Awesome 图标字体(Free Solid 6.x)的码位:九宫格(table-cells-large)
    /// </summary>
    const   char*   kIconGrid   =   u8"\uf009";
    /// <summary>
    /// Font Awesome 图标字体(Free Solid 6.x)的码位:相机
    /// </summary>
    const   char*   kIconCamera =   u8"\uf030";
    /// <summary>
    /// Font Awesome 图标字体(Free Solid 6.x)的码位:退出(power-off)
    /// </summary>
    const   char*   kIconPower  =   u8"\uf011";
    /// <summary>
    /// Font Awesome 图标字体(Free Solid 6.x)的码位:关于(circle-info)
    /// </summary>
    const   char*   kIconInfo   =   u8"\uf05a";
    /// <summary>
    /// Font Awesome 图标字体(Free Solid 6.x)的码位:界面预览(window-maximize)
    /// </summary>
    const   char*   kIconWindow =   u8"\uf2d0";
    /// <summary>
    /// Font Awesome 图标字体(Free Solid 6.x)的码位:模型树(cube)
    /// </summary>
    const   char*   kIconCube   =   u8"\uf1b2";
    /// <summary>
    /// 图标字号(像素)
    /// </summary>
    const   float   kIconFontSize   =   24.0f;

    /// <summary>
    /// 用图标字体在 center 处居中绘制一个图标
    /// </summary>
    /// <param name="dl">绘制列表</param>
    /// <param name="font">图标字体</param>
    /// <param name="center">中心点</param>
    /// <param name="col">颜色</param>
    /// <param name="glyph">字形</param>
    void    drawRibbonIcon(ImDrawList* dl,ImFont* font,const ImVec2& center,ImU32 col,const char* glyph)
    {
        if (font == nullptr || glyph == nullptr)
            return;
        const   ImVec2  textSize    =   font->CalcTextSizeA(kIconFontSize,4096.0f,0.0f,glyph);
        dl->AddText(font,kIconFontSize,
                    ImVec2(center.x - textSize.x * 0.5f,center.y - textSize.y * 0.5f),col,glyph);
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
    /// 功能区大按钮:图标在上、文字在下,支持选中态
    /// </summary>
    /// <param name="id">ImGui ID</param>
    /// <param name="iconFont">图标字体</param>
    /// <param name="glyph">图标字形</param>
    /// <param name="label">按钮文本</param>
    /// <param name="selected">是否选中</param>
    /// <returns>true 表示本帧被点击</returns>
    bool    ribbonBigButton(const char* id,ImFont* iconFont,const char* glyph,const char* label,bool selected)
    {
        const   ImVec2  size    =   ImVec2(kButtonW,kButtonH);
        const   ImVec2  pos     =   ImGui::GetCursorScreenPos();
        const   ImVec2  end     =   ImVec2(pos.x + size.x,pos.y + size.y);
        const   bool    pressed =   ImGui::InvisibleButton(id,size);
        const   bool    hovered =   ImGui::IsItemHovered();
        ImDrawList*     dl      =   ImGui::GetWindowDrawList();
        if (selected)
        {
            dl->AddRectFilled(pos,end,IM_COL32(74,144,226,80),3.0f);
            dl->AddRect(pos,end,kAccent,3.0f);
        }
        else if (hovered)
        {
            dl->AddRectFilled(pos,end,IM_COL32(255,255,255,20),3.0f);
            dl->AddRect(pos,end,IM_COL32(255,255,255,40),3.0f);
        }
        const   ImU32   fg      =   selected ? IM_COL32(255,255,255,255) : kIconColor;
        drawRibbonIcon(dl,iconFont,ImVec2(pos.x + size.x * 0.5f,pos.y + 15.0f),fg,glyph);
        const   ImVec2  textSize=   ImGui::CalcTextSize(label);
        dl->AddText(ImVec2(pos.x + (size.x - textSize.x) * 0.5f,pos.y + size.y - textSize.y - 4.0f),fg,label);
        return  pressed;
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
        /// Ribbon 大按钮图标:Font Awesome 实心图标字体,
        /// 只加载用到的 4 个私有区码位,避免字体图集被整套图标撑大
        /// </summary>
        {
            static  const   ImWchar kIconRanges[]   =
                {
                    0xF009,0xF009,  0xF011,0xF011,  0xF030,0xF030,  0xF05A,0xF05A,
                    0xF06E,0xF06E,  0xF070,0xF070,  0xF1B2,0xF1B2,  0xF2D0,0xF2D0,  0
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
        _showGrid   =   true;
        _activeTab  =   1;
        _guiDrag[0] =   _guiDrag[1] = _guiDrag[2] = false;
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
        _factory    =   new FEEditorImGuiFactory(ctx,_backend);
        _factory->_buildUI  =   [this]() {   this->drawRibbon();  };
        scene->factoryMgr().addObject(_factory);
        _attached   =   true;
        return  true;
    }

    void    FEEditorUi::detach()
    {
        /// <summary>
        /// 先断开回调与后端链接,避免工厂在后续帧继续访问已释放对象
        /// </summary>
        if (_factory != nullptr)
        {
            _factory->_buildUI  =   nullptr;
            _factory->invalidateBackend();
        }
        _factory    =   nullptr;
        _scene      =   nullptr;
        _iconFont   =   nullptr;
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

    void    FEEditorUi::drawRibbon()
    {
        ImGuiIO&    io      =   ImGui::GetIO();
        const float width   =   io.DisplaySize.x;
        if (width < 1.0f)
            return;

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
            /// 标签页行
            /// </summary>
            ImGui::SetCursorPos(ImVec2(6.0f,kTabRowY));
            auto    tab =   [this](const char* label,int index)
            {
                if (ribbonTab(label,_activeTab == index))
                    _activeTab  =   index;
                ImGui::SameLine(0.0f,2.0f);
            };
            tab("文件",0);
            tab("开始",1);
            tab("视图",2);
            tab("帮助",3);
            tab("界面",4);

            /// <summary>
            /// 右上角:程序名与帧率
            /// </summary>
            const   float   infoX   =   (width > 460.0f) ? (width - 196.0f) : 240.0f;
            ImGui::SetCursorPos(ImVec2(infoX,kTabRowY + 5.0f));
            ImGui::TextDisabled("FESceneEditor   %.1f FPS",io.Framerate);

            /// <summary>
            /// 功能区:组标题与组间竖线,纯绘制,不参与布局
            /// </summary>
            auto    frame       =   [&](float x,float groupW,const char* caption)
            {
                ImDrawList*     dl      =   ImGui::GetWindowDrawList();
                const   ImVec2  ts      =   ImGui::CalcTextSize(caption);
                dl->AddText(ImVec2(wp.x + x + (groupW - ts.x) * 0.5f,wp.y + kGroupRowY + kButtonH + 6.0f),
                            kTextMuted,caption);
                const   float   lineX   =   wp.x + x + groupW + 5.0f;
                dl->AddLine(ImVec2(lineX,wp.y + kGroupRowY - 2.0f),
                            ImVec2(lineX,wp.y + kGroupRowY + kButtonH + 16.0f),IM_COL32(255,255,255,26));
            };
            /// <summary>
            /// 在功能区指定位置放大按钮
            /// </summary>
            auto    bigAt       =   [this](float x,const char* id,const char* glyph,const char* label,bool selected)
            {
                ImGui::SetCursorPos(ImVec2(x,kGroupRowY + 2.0f));
                return  ribbonBigButton(id,_iconFont,glyph,label,selected);
            };

            switch (_activeTab)
            {
            /// <summary>
            /// 文件
            /// </summary>
            case 0:
                if (bigAt(8.0f,"###quit",kIconPower,"退出",false))
                {
                    if (_onQuit)
                        _onQuit();
                }
                frame(6.0f,76.0f,"文件");
                break;
            /// <summary>
            /// 视图
            /// </summary>
            case 2:
                if (bigAt(8.0f,"###view_grid",kIconGrid,"显示网格",_showGrid))
                    toggleGrid();
                frame(6.0f,76.0f,"显示");
                break;
            /// <summary>
            /// 帮助
            /// </summary>
            case 3:
                if (bigAt(8.0f,"###about",kIconInfo,"关于",false))
                    _aboutOpen  =   true;
                frame(6.0f,76.0f,"支持");
                break;
            /// <summary>
            /// 界面
            /// </summary>
            case 4:
                if (bigAt(8.0f,"###model_tree",kIconCube,"模型树",_treeOpen))
                    _treeOpen   =   !_treeOpen;
                frame(6.0f,76.0f,"面板");
                if (bigAt(94.0f,"###ui_preview",kIconWindow,"UI 预览",_uiPreviewOpen))
                    _uiPreviewOpen  =   !_uiPreviewOpen;
                frame(92.0f,76.0f,"界面");
                break;
            /// <summary>
            /// 开始
            /// </summary>
            default:
                if (bigAt(8.0f,"###home_grid",kIconGrid,"显示网格",_showGrid))
                    toggleGrid();
                frame(6.0f,76.0f,"场景");
                if (bigAt(94.0f,"###home_reset",kIconCamera,"重置视图",false))
                    resetCamera();
                frame(92.0f,76.0f,"相机");
                break;
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(3);

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

        drawModelTree();
        drawUiPreview();
    }

    void    FEEditorUi::drawModelTree()
    {
        if (!_treeOpen)
            return;
        ImGui::SetNextWindowPos(ImVec2(8.0f,kRibbonH + 8.0f),ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300.0f,420.0f),ImGuiCond_FirstUseEver);
        if (ImGui::Begin("模型树",&_treeOpen))
        {
            _modelTree.draw(_scene,_iconFont);
        }
        ImGui::End();
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
