#include    <vector>
#include    <cmath>
#include    <algorithm>

#include    "FETextureHelper.hpp"
#include    "../../inc/graphic/FEDevice.h"
#include    "../../inc/FEConst.h"
#include    "../FEImageHelper.hpp"

namespace   FE
{
    namespace
    {
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

        /// <summary>
        /// equirect(经纬度)float 图像的双线性采样器(u 环绕,v 钳制,线性空间)
        /// 约定与 cubeImageFromEquirect 一致:图像顶部 = +Y(天顶),phi = atan2(d.z, d.x)
        /// </summary>
        struct  EquirectSampler
        {
            FEImageSource   plane;
            uint32  w   =   0;
            uint32  h   =   0;
            uint32  ch  =   0;

            EquirectSampler(const FEImage& img)
            {
                const auto& cInfo   =   img.cInfo();
                w   =   cInfo._extent.x;
                h   =   cInfo._extent.y;
                ch  =   FEFormatHelper::channelOf(cInfo._format);
                if (w && h && ch >= 3)
                    plane   =   img.plane(0,0);
            }
            bool    valid() const   {   return  plane._buffer != nullptr;   }

            float3  sample(const float3& d) const
            {
                const float phi     =   atan2f(d.z,d.x);                        /// [-PI, PI]
                const float theta   =   acosf(clamp(d.y,-1.0f,1.0f));           /// [0, PI]
                float       uf      =   (phi * (0.5f / float(PI)) + 0.5f) * float(w) - 0.5f;
                float       vf      =   theta * (1.0f / float(PI)) * float(h) - 0.5f;
                uf  =   uf - floorf(uf / float(w)) * float(w);                  /// 环绕到 [0, w)
                vf  =   clamp(vf,0.0f,float(h - 1));                            /// 钳制到图内

                const int   x0  =   int(floorf(uf));
                const int   y0  =   int(floorf(vf));
                const int   x1  =   (x0 + 1) % int(w);
                const int   y1  =   y0 + 1 < int(h) ? y0 + 1 : int(h) - 1;
                const float tx  =   uf - floorf(uf);
                const float ty  =   vf - floorf(vf);

                const float*    row0    =   plane.rowAt<float>(uint32(y0));
                const float*    row1    =   plane.rowAt<float>(uint32(y1));
                const float*    p00     =   row0 + size_t(x0) * ch;
                const float*    p10     =   row0 + size_t(x1) * ch;
                const float*    p01     =   row1 + size_t(x0) * ch;
                const float*    p11     =   row1 + size_t(x1) * ch;
                float3          c;
                for (uint32 i = 0; i < 3; ++i)
                {
                    const float top     =   p00[i] * (1.0f - tx) + p10[i] * tx;
                    const float bottom  =   p01[i] * (1.0f - tx) + p11[i] * tx;
                    c[i]    =   top * (1.0f - ty) + bottom * ty;
                }
                return  c;
            }
        };

        /// 标准归一化实球谐基(9 项,y-up 约定),投影与求值共用同一实现
        inline  void    shBasis(const float3& d,float outBasis[9])
        {
            const float x   =   d.x;
            const float y   =   d.y;
            const float z   =   d.z;
            const float xx  =   x * x;
            const float yy  =   y * y;
            const float zz  =   z * z;
            outBasis[0] =   0.282095f;                    /// Y00
            outBasis[1] =   0.488603f * y;                /// Y1,-1
            outBasis[2] =   0.488603f * z;                /// Y1,0
            outBasis[3] =   0.488603f * x;                /// Y1,1
            outBasis[4] =   1.092548f * x * y;            /// Y2,-2
            outBasis[5] =   1.092548f * y * z;            /// Y2,-1
            outBasis[6] =   0.315392f * (3.0f * zz - 1.0f);   /// Y2,0
            outBasis[7] =   1.092548f * x * z;            /// Y2,1
            outBasis[8] =   0.546274f * (xx - yy);        /// Y2,2
        }

        /// ===== BRDF LUT(split-sum 第二项,Karis 2013)数值积分辅助 =====
        /// RadicalInverse 位反转(Van der Corput),Hammersley 第二维
        inline  float   radicalInverseVdC(uint32 bits)
        {
            bits    =   (bits << 16u) | (bits >> 16u);
            bits    =   ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
            bits    =   ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
            bits    =   ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
            bits    =   ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
            return  float(bits) * 2.3283064365386963e-10f;    /// / 2^32
        }
        inline  float2  hammersley(uint32 i,uint32 n)
        {
            return  float2(float(i) / float(n),radicalInverseVdC(i));
        }
        /// GGX 重要性采样半程向量(切线空间,N=(0,0,1),alpha=roughness^2 与 shader 一致)
        inline  float3  importanceSampleGGX(const float2& xi,float roughness)
        {
            const float a       =   roughness * roughness;
            const float phi     =   2.0f * float(PI) * xi.x;
            const float cosT    =   sqrtf((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
            const float sinT    =   sqrtf(1.0f - cosT * cosT);
            return  float3(sinT * cosf(phi),sinT * sinf(phi),cosT);
        }
        /// IBL 版几何遮蔽:k = alpha/2(Epic 对 IBL 的重参数化,非 punctual 的 (r+1)^2/8)
        inline  float   geometrySchlickIBL(float ndot,float roughness)
        {
            const float a   =   roughness * roughness;
            const float k   =   a * 0.5f;
            return  ndot / (ndot * (1.0f - k) + k + 1e-7f);
        }
        /// 逐 texel 积分:scale = ∫(1-Fc)·G_Vis,offset = ∫Fc·G_Vis(Fc=(1-VdotH)^5)
        inline  float2  integrateBRDF(float ndotv,float roughness,uint32 samples = 1024)
        {
            const float3    v(sqrtf(1.0f - ndotv * ndotv),0.0f,ndotv);
            float   a   =   0.0f;
            float   b   =   0.0f;
            for (uint32 i = 0; i < samples; ++i)
            {
                const float2    xi  =   hammersley(i,samples);
                const float3    h   =   importanceSampleGGX(xi,roughness);
                const float     vdh =   clamp(v.x * h.x + v.z * h.z,0.0f,1.0f);   /// dot(V,H),V.y=0
                const float3    l   =   float3(2.0f * vdh * h.x - v.x,
                                               2.0f * vdh * h.y,
                                               2.0f * vdh * h.z - v.z);          /// L=2(V·H)H-V
                const float     ndl =   l.z;
                if  (ndl > 0.0f)
                {
                    const float g       =   geometrySchlickIBL(ndotv,roughness) *
                                            geometrySchlickIBL(ndl,roughness);
                    const float gVis    =   (g * vdh) / (h.z * ndotv + 1e-7f);    /// h.z=NdotH
                    const float fc      =   powf(1.0f - vdh,5.0f);
                    a   +=  (1.0f - fc) * gVis;
                    b   +=  fc * gVis;
                }
            }
            return  float2(a / float(samples),b / float(samples));
        }

        /// ===== GGX 环境预滤波(split-sum 第一项)辅助 =====
        /// equirect 的紧凑 float mip 链(2x2 盒式下采样,u 环绕 v 钳制)。
        /// 预滤波按 Epic 的 saSample/saTexel 公式选 mip 做预积分,大幅降低收敛所需样本数
        struct  EquirectMipChain
        {
            struct  Level
            {
                uint32              w   =   0;
                uint32              h   =   0;
                std::vector<float>  px;     /// 紧凑 RGB
            };
            std::vector<Level>  levels;

            bool    build(const FEImage& img)
            {
                const auto&     cInfo   =   img.cInfo();
                const uint32    w       =   cInfo._extent.x;
                const uint32    h       =   cInfo._extent.y;
                const uint32    ch      =   FEFormatHelper::channelOf(cInfo._format);
                const bool      bFloat  =   (FEFormatHelper::sizeOf(cInfo._format) == ch * uint32(sizeof(float)));
                if  (w == 0 || h == 0 || ch < 3 || !bFloat)
                    return  false;
                const auto  plane   =   img.plane(0,0);
                if  (plane._buffer == nullptr)
                    return  false;
                Level   l0;
                l0.w    =   w;
                l0.h    =   h;
                l0.px.resize(size_t(w) * h * 3);
                for (uint32 y = 0; y < h; ++y)
                {
                    const float*    row =   plane.rowAt<float>(y);
                    float*          dst =   l0.px.data() + size_t(y) * w * 3;
                    for (uint32 x = 0; x < w; ++x)
                    {
                        dst[x * 3 + 0]  =   row[size_t(x) * ch + 0];
                        dst[x * 3 + 1]  =   row[size_t(x) * ch + 1];
                        dst[x * 3 + 2]  =   row[size_t(x) * ch + 2];
                    }
                }
                levels.push_back(std::move(l0));
                /// 逐级 2x2 盒式下采样(u 环绕取模,v 越界跳过;单边到 1 后该边停止减半)
                while (levels.back().w > 1 || levels.back().h > 1)
                {
                    const Level&    s   =   levels.back();
                    Level   d;
                    d.w =   s.w > 1 ? s.w >> 1 : 1;
                    d.h   =   s.h > 1 ? s.h >> 1 : 1;
                    d.px.resize(size_t(d.w) * d.h * 3);
                    for (uint32 y = 0; y < d.h; ++y)
                    {
                        for (uint32 x = 0; x < d.w; ++x)
                        {
                            float   acc[3]  =   { 0.0f, 0.0f, 0.0f };
                            uint32  cnt     =   0;
                            for (uint32 dy = 0; dy < 2; ++dy)
                            {
                                const uint32    sy  =   y * 2 + dy;
                                if  (sy >= s.h)
                                    continue;
                                for (uint32 dx = 0; dx < 2; ++dx)
                                {
                                    const uint32    sx  =   s.w > 1 ? (x * 2 + dx) % s.w : 0;
                                    const float*    p   =   s.px.data() + (size_t(sy) * s.w + sx) * 3;
                                    acc[0] += p[0]; acc[1] += p[1]; acc[2] += p[2];
                                    ++cnt;
                                }
                            }
                            float*          dst =   d.px.data() + (size_t(y) * d.w + x) * 3;
                            const float     inv =   1.0f / float(cnt);
                            dst[0]  =   acc[0] * inv;
                            dst[1]  =   acc[1] * inv;
                            dst[2]  =   acc[2] * inv;
                        }
                    }
                    levels.push_back(std::move(d));
                }
                return  true;
            }

            /// 按方向采样指定 mip(双线性,u 环绕,v 钳制;约定与 EquirectSampler 一致)
            float3  sample(const float3& dir,uint32 mip) const
            {
                const Level&    l   =   levels[mip < levels.size() ? mip : uint32(levels.size()) - 1];
                const float phi     =   atan2f(dir.z,dir.x);
                const float theta   =   acosf(clamp(dir.y,-1.0f,1.0f));
                float       uf      =   (phi * (0.5f / float(PI)) + 0.5f) * float(l.w) - 0.5f;
                float       vf      =   theta * (1.0f / float(PI)) * float(l.h) - 0.5f;
                uf  =   uf - floorf(uf / float(l.w)) * float(l.w);
                vf  =   clamp(vf,0.0f,float(l.h - 1));

                const int   x0  =   int(floorf(uf));
                const int   y0  =   int(floorf(vf));
                const int   x1  =   (x0 + 1) % int(l.w);
                const int   y1  =   y0 + 1 < int(l.h) ? y0 + 1 : int(l.h) - 1;
                const float tx  =   uf - floorf(uf);
                const float ty  =   vf - floorf(vf);

                const float*    p00 =   l.px.data() + (size_t(y0) * l.w + uint32(x0)) * 3;
                const float*    p10 =   l.px.data() + (size_t(y0) * l.w + uint32(x1)) * 3;
                const float*    p01 =   l.px.data() + (size_t(y1) * l.w + uint32(x0)) * 3;
                const float*    p11 =   l.px.data() + (size_t(y1) * l.w + uint32(x1)) * 3;
                float3          c;
                for (uint32 i = 0; i < 3; ++i)
                {
                    const float top     =   p00[i] * (1.0f - tx) + p10[i] * tx;
                    const float bottom  =   p01[i] * (1.0f - tx) + p11[i] * tx;
                    c[i]    =   top * (1.0f - ty) + bottom * ty;
                }
                return  c;
            }
        };

        /// 由法线方向构造正交切线基(GGX 采样从切线空间转世界空间用)
        inline  void    tangentBasis(const float3& n,float3& t,float3& b)
        {
            const float3    up  =   fabsf(n.y) < 0.999f ? float3(0.0f,1.0f,0.0f) : float3(1.0f,0.0f,0.0f);
            t   =   normalize(cross(up,n));
            b   =   cross(n,t);
        }

        /// <summary>
        /// GGX 重要性采样预滤波:把 cubemap 图像的 mip 1..N 重写为对应粗糙度的环境
        /// 卷积(Epic split-sum 第一项,与 glTF-Sample-Viewer 的 GPU 预滤波同款)。
        /// 每 texel 视 V = N = R,Hammersley 低差异序列 + GGX 重要性采样半程向量 H,
        /// 反射得 L 后按 NdotL 加权累加环境色;采样时按 pdf 估计样本立体角并选择
        /// equirect mip 预积分(saSample/saTexel,Epic 降噪技巧),低样本数即可收敛。
        /// 样本数随粗糙度自适应(低粗糙度 lobe 窄、收敛快)。
        /// mip m 对应 roughness = m/(levels-1),与 shader 侧 textureLod(R, r*maxLod) 一致
        /// </summary>
        inline  void    prefilterCubeGGX(const EquirectMipChain& eq,FEImage& cube)
        {
            const auto&     ci      =   cube.cInfo();
            const uint32    size    =   ci._extent.x;
            const uint32    levels  =   ci._levels;
            if  (size == 0 || levels < 2 || eq.levels.empty())
                return;
            const float     invMaxLod   =   1.0f / float(levels - 1);
            /// equirect level0 平均 texel 立体角(经纬图整球 4π 均摊)
            const float     saTexel     =   float(4.0 * PI) / float(eq.levels[0].w * eq.levels[0].h);
            for (uint32 m = 1; m < levels; ++m)
            {
                const float     roughness   =   float(m) * invMaxLod;
                const uint32    dim         =   (std::max<uint32>)(size >> m,1);
                const uint32    samples     =   roughness < 0.2f ? 32u :
                                                roughness < 0.5f ? 64u :
                                                roughness < 0.8f ? 128u : 256u;
                const float     a           =   roughness * roughness;
                const float     aa          =   a * a;
                for (uint32 face = 0; face < 6; ++face)
                {
                    auto    plane   =   cube.plane(m,face);
                    for (uint32 y = 0; y < dim; ++y)
                    {
                        for (uint32 x = 0; x < dim; ++x)
                        {
                            const float     fa  =   2.0f * (float(x) + 0.5f) / float(dim) - 1.0f;
                            const float     fb  =   2.0f * (float(y) + 0.5f) / float(dim) - 1.0f;
                            const float3    n   =   normalize(cubeFaceDir(int(face),fa,fb));
                            float3          t, b;
                            tangentBasis(n,t,b);
                            float3          acc(0.0f,0.0f,0.0f);
                            float           wsum    =   0.0f;
                            for (uint32 i = 0; i < samples; ++i)
                            {
                                const float2    xi  =   hammersley(i,samples);
                                const float3    ht  =   importanceSampleGGX(xi,roughness);
                                /// 切线空间 N=(0,0,1):N·H = ht.z;L = 2(N·H)H - N,N·L = 2(N·H)² - 1
                                const float     ndh =   ht.z;
                                const float     ndl =   2.0f * ndh * ndh - 1.0f;
                                if  (ndl <= 0.0f)
                                    continue;
                                const float3    h   =   t * ht.x + b * ht.y + n * ht.z;
                                const float3    l   =   h * (2.0f * ndh) - n;
                                /// pdf_L = D·NdotH/(4·VdotH),V = N 时 VdotH = NdotH,即 D/4
                                const float     dd  =   ndh * ndh * (aa - 1.0f) + 1.0f;
                                const float     pdf =   aa / (4.0f * float(PI) * dd * dd + 1e-7f);
                                const float     saSample    =   1.0f / (float(samples) * pdf + 1e-7f);
                                const float     mipF        =   0.5f * log2f(saSample / saTexel);
                                const uint32    mip         =   mipF <= 0.0f ? 0u : uint32(mipF + 0.5f);
                                const float3    c           =   eq.sample(l,mip);
                                acc     +=  c * ndl;
                                wsum    +=  ndl;
                            }
                            float*  px  =   plane.dataOffset<float>(y,x);
                            if  (wsum > 0.0f)
                            {
                                px[0]   =   acc.x / wsum;
                                px[1]   =   acc.y / wsum;
                                px[2]   =   acc.z / wsum;
                            }
                            else
                            {
                                px[0] = px[1] = px[2] = 0.0f;
                            }
                            px[3]   =   1.0f;
                        }
                    }
                }
            }
        }
    }

    TexPtr  FETextureHelper::create(FEContext& ctx,const FEImage& image,
                                    FEFilter magFilter,FEFilter minFilter,FEAddressMode addressMode)
    {
        const auto&     cInfo       =   image.cInfo();
        const uint32    width       =   cInfo._extent.x;
        const uint32    height      =   cInfo._extent.y;
        const uint32    layers      =   cInfo._layers ? cInfo._layers : 1;
        const uint32    levels      =   cInfo._levels ? cInfo._levels : 1;
        const uint32    pixelByte   =   FEFormatHelper::sizeOf(cInfo._format);
        /// 仅支持 2D(1层) 或 cubemap(6层,面为正方形)
        const bool      cube        =   (layers == 6);
        if  (width == 0 || height == 0 || pixelByte == 0)
            return  nullptr;
        if  ((!cube && layers != 1) || (cube && width != height))
            return  nullptr;

        /// 图像每行按 _align 对齐,GPU 需要紧凑行,这里逐行搬运
        std::vector<uint8>  pixels;
        for (uint32 l = 0; l < layers; ++l)
        {
            for (uint32 m = 0; m < levels; ++m)
            {
                const auto  plane   =   image.plane(m,l);
                if (plane._buffer == nullptr)
                    return  nullptr;
                const size_t    rowByte =   size_t(plane.width()) * pixelByte;
                for (uint32 y = 0; y < plane.height(); ++y)
                {
                    const uint8*    row =   plane.dataOffset<uint8>(y,0);
                    pixels.insert(pixels.end(),row,row + rowByte);
                }
            }
        }

        TexPtr  texture =   ctx.device().createTexture();
        if (texture == nullptr)
            return  nullptr;

        FETexture::CreateInfo   texInfo;
        texInfo._width      =   width;
        texInfo._height     =   height;
        texInfo._format     =   cInfo._format;
        texInfo._magFilter  =   magFilter;
        texInfo._minFilter  =   minFilter;
        texInfo._addressMode=   addressMode;
        texInfo._mips       =   levels;
        texInfo._cube       =   cube;
        if  (!texture->create(texInfo))
            return  nullptr;
        if  (!texture->update(pixels.data(),pixels.size()))
            return  nullptr;
        return  texture;
    }

    TexPtr  FETextureHelper::load(FEContext& ctx,const char* fileName,
                                  FEFilter magFilter,FEFilter minFilter,FEAddressMode addressMode)
    {
        ImagePtr    image   =   FEImageHelper::load(ctx,fileName);
        if (image == nullptr)
            return  nullptr;
        return  create(ctx,*image,magFilter,minFilter,addressMode);
    }

    ImagePtr    FETextureHelper::cubeImageFromEquirect(FEContext& ctx,const FEImage& equirect,uint size)
    {
        const auto&     cInfo   =   equirect.cInfo();
        const uint32    eqW     =   cInfo._extent.x;
        const uint32    eqH     =   cInfo._extent.y;
        const uint32    channel =   FEFormatHelper::channelOf(cInfo._format);
        /// 输入必须是浮点格式(每通道 4 字节)且至少 3 通道
        const bool      bFloat  =   (FEFormatHelper::sizeOf(cInfo._format) == channel * uint32(sizeof(float)));
        if  (size == 0 || eqW == 0 || eqH == 0 || channel < 3 || !bFloat)
            return  nullptr;
        EquirectSampler eq(equirect);
        if  (!eq.valid())
            return  nullptr;

        uint    mipCount    =   1;
        for (uint s = size; s > 1; s >>= 1)
            ++mipCount;

        ImagePtr    image   =   new FEImage(ctx);
        FEImage::CreateInfo ci;
        ci._extent  =   uint3(size,size,1);
        ci._format  =   FMT_R32G32B32A32_FLOAT;
        ci._layers  =   6;
        ci._levels  =   uint16(mipCount);
        ci._align   =   4;
        if  (!image->create(ci))
            return  nullptr;

        /// level0:逐面逐像素做 equirect -> cube 重投影
        for (uint face = 0; face < 6; ++face)
        {
            auto    plane   =   image->plane(0,face);
            for (uint y = 0; y < size; ++y)
            {
                for (uint x = 0; x < size; ++x)
                {
                    const float a   =   2.0f * (x + 0.5f) / float(size) - 1.0f;
                    const float b   =   2.0f * (y + 0.5f) / float(size) - 1.0f;
                    const float3    c = eq.sample(normalize(cubeFaceDir(int(face),a,b)));
                    float*  px  =   plane.dataOffset<float>(y,x);
                    px[0]   =   c.x;
                    px[1]   =   c.y;
                    px[2]   =   c.z;
                    px[3]   =   1.0f;
                }
            }
        }

        /// mip 链:2x2 盒式滤波(逐面独立,float 域平均保留 HDR 范围)
        for (uint m = 1; m < mipCount; ++m)
        {
            const uint  dstDim  =   (std::max<uint32>)(size >> m,1);
            for (uint face = 0; face < 6; ++face)
            {
                const auto  src =   image->plane(m - 1,face);
                auto        dst =   image->plane(m,face);
                for (uint y = 0; y < dstDim; ++y)
                {
                    for (uint x = 0; x < dstDim; ++x)
                    {
                        float   sum[3]  =   { 0.0f, 0.0f, 0.0f };
                        for (int dy = 0; dy < 2; ++dy)
                        {
                            for (int dx = 0; dx < 2; ++dx)
                            {
                                const float*    px  =   src.dataOffset<float>(y * 2 + dy,x * 2 + dx);
                                sum[0] += px[0]; sum[1] += px[1]; sum[2] += px[2];
                            }
                        }
                        float*  px  =   dst.dataOffset<float>(y,x);
                        px[0]   =   sum[0] * 0.25f;
                        px[1]   =   sum[1] * 0.25f;
                        px[2]   =   sum[2] * 0.25f;
                        px[3]   =   1.0f;
                    }
                }
            }
        }
        return  image;
    }

    TexPtr  FETextureHelper::createCube(FEContext& ctx,const FEImage& equirect,uint size,
                                        FEFilter magFilter,FEFilter minFilter,FEAddressMode addressMode)
    {
        ImagePtr    cube    =   cubeImageFromEquirect(ctx,equirect,size);
        if (cube == nullptr)
            return  nullptr;
        /// IBL 镜面预滤波:mip 1..N 重写为 GGX 重要性采样卷积。
        /// 盒式滤波 mip 与 GGX lobe 形状不匹配(各向同性过度模糊且能量不守恒),
        /// 车漆等低粗糙度表面上呈错误高光宽度与噪点,必须按粗糙度逐 mip 卷积
        EquirectMipChain    eqMips;
        if  (eqMips.build(equirect))
            prefilterCubeGGX(eqMips,*cube);
        return  create(ctx,*cube,magFilter,minFilter,addressMode);
    }

    TexPtr  FETextureHelper::createIrradianceCube(FEContext& ctx,const FEImage& equirect,uint size)
    {
        const auto&     cInfo   =   equirect.cInfo();
        const uint32    eqW     =   cInfo._extent.x;
        const uint32    eqH     =   cInfo._extent.y;
        const uint32    channel =   FEFormatHelper::channelOf(cInfo._format);
        const bool      bFloat  =   (FEFormatHelper::sizeOf(cInfo._format) == channel * uint32(sizeof(float)));
        if  (size == 0 || eqW == 0 || eqH == 0 || channel < 3 || !bFloat)
            return  nullptr;
        EquirectSampler eq(equirect);
        if  (!eq.valid())
            return  nullptr;

        /// 1. 环境投影为 2 阶球谐:L_k = ∫ L(s)·Y_k(s)·dω
        ///    逐行解析计算立体角(dφ·Δcosθ),无需蒙特卡洛采样
        double  sh[9][3]    =   {};
        const float dPhi   =   float(2.0 * PI / double(eqW));
        for (uint32 y = 0; y < eqH; ++y)
        {
            const float t0  =   (float(y)         / float(eqH)) * float(PI);    /// 行上边缘极角
            const float t1  =   (float(y + 1)     / float(eqH)) * float(PI);    /// 行下边缘极角
            const float rowSolidAngle   =   dPhi * (cosf(t0) - cosf(t1));
            const float theta   =   (float(y + 0.5) / float(eqH)) * float(PI);  /// 行中心极角
            const float sinT    =   sinf(theta);
            const float cosT    =   cosf(theta);
            for (uint32 x = 0; x < eqW; ++x)
            {
                const float phi =   ((float(x) + 0.5f) / float(eqW)) * float(2.0 * PI) - float(PI);
                const float3    d(sinT * cosf(phi), cosT, sinT * sinf(phi));
                const float*    px  =   eq.plane.rowAt<float>(y) + size_t(x) * channel;
                float   basis[9];
                shBasis(d,basis);
                for (int k = 0; k < 9; ++k)
                {
                    sh[k][0]    +=  double(basis[k]) * double(px[0]) * rowSolidAngle;
                    sh[k][1]    +=  double(basis[k]) * double(px[1]) * rowSolidAngle;
                    sh[k][2]    +=  double(basis[k]) * double(px[2]) * rowSolidAngle;
                }
            }
        }

        /// 2. 余弦卷积解析求值:E(n) = Σ_k Ǎ_k · L_k · Y_k(n)
        ///    (Ramamoorthi 2001:Ǎ_0 = π,Ǎ_1 = 2π/3,Ǎ_2 = π/4)
        static const float  kIrradCoeff[9]  =
        {
            float(PI),                          /// band 0
            float(2.0 * PI / 3.0),              /// band 1 ×3
            float(2.0 * PI / 3.0),
            float(2.0 * PI / 3.0),
            float(PI / 4.0),                    /// band 2 ×5
            float(PI / 4.0),
            float(PI / 4.0),
            float(PI / 4.0),
            float(PI / 4.0),
        };

        ImagePtr    image   =   new FEImage(ctx);
        FEImage::CreateInfo ci;
        ci._extent  =   uint3(size,size,1);
        ci._format  =   FMT_R32G32B32A32_FLOAT;
        ci._layers  =   6;
        ci._levels  =   1;      /// irradiance 平滑无细节,单 mip 即可
        ci._align   =   4;
        if  (!image->create(ci))
            return  nullptr;

        for (uint face = 0; face < 6; ++face)
        {
            auto    plane   =   image->plane(0,face);
            for (uint y = 0; y < size; ++y)
            {
                for (uint x = 0; x < size; ++x)
                {
                    const float a   =   2.0f * (x + 0.5f) / float(size) - 1.0f;
                    const float b   =   2.0f * (y + 0.5f) / float(size) - 1.0f;
                    const float3    n   =   normalize(cubeFaceDir(int(face),a,b));
                    float   basis[9];
                    shBasis(n,basis);
                    float3  e(0.0f,0.0f,0.0f);
                    for (int k = 0; k < 9; ++k)
                    {
                        const float w   =   kIrradCoeff[k] * basis[k];
                        e[0]    +=  w * float(sh[k][0]);
                        e[1]    +=  w * float(sh[k][1]);
                        e[2]    +=  w * float(sh[k][2]);
                    }
                    float*  px  =   plane.dataOffset<float>(y,x);
                    px[0]   =   e[0];
                    px[1]   =   e[1];
                    px[2]   =   e[2];
                    px[3]   =   1.0f;
                }
            }
        }
        return  create(ctx,*image,FILTER_LINEAR,FILTER_LINEAR,ADDRESS_CLAMP_TO_EDGE);
    }

    TexPtr  FETextureHelper::createBRDFLUT(FEContext& ctx,uint size)
    {
        if  (size == 0)
            return  nullptr;
        ImagePtr    image   =   new FEImage(ctx);
        FEImage::CreateInfo ci;
        ci._extent  =   uint3(size,size,1);
        ci._format  =   FMT_R32G32B32A32_FLOAT;
        ci._layers  =   1;
        ci._levels  =   1;
        ci._align   =   4;
        if  (!image->create(ci))
            return  nullptr;
        auto    plane   =   image->plane(0,0);
        if  (plane._buffer == nullptr)
            return  nullptr;
        for (uint y = 0; y < size; ++y)
        {
            const float roughness   =   (float(y) + 0.5f) / float(size);
            for (uint x = 0; x < size; ++x)
            {
                const float ndotv   =   (float(x) + 0.5f) / float(size);
                const float2    ab  =   integrateBRDF(ndotv,roughness);
                float*  px  =   plane.dataOffset<float>(y,x);
                px[0]   =   ab.x;   /// scale
                px[1]   =   ab.y;   /// offset
                px[2]   =   0.0f;
                px[3]   =   1.0f;
            }
        }
        return  create(ctx,*image,FILTER_LINEAR,FILTER_LINEAR,ADDRESS_CLAMP_TO_EDGE);
    }

    TexPtr  FETextureHelper::createStudioCube(FEContext& ctx,uint size)
    {
        if (size == 0)
            return  nullptr;
        uint    mipCount    =   1;
        for (uint s = size; s > 1; s >>= 1)
            ++mipCount;

        ImagePtr    image   =   new FEImage(ctx);
        FEImage::CreateInfo ci;
        ci._extent  =   uint3(size,size,1);
        ci._format  =   FMT_R32G32B32A32_FLOAT;
        ci._layers  =   6;
        ci._levels  =   uint16(mipCount);
        ci._align   =   4;
        if  (!image->create(ci))
            return  nullptr;

        /// 影棚柔光箱方向(朝向原点)与强度:主光/辅光/背光。
        /// 强度与锐度刻意温和(低峰宽斑):车漆 flakes 法线逐像素扰动反射方向时,
        /// 采样到的仍是平滑渐变而非黑底上的孤立亮点,避免盐椒噪点
        const float3    lightDirs[3]    =   { normalize(float3( 0.5f, 0.8f,  0.6f)),
                                              normalize(float3(-0.7f, 0.5f,  0.4f)),
                                              normalize(float3( 0.0f, 0.6f, -1.0f)) };
        const float     lightIntens[3]  =   { 6.0f, 3.5f, 4.5f };
        const float     lightSharp      =   8.0f;      /// 锥角锐度(宽斑柔光)

        for (uint face = 0; face < 6; ++face)
        {
            auto    plane   =   image->plane(0,face);
            for (uint y = 0; y < size; ++y)
            {
                for (uint x = 0; x < size; ++x)
                {
                    const float a   =   2.0f * (x + 0.5f) / float(size) - 1.0f;
                    const float b   =   2.0f * (y + 0.5f) / float(size) - 1.0f;
                    const float3    d   =   normalize(cubeFaceDir(int(face),a,b));
                    /// 天空/地面渐变:中等亮度(避免黑底盐椒噪点,也不过亮洗掉车漆深红)
                    const float up  =   d.y;
                    float3  sky     =   (up > 0.0f)
                                        ?   mix(float3(0.45f,0.47f,0.54f), float3(0.68f,0.74f,0.90f), powf(up,0.6f))
                                        :   mix(float3(0.45f,0.47f,0.54f), float3(0.24f,0.24f,0.26f), powf(-up,0.5f));
                    float3  c       =   sky;
                    /// 柔光箱:方向锥内叠加 HDR 强光(mip 模糊后呈平滑白色光带)
                    for (int i = 0; i < 3; ++i)
                    {
                        const float spot    =   powf((std::max)(dot(d,lightDirs[i]),0.0f),lightSharp);
                        c   +=   float3(1.0f,0.98f,0.94f) * (spot * lightIntens[i]);
                    }
                    float*  px  =   plane.dataOffset<float>(y,x);
                    px[0]   =   c.x;
                    px[1]   =   c.y;
                    px[2]   =   c.z;
                    px[3]   =   1.0f;
                }
            }
        }

        /// mip 链:2x2 盒式滤波(柔光箱与渐变天然平滑)
        for (uint m = 1; m < mipCount; ++m)
        {
            const uint  dstDim  =   (std::max<uint32>)(size >> m,1);
            for (uint face = 0; face < 6; ++face)
            {
                const auto  src =   image->plane(m - 1,face);
                auto        dst =   image->plane(m,face);
                for (uint y = 0; y < dstDim; ++y)
                {
                    for (uint x = 0; x < dstDim; ++x)
                    {
                        float   sum[3]  =   { 0.0f, 0.0f, 0.0f };
                        for (int dy = 0; dy < 2; ++dy)
                        {
                            for (int dx = 0; dx < 2; ++dx)
                            {
                                const float*    px  =   src.dataOffset<float>(y * 2 + dy,x * 2 + dx);
                                sum[0] += px[0]; sum[1] += px[1]; sum[2] += px[2];
                            }
                        }
                        float*  px  =   dst.dataOffset<float>(y,x);
                        px[0]   =   sum[0] * 0.25f;
                        px[1]   =   sum[1] * 0.25f;
                        px[2]   =   sum[2] * 0.25f;
                        px[3]   =   1.0f;
                    }
                }
            }
        }
        return  create(ctx,*image);
    }
}
