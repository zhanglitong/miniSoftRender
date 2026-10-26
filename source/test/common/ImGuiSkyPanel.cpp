/// 公共调试 GUI:天空参数面板 + ImGui Overlay 工厂。
/// 供 test.sky 与 test.pbr.* 共用,各测试只需一次 attach/detach,
/// 是否显示由 FESkyComponent::panelVisible() 接口控制。

#include    "ImGuiSkyPanel.hpp"
#include    "../../inc/component/FESkyComponent.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/FEMessage.hpp"

#include    <imgui.h>
#include    <cmath>
#include    <iostream>

namespace   FE
{
    FEImGuiFactory::FEImGuiFactory(FEContext& ctx,FEImGuiBackend* backend)
        :FEFactoryRender(ctx)
        ,_backend(backend)
    {
        /// 在最后一层(Overlay)绘制:场景工厂默认归属 USAGE_Scene,本工厂放到
        /// 其后的 Overlay 层,保证 UI 绝对最后绘制、不被场景覆盖
        _usages =   ViewerUsage::USAGE_Overlay;
        riority(FEFactory::PT_Render).setPriority(EP_Last).setOrder(int16_t(32767));
        riority(FEFactory::PT_Update).setPriority(EP_Last).setOrder(int16_t(32767));
        /// 常驻工厂:场景 clear 时不销毁
        setResident(true);
    }

    void    FEImGuiFactory::update(CMDPtr cmd)
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

    void    FEImGuiFactory::render(CMDPtr cmd)
    {
        if (_backend == nullptr || cmd == nullptr)
            return;
        _backend->render(_drawData,cmd.get());
    }

    void    FEImGuiFactory::extractDrawData()
    {
        ImDrawData* dd  =   ImGui::GetDrawData();
        _cmds.clear();
        _vtx.clear();
        _idx.clear();
        if (dd == nullptr || dd->CmdListsCount == 0)
        {
            _drawData  =   FEImGuiDrawData();
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
            /// ImGui 1.65:ImDrawCmd 无 VtxOffset/IdxOffset 字段,同一 list 内各命令
            /// 按 ElemCount 顺序消费该 list 的索引,故需自行累加命令内的索引偏移
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
        /// ImGui 1.65 的 ImDrawData 无 FramebufferScale 字段,直接按 1:1 渲染
        _drawData._scaleX       =   1.0f;
        _drawData._scaleY       =   1.0f;
    }

    static  float   clampf(float v,float mn,float mx)
    {
        return  v < mn ? mn : (v > mx ? mx : v);
    }

    bool    FEImGuiSkyPanel::attach(FEContext& ctx,FEScene* scene,bool visible)
    {
        detach();
        if (scene == nullptr || !visible)
            return  false;

        ImGui::CreateContext();
        _hasContext =   true;
        ImGui::StyleColorsDark();
        ImGuiIO&    io  =   ImGui::GetIO();
        /// 不落盘 imgui.ini,避免测试产物污染工作目录
        io.IniFilename  =   nullptr;

        _backend    =   scene->device()->createImGuiBackend();
        if (_backend == nullptr || !_backend->init(scene->swapchainColorFormat()))
        {
            std::cerr << "create ImGui backend failed (sky panel disabled)" << std::endl;
            detach();
            return  false;
        }

        /// 上传字体图集(RGBA8);TexID 仅作占位,后端始终绑定该纹理。
        /// 先加载宋体作为默认字体以支持中文,失败时回退 ImGui 内置字体
        ImFont* cnFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\simsun.ttc", 16.0f, nullptr,
                                                      io.Fonts->GetGlyphRangesChineseFull());
        if (cnFont)
            io.FontDefault = cnFont;

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

        /// 天空节点由场景默认持有;回调只保存裸指针,场景销毁时工厂一并销毁
        FESkyComponent* sky =   scene->sky();
        if  (sky != nullptr)
        {
            /// 用天空当前参数初始化滑条,避免面板显示值与实际状态不一致
            const   UBOSky& u   =   sky->sky();
            _turbidity      =   u._params.x;
            _rayleigh       =   u._params.y;
            _mie            =   u._params.z;
            _mieG           =   u._params.w;
            _luminance      =   u._sunDirection.w;
            _cloudCoverage  =   u._cloud.x;
            _cloudDensity   =   u._cloud.y;
            _cloudElevation =   u._cloud.z;
            _cloudSpeed     =   u._anim.z;
            _elevation      =   asinf(clampf(u._sunDirection.z,-1.0f,1.0f)) * (180.0f / float(PI));
            _azimuth        =   atan2f(u._sunDirection.x,u._sunDirection.y) * (180.0f / float(PI));
            if (_azimuth < 0.0f)
                _azimuth    +=  360.0f;
            _darkWhenSunHidden  =   sky->darkWhenSunHidden();
        }
        _factory    =   new FEImGuiFactory(ctx,_backend);
        _factory->_buildUI  =   [this,sky]() {   this->draw(sky);   };
        scene->factoryMgr().addObject(_factory);
        _attached   =   true;
        return  true;
    }

    void    FEImGuiSkyPanel::detach()
    {
        /// 先断开回调与后端链接,避免工厂在后续帧继续访问已释放对象
        if (_factory != nullptr)
        {
            _factory->_buildUI  =   nullptr;
            _factory->invalidateBackend();
        }
        _factory    =   nullptr;
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

    bool    FEImGuiSkyPanel::onMessage(const FEMessage& msg)
    {
        if (!_attached || !_hasContext)
            return  false;

        ImGuiIO&        io      =   ImGui::GetIO();
        const   auto    msgId   =   msg.msgId();
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
        default:
            break;
        }
        /// GUI 捕获鼠标时不把鼠标消息交给场景,避免拖动滑条同时旋转相机
        if (io.WantCaptureMouse)
        {
            switch (msgId)
            {
            case MSG_MOUSE_MOVE:
            case MSG_LBUTTON_DOWN:
            case MSG_LBUTTON_UP:
            case MSG_RBUTTON_DOWN:
            case MSG_RBUTTON_UP:
            case MSG_MBUTTON_DOWN:
            case MSG_MBUTTON_UP:
            case MSG_MOUSE_WHEEL:
                block   =   true;
                break;
            default:
                break;
            }
        }
        return  block;
    }

    void    FEImGuiSkyPanel::draw(FESkyComponent* sky)
    {
        /// 代码接口控制显隐:panelVisible()==false 时整个面板不绘制,
        /// ImGui 上下文与后端仍保留,可随时通过接口重新打开
        if (sky == nullptr || !sky->panelVisible())
            return;

        /// 自动模式下太阳由组件推进,读回当前角度避免与滑条互相覆盖
        if (_autoSun)
        {
            const UBOSky&   u   =   sky->sky();
            _elevation  =   asinf(clampf(u._sunDirection.z,-1.0f,1.0f)) * (180.0f / float(PI));
            _azimuth    =   atan2f(u._sunDirection.x,u._sunDirection.y) * (180.0f / float(PI));
            if (_azimuth < 0.0f)
                _azimuth    +=  360.0f;
        }

        /// 固定在画面右侧,避开控制台窗口
        float   posX    =   ImGui::GetIO().DisplaySize.x - 340.0f;
        if  (posX < 0.0f)
            posX    =   0.0f;
        ImGui::SetNextWindowPos(ImVec2(posX,100.0f),ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(320.0f,470.0f),ImGuiCond_Always);
        ImGui::Begin(u8"天空参数（运行时）");
        if (ImGui::SliderFloat(u8"浑浊度##turbidity",&_turbidity,1.0f,20.0f,"%.2f"))
            sky->setTurbidity(_turbidity);
        if (ImGui::SliderFloat(u8"瑞利散射##rayleigh",&_rayleigh,0.0f,5.0f,"%.2f"))
            sky->setRayleigh(_rayleigh);
        if (ImGui::SliderFloat(u8"米氏系数##mie",&_mie,0.0f,0.02f,"%.5f"))
            sky->setMieCoefficient(_mie);
        if (ImGui::SliderFloat(u8"米氏方向 G##mieg",&_mieG,0.0f,1.0f,"%.3f"))
            sky->setMieDirectionalG(_mieG);
        if (ImGui::SliderFloat(u8"曝光##exposure",&_luminance,0.0f,1.0f,"%.3f"))
            sky->setLuminance(_luminance);

        /// ---- 云(命名与 three.js webgpu_sky 示例面板一致)----
        ImGui::Separator();
        ImGui::Text(u8"云");
        if (ImGui::SliderFloat(u8"云量##coverage",&_cloudCoverage,0.0f,1.0f,"%.2f"))
            sky->setCloudCoverage(_cloudCoverage);
        if (ImGui::SliderFloat(u8"云密度##density",&_cloudDensity,0.0f,1.0f,"%.2f"))
            sky->setCloudDensity(_cloudDensity);
        if (ImGui::SliderFloat(u8"云层高度##celev",&_cloudElevation,0.0f,1.0f,"%.2f"))
            sky->setCloudElevation(_cloudElevation);
        if (ImGui::SliderFloat(u8"云速##cspeed",&_cloudSpeed,0.0f,0.001f,"%.5f"))
            sky->setCloudSpeed(_cloudSpeed);

        ImGui::Separator();
        if (ImGui::Checkbox(u8"自动太阳",&_autoSun))
            sky->setAutoSun(_autoSun,_elevation,_autoSpeed);
        if (ImGui::Checkbox(u8"夜间环境压暗",&_darkWhenSunHidden))
            sky->setDarkWhenSunHidden(_darkWhenSunHidden);

        if (ImGui::SliderFloat(u8"太阳高度角##elev",&_elevation,-10.0f,90.0f,"%.1f"))
        {
            _autoSun    =   false;
            sky->setAutoSun(false);
            sky->setSunElevationAzimuth(_elevation,_azimuth);
        }
        if (ImGui::SliderFloat(u8"太阳方位角##azim",&_azimuth,0.0f,360.0f,"%.1f"))
            sky->setSunElevationAzimuth(_elevation,_azimuth);
        if (ImGui::SliderFloat(u8"自动速度##speed",&_autoSpeed,0.5f,30.0f,"%.1f"))
        {
            if (_autoSun)
                sky->setAutoSun(true,_elevation,_autoSpeed);
        }

        if (ImGui::Button(u8"复位"))
        {
            _turbidity  =   10.0f;  _rayleigh   =   3.0f;  _mie    =   0.005f;
            _mieG       =   0.7f;   _luminance  =   0.05f;
            _elevation  =   65.0f;  _azimuth    =   0.0f;
            _darkWhenSunHidden  =   true;
            _cloudCoverage  =   0.4f;   _cloudDensity   =   0.4f;
            _cloudElevation =   0.5f;   _cloudSpeed     =   0.00002f;
            sky->setTurbidity(_turbidity);
            sky->setRayleigh(_rayleigh);
            sky->setMieCoefficient(_mie);
            sky->setMieDirectionalG(_mieG);
            sky->setLuminance(_luminance);
            sky->setDarkWhenSunHidden(_darkWhenSunHidden);
            sky->setSunElevationAzimuth(_elevation,_azimuth);
            sky->setCloudCoverage(_cloudCoverage);
            sky->setCloudDensity(_cloudDensity);
            sky->setCloudElevation(_cloudElevation);
            sky->setCloudSpeed(_cloudSpeed);
        }

        ImGui::Text(u8"帧率 %.1f",ImGui::GetIO().Framerate);
        ImGui::End();
    }
}
