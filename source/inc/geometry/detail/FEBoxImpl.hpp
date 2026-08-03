#pragma     once
#include    "../../FEObject.h"

namespace   FE
{
    class   FEBoxImpl :public FEObject
    {
    public:
        FEBoxImpl(FEContext& ctx)
            :FEObject(ctx)
        {}
        uints       indices;
        floats      vertices;
        floats      normals;
        floats      uvs;
        uint32_t    numberOfVertices    =   0;
    };

    using   FEBoxImplPtr    =   SharedPtr<FEBoxImpl>;
    /// <summary>
    /// 模板的目的，静态编译，避免循中判断
    /// </summary>
    /// <typeparam name="bNormal"></typeparam>
    /// <typeparam name="bUV"></typeparam>
    template<bool bNormal = true,bool bUV = true>
    struct  TBoxHelper :public FEBoxImpl
    {
    public:
        float3      size;
        uint3       segs;
        explicit TBoxHelper(FEContext& ctx,const float3& vSize,const uint3& vSegs,bool bSphere = false)
            :FEBoxImpl(ctx)
        {
            this->size   =   vSize;
            this->segs   =   vSegs;
            build();
            if (bSphere)    toSphere();
        }
    protected:
        void    toSphere()
        {
            uint    len =   (uint)(vertices.size() / 3);
            for (uint i = 0; i < len; ++i)
            {
                const auto  idx =   i * 3;
                const auto  x   =   vertices[idx + 0];
                const auto  y   =   vertices[idx + 1];
                const auto  z   =   vertices[idx + 2];

                vertices[idx + 0]     =   x * std::sqrt(1.0f - (y * y / 2.f) - (z * z / 2.f) + (y * y * z * z / 3.f));
                vertices[idx + 1]     =   y * std::sqrt(1.0f - (z * z / 2.f) - (x * x / 2.f) + (z * z * x * x / 3.f));
                vertices[idx + 2]     =   z * std::sqrt(1.0f - (x * x / 2.f) - (y * y / 2.f) + (x * x * y * y / 3.f));

                float3  vNor(vertices[idx + 0],vertices[idx + 1],vertices[idx + 2]);
                vNor    =   FE::normalize(vNor);
                normals[idx + 0]     =      vNor.x;
                normals[idx + 1]     =      vNor.y;
                normals[idx + 2]     =      vNor.z;
            }
        }
        void    build()
        {
            const auto  width   =   size.x;
            const auto  height  =   size.y;
            const auto  depth   =   size.z;
            const auto  wSegs   =   segs.x;
            const auto  hSegs   =   segs.y;
            const auto  dSegs   =   segs.z;


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
            vertices.reserve(nSize * 3);
            if (bNormal)
                normals.reserve(nSize * 3);
            if (bUV)
                uvs.reserve(nSize * 2);

            indices.reserve(nIndex * 6);

            buildPlane(2, 1, 0, -1, -1, depth, height, +width, dSegs, hSegs); // px
            buildPlane(2, 1, 0, +1, -1, depth, height, -width, dSegs, hSegs); // nx
            buildPlane(0, 2, 1, +1, +1, width, depth,  +height,wSegs, dSegs); // py
            buildPlane(0, 2, 1, +1, -1, width, depth,  -height,wSegs, dSegs); // ny
            buildPlane(0, 1, 2, +1, -1, width, height, +depth, wSegs, hSegs); // pz
            buildPlane(0, 1, 2, -1, -1, width, height, -depth, wSegs, hSegs); // nz
        }
        void    buildPlane(int u, int v, int w, float udir, float vdir, float width, float height, float depth, uint gridX, uint gridY)
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
                    vertices.insert(vertices.end(), {vector.x, vector.y, vector.z});
                    // set values to correct vector component
                    vector[u] = 0;
                    vector[v] = 0;
                    vector[w] = depth > 0 ? 1.f : -1.f;
                    // now apply vector to normal buffer
                    if constexpr (bNormal)
                    {
                        normals.insert(normals.end(), {vector.x, vector.y, vector.z});
                    }
                    // uvs
                    if constexpr (bUV)
                    {
                        uvs.emplace_back(static_cast<float>(ix) / static_cast<float>(gridX));
                        uvs.emplace_back(1.0f - (static_cast<float>(iy) / static_cast<float>(gridY)));
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
                    const auto a = numberOfVertices + ix + gridX1 * iy;
                    const auto b = numberOfVertices + ix + gridX1 * (iy + 1);
                    const auto c = numberOfVertices + (ix + 1) + gridX1 * (iy + 1);
                    const auto d = numberOfVertices + (ix + 1) + gridX1 * iy;
                    // faces
                    indices.insert(indices.end(), {a, b, d});
                    indices.insert(indices.end(), {b, c, d});
                }
            }
            // update total number of vertices
            numberOfVertices += vertexCounter;
        }
    };
}
