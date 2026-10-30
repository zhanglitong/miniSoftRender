/// <summary>
/// FESceneEditor 主界面实现:
///   - ImGui 上下文 / 渲染后端 / 字体图集 / Overlay 工厂的一次性挂接;
///   - 以 bin/data/ui/MainWindow.ui 作为界面定义,Ribbon 功能区由其中的
///     菜单栏生成:主菜单为 Page,子菜单为分组,动作(QAction)为按钮;
///   - 鼠标消息转发,功能区被操作时宿主跳过场景处理。
/// </summary>

#include    "FEEditorUi.hpp"

#include    "FEIconGlyphs.hpp"
#include    "FEUiMetrics.hpp"
#include    "FEUiStrings.hpp"
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

#include    "FEEditorUiInternal.hpp"

using   namespace   FE::ui;
using   namespace   FE::ui::internal;

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
        ImFont* cnFont  =   io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\simsun.ttc",kDefaultFontSize,nullptr,
                                                          io.Fonts->GetGlyphRangesChineseFull());
        if (cnFont != nullptr)
            io.FontDefault  =   cnFont;
        /// <summary>
        /// 关于页标题:同一字体文件只加载拉丁字形,字号更大且不会撑大图集
        /// </summary>
        _titleFont  =   io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\simsun.ttc",
                                                     kAboutTitleFontSize);

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
            _shotDeadline   =   scriptClockMs() + kShotTimeoutMs;
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
        ImGui::SetNextWindowSize(ImVec2(kSettingWindowW,kSettingWindowH),ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(kSettingWindowX,kRibbonHeight + kSettingWindowYOff),
                               ImGuiCond_FirstUseEver);
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
        const   ImVec2  dlgSize =   ImVec2(std::min(kShortcutDlgW,display.x - kShortcutScreenPad),
                                           std::min(kShortcutDlgH,
                                                    std::max(kShortcutDlgMinH,
                                                             display.y - kRibbonHeight - kShortcutBottomPad)));
        const   float   posX    =   std::max(kShortcutPosPad,(display.x - dlgSize.x) * 0.5f);
        ImGui::SetNextWindowSize(dlgSize,ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(posX,kRibbonHeight + kShortcutYOff),ImGuiCond_FirstUseEver);
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
                                    +   ImGui::GetStyle().ItemSpacing.y + kShortcutFooterGap;
            ImGui::BeginChild("##shortcut_list",ImVec2(0.0f,-footerH),true);
            {
                /// <summary>
                /// 表头:动作 / 快捷键(两列居中,与参考图一致)
                /// </summary>
                const   float   split   =   ImGui::GetWindowContentRegionWidth() * kShortcutSplitRatio;
                const   float   rowH    =   ImGui::GetTextLineHeight() + kShortcutRowPadH;
                const   float   listW   =   ImGui::GetWindowContentRegionWidth();
                const   ImU32   gridCol =   kShortcutGrid;
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
                    dl->AddLine(ImVec2(pos.x,pos.y + rowH - kShortcutGridLineW),
                                ImVec2(pos.x + listW,pos.y + rowH - kShortcutGridLineW),gridCol);
                    dl->AddText(ImVec2(pos.x + kShortcutCellPadX,textY),color,item._text.c_str());
                    if (recording)
                    {
                        std::string preview =   "按下按键…";
                        if (_recordPressed && _recordKey != 0)
                            preview =   FEShortcutConfig::comboText(_recordKey,_recordCtrl,
                                                                    _recordShift,_recordAlt);
                        dl->AddText(ImVec2(pos.x + split + kShortcutCellPadX,textY),kAccent,preview.c_str());
                    }
                    else
                    {
                        const   std::string combo   =   _shortcuts.text(i);
                        dl->AddText(ImVec2(pos.x + split + kShortcutCellPadX,textY),
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
            const   float   closeW  =   ImGui::CalcTextSize(text::kClose).x + kShortcutBtnPadW;
            const   float   okW     =   ImGui::CalcTextSize(text::kConfirm).x + kShortcutBtnPadW;
            const   float   rightX  =   ImGui::GetWindowContentRegionMax().x - closeW - okW
                                     -   ImGui::GetStyle().ItemSpacing.x;
            ImGui::SameLine();
            ImGui::SetCursorPosX(rightX);
            if (ImGui::Button(text::kClose,ImVec2(closeW,0.0f)))
            {
                discardShortcutChanges();
                _shortcutOpen   =   false;
                open            =   false;
            }
            ImGui::SameLine();
            if (ImGui::Button(text::kConfirm,ImVec2(okW,0.0f)))
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
        /// 标题栏的 X 与text::kClose等价:丢弃未保存的改动
        /// </summary>
        if (!open && _shortcutOpen)
        {
            discardShortcutChanges();
            _shortcutOpen   =   false;
        }
    }

}
