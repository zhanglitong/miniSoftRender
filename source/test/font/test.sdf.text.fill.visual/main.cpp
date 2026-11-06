/// test.sdf.text.fill.visual
/// 矢量填充模式可视化测试:创建渲染窗口,用 FEFont 的矢量填充/挤出网格实际绘制文本。
/// 纯算法校验见独立工程 test.sdf.text.fill。
///
/// 绘制内容:
///   1. 4 行彩色矢量填充文本(v3c4:TRIANGLES + 顶点色)
///   2. 中文"中华人民共和国万岁"实体挤出(v3n3c4:厚度 + 法线 + 顶点色)
///   3. "PBR MATERIAL" 实体挤出(FEMaterialPBR:pbr2s 双面金属度-粗糙度)
/// 用法: test.sdf.text.fill.visual.exe [--vulkan|--webgpu]

#include    <iostream>
#include    <cmath>
#include    <string>
#include    <windows.h>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/font/FEFont.hpp"
#include    "../../../inc/material/FEMaterialV3C4.hpp"
#include    "../../../inc/material/FEMaterialV3N3C4.hpp"
#include    "../../../inc/material/FEMaterialPBR.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    /// 可执行文件所在目录下的测试字体
    String  fontPath()
    {
        char    buf[MAX_PATH]   =   {0};
        GetModuleFileNameA(nullptr,buf,MAX_PATH);
        String  p   =   buf;
        size_t  s   =   p.find_last_of("\\/");
        if (s != String::npos)
            p.resize(s);
        return  p + "/../data/fonts/consola.ttf";
    }

    /// 在系统字体目录中返回首个存在的字体文件(按优先级)
    String  findSystemFont(const char* const* names,uint count)
    {
        char    winDir[MAX_PATH]    =   {0};
        if (GetWindowsDirectoryA(winDir,MAX_PATH) == 0)
            return  {};
        const   String  dir =   String(winDir) + "/Fonts/";
        for (uint i = 0; i < count; ++i)
        {
            const   String  p   =   dir + names[i];
            if (GetFileAttributesA(p.c_str()) != INVALID_FILE_ATTRIBUTES)
                return  p;
        }
        return  {};
    }

    /// 只烘焙测试所需字符,不需要图集上传
    Font    makeFont()
    {
        FEFontCreateInfo    ci;
        ci._pixelSize       =   48;
        ci._spread          =   8;
        ci._atlasWidth      =   512;
        /// 矢量填充不采样图集,这里只烘焙 ASCII 以便度量/步进可用
        ci._uploadTexture   =   false;

        Font    font    =   new FEFont(FEContext::instance());
        if (!font->loadFromFile(fontPath(),ci))
            return  nullptr;
        return  font;
    }

    /// 实际绘制:窗口 + 透视相机渲染矢量填充/挤出文本
    int     drawDemo(FEContext& ctx,Font font,FEUuid rendererId)
    {
        FEScene*    pScene  =   nullptr;
        FEApp::CreateInfo   info   =   {};
        info._width     =   1280;
        info._height    =   720;
        info._appInst   =   GetModuleHandle(nullptr);
        info._notify    =   [&](const FEMessage& msg)
        {
            if (pScene)
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
            scene   =   nullptr;
            app->destroy();
            return  1;
        }

        /// 透视相机:位于 +z 方向看向原点,近大远小可体现挤出厚度
        {
            Camera  cam =   scene->camera();
            cam->setEye(real3(0.0f,0.4f,7.0f));
            cam->setTarget(real3(0.0f,0.0f,0.0f));
            cam->setUp(real3(0.0f,1.0f,0.0f));
            cam->setNear(0.1f);
            cam->setFar(100.0f);
            /// glm::perspective 的 fov 为弧度,45 度
            cam->setFov(glm::radians(45.0f));
            cam->updatePerspective();
        }

        FEMaterialV3C4* pMat    =   new FEMaterialV3C4(ctx);
        if (pMat->pipeline(PRI_TRIANGLES) == nullptr)
        {
            std::cerr << "pipeline v3c4/TRIANGLES not found" << std::endl;
            scene   =   nullptr;
            app->destroy();
            return  1;
        }

        /// 受光材质(厚度 + 法线 + 顶点色)
        FEMaterialV3N3C4*   pLitMat    =   new FEMaterialV3N3C4(ctx);
        if (pLitMat->pipeline(PRI_TRIANGLES) == nullptr)
        {
            std::cerr << "pipeline v3n3c4/TRIANGLES not found" << std::endl;
            scene   =   nullptr;
            app->destroy();
            return  1;
        }

        /// PBR 材质:金属度-粗糙度工作流,双面渲染(侧壁/背盖不依赖绕序),
        /// 并通过 data() 设置基础色/粗糙度/金属度与少量自发光
        FEMaterialPBR*  pPbrMat =   new FEMaterialPBR(ctx);
        pPbrMat->setDoubleSided(true);
        pPbrMat->data()._value._diffuse     =   float4(0.85f,0.62f,0.30f,1.0f);
        pPbrMat->data()._value._roughness   =   0.45f;
        pPbrMat->data()._value._metallic    =   0.2f;
        /// 默认环境贴图为黑色,加少量自发光保证材质可见
        pPbrMat->data()._value._emissive    =   float4(0.2f,0.14f,0.06f,1.0f);
        pPbrMat->data().update();
        if (pPbrMat->pipeline(PRI_TRIANGLES) == nullptr)
        {
            std::cerr << "pipeline pbr2s/TRIANGLES not found" << std::endl;
            scene   =   nullptr;
            app->destroy();
            return  1;
        }

        /// 中文字体:中文码点不连续,用显式字符集只烘焙本行所需字形
        const   char*   cjkText =   "中华人民共和国万岁";
        const   char*   cjkCandidates[]    =   { "simsun.ttc", "simhei.ttf", "msyh.ttc" };
        const   String  cjkFile            =   findSystemFont(cjkCandidates,3);
        Font            cjkFont            =   nullptr;
        if (!cjkFile.empty())
        {
            FEFontCreateInfo    cci;
            cci._pixelSize      =   48;
            cci._spread         =   8;
            cci._atlasWidth     =   512;
            cci._characters     =   cjkText;
            cci._uploadTexture  =   false;

            cjkFont =   new FEFont(ctx);
            if (!cjkFont->loadFromFile(cjkFile,cci))
                cjkFont =   nullptr;
        }
        if (cjkFont == nullptr)
        {
            std::cerr << "system Chinese font not available: " << cjkFile << std::endl;
            scene   =   nullptr;
            app->destroy();
            return  1;
        }
        std::cout << "cjk font: " << cjkFile
                  << " family=" << cjkFont->familyName()
                  << " glyphs=" << cjkFont->glyphCount() << std::endl;

        struct  DemoLine
        {
            const   char*   _text;
            Rgba8           _color;
        };
        const   DemoLine    lines[] =
        {
            { "VECTOR FILL MODE",       Rgba8(255,255,255,255) },
            { "ABCDEFG 0123456789",     Rgba8(120,220,255,255) },
            { "O o Q a e B holes",      Rgba8(255,190,90,255)  },
            { "Vector Font Rendering",  Rgba8(120,255,160,255) },
        };
        const   uint    lineCount   =   uint(sizeof(lines) / sizeof(lines[0]));

        /// 像素坐标 -> 世界单位;行距为像素步进
        const   float   worldScale  =   0.012f;
        const   float   lineStep    =   64.0f;

        /// 文本网格 AABB 是像素偏移,视锥剔除无意义
        const   auto    dispatchNode   =   [&](Node node)
        {
            DispatchResult  dr;
            scene->dispatchNodesToSystem({node},&dr);
            /// 立即常驻:后续 dispatch 会销毁非常驻工厂
            for (auto& f : dr.rFactorys)
            {
                if (!f->isResident())
                {
                    f->setResident(true);
                    f->setGPUCull(false);
                }
            }
        };

        bool    ok  =   true;
        for (uint i = 0; i < lineCount; ++i)
        {
            Mesh    mesh    =   font->createTextFillMesh(lines[i]._text,
                                                         float2(0.0f,(float)i * lineStep),
                                                         1.0f,12,lines[i]._color);
            if (mesh == nullptr)
            {
                std::cerr << "createTextFillMesh failed: " << lines[i]._text << std::endl;
                ok  =   false;
                break;
            }
            std::cout << "  fill line " << i << " vertices="
                      << mesh->get(FEAttribute(IS_VERTEX_POS))->count() << std::endl;

            Node    node    =   new FENode(ctx);
            node->setMesh(mesh);
            node->setMaterial(pMat);
            node->setColor(Rgba8(255,255,255,255));
            /// 像素 y 向下,相机 y 向上:负 y 缩放完成翻转;
            /// z 也按 worldScale 缩放(像素厚度 <-> 世界厚度)
            node->setLocalScaling(real3(worldScale,-worldScale,worldScale));
            /// 文本块上移量取 2.2,使 6 行内容在透视视锥内垂直居中
            node->setLocalTranslation(real3(-2.8f,2.2f,0.0f));
            node->makeDirty();
            node->update();
            dispatchNode(node);
        }

        /// 中文字体行:实体挤出(厚度 + 法线 + 顶点色),排在最下方
        if (ok)
        {
            /// 挤出厚度按像素计:8px 约为字高(48px)的 1/6,
            /// 在 0.012 缩放下约 0.096 世界单位,避免看起来过厚
            Mesh    mesh    =   cjkFont->createTextExtrudeMesh(cjkText,
                                                              float2(0.0f,(float)lineCount * lineStep),
                                                              1.0f,8.0f,12,
                                                              Rgba8(255,220,120,255));
            if (mesh == nullptr)
            {
                std::cerr << "createTextExtrudeMesh(cjk) failed" << std::endl;
                ok  =   false;
            }
            else
            {
                std::cout << "  extrude cjk vertices="
                          << mesh->get(FEAttribute(IS_VERTEX_POS))->count() << std::endl;

                Node    node    =   new FENode(ctx);
                node->setMesh(mesh);
                node->setMaterial(pLitMat);
                node->setColor(Rgba8(255,255,255,255));
                /// z 轴同样按 worldScale 缩放,否则像素厚度会变成世界厚度
                node->setLocalScaling(real3(worldScale,-worldScale,worldScale));
                /// 绕 Y 轻微旋转,露出侧壁厚度
                node->setLocalRotation(quatr(glm::radians(real3(0.0f,-12.0f,0.0f))));
                node->setLocalTranslation(real3(-2.8f,2.2f,0.0f));
                node->makeDirty();
                node->update();
                dispatchNode(node);
            }
        }

        /// PBR 行:同样的挤出网格,换用 pbr2s(双面金属度-粗糙度)材质
        if (ok)
        {
            Mesh    mesh    =   font->createTextExtrudeMesh("PBR MATERIAL",
                                                            float2(0.0f,(float)(lineCount + 1) * lineStep),
                                                            1.0f,8.0f,12,
                                                            Rgba8(255,255,255,255));
            if (mesh == nullptr)
            {
                std::cerr << "createTextExtrudeMesh(pbr) failed" << std::endl;
                ok  =   false;
            }
            else
            {
                std::cout << "  pbr extrude vertices="
                          << mesh->get(FEAttribute(IS_VERTEX_POS))->count() << std::endl;

                Node    node    =   new FENode(ctx);
                node->setMesh(mesh);
                node->setMaterial(pPbrMat);
                node->setColor(Rgba8(255,255,255,255));
                node->setLocalScaling(real3(worldScale,-worldScale,worldScale));
                node->setLocalRotation(quatr(glm::radians(real3(0.0f,-12.0f,0.0f))));
                node->setLocalTranslation(real3(-2.8f,2.2f,0.0f));
                node->makeDirty();
                node->update();
                dispatchNode(node);
            }
        }

        if (!ok)
        {
            scene   =   nullptr;
            app->destroy();
            return  1;
        }

        /// 窗口关闭时先释放字体对象,再做场景销毁
        app->messageBus().subscribe(MSG_CLOSE,[&](const FEMessage&)
        {
            font        =   nullptr;
            cjkFont     =   nullptr;
        });

        std::cout << "fill demo ok, entering render loop..." << std::endl;
        app->run();

        scene   =   nullptr;
        app->destroy();
        return  0;
    }
}

int     main(int argc, char** argv)
{
    FEUuid  rendererId  =   RS_VULKAN;
    for (int i = 1; i < argc; ++i)
    {
        const   char*   arg =   argv[i];
        if (_stricmp(arg,"--webgpu") == 0 || _stricmp(arg,"-webgpu") == 0 ||
            _stricmp(arg,"/webgpu") == 0)
        {
            rendererId  =   RS_WEBGPU;
        }
    }

    FEContext&  ctx =   FEContext::instance();
    std::cout << "font: " << fontPath() << std::endl;

    Font    font    =   makeFont();
    if (font == nullptr)
    {
        std::cerr << "test font not available: " << fontPath() << std::endl;
        return  1;
    }

    return  drawDemo(ctx,font,rendererId);
}
