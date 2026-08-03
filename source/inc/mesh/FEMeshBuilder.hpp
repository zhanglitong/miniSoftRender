#pragma     once

#include    "FEMesh.hpp"
#include    "FEPrimitive.hpp"
#include    "FEPrimitiveHelper.hpp"
#include    "FEAttributeLibrary.hpp"

namespace   FE
{
    class   FEMeshBuilder
    {
    public:
        /// <summary>
        /// 辅助函数，实现创建mesh
        /// </summary>
        /// <param name="_ctx"></param>
        /// <param name="attrs">属性集</param>
        /// <param name="poss">位置数据，可以为empty</param>
        /// <param name="normals">法线数据，可以为empty</param>
        /// <param name="uvs">uv数据，可以为empty</param>
        /// <param name="indices"></param>
        /// <returns></returns>
        static  Mesh    makeMesh(FEContext& _ctx,const Attrs& inputs,const floats& poss,const floats& normals,const Rgba8s& colors,const floats& uvs,const uints& indices)
        {
            if (poss.empty() && normals.empty() && uvs.empty())
                return  nullptr;
            if (indices.empty())
                return  nullptr;

            MeshPtr mesh        =   new FEMesh(_ctx);
            /// 遍历所有属性
            for (auto& var : inputs)
            {
                Buffer  buffer  =   new FEBuffer(_ctx);
                size_t  stride  =   var.stride();
                /// 获取实际元素格式,确保不为空
                size_t  count   =   (std::max)(poss.size(),normals.size());
                        count   =   (std::max)(poss.size(),count);
                buffer->cInfo()._buffer.resize(stride * uvs.size());
                auto    pData   =   buffer->cInfo()._buffer.data();
                if (var.slot() & (IS_VERTEX_POS))
                {
                    auto    pStart  =   pData;
                    for (auto& var : poss)
                    {
                        memcpy(pStart,&var,sizeof(var));
                        pStart  +=  stride;
                    }
                }
                if (var.slot() & (IS_VERTEX_NOR))
                {
                    auto    pStart  =   pData;
                    for (auto& var :normals)
                    {
                        memcpy(pStart,&var,sizeof(var));
                        pStart  +=  stride;
                    }
                }
                if (var.slot() & (IS_VERTEX_COLOR0))
                {
                    auto    pStart  =   pData;
                    for (auto& var : colors)
                    {
                        memcpy(pStart,&var,sizeof(var));
                        pStart  +=  stride;
                    }
                }
                if (var.slot() & (IS_VERTEX_TEXCOORD0))
                {
                    auto    pStart  =   pData;
                    for (auto& var : uvs)
                    {
                        memcpy(pStart,&var,sizeof(var));
                        pStart  +=  stride;
                    }
                }
            }
            auto    pri =   FEPrimitiveHelper::createIndex(_ctx,indices);
            mesh->setPrimitives({pri});
            return  mesh;
        }
        static  Mesh    makeMeshLine(FEContext& _ctx,const Attrs& inputs,const float3s& poss,const Rgba8s& colors)
        {
            if (poss.empty() && colors.empty())
                return  nullptr;
            MeshPtr mesh        =   new FEMesh(_ctx);
            /// 遍历所有属性
            for (auto& atrr : inputs)
            {
                auto&       subMesh     =   mesh->getOrCreate(atrr);
                
                if (atrr.slot() &(IS_VERTEX_POS))
                {
                    subMesh.setBuffer(poss.data(),poss.size()  * sizeof(float3));
                }
                else if (atrr.slot() & (IS_VERTEX_COLOR0))
                {
                    subMesh.setBuffer(colors.data(),colors.size()* sizeof(Rgba8));
                }
            }
            auto    pri =   new FEDrawArray(_ctx);
            pri->setStart(0);
            pri->setCount((uint)poss.size());
            pri->setPrimitive(PRI_LINES);
            mesh->setPrimitives({pri});
            mesh->updateAabb();
            return  mesh;
        }

        static  Mesh    makeMesh(FEContext& ctx,const float3s& poss,const Rgba8s& colors = {} ,const float3s& normals = {},const float2s& uvs = {})
        {
            if (poss.empty())
                return  nullptr;
            MeshPtr     mesh        =   new FEMesh(ctx);
            {
                auto        attr        =   FEAttribute(IS_VERTEX_POS,     FMT_R32G32B32_FLOAT);
                auto&       subMesh     =   mesh->getOrCreate(attr);
                subMesh.setBuffer(poss.data(),poss.size()  * sizeof(float3));
            }
            if (colors.size() >=  poss.size())
            {
                auto        attr        =   FEAttribute(IS_VERTEX_COLOR0,   FMT_R8G8B8A8_UNORM);
                auto&       subMesh     =   mesh->getOrCreate(attr);
                subMesh.setBuffer(colors.data(),colors.size()  * sizeof(float3));
            }
            if (normals.size() >=  poss.size())
            {
                auto        attr        =   FEAttribute(IS_VERTEX_NOR,      FMT_R32G32B32_FLOAT);
                auto&       subMesh     =   mesh->getOrCreate(attr);
                subMesh.setBuffer(normals.data(),normals.size()  * sizeof(float3));
            }
            if (uvs.size() >=  poss.size())
            {
                auto        attr        =   FEAttribute(IS_VERTEX_TEXCOORD0,FMT_R32G32_FLOAT);
                auto&       subMesh     =   mesh->getOrCreate(attr);
                subMesh.setBuffer(uvs.data(),uvs.size()  * sizeof(float2));
            }
            DrawArray   drawArray   =   new FEDrawArray(ctx);
            drawArray->setPrimitive(PRI_TRIANGLES);
            drawArray->setStart(0);
            drawArray->setCount((uint)poss.size());

            mesh->setPrimitives({drawArray.get()});
            return  mesh;
        }

        static  Mesh    makePointMesh(FEContext& ctx,const float3s& poss,const Rgba8s& colors = {})
        {
            if (poss.empty())
                return  nullptr;
            MeshPtr     mesh        =   new FEMesh(ctx);
            {
                auto        attr        =   FEAttribute(IS_VERTEX_POS,     FMT_R32G32B32_FLOAT);
                auto&       subMesh     =   mesh->getOrCreate(attr);
                subMesh.setBuffer(poss.data(),poss.size()  * sizeof(float3));
            }
            if (colors.size() >=  poss.size())
            {
                auto        attr        =   FEAttribute(IS_VERTEX_COLOR0,   FMT_R8G8B8A8_UNORM);
                auto&       subMesh     =   mesh->getOrCreate(attr);
                subMesh.setBuffer(colors.data(),colors.size()  * sizeof(float3));
            }
            DrawArray   drawArray   =   new FEDrawArray(ctx);
            drawArray->setPrimitive(PRI_POINTS);
            drawArray->setStart(0);
            drawArray->setCount((uint)poss.size());
            mesh->setPrimitives({drawArray.get()});
            return  mesh;
        }
    };
}

