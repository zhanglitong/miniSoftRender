#pragma     once

/// PBR 测试共享的环境贴图(IBL)工具:
///   加载 equirect HDR(Radiance RGBE,已是线性空间)
///   -> 逐面重投影为 cubemap(含完整 mip 链,CPU 盒式滤波)
///   -> 上传为 float RGBA cubemap(FMT_R32G32B32A32_FLOAT)
/// 使用方需在包含本头文件之前定义 STB_IMAGE_IMPLEMENTATION(每个可执行文件一次)。
/// 环境资源使用与 glTF-Sample-Viewer 参考查看器默认环境相同的 Cannon_Exterior.hdr。

#include    <vector>
#include    <iostream>

#include    "../../inc/fileFormat/gltf/stb_image.h"
#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEConst.h"
#include    "../../inc/graphic/FEDevice.h"
#include    "../../inc/graphic/FETexture.h"

namespace   PBRTest
{
    using   namespace   FE;

    /// cubemap 面内像素方向(面序 +X,-X,+Y,-Y,+Z,-Z,与 Vulkan/GL 规范一致)
    inline  float3  cubeFaceDir(int face,float a,float b)
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
    inline  float3  sampleEquirect(const float* img,int w,int h,const float3& d)
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
    inline  TexPtr  createEnvCube(FEContext& ctx,const char* hdrPath,uint size)
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
}
