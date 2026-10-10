#pragma     once
#include    "../../FEObject.h"

namespace   FE
{
    struct  FEParamBox
    {
        /// <summary>
        /// 立方体的大小
        /// </summary>
        float3  _size   =   float3(1,1,1);
        /// <summary>
        /// 分段数量
        /// </summary>
        uint3   _segs   =   uint3(1,1,1);
    };

    class   FEBoxImpl
    {
    public:
        uints       indices;
        floats      vertices;
        floats      normals;
        floats      uvs;
        uint32_t    numberOfVertices    =   0;
    public:
        template<bool bNormal,bool bUV>
        static  void    build(const FEParamBox& params,FEBoxImpl& result)
        {
            const auto  width   =   params._size.x;
            const auto  height  =   params._size.y;
            const auto  depth   =   params._size.z;
            const auto  wSegs   =   params._segs.x;
            const auto  hSegs   =   params._segs.y;
            const auto  dSegs   =   params._segs.z;

            const auto  wSeg    =   wSegs + 1;
            const auto  hSeg    =   hSegs + 1;
            const auto  dSeg    =   dSegs + 1;

            uint        nSize   =   (dSeg * hSeg)
                +   (dSeg * hSeg)
                +   (wSeg * hSeg)
                +   (wSeg * dSeg)
                +   (wSeg * hSeg)
                +   (hSeg * hSeg);
            uint        nIndex  =   (dSegs * hSegs)
                +   (dSegs * hSegs)
                +   (wSegs * hSegs)
                +   (wSegs * dSegs)
                +   (wSegs * hSegs)
                +   (hSegs * hSegs);
            result.vertices.reserve(nSize * 3);
            if constexpr (bNormal)
                result.normals.reserve(nSize * 3);
            if constexpr (bUV)
                result.uvs.reserve(nSize * 2);

            result.indices.reserve(nIndex * 6);

            buildPlane<bNormal,    bUV>(2, 1, 0, -1, -1, depth, height, +width, dSegs, hSegs, result); // px
            buildPlane<bNormal,    bUV>(2, 1, 0, +1, -1, depth, height, -width, dSegs, hSegs, result); // nx
            buildPlane<bNormal,    bUV>(0, 2, 1, +1, +1, width, depth,  +height,wSegs, dSegs, result); // py
            buildPlane<bNormal,    bUV>(0, 2, 1, +1, -1, width, depth,  -height,wSegs, dSegs, result); // ny
            buildPlane<bNormal,    bUV>(0, 1, 2, +1, -1, width, height, +depth, wSegs, hSegs, result); // pz
            buildPlane<bNormal,    bUV>(0, 1, 2, -1, -1, width, height, -depth, wSegs, hSegs, result); // nz
        }
    protected:
        template<bool bNormal,bool bUV>
        static  void    buildPlane(int u, int v, int w, float udir, float vdir, float width, float height, float depth, uint gridX, uint gridY, FEBoxImpl& result)
        {
            const auto  segmentWidth     =   width / static_cast<float>(gridX);
            const auto  segmentHeight    =   height / static_cast<float>(gridY);

            const auto  widthHalf       =   width  * 0.5f;
            const auto  heightHalf      =   height * 0.5f;
            const auto  depthHalf       =   depth  * 0.5f;

            const auto  gridX1          =   gridX + 1;
            const auto  gridY1          =   gridY + 1;

            int         vertexCounter   =   0;

            float3      vector;
            // generate vertices, normals and uvs
            for (unsigned iy = 0; iy < gridY1; iy++)
            {
                const auto y = static_cast<float>(iy) * segmentHeight - heightHalf;
                for (unsigned ix = 0; ix < gridX1; ix++)
                {
                    const auto x = static_cast<float>(ix) * segmentWidth - widthHalf;
                    // set values to correct vector component
                    vector[u] = x * udir;
                    vector[v] = y * vdir;
                    vector[w] = depthHalf;

                    // now apply vector to vertex buffer
                    result.vertices.insert(result.vertices.end(), {vector.x, vector.y, vector.z});
                    // set values to correct normal component
                    vector[u] = 0;
                    vector[v] = 0;
                    vector[w] = depth > 0 ? 1.f : -1.f;
                    // now apply vector to normal buffer
                    if constexpr (bNormal)
                    {
                        result.normals.insert(result.normals.end(), {vector.x, vector.y, vector.z});
                    }
                    // uvs
                    if constexpr (bUV)
                    {
                        result.uvs.emplace_back(static_cast<float>(ix) / static_cast<float>(gridX));
                        result.uvs.emplace_back(1.0f - (static_cast<float>(iy) / static_cast<float>(gridY)));
                    }
                    // counters
                    ++vertexCounter;
                }
            }
            // indices
            // 1. you need three indices to draw a single face
            // 2. a single segment consists of two faces
            // 3. so we need to generate six (2*3) indices per segment
            for (unsigned iy = 0; iy < gridY; iy++)
            {
                for (unsigned ix = 0; ix < gridX; ix++)
                {
                    const auto a = result.numberOfVertices + ix + gridX1 * iy;
                    const auto b = result.numberOfVertices + ix + gridX1 * (iy + 1);
                    const auto c = result.numberOfVertices + (ix + 1) + gridX1 * (iy + 1);
                    const auto d = result.numberOfVertices + (ix + 1) + gridX1 * iy;
                    // faces
                    result.indices.insert(result.indices.end(), {a, b, d});
                    result.indices.insert(result.indices.end(), {b, c, d});
                }
            }
            // update total number of vertices
            result.numberOfVertices += vertexCounter;
        }
    };
}
