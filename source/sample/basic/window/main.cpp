
#include    <iostream>
#include    <stdio.h>
#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEDateTime.hpp"
#include    "../../../inc/FESetting.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../core/FEImageHelper.hpp"

using   namespace   FE;

int     main(int argc,char** argv)
{
    (void)argc;
    (void)argv;

    FESetting       cfg;
    cfg.get("test")
        .set<int>(100)
        .get("111")
        .value<int>(123);

    FEDateTime      dt;
    char            sx[64]  =   {};
    dt.toDateString(sx);

    FEContext       ctx;
    FEApp::CreateInfo   info    =   {};
    info._appInst   =   GetModuleHandle(nullptr);
    App     app     =   FEAppHelper::create(ctx, info);
    if (app == nullptr)
        return  0;

    ctx.setWindow(app.get());
    ctx.setWorkPath(app->path());
    ctx.setResourcePath(app->path() + "/../");

    FEScene*    scene   =   new FEScene(ctx);
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
