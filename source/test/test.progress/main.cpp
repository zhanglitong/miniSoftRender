/// test.progress
/// 不依赖 ImGui 的屏幕进度条(FEProgressBar):
///   默认: 打开渲染窗口,进度条在底部居中显示并随帧循环推进(0~100% 循环),
///         百分比文本随进度实时更新,可直接观察最终效果。
///   --headless: 离屏像素校验(256x256,纯黑背景,关闭天空与网格):
///     1. 分别以 0% / 100% / 50% 渲染,统计进度条区域内的绿色填充像素:
///          - 0%   : 填充像素为 0
///          - 100% : 填充覆盖整个内宽,像素数 > 800
///          - 50%  : 填充像素约为 100% 时的一半
///     2. 50% 时进度条上方应出现白色百分比文本像素(SDF 文本)
///     3. setProgress 的钳制行为(负数->0,大于 1->1)
/// 用法: test.progress.exe [--vulkan|--webgpu] [--headless]

#include    <iostream>
#include    <cstdlib>
#include    <filesystem>
#include    <Windows.h>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/FEMessage.hpp"
#include    "../../../inc/graphic/FEOffscreenRenderer.h"
#include    "../../../inc/graphic/FEProgressBar.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    int     g_pass  =   0;
    int     g_fail  =   0;

    void    check(bool ok,const char* label)
    {
        if (ok)     {   ++  g_pass;     printf("[PASS] %s\n",label);    }
        else        {   ++  g_fail;     printf("[FAIL] %s\n",label);    }
        fflush(stdout);
    }
    String  exePath()
    {
        char    szTemp[2048]    =   {};
        GetModuleFileNameA(nullptr,szTemp,sizeof(szTemp));
        return  std::filesystem::path(szTemp).parent_path().string();
    }
    /// <summary>
    /// 统计绿色(填充)与白色(百分比文本)像素数量
    /// </summary>
    struct  PixelCount
    {
        size_t  _green  =   0;
        size_t  _white  =   0;
    };
    PixelCount  countPixels(const uint8s& pixels,int width,int yBegin,int yEnd)
    {
        PixelCount  result;
        if (pixels.empty() || width <= 0)
            return  result;
        const   int height  =   int(pixels.size() / 4) / width;
        for (int y = (std::max)(yBegin,0);y < (std::min)(yEnd,height);++y)
        {
            for (int x = 0;x < width;++x)
            {
                const   size_t  i   =   (size_t(y) * width + x) * 4;
                if (i + 2 >= pixels.size())
                    continue;
                const   int r   =   pixels[i];
                const   int g   =   pixels[i + 1];
                const   int b   =   pixels[i + 2];
                /// 填充色 (0.2, 0.85, 0.35) 明显偏绿
                if (g > 120 && g > r + 40 && g > b + 40)
                    ++result._green;
                /// 白色百分比文本(含描边内部的高亮像素)
                if (r > 150 && g > 150 && b > 150)
                    ++result._white;
            }
        }
        return  result;
    }
    /// <summary>
    /// 离屏像素校验:进度条填充比例、百分比文本与钳制行为
    /// </summary>
    int     runHeadless(FEContext& ctx,const FEUuid& rendererId)
    {
        const   String  path    =   exePath();
        printf("=== test.progress (headless): ImGui-free progress bar ===\n");

        OffscreenRenderer   renderer    =   new FEOffscreenRenderer(ctx);
        FEOffscreenRenderer::CreateInfo renderInfo  =   {};
        renderInfo._width       =   256;
        renderInfo._height      =   256;
        renderInfo._rendererId  =   rendererId;
        renderInfo._workPath    =   path;
        renderInfo._resourcePath=   path + "/../";
        if (!renderer->create(renderInfo))
        {
            std::cerr << "FEOffscreenRenderer::create failed" << std::endl;
            renderer    =   nullptr;
            return  1;
        }
        Scene   scene   =   renderer->scene();
        scene->showSky(false);
        scene->showGrid(false);
        scene->setClearColor(float4(0,0,0,1));

        /// ---- 1. 创建进度条并挂到场景 ----
        auto    bar =   new FEProgressBar(ctx);
        scene->factoryMgr().addObject(bar);
        check(bar->progress() == 0.0f,"bar: initial progress is 0");

        /// 位置参数:256x256 屏幕,底部居中,60% 宽,10px 高,底部边距 32px
        ///   bar: x≈51..205, y≈214..224;文本在进度条上方
        const   int barTop      =   210;
        const   int barBottom   =   230;
        const   int textTop     =   150;

        /// ---- 2. 0% ----
        bar->setProgress(0.0f);
        check(renderer->render(2),"render: 0%");
        PixelCount  zero    =   countPixels(renderer->pixels(),256,barTop,barBottom);
        printf("       0%%  bar green pixels = %zu\n",zero._green);
        check(zero._green <= 2,"bar: no fill at 0%");

        /// ---- 3. 100% ----
        bar->setProgress(1.0f);
        check(renderer->render(2),"render: 100%");
        PixelCount  full    =   countPixels(renderer->pixels(),256,barTop,barBottom);
        printf("       100%% bar green pixels = %zu\n",full._green);
        check(full._green > 800,"bar: fill covers most of the bar at 100%");

        /// ---- 4. 50% ----
        bar->setProgress(0.5f);
        check(renderer->render(2),"render: 50%");
        check(std::abs(bar->progress() - 0.5f) < 1e-5f,"bar: progress() returns 0.5");
        const   uint8s  pixels  =   renderer->pixels();
        PixelCount  half    =   countPixels(pixels,256,barTop,barBottom);
        printf("       50%%  bar green pixels = %zu\n",half._green);
        check(half._green > 300,"bar: fill visible at 50%");
        if (full._green > 0)
        {
            const   real    ratio   =   real(half._green) / real(full._green);
            printf("       fill ratio 50%%/100%% = %.3f\n",double(ratio));
            check(ratio > 0.4f && ratio < 0.6f,"bar: 50% fill is about half of 100%");
        }

        /// ---- 5. 百分比文本(50% 时进度条上方应有白色文本像素) ----
        PixelCount  text    =   countPixels(pixels,256,textTop,barTop);
        printf("       text white pixels above bar = %zu\n",text._white);
        check(text._white > 20,"text: percentage rendered above the bar");

        /// ---- 6. 进度钳制 ----
        bar->setProgress(-1.0f);
        check(bar->progress() == 0.0f,"bar: setProgress(-1) clamps to 0");
        bar->setProgress(2.0f);
        check(bar->progress() == 1.0f,"bar: setProgress(2) clamps to 1");

        printf("=== %d passed, %d failed ===\n",g_pass,g_fail);
        return  (g_fail == 0) ? 0 : 1;
    }
    /// <summary>
    /// 窗口预览:进度按时间推进(约 5 秒一轮),实时显示填充与百分比文本,
    /// 关闭窗口结束
    /// </summary>
    int     runWindow(FEContext& ctx,const FEUuid& rendererId)
    {
        printf("=== test.progress (window): ImGui-free progress bar ===\n");
        printf("进度条每约 5 秒从 0%% 走到 100%%,关闭窗口结束\n");
        fflush(stdout);

        FEScene*        pScene  =   nullptr;
        FEProgressBar*  bar     =   nullptr;
        real            progress=   real(0);

        FEApp::CreateInfo  info =   {};
        info._width     =   960;
        info._height    =   540;
        info._appInst   =   GetModuleHandle(nullptr);
        info._notify    =   [&](const FEMessage& msg)
        {
            /// 先推进进度,再由场景处理消息(场景 update 阶段读取最新进度)
            if (bar != nullptr && msg.msgId() == MSG_UPDATE)
            {
                progress    +=  ctx.deltaTime() * real(0.2);
                if (progress > real(1))
                    progress    =   real(0);
                bar->setProgress(progress);
            }
            if (pScene != nullptr)
                pScene->onMessage(msg);
        };
        App app =   FEAppHelper::create(ctx,info);
        if (app == nullptr)
        {
            std::cerr << "create app failed" << std::endl;
            return  1;
        }
        ctx.setWindow(app.get());
        ctx.setWorkPath(app->path());
        ctx.setResourcePath(app->path() + "/../");

        Scene   scene   =   new FEScene(ctx);
        pScene          =   scene.get();
        if (!scene->setup(app,rendererId))
        {
            std::cerr << "scene setup failed" << std::endl;
            pScene  =   nullptr;
            scene   =   nullptr;
            app->destroy();
            return  1;
        }
        /// 深色背景便于观察进度条与文本
        scene->showSky(false);
        scene->showGrid(false);
        scene->setClearColor(float4(0,0,0,1));

        bar =   new FEProgressBar(ctx);
        scene->factoryMgr().addObject(bar);

        app->run();

        /// 先松开节点树外的引用,再销毁场景
        bar     =   nullptr;
        pScene  =   nullptr;
        scene   =   nullptr;
        app->destroy();
        return  0;
    }
}

int     main(int argc,char** argv)
{
    FEUuid  rendererId  =   RS_VULKAN;
    bool    headless    =   false;
    for (int i = 1;i < argc;++i)
    {
        const   char*   arg =   argv[i];
        if (_stricmp(arg,"--webgpu") == 0 || _stricmp(arg,"-webgpu") == 0)
            rendererId  =   RS_WEBGPU;
        else if (_stricmp(arg,"--vulkan") == 0 || _stricmp(arg,"-vulkan") == 0)
            rendererId  =   RS_VULKAN;
        else if (_stricmp(arg,"--headless") == 0 || _stricmp(arg,"-headless") == 0)
            headless    =   true;
    }

    setvbuf(stdout,nullptr,_IONBF,0);

    FEContext&  ctx =   FEContext::instance();
    const   int code    =   headless ? runHeadless(ctx,rendererId)
                                    : runWindow(ctx,rendererId);
    /// 已知问题:引擎 device 全局析构顺序会导致进程退出崩溃(与进度条无关),
    /// 这里直接用退出码结束,跳过全局析构以便脚本判定结果
    std::quick_exit(code);
}
