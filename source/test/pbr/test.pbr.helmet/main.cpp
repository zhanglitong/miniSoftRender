/// test.pbr.helmet
/// 用 glTF 官方示例模型 DamagedHelmet 验证 FEMaterialPBR 的全套贴图链路:
///   1. 创建引擎 + 渲染窗口(Vulkan)
///   2. 经 FEFormatGLTFReader 加载 DamagedHelmet.gltf(网格 + 材质 + 贴图)
///   3. glTF 材质的 baseColor/metallicRoughness/normal/occlusion/emissive
///      由解析器自动创建纹理并设置到 FEMaterialPBR
///   4. 环境图使用与 glTF-Sample-Viewer 参考查看器默认环境相同的
///      Cannon_Exterior.hdr(equirect,线性空间),运行时转换为 float cubemap
/// 验证点:
///   a. 头盔呈现 albedo 贴图细节(说明纹理加载与 UV 采样正确)
///   b. 金属/粗糙度随贴图变化(高光强弱分布)
///   c. 法线贴图产生表面凹凸
///   d. AO 使凹槽/接缝处变暗
///   e. emissive 区域自发光
///   f. IBL:金属反射 Cannon 户外环境,背光面被环境照亮
/// 用法: test.pbr.helmet.exe

#include    <iostream>
#include    <vector>
#include    <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include    "../../../inc/fileFormat/gltf/stb_image.h"

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/graphic/FELightDir.h"
#include    "../../../inc/FEFileFormatHelper.hpp"
#include    "../../../inc/material/FEMaterialPBR.hpp"

using   namespace   FE;

USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    /// cubemap 面内像素方向(面序 +X,-X,+Y,-Y,+Z,-Z,与 Vulkan/GL 规范一致)
    float3  cubeFaceDir(int face,float a,float b)
    {
        switch (face)
        {
        case 0:  return float3( 1.0f, -b, -a);   /// +X
        case 1:  return float3(-1.0f, -b,  a);   /// -X
        case 2:  return float3( a,  1.0f,  b);   /// +Y
        case 3:  return float3( a, -1.0f, -b);   /// -Y
        case 4:  return float3( a, -b,  1.0f);   /// +Z
        default: return float3(-a, -b, -1.0f);   /// -Z
        }
    }

    /// equirect HDR 采样(双线性,u 环绕,v 钳制;线性空间)
    /// 约定:图像顶部 = +Y(天顶),经度 phi = atan2(d.z, d.x)
    float3  sampleEquirect(const float* img,int w,int h,const float3& d)
    {
        float   phi     =   atan2f(d.z, d.x);                    /// [-PI, PI]
        float   theta   =   acosf(clamp(d.y, -1.0f, 1.0f));      /// [0, PI]
        float   uf      =   (phi * (0.5f / float(PI)) + 0.5f) * w - 0.5f;
        float   vf      =   theta * (1.0f / float(PI)) * h - 0.5f;
        uf = uf - floorf(uf / w) * w;                            /// 环绕到 [0, w)
        vf = clamp(vf, 0.0f, float(h - 1));                      /// 钳制到图内

        int     x0  =   int(floorf(uf));
        int     y0  =   int(floorf(vf));
        int     x1  =   (x0 + 1) % w;
        int     y1  =   y0 + 1 < h ? y0 + 1 : h - 1;
        float   tx  =   uf - floorf(uf);
        float   ty  =   vf - floorf(vf);

        const float*    p00 =   img + (size_t(y0) * w + x0) * 4;
        const float*    p10 =   img + (size_t(y0) * w + x1) * 4;
        const float*    p01 =   img + (size_t(y1) * w + x0) * 4;
        const float*    p11 =   img + (size_t(y1) * w + x1) * 4;
        float3  c;
        for (int i = 0; i < 3; ++i)
        {
            float   top     =   p00[i] * (1.0f - tx) + p10[i] * tx;
            float   bottom  =   p01[i] * (1.0f - tx) + p11[i] * tx;
            c[i] = top * (1.0f - ty) + bottom * ty;
        }
        return c;
    }

    /// 从 equirect HDR 生成环境 cubemap(float RGBA,含完整 mip 链,CPU 盒式滤波)
    TexPtr  createEnvCube(FEContext& ctx,const char* hdrPath,uint size)
    {
        /// 1. 加载 HDR(stb_image 解码 Radiance RGBE,强制 RGBA float)
        int     eqW = 0, eqH = 0;
        float*  eq  =   stbi_loadf(hdrPath, &eqW, &eqH, nullptr, 4);
        if (eq == nullptr)
        {
            std::cerr << "load hdr failed: " << hdrPath << "\n";
            return  nullptr;
        }
        float   maxC = 0.0f;
        for (size_t i = 0, n = size_t(eqW) * eqH * 3; i < n; ++i)
            maxC = eq[i] > maxC ? eq[i] : maxC;
        std::cout << "hdr equirect " << eqW << "x" << eqH << " rgbMax=" << maxC << "\n";

        uint    mipCount    =   1;
        for (uint s = size; s > 1; s >>= 1)
            ++mipCount;

        /// 2. level0:逐面逐像素做 equirect -> cube 重投影
        std::vector<std::vector<float>> levels(mipCount);
        levels[0].resize(size_t(size) * size * 4 * 6);
        for (int face = 0; face < 6; ++face)
        {
            float*  dst =   levels[0].data() + size_t(face) * size * size * 4;
            for (uint y = 0; y < size; ++y)
            {
                for (uint x = 0; x < size; ++x)
                {
                    float   a   =   2.0f * (x + 0.5f) / size - 1.0f;
                    float   b   =   2.0f * (y + 0.5f) / size - 1.0f;
                    float3  d   =   normalize(cubeFaceDir(face, a, b));
                    float3  c   =   sampleEquirect(eq, eqW, eqH, d);
                    float*  px  =   dst + (size_t(y) * size + x) * 4;
                    px[0]   =   c.x;
                    px[1]   =   c.y;
                    px[2]   =   c.z;
                    px[3]   =   1.0f;
                }
            }
        }
        stbi_image_free(eq);

        /// 3. mip 链:2x2 盒式滤波(逐面独立,float 域平均保留 HDR 范围)
        uint    mipSize =   size;
        for (uint m = 1; m < mipCount; ++m)
        {
            uint    half    =   mipSize > 1 ? mipSize >> 1 : 1;
            levels[m].resize(size_t(half) * half * 4 * 6);
            const auto& prev    =   levels[m - 1];
            auto&       curr    =   levels[m];
            for (int face = 0; face < 6; ++face)
            {
                const float*    src =   prev.data() + size_t(face) * mipSize * mipSize * 4;
                float*          dst =   curr.data() + size_t(face) * half * half * 4;
                for (uint y = 0; y < half; ++y)
                {
                    for (uint x = 0; x < half; ++x)
                    {
                        float   r = 0.0f, g = 0.0f, b = 0.0f;
                        for (int dy = 0; dy < 2; ++dy)
                        {
                            for (int dx = 0; dx < 2; ++dx)
                            {
                                const float*    px  =   src + (size_t(y * 2 + dy) * mipSize + (x * 2 + dx)) * 4;
                                r += px[0]; g += px[1]; b += px[2];
                            }
                        }
                        float*  px  =   dst + (size_t(y) * half + x) * 4;
                        px[0]   =   r * 0.25f;
                        px[1]   =   g * 0.25f;
                        px[2]   =   b * 0.25f;
                        px[3]   =   1.0f;
                    }
                }
            }
            mipSize =   half;
        }

        /// 4. 打包:面主序、面内 mip 顺序(与 FETexture::CreateInfo 注释一致)
        std::vector<float>  packed;
        for (int face = 0; face < 6; ++face)
        {
            for (uint m = 0; m < mipCount; ++m)
            {
                size_t  faceFloats  =   levels[m].size() / 6;
                const float*    src =   levels[m].data() + size_t(face) * faceFloats;
                packed.insert(packed.end(), src, src + faceFloats);
            }
        }

        /// 5. 创建 float cubemap 并上传
        auto    tex =   ctx.device().createTexture();
        FETexture::CreateInfo   ci{};
        ci._width       =   size;
        ci._height      =   size;
        ci._format      =   FMT_R32G32B32A32_FLOAT;
        ci._magFilter   =   FILTER_LINEAR;
        ci._minFilter   =   FILTER_LINEAR;
        ci._addressMode =   ADDRESS_CLAMP_TO_EDGE;
        ci._mips        =   mipCount;
        ci._cube        =   true;
        if (!tex->create(ci))
            return  nullptr;
        if (!tex->update(packed.data(), packed.size() * sizeof(float)))
            return  nullptr;
        return  tex;
    }

    /// 递归为节点树上的所有 FEMaterialPBR 设置环境贴图
    void    applyEnvMap(const Node& node,TexPtr env)
    {
        if (node == nullptr)
            return;
        auto    mat =   node->material();
        if (mat != nullptr)
        {
            auto    pbr =   mat->as<FEMaterialPBR>();
            if (pbr != nullptr)
                pbr->setEnvironmentMap(env);
        }
        for (auto& child : node->children())
            applyEnvMap(child, env);
    }
}

int main(int argc, char** argv)
{
    (void)argc; (void)argv;
    FEContext& ctx = FEContext::instance();
    FEScene* pScene = nullptr;

    FEApp::CreateInfo info{};
    info._width = 1280; info._height = 720;
    info._appInst = GetModuleHandle(nullptr);
    info._notify = [&](const FEMessage& msg) { if (pScene) pScene->onMessage(msg); };
    App app = FEAppHelper::create(ctx, info);
    if (!app) { std::cerr << "create app failed\n"; return 1; }
    ctx.setWindow(app.get());
    ctx.setWorkPath(app->path());
    ctx.setResourcePath(app->path() + "/../");

    Scene scene = new FEScene(ctx);
    pScene = scene.get();
    if (!scene->setup(app, RS_VULKAN))
    {
        std::cerr << "scene setup failed\n";
        scene = nullptr; app->destroy(); return 1;
    }

    /// 调亮默认平行光并调整方向,使 PBR 光照层次清晰可见
    {
        Light mainLight;
        if (ctx.device().lights().query("main", mainLight) && mainLight)
        {
            auto dir = mainLight->as<FELightDir>();
            if (dir)
            {
                dir->_color = float3(2.5f, 2.5f, 2.5f);
                dir->_dir   = float3(0.4f, 0.7f, 0.8f);  /// 斜上方照射
            }
        }
    }

    /// 加载 DamagedHelmet.gltf(网格/材质/贴图全部由解析器完成)
    const String gltfFile = R"(D:\study\glTF-Sample-Models\2.0\DamagedHelmet\glTF\DamagedHelmet.gltf)";
    FEFileFormat fmt(".gltf", "1.0.0.0", "GLTF text Format!");
    auto reader = FEFileFormatHelper::queryReader(ctx, fmt);
    if (!reader)
    {
        std::cerr << "query gltf reader failed\n";
        scene = nullptr; app->destroy(); return 1;
    }
    auto objects = reader->readFiles({gltfFile});
    Nodes nodes;
    for (auto var : objects)
    {
        Node node = var->cast<FENode>();
        if (node != nullptr)
            nodes.push_back(node);
    }
    if (nodes.empty())
    {
        std::cerr << "load gltf failed: " << gltfFile << "\n";
        scene = nullptr; app->destroy(); return 1;
    }

    scene->dispatchNodesToSystem(nodes);
    scene->addNodesToTree(nodes);

    /// 从参考查看器默认环境 HDR 生成 cubemap,挂到头盔所有 PBR 材质上(IBL)
    const char* hdrFile =   R"(D:\miniSoftRender\bin\data\env\Cannon_Exterior.hdr)";
    auto    envCube =   createEnvCube(ctx, hdrFile, 256);
    if (envCube == nullptr)
    {
        std::cerr << "create environment cube failed\n";
        scene = nullptr; app->destroy(); return 1;
    }
    for (auto& node : nodes)
        applyEnvMap(node, envCube);

    std::cout << "pbr helmet test ready, entering render loop...\n";
    app->run();

    scene = nullptr;
    app->destroy();
    return 0;
}
