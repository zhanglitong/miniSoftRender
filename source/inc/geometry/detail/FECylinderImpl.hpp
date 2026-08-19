
#pragma     once

#include    "../../FEObject.h"


namespace   FE
{
    struct  FEParamCylinder
    {
        /// <summary>
        /// 圆柱体顶部圆盘的半径
        /// 说明：默认值为 1。如果设置为 0，则会生成一个圆锥体
        /// </summary>
        float   radiusTop       =   1.0f;
        /// <summary>
        /// 圆柱体底部圆盘的半径
        /// </summary>
        float   radiusBottom    =   1.0f;
        /// <summary>
        /// 圆柱体的高度(即顶部与底部圆心之间的距离)
        /// </summary>
        float   height          =   1.0f;
        /// <summary>
        /// 圆周方向的分段数（侧面由多少个平面组成）
        /// 默认值为 16。数值越大，圆柱侧面越圆滑；数值越小(如 3 或 4)，可以生成三棱柱或四棱柱.
        /// </summary>
        uint    radialSegments  =   16;
        /// <summary>
        /// 默认值为 1。如果需要对圆柱进行顶点位移(如弯曲效果)，通常需要增加此值.
        /// </summary>
        uint    heightSegments  =   1;
        /// <summary>
        /// 圆周起始角度,默认值为 0(通常对应 X 轴正方向).
        /// </summary>
        float   thetaStart      =   0.0f;
        /// <summary>
        /// 圆周绘制的中心角大小
        /// 默认值为 TWO_PI(即 2π，完整的一圈)。如果设置小于 2π，则会生成缺口圆柱(扇形柱体).
        /// </summary>
        float   thetaLength     =   float(TWO_PI);
        /// <summary>
        /// 是否封口,默认值为 false(有顶盖和底盖)。如果设为 true，则圆柱体是两端打通的管状.
        /// </summary>
        bool    openEnded       =   false;
    };

    using   Param   =   FEParamCylinder;

    class   FECylinderImpl
    {
    public:
        uints       indices;
        floats      vertices;
        floats      normals;
        floats      uvs;
    public:
        template<bool bNormal,bool bUV>
        static  void    build(const Param& params,FECylinderImpl& result)
        {
            build<bNormal,bUV>(params,result.vertices,result.normals,result.uvs,result.indices);
        }
        template<bool bNormal,bool bUV>
        static  void    build(   const Param& params
                                ,floats&    vs
                                ,floats&    ns
                                ,floats&    us
                                ,uints&     ids)
        {
            uints&  indices     =   ids;
            uint    index       =   0;
            auto    halfHeight  =   params.height / 2;
            std::vector<uints> indexArray;

            auto    generateTorso   =   [&]
            {
                float3  normal;
                float3  vertex;
                // this will be used to calculate the normal
                const auto slope = (params.radiusBottom - params.radiusTop) / params.height;
                // generate vertices, normals and uvs
                for (uint y = 0; y <= params.heightSegments; y++)
                {
                    uints   indexRow;
                    const auto  v       =   (float)(y) / (float)(params.heightSegments);
                    // calculate the radius of the current row
                    const auto radius   =   v * (params.radiusBottom - params.radiusTop) + params.radiusTop;

                    for (uint x = 0; x <= params.radialSegments; x++) {

                        const auto u        =   (float)(x) / (float)(params.radialSegments);
                        const auto theta    =   u * params.thetaLength + params.thetaStart;
                        const auto sinTheta =   std::sin(theta);
                        const auto cosTheta =   std::cos(theta);
                        // vertex
                        vertex.x    =   radius * sinTheta;
                        vertex.y    =   -v * params.height + halfHeight;
                        vertex.z    =   radius * cosTheta;
                        vs.insert(vs.end(),{vertex.x,vertex.y,vertex.z});
                        // normal
                        if constexpr(bNormal)
                        {
                            normal  =   float3(sinTheta, slope, cosTheta);
                            normal  =   FE::normalize(normal);
                            ns.insert(ns.end(),{normal.x,normal.y,normal.z});
                        }
                        // uv
                        if constexpr(bUV)
                        {
                            us.insert(us.end(),{u, 1 - v});
                        }
                        // save index of vertex in respective row
                        indexRow.emplace_back(index++);
                    }

                    // now save vertices of the row in our index array
                    indexArray.emplace_back(indexRow);
                }
                // generate indices
                for (uint x = 0; x < params.radialSegments; x++)
                {
                    for (uint y = 0; y < params.heightSegments; y++)
                    {
                        const auto a = indexArray[y][x];
                        const auto b = indexArray[y + 1][x];
                        const auto c = indexArray[y + 1][x + 1];
                        const auto d = indexArray[y][x + 1];
                        // faces
                        indices.insert(indices.end(), {a, b, d});
                        indices.insert(indices.end(), {b, c, d});
                    }
                }
            };

            generateTorso();

            auto generateCap = [&](bool top)
            {
                // save the index of the first center vertex
                auto    centerIndexStart = index;
                float2  uv;
                float3  vertex;
                float   radius    = (top) ? params.radiusTop : params.radiusBottom;
                float   sign      = (top) ? 1.0f : -1.0f;

                // first we generate the center vertex data of the cap.
                // because the geometry needs one set of uvs per face,
                // we must generate a center vertex per face/segment

                for (uint x = 1; x <= params.radialSegments; x++)
                {
                    vs.insert(vs.end(),{0, halfHeight * sign, 0});
                    // normal
                    if constexpr(bNormal)
                        ns.insert(ns.end(),{0, sign, 0});
                    // uv
                    if constexpr(bUV)
                        us.insert(us.end(),{0.5f, 0.5f});
                    // increase index
                    index++;
                }
                // save the index of the last center vertex
                const auto centerIndexEnd = index;
                // now we generate the surrounding vertices, normals and uvs
                for (uint x = 0; x <= params.radialSegments; x++)
                {
                    float u         =   (float)(x)/(float)(params.radialSegments);
                    float theta     =   u * params.thetaLength + params.thetaStart;

                    float cosTheta  =   std::cos(theta);
                    float sinTheta  =   std::sin(theta);
                    // vertex
                    vertex.x        =   radius * sinTheta;
                    vertex.y        =   halfHeight * sign;
                    vertex.z        =   radius * cosTheta;
                    vs.insert(vs.end(),{vertex.x,vertex.y,vertex.z});
                    // normal
                    if constexpr(bNormal)
                        ns.insert(ns.end(), {0, sign, 0});
                    // uv
                    if constexpr(bUV)
                    {
                        uv.x    =   (cosTheta * 0.5f) + 0.5f;
                        uv.y    =   (sinTheta * 0.5f * sign) + 0.5f;
                        us.insert(us.end(),{uv.x, uv.y});
                    }
                    // increase index
                    ++index;
                }
                // generate indices
                for (uint x = 0; x < params.radialSegments; x++)
                {
                    uint c = centerIndexStart + x;
                    uint i = centerIndexEnd + x;
                    // face top
                    if (top)
                        indices.insert(indices.end(), {i, i + 1, c});
                    // face bottom
                    else
                        indices.insert(indices.end(), {i + 1, i, c});
                }
            };

            if (!params.openEnded)
            {
                if (params.radiusTop > 0)      generateCap(true);
                if (params.radiusBottom > 0)   generateCap(false);
            }
        }
    };
}
