/// FEViewer
/// 通用模型/场景浏览器:
///   1. 命令行传入模型文件(.gltf/.glb)或场景工程文件(.fepj),启动后直接显示
///   2. 鼠标左键环绕、右键平移、滚轮缩放(由场景内置 FESceneBrowse 处理)
///   3. 快捷键:F 重新构图 / R 复位 / E 切换 HDR 环境 / G 显示隐藏网格
///   4. --register / --unregister 把 .fepj 关联到本程序(HKCU,无需管理员权限),
///      关联后双击 .fepj 即用 FEViewer 打开
/// 用法:
///   FEViewer.exe <model.gltf|model.glb|scene.fepj> [--vulkan|--webgpu]
///                [--msaa 1|2|4|8] [--env <name|path>] [--bg r,g,b] [--grid]
///   FEViewer.exe --register
///   FEViewer.exe --unregister

#include    <iostream>
#include    <string>
#include    <vector>
#include    <algorithm>
#include    <cstdio>
#include    <cstring>
#include    <filesystem>

#ifdef  _WIN32
#include    <windows.h>
#include    <shellapi.h>
#include    <shlobj.h>
#endif

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEFileFormatRegistry.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/graphic/FELightDir.h"
#include    "../../../inc/FEFileFormatHelper.hpp"
#include    "../../../inc/animation/FEAnimationSys.hpp"
#include    "../../../inc/node/FESkin.hpp"
#include    "../../../inc/FETaskMgr.hpp"
#include    "../../../inc/graphic/FEProgressBar.hpp"
#include    "../../../inc/material/FEMaterialPBR.hpp"
#include    "../../../inc/component/FESkyComponent.hpp"
#include    "../../../core/FEImageHelper.hpp"
#include    "../../../core/graphic/FETextureHelper.hpp"
#include    "cmdline.h"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    /// 可切换的 HDR 环境(bin/data/env 下)
    struct  EnvEntry
    {
        const char* name;
        const char* file;
    };
    const   EnvEntry    kEnvs[] =
    {
        { "pisa",            "/data/env/pisa.hdr"            },
        { "Cannon_Exterior", "/data/env/Cannon_Exterior.hdr" },
        { "Colorful_Studio", "/data/env/Colorful_Studio.hdr" },
    };
    constexpr int   kEnvCount   =   int(sizeof(kEnvs) / sizeof(kEnvs[0]));

    struct  ViewerOptions
    {
        String  _file;
        FEUuid  _rendererId =   RS_VULKAN;
        uint    _sampleCount    =   4;
        String  _env            =   "Cannon_Exterior";
        float4  _clearColor     =   float4(0.22f, 0.22f, 0.22f, 1.0f);
        bool    _showGrid       =   true;
        bool    _regAssociation =   false;
        bool    _unregAssociation   =   false;
        bool    _help           =   false;
    };

    String  toUtf8(const wchar_t* wstr)
    {
        if (wstr == nullptr)
            return  String();
#ifdef  _WIN32
        int     len =   WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
        if (len <= 1)
            return  String();
        String  out(size_t(len - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr, -1, out.data(), len, nullptr, nullptr);
        return  out;
#else
        return  String(wstr);
#endif
    }

    String  toLower(String text)
    {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch)
        {
            return  char(std::tolower(ch));
        });
        return  text;
    }

    String  fileExtension(const String& path)
    {
        const size_t    dot =   path.find_last_of('.');
        if (dot == String::npos)
            return  String();
        return  toLower(path.substr(dot));
    }

    bool    fileExists(const String& path)
    {
        FILE*   fp  =   fopen(path.c_str(), "rb");
        if (fp == nullptr)
            return  false;
        fclose(fp);
        return  true;
    }

    /// 递归把 IBL 资源挂到所有 PBR 材质上
    void    applyEnvMap(const Node& node, TexPtr env, TexPtr irradiance, TexPtr brdfLUT, FEScene* scene)
    {
        if (node == nullptr)
            return;
        auto    mat =   node->material();
        if (mat != nullptr)
        {
            auto    pbr =   mat->cast<FEMaterialPBR>();
            if (pbr != nullptr)
            {
                pbr->setEnvironmentMap(env, irradiance);
                pbr->setBRDFLUT(brdfLUT);
                if (scene != nullptr && scene->sceneTexture())
                    pbr->setSceneTexture(scene->sceneTexture());
            }
        }
        for (auto& child : node->children())
            applyEnvMap(child, env, irradiance, brdfLUT, scene);
    }

    /// 加载 HDR 并生成 IBL 资源(镜面预滤波 cubemap + 辐照度 cubemap + BRDF LUT)
    bool    applyEnvironment(FEContext& ctx, const Nodes& rootNodes, const String& hdrFile,
                             TexPtr& envCube, TexPtr& irrCube, TexPtr& brdfLUT, FEScene* scene)
    {
        ImagePtr    envImage    =   FEImageHelper::load(ctx, hdrFile.c_str());
        if (envImage == nullptr)
        {
            std::cerr << "load environment hdr failed: " << hdrFile << "\n";
            return  false;
        }
        envCube =   FETextureHelper::createCube(ctx, *envImage, 256);
        if (envCube == nullptr)
        {
            std::cerr << "create environment cube failed\n";
            return  false;
        }
        irrCube =   FETextureHelper::createIrradianceCube(ctx, *envImage, 32);
        if (irrCube == nullptr)
        {
            std::cerr << "create irradiance cube failed\n";
            return  false;
        }
        if (brdfLUT == nullptr)
        {
            brdfLUT =   FETextureHelper::createBRDFLUT(ctx, 512);
            if (brdfLUT == nullptr)
            {
                std::cerr << "create brdf lut failed\n";
                return  false;
            }
        }
        for (auto& node : rootNodes)
            applyEnvMap(node, envCube, irrCube, brdfLUT, scene);
        return  true;
    }

    aabb3dr computeAabb(const Nodes& rootNodes)
    {
        aabb3dr box;
        for (auto& node : rootNodes)
        {
            if (node == nullptr)
                continue;
            box.merge(node->globalAabb());
        }
        return  box;
    }

    /// 按当前模型包围盒重新构图相机,并把场景锚点放到包围盒中心
    void    frameCamera(FEContext& ctx, FEScene* scene, const Nodes& rootNodes)
    {
        if (scene == nullptr || rootNodes.empty())
            return;
        auto    cam =   scene->camera();
        if (cam == nullptr)
            return;
        aabb3dr box =   computeAabb(rootNodes);
        /// 正视构图:相机在模型正前方(+Z 轴),视线正对包围盒中心,up 为 +Y。
        /// 不用 lookAtTo:它的 FaceDirs 方向表会把相机放到模型下方,看到车底
        const   real3   center  =   box.center();
        const   real    radius  =   length(box.getSize()) * real(0.5);
        const   real    dist    =   radius / sin((std::max)(cam->getFov(),real(0.1)) * real(0.5)) * real(1.15);
        const   real3   eye     =   center + real3(0.0f,0.0f,1.0f) * dist;
        cam->setEye(eye);
        cam->setTarget(center);
        cam->setUp(real3(0.0f,1.0f,0.0f));
        cam->update();

        real3   size    =   box.getSize();
        real    maxDim  =   size.x > size.y ? size.x : size.y;
        if (size.z > maxDim)
            maxDim  =   size.z;
        if (maxDim <= real(0))
            maxDim  =   real(1);
        cam->setNear(maxDim * real(0.001));
        cam->setFar (maxDim * real(100));
        cam->updatePerspective();

        FEPickup    pick;
        pick.point  =   box.center();
        ctx.anchor().setPickup(pick);
        ctx.anchor().fireNotify();
    }

#ifdef  _WIN32
    WString executablePath()
    {
        wchar_t buffer[MAX_PATH * 2] =   {};
        DWORD   len =   GetModuleFileNameW(nullptr, buffer, DWORD(sizeof(buffer) / sizeof(buffer[0])));
        return  WString(buffer, len);
    }

    bool    setRegistryString(HKEY root, const wchar_t* subKey, const wchar_t* valueName, const std::wstring& value)
    {
        HKEY    key =   nullptr;
        if (RegCreateKeyExW(root, subKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
            return  false;
        const   LONG    result  =   RegSetValueExW(key, valueName, 0, REG_SZ,
            reinterpret_cast<const BYTE*>(value.c_str()),
            DWORD((value.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(key);
        return  result == ERROR_SUCCESS;
    }

    /// 把 .fepj 关联到当前 FEViewer.exe(HKEY_CURRENT_USER,不需要管理员权限)
    bool    registerFepjAssociation()
    {
        const   std::wstring    exe     =   executablePath();
        const   std::wstring    command =   L"\"" + exe + L"\" \"%1\"";
        const   std::wstring    icon    =   L"\"" + exe + L"\",0";
        bool    ok  =   true;
        ok  =   setRegistryString(HKEY_CURRENT_USER, L"Software\\Classes\\.fepj", nullptr, L"FEViewer.fepj") && ok;
        ok  =   setRegistryString(HKEY_CURRENT_USER, L"Software\\Classes\\FEViewer.fepj", nullptr, L"FEPJ Scene") && ok;
        ok  =   setRegistryString(HKEY_CURRENT_USER, L"Software\\Classes\\FEViewer.fepj\\DefaultIcon", nullptr, icon) && ok;
        ok  =   setRegistryString(HKEY_CURRENT_USER, L"Software\\Classes\\FEViewer.fepj\\shell\\open\\command", nullptr, command) && ok;
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
        return  ok;
    }

    bool    unregisterFepjAssociation()
    {
        RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\FEViewer.fepj");
        RegDeleteKeyW (HKEY_CURRENT_USER, L"Software\\Classes\\.fepj");
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
        return  true;
    }
#endif

    void    printUsage()
    {
        std::cout
            << "FEViewer - miniSoftRender model/scene viewer\n"
            << "\n"
            << "Usage:\n"
            << "  FEViewer.exe <model.gltf|model.glb|scene.fepj> [options]\n"
            << "  FEViewer.exe --register      associate .fepj with FEViewer (current user)\n"
            << "  FEViewer.exe --unregister    remove the .fepj association\n"
            << "\n"
            << "Options:\n"
            << "  --vulkan | --webgpu    render backend (default: vulkan)\n"
            << "  --msaa <1|2|4|8>       MSAA sample count (default: 4)\n"
            << "  --env <name|path>      HDR environment: pisa / Cannon_Exterior /\n"
            << "                         Colorful_Studio, or a .hdr file path\n"
            << "  --bg <r,g,b>           clear color in 0..1 (default: 0.22,0.22,0.22)\n"
            << "  --grid                 show the editor grid (default: shown)\n"
            << "\n"
            << "Keys:\n"
            << "  F / R  frame model        E  cycle HDR environment\n"
            << "  G      toggle grid\n";
    }

    /// <summary>
    /// 解析命令行(cmdline.h):第一个位置参数是要打开的文件,
    /// 其余为 --vulkan/--webgpu/--msaa/--env/--bg/--grid/--register/--unregister
    /// </summary>
    bool    parseOptions(const Strings& args, ViewerOptions& options)
    {
        CMDLine::Parser  parser;
        parser.add("help",      'h', "print this message");
        parser.add("vulkan",     0,  "use Vulkan backend (default)");
        parser.add("webgpu",     0,  "use WebGPU backend");
        parser.add("msaa",       0,  "MSAA sample count: 1|2|4|8", false, uint(4),
                   CMDLine::oneof<uint>(1, 2, 4, 8));
        parser.add("env",        0,  "HDR environment: pisa / Cannon_Exterior / Colorful_Studio / .hdr path",
                   false, String("Cannon_Exterior"));
        parser.add("bg",         0,  "clear color as r,g,b in 0..1", false, String("0.22,0.22,0.22"));
        parser.add("grid",       0,  "show the editor grid (default)");
        parser.add("register",   0,  "associate .fepj with FEViewer (current user)");
        parser.add("unregister", 0,  "remove the .fepj association");
        /// 解析器约定第 0 个参数是程序名,这里补上
        CMDLine::Strings    cli;
        cli.reserve(args.size() + 1);
        cli.push_back("FEViewer");
        cli.insert(cli.end(), args.begin(), args.end());
        if (!parser.parse(cli))
        {
            std::cerr << parser.error() << std::endl;
            return  false;
        }

        options._help           =   parser.exist("help");
        options._rendererId     =   parser.exist("webgpu") ? RS_WEBGPU : RS_VULKAN;
        options._sampleCount    =   parser.get<uint>("msaa");
        options._env            =   parser.get<String>("env");
        options._showGrid       =   true;
        options._regAssociation =   parser.exist("register");
        options._unregAssociation   =   parser.exist("unregister");

        /// 背景色:文本 r,g,b 转换失败时沿用默认值
        float   r =   0.22f, g =   0.22f, b =   0.22f;
        if (sscanf_s(parser.get<String>("bg").c_str(), "%f,%f,%f", &r, &g, &b) == 3)
            options._clearColor =   float4(r, g, b, 1.0f);

        /// 位置参数只取第一个作为待打开文件
        const   auto&   rest    =   parser.rest();
        if (!rest.empty())
            options._file   =   rest.front();

        return  options._help || options._regAssociation || options._unregAssociation ||
                !options._file.empty();
    }
}

int main(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);

    Strings args;
#ifdef  _WIN32
    int     wargc   =   0;
    LPWSTR* wargv   =   CommandLineToArgvW(GetCommandLineW(), &wargc);
    if (wargv != nullptr)
    {
        for (int i = 1; i < wargc; ++i)
            args.push_back(toUtf8(wargv[i]));
        LocalFree(wargv);
    }
#else
    for (int i = 1; i < argc; ++i)
        args.push_back(argv[i]);
#endif

    ViewerOptions   options;
    if (!parseOptions(args, options))
    {
        printUsage();
        return  1;
    }
    if (options._help)
    {
        printUsage();
        return  0;
    }
#ifdef  _WIN32
    if (options._regAssociation)
    {
        if (registerFepjAssociation())
            std::cout << ".fepj association registered for FEViewer\n";
        else
            std::cerr << "register .fepj association failed\n";
        return  0;
    }
    if (options._unregAssociation)
    {
        unregisterFepjAssociation();
        std::cout << ".fepj association removed\n";
        return  0;
    }
#else
    if (options._regAssociation || options._unregAssociation)
    {
        std::cerr << "file association is only supported on Windows\n";
        return  1;
    }
#endif

    /// 命令行参数是 UTF-8(来自 CommandLineToArgvW),转成本机窄编码后统一使用
    options._file   =   std::filesystem::u8path(options._file).string();

    String      extension   =   fileExtension(options._file);
    /// 支持哪些格式由注册表决定,不在调用方写死扩展名
    FEContext&  ctx         =   FEContext::instance();
    FEFileFormatRegistry    formats(ctx);
    auto        fmt         =   formats.reader(extension);
    if (fmt == nullptr)
    {
        std::cerr << "unsupported file type: " << options._file << "\n"
                  << "supported: " << formats.readerExtensionsText() << "\n";
        return  1;
    }
    if (!fileExists(options._file))
    {
        std::cerr << "file not found: " << options._file << "\n";
        return  1;
    }

    Scene       scene      =   nullptr;
    Nodes       rootNodes;
    TexPtr      envCube;
    TexPtr      irrCube;
    TexPtr      brdfLUT;
    bool        envReady    =   false;
    PrgBar      loadBar     =   nullptr;
    String      envFile;
    int         envIndex    =   -1;
    for (int i = 0; i < kEnvCount; ++i)
    {
        if (_stricmp(options._env.c_str(), kEnvs[i].name) == 0)
        {
            envIndex    =   i;
            break;
        }
    }

    FEApp::CreateInfo info{};
    info._width     =   1280;
    info._height    =   720;
    info._appInst   =   GetModuleHandle(nullptr);
    info._notify    =   [&](const FEMessage& msg)
    {
        if (scene != nullptr)
            scene->onMessage(msg);
        if (msg.msgId() != MSG_KEYDOWN || scene == nullptr)
            return;
        const   auto&   key =   static_cast<const MsgKeyDown&>(msg)._info._key;
        if (key == 'F' || key == 'R')
        {
            frameCamera(ctx, scene, rootNodes);
        }
        else if (key == 'E' && !rootNodes.empty())
        {
            envIndex    =   (envIndex + 1) % kEnvCount;
            envFile     =   ctx.resourcePath() + kEnvs[envIndex].file;
            /// IBL 生成/旧纹理释放与后台加载线程的纹理上传互斥
            FEMutex::ScopeLock  lock(ctx.device().gpuMutex());
            if (applyEnvironment(ctx, rootNodes, envFile, envCube, irrCube, brdfLUT, scene))
            {
                envReady    =   true;
                std::cout << "environment: " << kEnvs[envIndex].name << "\n";
            }
        }
        else if (key == 'G' && scene != nullptr)
        {
            options._showGrid   =   !options._showGrid;
            scene->showGrid(options._showGrid);
        }
    };

    App app =   FEAppHelper::create(ctx, info);
    if (!app)
    {
        std::cerr << "create app failed\n";
        return  1;
    }
    ctx.setWindow(app.get());
    ctx.setWorkPath(app->path());
    ctx.setResourcePath(app->path() + "/../");

    scene   =   new FEScene(ctx);
    scene->setSampleCount(options._sampleCount);
    if (!scene->setup(app, options._rendererId))
    {
        std::cerr << "scene setup failed\n";
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 查看器不需要天空参数面板
    if (scene->sky() != nullptr)
        scene->sky()->setPanelVisible(false);
    scene->showGrid(options._showGrid);
    scene->setClearColor(options._clearColor);

    /// 调亮默认平行光,保证没有 IBL 的材质也可见
    {
        Light   mainLight;
        if (ctx.device().lights().query("main", mainLight) && mainLight)
        {
            auto    dir =   mainLight->as<FELightDir>();
            if (dir != nullptr)
            {
                dir->_color =   float3(2.0f, 2.0f, 2.0f);
                dir->_dir   =   float3(0.4f, 0.7f, 0.8f);
            }
        }
    }
    /// ---- 环境 HDR 路径(加载任务完成时也要用,先算好) ----
    if (envIndex >= 0)
        envFile =   ctx.resourcePath() + kEnvs[envIndex].file;
    else if (options._env.size() > 1 && (options._env[0] == '/' || options._env[1] == ':'))
        envFile =   options._env;
    else
        envFile =   ctx.resourcePath() + options._env;
    FETaskMgr   taskMgr(1);
    /// 释放顺序:先停工作线程(等待正在执行或等待主线程派发的任务返回),再释放 main 持有的 GPU
    /// 资源,最后场景销毁 device。stop() 之前必须关闭场景的可执行队列:完成回调可能正阻塞在
    /// sendRunable 里等主线程执行,不丢弃挂起项的话 join 会互相等待
    app->messageBus().subscribe(MSG_CLOSE, [&](const FEMessage&)
    {
        scene->closeRunables();
        taskMgr.stop();
        if (loadBar != nullptr)
        {
            loadBar->bindProgress(nullptr);
            loadBar->setVisible(false);
        }
        envCube =   nullptr;
        irrCube =   nullptr;
        brdfLUT =   nullptr;
        rootNodes.clear();
    });

    /// ---- 打开模型或工程:格式由注册表按扩展名提供,统一走读取器加载 ----
    {
        /// 引擎文件接口按本机窄编码解释文件名(已在上方统一转换)
        Strings files       =   { options._file };
        Task    loadTask    =   new FETask(ctx);
        loadTask->setName("LoadModel");
        /// 主线程先取好 reader:进度条直接轮询它的进度(reader 每阶段上报),
        /// 任务本身只在完成时才有结果码
        auto    reader      =   FEFileFormatHelper::queryReader(ctx, *fmt);
        /// 工作线程:执行加载(纹理创建/上传与主线程渲染由设备锁串行化)
        loadTask->setExec([&ctx, files, reader](FETask& task) -> bool
        {
            if (reader == nullptr)
            {
                task.setCode(-1);
                return  true;
            }
            /// const 拷贝(SharedPtr 的 const operator-> 返回 const 指针)
            auto    streamReader    =   reader;
            Objects objects =   streamReader->readFiles(files);
            if (objects.empty())
            {
                task.setCode(-2);
                return  true;
            }
            task.setObjects(std::move(objects));
            task.setCode(0);
            return  true;
        });
        /// 进度条:场景创建时已建好(常驻、默认隐藏),这里按接口取回后启用
        loadBar =   scene->progressBar();
        if (loadBar != nullptr)
        {
            loadBar->setVisible(true);
            loadBar->bindProgress(reader.get());
        }
        /// 工作线程:完成回调只做投递,真正的落地动作在主线程执行
        loadTask->setFinish([&](FETask& task)
        {
            QueueItems  items;
            QueueItem   item    =   new FEQueueItem(ctx);
            item->_runable  +=  [&]
            {
                /// 加载结束(成功或失败)立即隐藏进度条并解除引用
                if (loadBar != nullptr)
                {
                    loadBar->bindProgress(nullptr);
                    loadBar->setVisible(false);
                }
                if (task.code() < 0 || task.objects().empty())
                {
                    std::cerr << "load model failed: " << options._file << "\n";
                    return;
                }
                /// 读取器返回的根对象:节点进场景,播放器(fepj)登记回动画系统
                for (auto var : task.objects())
                {
                    Node    node        =   var->cast<FENode>();
                    if (node != nullptr)
                    {
                        node->flags().addFlag(FE::FLAG_UPDATE);
                        node->update();
                        rootNodes.push_back(node);
                        continue;
                    }
                    auto    playback    =   var->cast<FEPlayback>();
                    if (playback != nullptr)
                    {
                        if (auto sys = scene->animationSystem())
                            sys->addPlayback(playback);
                    }
                }
                if (rootNodes.empty())
                {
                    std::cerr << "load model failed: " << options._file << "\n";
                    return;
                }
                scene->dispatchNodesToSystem(rootNodes);
                scene->addNodesToTree(rootNodes);
                envReady    =   applyEnvironment(ctx, rootNodes, envFile, envCube, irrCube, brdfLUT, scene.get());
                frameCamera(ctx, scene.get(), rootNodes);

                std::cout << "FEViewer ready: " << options._file << "\n"
                          << "LButton orbit / RButton pan / Wheel zoom, F frame, E environment, G grid\n";
            };
            items.emplace_back(item);
            /// 主线程执行并等待完成:投递项在场景帧首、设备锁内执行,可直接操作场景与 GPU
            scene->sendRunable(std::move(items));
        });
        taskMgr.addTask(loadTask);
        std::cout << "loading: " << options._file << "\n";
    }

    app->run();
    /// 兜底:确保工作线程在场景/设备销毁前结束(sendRunable 可能仍在等主线程,先关队列)
    if (scene != nullptr)
        scene->closeRunables();
    taskMgr.stop();

    scene   =   nullptr;
    app->destroy();
    return  0;
}
