
#include    <iostream>
#include    <stdio.h>
#include    <cstdlib>
#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEDateTime.hpp"
#include    "../../../inc/FESetting.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../core/FEImageHelper.hpp"

using   namespace   FE;

int     main(int argc,char** argv)
{
    /// 解析多采样数,默认 1x。用法: window.exe [--msaa N]
    uint    msaa    =   1;
    for (int i = 1; i < argc; ++i)
    {
        if (_stricmp(argv[i], "--msaa") == 0 && i + 1 < argc)
            msaa    =   (uint)atoi(argv[++i]);
    }

    FESetting       cfg;
    cfg.get("test")
        .set<int>(100)
        .get("111")
        .setIfUnset<int>(123);

    FEDateTime      dt;
    char            sx[64]  =   {};
    dt.toDateString(sx);

    FEContext&      ctx     =   FEContext::instance();
    FEApp::CreateInfo   info    =   {};
    info._appInst   =   GetModuleHandle(nullptr);
    App     app     =   FEAppHelper::create(ctx, info);
    if (app == nullptr)
        return  0;

    ctx.setWindow(app.get());
    ctx.setWorkPath(app->path());
    ctx.setResourcePath(app->path() + "/../");

    FEScene*    scene   =   new FEScene(ctx);
    /// 多采样数必须在 setup 之前设置,setup 内会按设备能力钳制
    scene->setSampleCount(msaa);
    scene->setup(app, RS_VULKAN);

    /// 测试 CPU 图像保存 BMP
    {
        FEImage img(ctx, 512, 512, FMT_R8G8B8A8_UNORM);
        uint8_t*    pData   =   reinterpret_cast<uint8_t*>(img.pixel(0));
        for (uint32 i = 0; i < 512 * 512; ++i)
        {
            pData[i * 4 + 0] = 255;  /// R
            pData[i * 4 + 1] = 0;    /// G
            pData[i * 4 + 2] = 0;    /// B
            pData[i * 4 + 3] = 255;  /// A
        }
        FEImageHelper::saveBmp(img, "d:/xx.bmp");
    }

    app->run();
    app->destroy();

    return  0;
}
