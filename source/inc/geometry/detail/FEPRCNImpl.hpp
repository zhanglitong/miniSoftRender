#pragma     once
#include    "../../FEObject.h"
#include    "../../FEMath.hpp"

namespace   FE
{
    /// <summary>
    /// 圆台体参数 (Frustum with offset and tilt)
    /// </summary>
    struct  FEParamPRCN
    {
        /// <summary>下表面圆半径</summary>
        float   bottomRadius     =   1.0f;
        /// <summary>上表面圆半径</summary>
        float   topRadius        =   0.8f;
        /// <summary>高度</summary>
        float   height           =   1.0f;
        /// <summary>上表面圆心 X 方向偏移</summary>
        float   xOffset          =   0.0f;
        /// <summary>上表面圆心 Y 方向偏移</summary>
        float   yOffset          =   0.0f;
        /// <summary>下表面与 X 轴偏角(弧度)</summary>
        float   bottomTiltX      =   0.0f;
        /// <summary>下表面与 Y 轴偏角(弧度)</summary>
        float   bottomTiltY      =   0.0f;
        /// <summary>上表面与 X 轴偏角(弧度)</summary>
        float   topTiltX         =   0.0f;
        /// <summary>上表面与 Y 轴偏角(弧度)</summary>
        float   topTiltY         =   0.0f;
        /// <summary>圆周分段数</summary>
        uint    radialSegments   =   32;
    };

    class   FEPRCNImpl
    {
    public:
        uints       indices;
        floats      vertices;
        floats      normals;
        floats      uvs;
    public:
        template<bool bNormal,bool bUV>
        static  void    build(const FEParamPRCN& params,FEPRCNImpl& result)
        {
            auto&   vs      =   result.vertices;
            auto&   ns      =   result.normals;
            auto&   us      =   result.uvs;
            auto&   ids     =   result.indices;

            const uint  segs    =   std::max(params.radialSegments, 3u);
            const float halfH   =   params.height * 0.5f;

            /// 底面倾斜旋转矩阵
            auto    fnRotTilt = [](float tiltX, float tiltY)
            {
                float3  axisX(1,0,0);
                float3  axisY(0,0,1);
                /// 先绕 X 轴旋转, 再绕 Z 轴旋转 (Y 轴偏角用 Z 轴旋转近似)
                auto    rx  =   glm::angleAxis(tiltX, axisX);
                auto    rz  =   glm::angleAxis(tiltY, float3(0,1,0));
                return  rz * rx;
            };

            auto    botRot  =   fnRotTilt(params.bottomTiltX, params.bottomTiltY);
            auto    topRot  =   fnRotTilt(params.topTiltX, params.topTiltY);

            uint    index   =   0;
            std::vector<uint> botIdx(segs + 1), topIdx(segs + 1);

            for (uint i = 0; i <= segs; ++i)
            {
                float   t       =   (float)i / (float)segs;
                float   theta   =   t * float(TWO_PI);
                float   ct      =   std::cos(theta);
                float   st      =   std::sin(theta);

                /// 底面顶点 (中心在原点, y=-halfH)
                float3  bp(ct * params.bottomRadius, -halfH, st * params.bottomRadius);
                bp      =   botRot * bp;
                float3  bn  =   botRot * float3(0, -1, 0);
                vs.insert(vs.end(),{bp.x, bp.y, bp.z});
                if constexpr (bNormal) ns.insert(ns.end(),{bn.x, bn.y, bn.z});
                if constexpr (bUV)     us.insert(us.end(),{t, 0});
                botIdx[i]   =   index++;

                /// 顶面顶点 (中心偏移, y=+halfH)
                float3  tp(ct * params.topRadius + params.xOffset, halfH, st * params.topRadius + params.yOffset);
                tp      =   topRot * tp + float3(params.xOffset, 0, params.yOffset) - topRot * float3(params.xOffset, 0, params.yOffset);
                float3  tn  =   topRot * float3(0, 1, 0);
                vs.insert(vs.end(),{tp.x, tp.y, tp.z});
                if constexpr (bNormal) ns.insert(ns.end(),{tn.x, tn.y, tn.z});
                if constexpr (bUV)     us.insert(us.end(),{t, 1});
                topIdx[i]   =   index++;
            }

            /// 侧面
            for (uint i = 0; i < segs; ++i)
            {
                uint    b0 = botIdx[i], b1 = botIdx[i+1];
                uint    t0 = topIdx[i], t1 = topIdx[i+1];
                ids.insert(ids.end(),{b0, b1, t0});
                ids.insert(ids.end(),{b1, t1, t0});
            }

            /// 底面封口
            uint    botCenter = index;
            vs.insert(vs.end(),{0, -halfH, 0});
            if constexpr (bNormal) ns.insert(ns.end(),{0, -1, 0});
            if constexpr (bUV)     us.insert(us.end(),{0.5f, 0.5f});
            index++;
            for (uint i = 0; i < segs; ++i)
            {
                ids.insert(ids.end(),{botCenter, botIdx[i+1], botIdx[i]});
            }

            /// 顶面封口
            uint    topCenter = index;
            vs.insert(vs.end(),{params.xOffset, halfH, params.yOffset});
            if constexpr (bNormal) ns.insert(ns.end(),{0, 1, 0});
            if constexpr (bUV)     us.insert(us.end(),{0.5f, 0.5f});
            index++;
            for (uint i = 0; i < segs; ++i)
            {
                ids.insert(ids.end(),{topCenter, topIdx[i], topIdx[i+1]});
            }
        }
    };
}
