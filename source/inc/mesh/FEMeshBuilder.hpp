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
            /// 遍历属性:slot 定位源通道(作用),format 决定打包字节格式
            for (auto& var : inputs)
            {
                uint8s  packed;
                if (var.slot() & (IS_VERTEX_POS))               packed  =   FEFormatHelper::packFloats(poss,   var.format(),3);
                else if (var.slot() & (IS_VERTEX_NOR))          packed  =   FEFormatHelper::packFloats(normals,var.format(),3);
                else if (var.slot() & (IS_VERTEX_TEXCOORD0))    packed  =   FEFormatHelper::packFloats(uvs,   var.format(),2);
                else if (var.slot() & (IS_VERTEX_COLOR0))       packed  =   FEFormatHelper::packColors(colors,var.format());
                else                                            continue;
                if (packed.empty())     continue;   /// 源数据为空或格式不支持(assert 已触发)
                mesh->getOrCreate(var).setBuffer(std::move(packed));
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
            /// 遍历属性:slot 定位源通道(作用),format 决定打包字节格式
            for (auto& atrr : inputs)
            {
                uint8s  packed;
                if (atrr.slot() & (IS_VERTEX_POS))          packed  =   FEFormatHelper::packFloats(poss,atrr.format());
                else if (atrr.slot() & (IS_VERTEX_COLOR0))  packed  =   FEFormatHelper::packColors(colors,atrr.format());
                else                                        continue;
                if (packed.empty()) continue;   /// 源数据为空或格式不支持(assert 已触发)
                mesh->getOrCreate(atrr).setBuffer(std::move(packed));
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
            MeshPtr     mesh    =   new FEMesh(ctx);
            /// 按 slot+format 挂接属性缓冲,字节数由容器元素类型推导(sizeof 不求值,空容器也安全)
            auto        add     =   [&mesh](FEAttribute attr,const auto& data)
            {
                mesh->getOrCreate(attr).setBuffer(data.data(),data.size()*sizeof(*data.data()));
            };
            add(FEAttribute(IS_VERTEX_POS       ,FMT_R32G32B32_FLOAT)   ,poss);
            if (colors.size()   >=  poss.size())    add(FEAttribute(IS_VERTEX_COLOR0    ,FMT_R8G8B8A8_UNORM)    ,colors);
            if (normals.size()  >=  poss.size())    add(FEAttribute(IS_VERTEX_NOR       ,FMT_R32G32B32_FLOAT)   ,normals);
            if (uvs.size()      >=  poss.size())    add(FEAttribute(IS_VERTEX_TEXCOORD0 ,FMT_R32G32_FLOAT)      ,uvs);
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
                auto    attr        =   FEAttribute(IS_VERTEX_POS,     FMT_R32G32B32_FLOAT);
                auto&   subMesh     =   mesh->getOrCreate(attr);
                subMesh.setBuffer(poss.data(),poss.size()  * sizeof(float3));
            }
            if (colors.size() >=  poss.size())
            {
                auto    attr        =   FEAttribute(IS_VERTEX_COLOR0,   FMT_R8G8B8A8_UNORM);
                auto&   subMesh     =   mesh->getOrCreate(attr);
                subMesh.setBuffer(colors.data(),colors.size()  * sizeof(Rgba8));
            }
            DrawArray   drawArray   =   new FEDrawArray(ctx);
            drawArray->setPrimitive(PRI_POINTS);
            drawArray->setStart(0);
            drawArray->setCount((uint)poss.size());
            mesh->setPrimitives({drawArray.get()});
            return  mesh;
        }
        /// <summary>
        /// 8 字节/点的紧凑点云网格，输入为两个 32 位字:
        ///   word0 = x(int16) | (y(int16) &lt;&lt; 16)
        ///   word1 = z(int16) | (rgb565 &lt;&lt; 16)
        /// 位置按 R16G16B16_SNORM 存放(int16 / 32767 归一化到 [-1,1])，节点缩放
        /// 负责还原为世界单位；颜色用 R16_UINT 保存原始 RGB565(硬件不支持 16 位打包
        /// 格式作为顶点属性,由着色器解包)，工厂按管线声明把两个属性交错成 8 字节顶点
        /// </summary>
        /// <param name="ctx"></param>
        /// <param name="packed">打包后的顶点字</param>
        /// <returns></returns>
        static  Mesh    makePointMesh565(FEContext& ctx,const uints& packed)
        {
            if (packed.size() < 2)
                return  nullptr;
            const   size_t  count       =   packed.size() / 2;
            int16s          positions(size_t(count) * 3);
            uint16s         colors(count);
            for (size_t i = 0; i < count; ++i)
            {
                const   uint32  w0  =   packed[i * 2 + 0];
                const   uint32  w1  =   packed[i * 2 + 1];
                positions[i * 3 + 0]    =   int16(w0 & 0xFFFFu);
                positions[i * 3 + 1]    =   int16(w0 >> 16);
                positions[i * 3 + 2]    =   int16(w1 & 0xFFFFu);
                colors[i]               =   uint16(w1 >> 16);
            }
            MeshPtr     mesh        =   new FEMesh(ctx);
            {
                auto        attr        =   FEAttribute(IS_VERTEX_POS,     FMT_R16G16B16_SNORM);
                auto&       subMesh     =   mesh->getOrCreate(attr);
                subMesh.setBuffer(positions.data(),positions.size()  * sizeof(int16));
            }
            {
                auto        attr        =   FEAttribute(IS_VERTEX_COLOR0,  FMT_R16_UINT);
                auto&       subMesh     =   mesh->getOrCreate(attr);
                subMesh.setBuffer(colors.data(),colors.size()  * sizeof(uint16));
            }
            DrawArray   drawArray   =   new FEDrawArray(ctx);
            drawArray->setPrimitive(PRI_POINTS);
            drawArray->setStart(0);
            drawArray->setCount((uint)count);
            mesh->setPrimitives({drawArray.get()});
            mesh->updateAabb();
            return  mesh;
        }
    };
}

