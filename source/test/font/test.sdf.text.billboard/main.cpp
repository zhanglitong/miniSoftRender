/// test.sdf.text.billboard
/// SDF 文本三维布告板验证:
///   文本锚定在世界空间位置,始终面向相机(布告板),且屏幕像素尺寸恒定
///   (不随锚点深度缩小)。
///
/// 验证点:
///   1. 三个锚点位于不同深度(z=0/-4/-9),同字号(28px)文本在屏幕上
///      应显示为完全相同的尺寸
///   2. 锚点处用 v3c4 彩色十字线(X红/Y绿/Z蓝)标示三维位置,
///      文本开启深度测试,与场景几何有正确的遮挡关系
///   3. 左上角保留一行屏幕叠加文本(模式 0),验证两种模式共存互不影响
///   4. 三条标签前方各有一个不透明几何体(A前 box / B、C前 sphere,不旋转)
///      部分覆盖文本——被遮部分应消失、露出部分保持完整,
///      验证布告板文本的深度测试与场景几何一致
///
/// 相机:透视投影(fov=45),固定机位不旋转,便于观察验证。
/// 用法: test.sdf.text.billboard.exe [--vulkan|--webgpu]

#include    <iostream>
#include    <vector>
#include    <string>
#include    <cstdint>
#include    <windows.h>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/font/FEFont.hpp"
#include    "../../../inc/material/FEMaterialText.hpp"
#include    "../../../inc/material/FEMaterialV3C4.hpp"
#include    "../../../inc/material/FEMaterialPBR.hpp"
#include    "../../../inc/geometry/FEGeometryBox.hpp"
#include    "../../../inc/geometry/FEGeometrySphere.hpp"
#include    "../../../inc/mesh/FEMeshHelper.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
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

    /// 一条三维布告板文本的绘制参数
    struct  Label
    {
        const   char*   _text;
        float           _x;     /// 世界锚点
        float           _y;
        float           _z;
        uint8_t         _r;
        uint8_t         _g;
        uint8_t         _b;
    };
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
    FEScene*    pScene  =   nullptr;

    /// 1. 创建渲染窗口
    FEApp::CreateInfo  info   =   {};
    info._width     =   1280;
    info._height    =   720;
    info._appInst   =   GetModuleHandle(nullptr);
    info._notify    =   [&](const FEMessage& msg)
    {
        if (pScene)
            pScene->onMessage(msg);
    };
    App app =   FEAppHelper::create(ctx, info);
    if (app == nullptr)
    {
        std::cerr << "create app failed" << std::endl;
        return  1;
    }
    ctx.setWindow(app.get());
    ctx.setWorkPath(app->path());
    ctx.setResourcePath(app->path() + "/../");

    /// 2. 创建引擎并初始化渲染后端
    Scene   scene   =   new FEScene(ctx);
    pScene          =   scene.get();
    if (!scene->setup(app, rendererId))
    {
        std::cerr << "scene setup failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 2b. 配置相机:透视投影,正前方俯视观察原点区域。
    ///     场景每帧调用 camera->update(),_isOrtho=false 时保持透视投影。
    {
        Camera  cam =   scene->camera();
        cam->setEye(real3(0.0f,0.0f,12.0f));
        cam->setTarget(real3(0.0f,0.0f,0.0f));
        cam->setUp(real3(0.0f,1.0f,0.0f));
        cam->setNear(0.1f);
        cam->setFar(100.0f);
        cam->updatePerspective();       /// _isOrtho=false,每帧保持透视
    }

    /// 3. 定位系统宋体
    const   char*   candidates[]    =   { "simsun.ttc", "simhei.ttf", "msyh.ttc" };
    const   String  fontFile        =   findSystemFont(candidates,3);
    if (fontFile.empty())
    {
        std::cerr << "system Chinese font not found (simsun.ttc/simhei.ttf/msyh.ttc)" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }
    std::cout << "system font: " << fontFile << std::endl;

    /// 4. 三维布告板文本:不同深度、同一字号(28px)。
    ///    若布告板正确,三行文本在屏幕上尺寸完全一致。
    const   float   labelSize   =   28.0f;
    const   Label   labels[]    =
    {
        { "布告板 A 深度 z=0", -4.0f,  2.0f,  0.0f, 255, 255, 255 },
        { "布告板 B 深度 z=-4", 0.5f,  0.2f, -4.0f, 120, 220, 255 },
        { "布告板 C 深度 z=-9", 3.0f, -1.8f, -9.0f, 255, 210, 100 },
    };
    const   uint    labelCount  =   uint(sizeof(labels) / sizeof(labels[0]));
    /// 屏幕叠加文本(模式 0,回归验证)
    const   char*   overlayText =   "屏幕叠加层:SDF 布告板演示(模式共存)";

    /// 5. 只烘焙本示例用到的码点
    String  charset =   overlayText;
    for (uint i = 0; i < labelCount; ++i)
        charset +=  labels[i]._text;

    FEFontCreateInfo    ci;
    ci._pixelSize       =   64;
    ci._spread          =   8;
    ci._characters      =   charset;
    ci._atlasWidth      =   512;

    Font    font    =   new FEFont(ctx);
    if (!font->loadFromFile(fontFile,ci))
    {
        std::cerr << "font load failed: " << fontFile << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }
    std::cout << "family=" << font->familyName()
              << " glyphs=" << font->glyphCount()
              << " atlas=" << font->atlasWidth() << "x" << font->atlasHeight()
              << " spread=" << font->sdfSpread() << std::endl;

    /// 6. 布告板文本材质(模式 1:三维锚点 + 面向相机 + 恒定屏幕尺寸)
    FEMaterialText* pMatBB    =   new FEMaterialText(ctx);
    pMatBB->setFont(font);
    pMatBB->setBillboard(true);
    if (pMatBB->pipeline(PRI_TRIANGLES) == nullptr)
    {
        std::cerr << "pipeline text/TRIANGLES not found" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 屏幕叠加材质(模式 0,与布告板共存验证)
    FEMaterialText* pMatHUD   =   new FEMaterialText(ctx);
    pMatHUD->setFont(font);

    /// 锚点十字线材质(v3c4 世界空间线框)
    FEMaterialV3C4* pMatCross =   new FEMaterialV3C4(ctx);
    if (pMatCross->pipeline(PRI_LINES) == nullptr)
    {
        std::cerr << "pipeline v3c4/LINES not found" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    bool    ok  =   (font->glyphCount() > 0);

    /// 7. 逐条生成布告板文本:网格以锚点为原点(水平居中,基线过锚点),
    ///    节点平移即世界锚点;不做旋转/缩放(布告板尺寸只由字号决定)
    const   float   labelScale  =   labelSize / (float)ci._pixelSize;
    for (uint i = 0; ok && i < labelCount; ++i)
    {
        const   float   w       =   font->measureWidth(labels[i]._text,labelScale);
        Mesh    mesh    =   font->createTextMesh(labels[i]._text,
                                                 float2(-w * 0.5f,0.0f),
                                                 labelScale);
        if (mesh == nullptr)
        {
            std::cerr << "createTextMesh failed for label " << i << std::endl;
            ok  =   false;
            break;
        }
        Node    node    =   new FENode(ctx);
        node->setMesh(mesh);
        node->setMaterial(pMatBB);
        node->setColor(Rgba8(labels[i]._r,labels[i]._g,labels[i]._b,255));
        node->setLocalTranslation(real3(labels[i]._x,labels[i]._y,labels[i]._z));
        node->makeDirty();
        node->update();
        DispatchResult  dr;
        scene->dispatchNodesToSystem({node},&dr);
        /// 网格 AABB 是像素偏移(数百"世界单位"),剔除无意义,常驻并关 GPU 剔除
        for (auto& f : dr.rFactorys)
        {
            f->setResident(true);
            f->setGPUCull(false);
        }
        std::cout << "  label " << i
                  << " anchor=(" << labels[i]._x << "," << labels[i]._y << "," << labels[i]._z << ")"
                  << " width=" << w << "px" << std::endl;
    }

    /// 8. 屏幕叠加文本(模式 0):验证与布告板模式共存
    if (ok)
    {
        const   float   hudSize     =   20.0f;
        const   float   hudScale    =   hudSize / (float)ci._pixelSize;
        Mesh    mesh    =   font->createTextMesh(overlayText,float2(20.0f,20.0f),hudScale);
        if (mesh == nullptr)
        {
            std::cerr << "createTextMesh(overlay) failed" << std::endl;
            ok  =   false;
        }
        else
        {
            Node    node    =   new FENode(ctx);
            node->setMesh(mesh);
            node->setMaterial(pMatHUD);
            node->setColor(Rgba8(255,255,255,255));
            node->makeDirty();
            node->update();
            DispatchResult  dr;
            scene->dispatchNodesToSystem({node},&dr);
            for (auto& f : dr.rFactorys)
            {
                f->setResident(true);
                f->setGPUCull(false);
            }
        }
    }

    /// 9. 锚点十字线:每个锚点一个 0.5 世界单位的三轴十字(X红/Y绿/Z蓝),
    ///    标示三维位置并验证文本与场景几何的深度关系
    if (ok)
    {
        const   float   h   =   0.5f;
        float3s verts;
        Rgba8s  colors;
        const   Rgba8   axisX(255, 90, 90,255);
        const   Rgba8   axisY( 90,255,120,255);
        const   Rgba8   axisZ( 90,160,255,255);
        for (uint i = 0; i < labelCount; ++i)
        {
            const   float3  c(labels[i]._x,labels[i]._y,labels[i]._z);
            /// X 轴
            verts.push_back(c + float3(-h,0,0));    colors.push_back(axisX);
            verts.push_back(c + float3( h,0,0));    colors.push_back(axisX);
            /// Y 轴
            verts.push_back(c + float3(0,-h,0));    colors.push_back(axisY);
            verts.push_back(c + float3(0, h,0));    colors.push_back(axisY);
            /// Z 轴
            verts.push_back(c + float3(0,0,-h));    colors.push_back(axisZ);
            verts.push_back(c + float3(0,0, h));    colors.push_back(axisZ);
        }
        Attrs   attrs   =   {
            {IS_VERTEX_POS,    FMT_R32G32B32_FLOAT},
            {IS_VERTEX_COLOR0, FMT_R8G8B8A8_UNORM},
        };
        Mesh    mesh    =   FEMeshBuilder::makeMeshLine(ctx, attrs, verts, colors);
        if (mesh == nullptr)
        {
            std::cerr << "makeMeshLine(cross) failed" << std::endl;
            ok  =   false;
        }
        else
        {
            Node    node    =   new FENode(ctx);
            node->setMesh(mesh);
            node->setMaterial(pMatCross);
            node->makeDirty();
            node->update();
            scene->dispatchNodesToSystem({node});
            std::cout << "  cross verts=" << verts.size() << std::endl;
        }
    }

    /// 9b. 遮挡测试:三条标签与相机之间各放一个不透明几何体(不旋转),
    ///     部分覆盖标签——被遮住的文字应消失,露出部分保持完整,
    ///     验证布告板文本的深度测试与场景几何一致。
    ///     (遮挡物全部位于标签锚点前方,即 z 更大、更靠近相机,
    ///      文本不写深度,故无论绘制先后视觉结果一致)
    if (ok)
    {
        struct  Occluder
        {
            bool    _sphere;        /// false=box(边长) / true=sphere(半径)
            float3  _pos;           /// 世界位置(z 大于对应标签锚点)
            float   _size;
            float4  _diffuse;
            float4  _emissive;
        };
        const   Occluder    occs[]  =
        {
            /// A(z=0)前方的 box:覆盖标签左半部分
            { false, float3(-4.5f, 2.0f,  2.0f), 2.0f, float4(0.95f,0.50f,0.15f,1.0f), float4(0.45f,0.20f,0.05f,1.0f) },
            /// B(z=-4)前方的 sphere:覆盖标签中部
            { true,  float3( 0.5f, 0.2f, -1.0f), 1.0f, float4(0.20f,0.85f,0.35f,1.0f), float4(0.05f,0.35f,0.12f,1.0f) },
            /// C(z=-9)前方的 sphere:覆盖标签右中部
            { true,  float3( 3.0f,-1.8f, -6.5f), 1.2f, float4(0.75f,0.35f,0.95f,1.0f), float4(0.28f,0.10f,0.40f,1.0f) },
        };
        for (uint i = 0; ok && i < uint(sizeof(occs) / sizeof(occs[0])); ++i)
        {
            const   Occluder&   o   =   occs[i];
            Mesh    mesh;
            if (o._sphere)
            {
                FEGeometrySphere    geo(ctx);
                geo.param().radius          =   o._size;
                geo.param().widthSegments   =   32;
                geo.param().heightSegments  =   24;
                mesh    =   geo.triangular({
                    {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
                    {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
                    {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });
            }
            else
            {
                FEGeometryBox   geo(ctx);
                geo.param()._size   =   float3(o._size,o._size,o._size);
                geo.param()._segs   =   uint3(1,1,1);
                mesh    =   geo.triangular({
                    {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
                    {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
                    {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });
            }
            if (mesh == nullptr)
            {
                std::cerr << "occluder " << i << " mesh build failed" << std::endl;
                ok  =   false;
                break;
            }
            FEMaterialPBR*  pMatOcc =   new FEMaterialPBR(ctx);
            pMatOcc->data()._value._diffuse     =   o._diffuse;
            pMatOcc->data()._value._emissive    =   o._emissive;
            pMatOcc->data().update();

            Node    node    =   new FENode(ctx);
            node->setMesh(mesh);
            node->setMaterial(pMatOcc);
            node->setLocalTranslation(real3(o._pos.x,o._pos.y,o._pos.z));
            node->makeDirty();
            node->update();
            scene->dispatchNodesToSystem({node});
            std::cout << "  occluder " << i << (o._sphere ? " sphere" : " box")
                      << " pos=(" << o._pos.x << "," << o._pos.y << "," << o._pos.z << ")"
                      << " size=" << o._size << std::endl;
        }
    }

    if (!ok)
    {
        std::cerr << "sdf billboard validation failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    std::cout << "sdf billboard ok, entering render loop..." << std::endl;
    std::cout << "expected: A/B/C 三条文本屏幕尺寸一致(深度不同),"
                 "始终正面朝向相机,十字线标示锚点;"
                 "前方几何体遮住的部分文字应消失,露出部分完整" << std::endl;

    /// 10. 进入渲染循环
    app->run();

    scene   =   nullptr;
    app->destroy();

    return  0;
}
