#pragma     once
#include    <map>
#include    <sstream>
#include    <utility>
#include    <algorithm>
#include    <cctype>
#include    <cstring>
#include    "../../FEStreamWriter.hpp"
#include    "../../FEWriterHelper.hpp"
#include    "../../FENodeHelper.hpp"
#include    "../../FEDateTime.hpp"
#include    "../../mesh/FEMesh.hpp"
#include    "../../material/FEMaterialPBR.hpp"
#include    "FEFormatGLTF.hpp"
/// tiny_gltf.h 的实现段(TINYGLTF_IMPLEMENTATION)位于其头文件保护之外,
/// 同一翻译单元内重复包含会重定义,因此统一由 Reader 头文件引入
#include    "FEFormatGLTFReader.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEFormatGLTFWriter,"{7C1A9E24-3B6D-4F58-8C2E-1D9A5B7E3F60}");

    /// <summary>
    /// glTF 2.0 保存器:把引擎节点树(FENode/FEMesh/FEPrimitive/FEMaterialPBR)
    /// 输出为 .gltf(内嵌 base64 二进制)或 .glb(二进制容器)
    /// 支持的 glTF 特性:
    ///   1. 节点层级 + TRS(默认值不写入)
    ///   2. 顶点属性 POSITION/NORMAL/TANGENT/TEXCOORD_n/COLOR_n/JOINTS_0/WEIGHTS_0
    ///   3. 索引图元(8/16/32 位)与 6 种拓扑模式
    ///   4. PBR 金属度-粗糙度材质 + KHR transmission/clearcoat/volume 扩展
    /// </summary>
    class   FEFormatGLTFWriter :public FEStreamWriter
    {
    public:
        /// <summary>
        /// 输出上下文:缓存已输出的二进制数据与材质/网格,避免重复写入
        /// </summary>
        struct  OutputCtx
        {
            /// <summary>
            /// FEBuffer → 在 glTF buffer 中的字节偏移
            /// </summary>
            std::map<const FEBuffer*,size_t>                        _bufferOffsets;
            /// <summary>
            /// 材质 → glTF 材质索引
            /// </summary>
            std::map<const FEMaterial*,int>                         _materials;
            /// <summary>
            /// (网格,材质) → glTF 网格索引,同一网格搭配同一材质只输出一次
            /// </summary>
            std::map<std::pair<const FEMesh*,const FEMaterial*>,int> _meshs;
            /// <summary>
            /// 实际写入的 KHR 材质扩展(用于 model.extensionsUsed)
            /// </summary>
            std::vector<std::string>                                _extensions;
        };
        /// <summary>
        /// 引擎顶点格式 → glTF accessor 分量信息
        /// </summary>
        struct  FormatMapping
        {
            int     _componentType  =   TINYGLTF_COMPONENT_TYPE_FLOAT;
            int     _type           =   TINYGLTF_TYPE_SCALAR;
            bool    _normalized     =   false;
        };
    public:
        IMPLEMENT_CLASS_REFLECT(FEFormatGLTFWriter)
    public:
        FEFormatGLTFWriter(FEContext& ctx)
            :FEStreamWriter(ctx)
        {}
        FEFormatGLTFWriter(const FEFormatGLTFWriter& other)
            :FEStreamWriter(other)
        {}
        /// <summary>
        /// 组件支持的格式数组
        /// </summary>
        /// <returns></returns>
        virtual Formats formats() const override
        {
            return  formatList();
        }
        /// <summary>
        /// 写文件,扩展名 .glb 时输出二进制容器,否则输出内嵌二进制的文本 glTF
        /// </summary>
        /// <param name="objects"></param>
        /// <param name="file"></param>
        /// <returns></returns>
        virtual bool    writeFile(const Objects& objects,const String& file) override
        {
            tinygltf::Model     model;
            OutputCtx           ctx;
            build(model,ctx,objects);

            tinygltf::TinyGLTF  gltf;
            bool                binary  =   isBinary(file);
            /// 图片/缓冲区全部内嵌,生成单文件 glTF
            return  gltf.WriteGltfSceneToFile(&model,file,true,true,true,binary);
        }
        /// <summary>
        /// 写入内存(文本 glTF,二进制数据以 base64 data URI 内嵌)
        /// </summary>
        /// <param name="objects"></param>
        /// <param name="writer"></param>
        /// <returns></returns>
        virtual bool    writeBuffer(const Objects& objects,FEWriter& writer) override
        {
            tinygltf::Model     model;
            OutputCtx           ctx;
            build(model,ctx,objects);

            std::ostringstream  stream;
            tinygltf::TinyGLTF  gltf;
            if (!gltf.WriteGltfSceneToStream(&model,stream,true,false))
                return  false;
            const   String  json    =   stream.str();
            return  writer.writeBuffer(json.data(),json.size()) == json.size();
        }
    public:
        static  Formats formatList()
        {
            return  FEGLTFFormat::formats(UUIDOF(FEFormatGLTFWriter));
        }
    protected:
        /// <summary>
        /// 是否输出二进制 .glb
        /// </summary>
        /// <param name="file"></param>
        /// <returns></returns>
        static  bool    isBinary(const String& file)
        {
            const   size_t  pos =   file.rfind('.');
            if (pos == String::npos)
                return  false;
            String  ext =   file.substr(pos);
            for (auto& ch : ext)
                ch  =   char(tolower(static_cast<unsigned char>(ch)));
            return  ext == ".glb";
        }
        /// <summary>
        /// 构建 glTF 模型
        /// </summary>
        /// <param name="model"></param>
        /// <param name="ctx"></param>
        /// <param name="objects"></param>
        void    build(tinygltf::Model& model,OutputCtx& ctx,const Objects& objects)
        {
            model.asset.version     =   "2.0";
            model.asset.generator   =   "FE Engine glTF Writer";

            tinygltf::Scene scene;
            scene.name  =   "Scene";
            for (auto& var : objects)
            {
                auto    node    =   var->cast<FENode>();
                if (node == nullptr)
                    continue;
                int     index   =   addNode(model,ctx,node);
                if (index >= 0)
                    scene.nodes.push_back(index);
            }
            model.scenes.push_back(std::move(scene));
            model.defaultScene  =   0;
            model.extensionsUsed    =   ctx._extensions;

            /// 至少一个 buffer,写出时 tinygltf 会把数据放进 buffer[0]
            if (model.buffers.empty())
                model.buffers.emplace_back();
        }
        /// <summary>
        /// 追加节点(含子节点),返回 glTF 节点索引
        /// </summary>
        /// <param name="model"></param>
        /// <param name="ctx"></param>
        /// <param name="node"></param>
        /// <returns></returns>
        int     addNode(tinygltf::Model& model,OutputCtx& ctx,const FENode* node)
        {
            if (node == nullptr)
                return  -1;

            tinygltf::Node  gltfNode;
            if (node->name() != nullptr)
                gltfNode.name  =   node->name();

            /// glTF 默认值:平移(0,0,0)/旋转(0,0,0,1)/缩放(1,1,1),默认时不写入
            const   real3   pos =   node->localTranslation();
            if (pos != real3(0,0,0))
                gltfNode.translation   =   {double(pos.x),double(pos.y),double(pos.z)};

            const   quatr   rot =   node->localRotation();
            if (rot != quatr(1,0,0,0))
                gltfNode.rotation      =   {double(rot.x),double(rot.y),double(rot.z),double(rot.w)};

            const   float3  scale   =   node->localScaling();
            if (scale != float3(1,1,1))
                gltfNode.scale         =   {double(scale.x),double(scale.y),double(scale.z)};

            if (node->mesh() != nullptr)
                gltfNode.mesh   =   addMesh(model,ctx,node->mesh().get(),node->material().get());

            /// 先递归子节点再入表:入表会使 model.nodes 扩容,不能持有其引用
            std::vector<int>    childs;
            for (auto& var : node->children())
            {
                int     child   =   addNode(model,ctx,var.get());
                if (child >= 0)
                    childs.push_back(child);
            }
            gltfNode.children   =   std::move(childs);

            int     index   =   int(model.nodes.size());
            model.nodes.push_back(std::move(gltfNode));
            return  index;
        }
        /// <summary>
        /// 追加网格(同一网格+同一材质只输出一次)
        /// </summary>
        /// <param name="model"></param>
        /// <param name="ctx"></param>
        /// <param name="mesh"></param>
        /// <param name="material"></param>
        /// <returns></returns>
        int     addMesh(tinygltf::Model& model,OutputCtx& ctx,const FEMesh* mesh,const FEMaterial* material)
        {
            if (mesh == nullptr)
                return  -1;

            const   auto    key =   std::make_pair(mesh,material);
            auto            itr =   ctx._meshs.find(key);
            if (itr != ctx._meshs.end())
                return  itr->second;

            const   int     meshIndex   =   int(model.meshes.size());
            const   int     matIndex    =   addMaterial(model,ctx,material);
            ctx._meshs[key] =   meshIndex;

            tinygltf::Mesh  gltfMesh;
            gltfMesh.name   =   "mesh_" + std::to_string(meshIndex);

            const   auto&   primitives  =   mesh->primitives();
            if (primitives.empty())
            {
                /// 无图元(DRAW_ARRAY)时按三角形输出顶点属性
                tinygltf::Primitive pri;
                pri.mode        =   TINYGLTF_MODE_TRIANGLES;
                pri.material    =   matIndex;
                addAttributes(model,ctx,mesh->buffers(),pri);
                gltfMesh.primitives.push_back(std::move(pri));
            }
            else
            {
                for (auto& var : primitives)
                {
                    tinygltf::Primitive pri;
                    pri.mode        =   modeOf(var->primitive());
                    pri.material    =   matIndex;
                    addAttributes(model,ctx,mesh->buffers(),pri);
                    pri.indices     =   addIndices(model,ctx,var.get());
                    gltfMesh.primitives.push_back(std::move(pri));
                }
            }
            model.meshes.push_back(std::move(gltfMesh));
            return  meshIndex;
        }
        /// <summary>
        /// 输出顶点属性(一个属性一个 accessor)
        /// </summary>
        /// <param name="model"></param>
        /// <param name="ctx"></param>
        /// <param name="buffers"></param>
        /// <param name="pri"></param>
        void    addAttributes(tinygltf::Model& model,OutputCtx& ctx,const FEMesh::AttrBuffers& buffers,tinygltf::Primitive& pri)
        {
            for (auto& var : buffers)
            {
                const   char*   semantic    =   semanticOf(var.attr().slot());
                if (semantic == nullptr)
                    continue;
                const   bool    isPosition  =   strcmp(semantic,"POSITION") == 0;
                int             accessor    =   addAttributeAccessor(model,ctx,var,isPosition);
                if (accessor >= 0)
                    pri.attributes[semantic]    =   accessor;
            }
        }
        /// <summary>
        /// 输出一个顶点属性 accessor
        /// </summary>
        /// <param name="model"></param>
        /// <param name="ctx"></param>
        /// <param name="attr"></param>
        /// <param name="isPosition"></param>
        /// <returns></returns>
        int     addAttributeAccessor(tinygltf::Model& model,OutputCtx& ctx,const FEAttribyteBuffer& attr,bool isPosition)
        {
            const   Buffer  buffer  =   attr.buffer();
            if (buffer == nullptr || buffer->data() == nullptr)
                return  -1;

            FormatMapping   map;
            if (!mapFormat(attr.attr().format(),map))
                return  -1;

            const   int     count   =   int(attr.count());
            if (count <= 0)
                return  -1;

            const   size_t  offset  =   bufferOffset(model,ctx,buffer.get());
            const   size_t  length  =   size_t(buffer->length());
            const   int     view    =   addBufferView(model,offset,length,TINYGLTF_TARGET_ARRAY_BUFFER);

            tinygltf::Accessor accessor;
            accessor.bufferView     =   view;
            accessor.byteOffset     =   0;
            accessor.componentType  =   map._componentType;
            accessor.type           =   map._type;
            accessor.normalized     =   map._normalized;
            accessor.count          =   size_t(count);
            /// POSITION 必须带 min/max
            if (isPosition && attr.attr().format() == FMT_R32G32B32_FLOAT)
                computeMinMax(accessor,(const float3*)attr.data(),count);

            model.accessors.push_back(accessor);
            return  int(model.accessors.size()) - 1;
        }
        /// <summary>
        /// 输出索引 accessor(8/16/32 位)
        /// </summary>
        /// <param name="model"></param>
        /// <param name="ctx"></param>
        /// <param name="pri"></param>
        /// <returns></returns>
        int     addIndices(tinygltf::Model& model,OutputCtx& ctx,const FEPrimitive* pri)
        {
            UNUSED(ctx);

            const   void*   data        =   nullptr;
            size_t          length      =   0;
            size_t          count       =   0;
            int             component   =   TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;

            if (auto p = dynamic_cast<const FEDrawElementUint8*>(pri))
            {
                data        =   p->index().data();
                count       =   p->index().size();
                length      =   count * sizeof(uint8);
                component   =   TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
            }
            else if (auto p16 = dynamic_cast<const FEDrawElementUint16*>(pri))
            {
                data        =   p16->index().data();
                count       =   p16->index().size();
                length      =   count * sizeof(uint16);
                component   =   TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
            }
            else if (auto p32 = dynamic_cast<const FEDrawElementUint32*>(pri))
            {
                data        =   p32->index().data();
                count       =   p32->index().size();
                length      =   count * sizeof(uint32);
                component   =   TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
            }
            else
            {
                /// DRAW_ARRAY 没有索引
                return  -1;
            }
            if (data == nullptr || count == 0)
                return  -1;

            const   size_t  offset  =   appendBytes(model,(const uint8*)data,length);
            const   int     view    =   addBufferView(model,offset,length,TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER);

            tinygltf::Accessor accessor;
            accessor.bufferView     =   view;
            accessor.byteOffset     =   0;
            accessor.componentType  =   component;
            accessor.type           =   TINYGLTF_TYPE_SCALAR;
            accessor.count          =   count;
            model.accessors.push_back(accessor);
            return  int(model.accessors.size()) - 1;
        }
        /// <summary>
        /// 输出材质(PBR 因子 + KHR 材质扩展)
        /// </summary>
        /// <param name="model"></param>
        /// <param name="ctx"></param>
        /// <param name="material"></param>
        /// <returns></returns>
        int     addMaterial(tinygltf::Model& model,OutputCtx& ctx,const FEMaterial* material)
        {
            if (material == nullptr)
                return  -1;

            auto    itr =   ctx._materials.find(material);
            if (itr != ctx._materials.end())
                return  itr->second;

            const   int     index   =   int(model.materials.size());
            ctx._materials[material]    =   index;

            tinygltf::Material  gltfMat;
            gltfMat.name    =   "material_" + std::to_string(index);
            gltfMat.alphaMode   =   material->isTransparent() ? "BLEND" : "OPAQUE";

            auto    pbr =   dynamic_cast<const FEMaterialPBR*>(material);
            if (pbr != nullptr)
            {
                const   auto&   value   =   pbr->data()._value;
                const   auto&   ext     =   pbr->texParams()._value;
                gltfMat.pbrMetallicRoughness.baseColorFactor    =   {double(value._diffuse.x),double(value._diffuse.y),double(value._diffuse.z),double(value._diffuse.w)};
                gltfMat.pbrMetallicRoughness.metallicFactor     =   double(value._metallic);
                gltfMat.pbrMetallicRoughness.roughnessFactor    =   double(value._roughness);
                gltfMat.emissiveFactor                          =   {double(value._emissive.x),double(value._emissive.y),double(value._emissive.z)};
                /// transmission>0 的材质按 BLEND 输出(与读取端约定一致)
                if (ext._transmissionFactor > 0.0f)
                    gltfMat.alphaMode   =   "BLEND";

                /// KHR_materials_transmission
                if (ext._transmissionFactor > 0.0f)
                {
                    tinygltf::Value::Object obj;
                    obj["transmissionFactor"]   =   tinygltf::Value(double(ext._transmissionFactor));
                    gltfMat.extensions["KHR_materials_transmission"]    =   tinygltf::Value(std::move(obj));
                    pushExtension(ctx,"KHR_materials_transmission");
                }
                /// KHR_materials_clearcoat
                if (ext._clearcoatStrength > 0.0f)
                {
                    tinygltf::Value::Object obj;
                    obj["clearcoatFactor"]           =   tinygltf::Value(double(ext._clearcoatStrength));
                    obj["clearcoatRoughnessFactor"]  =   tinygltf::Value(double(ext._clearcoatRoughness));
                    gltfMat.extensions["KHR_materials_clearcoat"]    =   tinygltf::Value(std::move(obj));
                    pushExtension(ctx,"KHR_materials_clearcoat");
                }
                /// KHR_materials_volume(厚度 > 0 才输出;attenuationDistance<=0 视为无穷大)
                if (ext._volume[0] > 0.0f)
                {
                    tinygltf::Value::Object obj;
                    obj["thicknessFactor"]  =   tinygltf::Value(double(ext._volume[0]));
                    obj["attenuationColor"] =   tinygltf::Value(tinygltf::Value::Array
                    {
                        tinygltf::Value(double(ext._volume[1])),
                        tinygltf::Value(double(ext._volume[2])),
                        tinygltf::Value(double(ext._volume[3]))
                    });
                    if (ext._volume[4] > 0.0f)
                        obj["attenuationDistance"]  =   tinygltf::Value(double(ext._volume[4]));
                    gltfMat.extensions["KHR_materials_volume"]   =   tinygltf::Value(std::move(obj));
                    pushExtension(ctx,"KHR_materials_volume");
                }
                /// KHR_materials_iridescence(强度 > 0 才输出)
                if (ext._iriFactor > 0.0f)
                {
                    tinygltf::Value::Object obj;
                    obj["iridescenceFactor"]            =   tinygltf::Value(double(ext._iriFactor));
                    obj["iridescenceIor"]               =   tinygltf::Value(double(ext._iriIor));
                    obj["iridescenceThicknessMinimum"]  =   tinygltf::Value(double(ext._iriTmin));
                    obj["iridescenceThicknessMaximum"]  =   tinygltf::Value(double(ext._iriTmax));
                    gltfMat.extensions["KHR_materials_iridescence"]  =   tinygltf::Value(std::move(obj));
                    pushExtension(ctx,"KHR_materials_iridescence");
                }
            }
            model.materials.push_back(std::move(gltfMat));
            return  index;
        }
        /// <summary>
        /// 记录一次扩展使用(去重)
        /// </summary>
        /// <param name="ctx"></param>
        /// <param name="name"></param>
        static  void    pushExtension(OutputCtx& ctx,const char* name)
        {
            for (auto& var : ctx._extensions)
            {
                if (var == name)
                    return;
            }
            ctx._extensions.emplace_back(name);
        }
        /// <summary>
        /// 把 FEBuffer 数据写入 glTF buffer[0],返回字节偏移(同一 buffer 只写一次)
        /// </summary>
        /// <param name="model"></param>
        /// <param name="ctx"></param>
        /// <param name="buffer"></param>
        /// <returns></returns>
        size_t  bufferOffset(tinygltf::Model& model,OutputCtx& ctx,const FEBuffer* buffer)
        {
            auto    itr =   ctx._bufferOffsets.find(buffer);
            if (itr != ctx._bufferOffsets.end())
                return  itr->second;

            const   size_t  offset  =   appendBytes(model,buffer->data(),size_t(buffer->length()));
            ctx._bufferOffsets[buffer]  =   offset;
            return  offset;
        }
        /// <summary>
        /// 追加原始字节到 glTF buffer[0],按 4 字节对齐,返回起始偏移
        /// </summary>
        /// <param name="model"></param>
        /// <returns></returns>
        static  size_t  appendBytes(tinygltf::Model& model,const uint8* data,size_t length)
        {
            if (model.buffers.empty())
                model.buffers.emplace_back();
            auto&   buffer  =   model.buffers[0];
            while (buffer.data.size() % 4 != 0)
                buffer.data.push_back(0);
            const   size_t  offset  =   buffer.data.size();
            buffer.data.insert(buffer.data.end(),data,data + length);
            return  offset;
        }
        /// <summary>
        /// 追加 bufferView
        /// </summary>
        /// <param name="model"></param>
        /// <param name="offset"></param>
        /// <param name="length"></param>
        /// <param name="target"></param>
        /// <returns></returns>
        static  int     addBufferView(tinygltf::Model& model,size_t offset,size_t length,int target)
        {
            tinygltf::BufferView view;
            view.buffer     =   0;
            view.byteOffset =   offset;
            view.byteLength =   length;
            view.target     =   target;
            model.bufferViews.push_back(view);
            return  int(model.bufferViews.size()) - 1;
        }
        /// <summary>
        /// 计算 POSITION 的 min/max
        /// </summary>
        /// <param name="accessor"></param>
        /// <param name="data"></param>
        /// <param name="count"></param>
        static  void    computeMinMax(tinygltf::Accessor& accessor,const float3* data,int count)
        {
            if (data == nullptr || count <= 0)
                return;
            double  minX =   double(data[0].x), minY = double(data[0].y), minZ = double(data[0].z);
            double  maxX =   minX, maxY = minY, maxZ = minZ;
            for (int i = 1; i < count; ++ i)
            {
                minX    =   (std::min)(minX,double(data[i].x));
                minY    =   (std::min)(minY,double(data[i].y));
                minZ    =   (std::min)(minZ,double(data[i].z));
                maxX    =   (std::max)(maxX,double(data[i].x));
                maxY    =   (std::max)(maxY,double(data[i].y));
                maxZ    =   (std::max)(maxZ,double(data[i].z));
            }
            accessor.minValues  =   {minX,minY,minZ};
            accessor.maxValues  =   {maxX,maxY,maxZ};
        }
        /// <summary>
        /// 引擎图元拓扑 → glTF mode
        /// </summary>
        /// <param name="pri"></param>
        /// <returns></returns>
        static  int     modeOf(EPrimitive pri)
        {
            switch (pri)
            {
            case PRI_POINTS         :   return  TINYGLTF_MODE_POINTS;
            case PRI_LINES          :   return  TINYGLTF_MODE_LINE;
            case PRI_LINE_STRIP     :   return  TINYGLTF_MODE_LINE_STRIP;
            case PRI_TRIANGLES      :   return  TINYGLTF_MODE_TRIANGLES;
            case PRI_TRIANGLE_STRIP :   return  TINYGLTF_MODE_TRIANGLE_STRIP;
            case PRI_TRIANGLE_FAN   :   return  TINYGLTF_MODE_TRIANGLE_FAN;
            default                 :   return  TINYGLTF_MODE_TRIANGLES;
            }
        }
        /// <summary>
        /// 引擎属性槽 → glTF 顶点语义,返回 nullptr 表示不支持
        /// </summary>
        /// <param name="slot"></param>
        /// <returns></returns>
        static  const   char*   semanticOf(FEInputSlot slot)
        {
            switch (slot)
            {
            case IS_VERTEX_POS          :   return  "POSITION";
            case IS_VERTEX_NOR          :   return  "NORMAL";
            case IS_VERTEX_TANGENT      :   return  "TANGENT";
            case IS_VERTEX_COLOR0       :   return  "COLOR_0";
            case IS_VERTEX_COLOR1       :   return  "COLOR_1";
            case IS_VERTEX_TEXCOORD0    :   return  "TEXCOORD_0";
            case IS_VERTEX_TEXCOORD1    :   return  "TEXCOORD_1";
            case IS_VERTEX_TEXCOORD2    :   return  "TEXCOORD_2";
            case IS_VERTEX_TEXCOORD3    :   return  "TEXCOORD_3";
            case IS_VERTEX_BONE_INDEX   :   return  "JOINTS_0";
            case IS_VERTEX_BONE_WEIGHT  :   return  "WEIGHTS_0";
            default                     :   return  nullptr;
            }
        }
        /// <summary>
        /// 引擎顶点格式 → glTF 分量类型
        /// </summary>
        /// <param name="fmt"></param>
        /// <param name="map"></param>
        /// <returns></returns>
        static  bool    mapFormat(FEFormat fmt,FormatMapping& map)
        {
            switch (fmt)
            {
            /// 32 位浮点族
            case FMT_R32_FLOAT          :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_FLOAT,1,false);
            case FMT_R32G32_FLOAT       :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_FLOAT,2,false);
            case FMT_R32G32B32_FLOAT    :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_FLOAT,3,false);
            case FMT_R32G32B32A32_FLOAT :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_FLOAT,4,false);
            /// 8 位无符号归一化族
            case FMT_R8_UNORM           :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,1,true);
            case FMT_R8G8_UNORM         :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,2,true);
            case FMT_R8G8B8A8_UNORM     :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,4,true);
            /// 8 位无符号整数族
            case FMT_R8_UINT            :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,1,false);
            case FMT_R8G8_UINT          :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,2,false);
            case FMT_R8G8B8A8_UINT      :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,4,false);
            /// 16 位无符号归一化族
            case FMT_R16_UNORM          :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,1,true);
            case FMT_R16G16_UNORM       :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,2,true);
            case FMT_R16G16B16_UNORM    :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,3,true);
            case FMT_R16G16B16A16_UNORM :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,4,true);
            /// 16 位无符号整数族(骨骼索引/权重)
            case FMT_R16_UINT           :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,1,false);
            case FMT_R16G16_UINT        :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,2,false);
            case FMT_R16G16B16A16_UINT  :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,4,false);
            /// 32 位无符号整数族
            case FMT_R32_UINT           :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,1,false);
            case FMT_R32G32_UINT        :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,2,false);
            case FMT_R32G32B32_UINT     :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,3,false);
            case FMT_R32G32B32A32_UINT  :   return  setFormat(map,TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,4,false);
            default                     :   return  false;
            }
        }
        /// <summary>
        /// 填写 FormatMapping
        /// </summary>
        /// <param name="map"></param>
        /// <param name="componentType"></param>
        /// <param name="components"></param>
        /// <param name="normalized"></param>
        /// <returns></returns>
        static  bool    setFormat(FormatMapping& map,int componentType,uint components,bool normalized)
        {
            map._componentType  =   componentType;
            map._normalized     =   normalized;
            switch (components)
            {
            case 1  :   map._type   =   TINYGLTF_TYPE_SCALAR;   return  true;
            case 2  :   map._type   =   TINYGLTF_TYPE_VEC2;     return  true;
            case 3  :   map._type   =   TINYGLTF_TYPE_VEC3;     return  true;
            case 4  :   map._type   =   TINYGLTF_TYPE_VEC4;     return  true;
            default :   return  false;
            }
        }
    };
}
