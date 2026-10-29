#pragma     once
#include    "../../FEStreamReader.hpp"
#include    "../../FEDateTime.hpp"
#include    "../../FEReaderHelper.hpp"
#include    "../../FEConst.h"
#include    "../../FEEntryList.hpp"
#include    "../../FEFileInfor.hpp"
#include    "../../FEFileSystem.hpp"
#include    "../../material/FEMaterialLibrary.hpp"
#include    "../../animation/FEAnimation.hpp"
#include    "../../graphic/FETexture.h"
#include    "../../graphic/FEScene.h"
#include    "../../FEFileBuffer.hpp"
#include    "FEFormatGLTF.hpp"
/// 纹理来源对象 → GPU 纹理(解码 + mip 链)的公共实现,与工程反序列化共用
#include    "../../../core/graphic/FETextureHelper.hpp"


#define     TINYGLTF_IMPLEMENTATION
#define     STB_IMAGE_IMPLEMENTATION
#define     TINYGLTF_NO_STB_IMAGE_WRITE
#ifdef      VK_USE_PLATFORM_ANDROID_KHR
    #define TINYGLTF_ANDROID_LOAD_FROM_ASSETS
#endif

#include    "tiny_gltf.h"
namespace   FE
{
    DEFINE_CLASS_UUID(FEFormatGLTFReader,"{43E4FF48-6EC0-4BAA-97AE-6168B5FF1752}");

    class   FEFormatGLTFReader :public FEStreamReader
    {
    public:
        using   MeshsIndex          =   std::map<int,Meshs>;
        using   MaterialIndex       =   std::map<int,Material>;
        using   NodeIndex           =   std::map<int,Node>;
        using   AnimatonIndex       =   std::map<int,Animation>;
        using   TextureIndex        =   std::map<int,TexPtr>;
        /// <summary>glTF 材质索引 → (变体索引,目标材质索引) 列表</summary>
        using   VariantMappingIndex =   std::map<int,std::vector<std::pair<int,int>>>;
        /// <summary>引用材质的节点及其 glTF 材质索引</summary>
        using   VariantNodeList     =   std::vector<std::pair<Node,int>>;
    public:
        IMPLEMENT_CLASS_REFLECT(FEFormatGLTFReader)
    protected:
        /// <summary>
        /// glTF texture 索引 → 引擎纹理缓存(每次 readBuffers 清空)
        /// </summary>
        TextureIndex            _texCache;
        /// <summary>
        /// 当前 glTF 文件所在目录(解析图片 uri 相对路径用)
        /// </summary>
        String                  _currentDir;
        /// <summary>
        /// ===== KHR_materials_variants =====
        /// 变体名表(顺序即 glTF 扩展中 variants 数组顺序)
        /// </summary>
        Strings                 _variantNames;
        /// <summary>
        /// glTF 材质索引 → (变体索引,目标材质索引) 列表
        /// </summary>
        VariantMappingIndex     _variantMappings;
        /// <summary>
        /// 本次加载的全部材质(gltf 索引 → 引擎材质),变体切换时查询目标材质
        /// </summary>
        MaterialIndex           _variantMaterials;
        /// <summary>
        /// 引用了材质的节点及对应 glTF 材质索引(变体切换时更新)
        /// </summary>
        VariantNodeList         _variantNodes;
    public:
        FEFormatGLTFReader(FEContext& ctx)
            :FEStreamReader(ctx)
        {}
        FEFormatGLTFReader(const FEFormatGLTFReader& other)
            :FEStreamReader(other)
        {}
        /// <summary>
        /// 构造对象查询/缓存回调(readFiles 与 readBuffers 共用):
        /// objectMap 提供 id→对象缓存;ctxRef 指向当前序列化上下文指针(可在回调外被替换);
        /// pReader/offset 用于按当前读取器位置累计进度
        /// </summary>
        static  FESerializeCtx::QueryObject    makeQueryObject(FEFormatGLTFReader* self,
                                                              FESerializeCtx*& ctxRef,
                                                              ObjectUMap& objectMap,
                                                              FEReader*& pReader,
                                                              uint64& offset)
        {
            return [self,&ctxRef,&objectMap,&pReader,&offset](const FEUuid& id,FEObject* object,FESerializeCtx::Option option)->FESerializeCtx::QResult
            {
                switch(option)
                {
                case FESerializeCtx::O_Query:
                    {
                        auto    itr =   objectMap.find(id);
                        if (itr != objectMap.end())
                            return  {itr->second.get(),0};
                        return  {};
                    }
                case FESerializeCtx::O_Cache:
                    objectMap[id]   =   object;
                    return  {object,0};
                case FESerializeCtx::O_Progress:
                    return  {(FEProgress*)self,0};
                case FESerializeCtx::O_UpdateProgress:
                    {
                        FEReader*   reader  =   object ? object->as<FEReader>() : nullptr;
                        if (reader != nullptr && ctxRef->change)
                        {
                            offset          +=  pReader ? pReader->length() : 0;
                            pReader         =   reader;
                            ctxRef->change  =   false;
                        }
                        if (reader)
                            ctxRef->doByte  =   offset + reader->tell();
                        const   auto    per =   double(ctxRef->doByte)/(std::max)(1.0,double(ctxRef->allByte));
                        self->setProgress(per);
                    }
                    return  {(FEProgress*)self,0};
                default:
                    return  {};
                }
            };
        }
        /// <summary>
        /// 组件支持的格式数组
        /// </summary>
        /// <returns></returns>
        virtual Formats formats() const override
        {
            return  formatList();
        }
        /// <summary>
        /// 接口调用加载文件
        /// </summary>
        /// <param name="file"></param>
        /// <returns></returns>
        virtual Objects readFiles(const Strings& files) override
        {
            FESerializeCtx  xsCtx;
            ObjectUMap      objectMap;
            Objects         result;
            FEReader*       pReader =   nullptr;
            uint64          offset  =   0;
            /// 查询函数(与 readBuffers 共用)
            FESerializeCtx* xsCtxPtr = &xsCtx;
            xsCtx.query     =   makeQueryObject(this,xsCtxPtr,objectMap,pReader,offset);
            /// 计算所有文件大小
            for (auto& var : files)
            {
                xsCtx.allByte +=  FEFileInfor::fileSize(var);
            }
            setParam(TBCXSerialize,&xsCtx);

           
            for (auto& file : files)
            {
                FEReaderFile    reader(_ctx,file.c_str());
                if (!reader.isValid())
                    continue;
                FEReaderHelper  helper(reader);
                Buffer  buffer  =   helper.read(_ctx);
                reader.close();
                setParam("fileName",    file);
                auto    temp  =   readBuffers({buffer});
                result.insert(result.end(),temp.begin(),temp.end());
            }
            return  result;
        }
        /// <summary>
        /// 输入多个buffer,从多个buffer中读取对象数据
        /// </summary>
        /// <param name="buffers"></param>
        /// <param name="params"></param>
        /// <returns></returns>
        virtual Objects readBuffers(const Buffers& buffers) override
        {
            FESerializeCtx  newCtx;
            FESerializeCtx* pCtx    =   &newCtx;
            ObjectUMap      objectMap;
            FEReader*       pReader =   nullptr;
            uint64          offset  =   0;
            for (auto var : buffers)
            {
                newCtx.allByte   +=  var->length();
            }
            /// 查询函数(与 readFiles 共用)
            newCtx.query        =   makeQueryObject(this,pCtx,objectMap,pReader,offset);
            /// 先查找是否已经设置了
            auto        parCtx  =   param(TBCXSerialize);
            /// 如果已经设置，则使用设置的
            if (!std::holds_alternative<std::monostate>(parCtx))
            {
                pCtx    =   std::get<FESerializeCtx*>(parCtx);
            }

            auto    fileNameParam   =   param("fileName");
            String  fileName;
            if (!std::holds_alternative<std::monostate>(fileNameParam))
            {
                fileName    =   std::get<std::string>(fileNameParam);
            }
            /// 存储入口对象列表
            Objects         entryList;
            Objects         results;
            for (auto var : buffers)
            {
                tinygltf::Model     gltfModel;
                tinygltf::TinyGLTF  gltfContext;
                /// 保留未知扩展的原始 JSON(KHR_materials_variants 等扩展解析依赖)
                gltfContext.SetStoreOriginalJSONForExtrasAndExtensions(true);
                String              error;
                String              warning;
                String              dir =   FEFileSystem::filePath(fileName.c_str());
                _currentDir         =   dir;
                if(!gltfContext.LoadASCIIFromString(&gltfModel,&error,&warning,var->dataAs<char>(),(uint)var->length(),dir))
                    continue;
                MeshsIndex          meshMap;
                MaterialIndex       matMap;
                AnimatonIndex       animations;
                NodeIndex           nodeMap;
                _texCache.clear();
                _variantNodes.clear();
                gltfMesh2Mesh(gltfModel,meshMap);
                gltfMat2Material(gltfModel,matMap);
                parseVariants(gltfModel);
                _variantMaterials   =   matMap;
                auto    nodes   =   parseNodes(gltfModel,meshMap,matMap,nodeMap);
                /// animation;
                results.reserve(results.size() + nodes.size());
                createNodeAnimation(gltfModel,animations,nodeMap);

                for (auto& node : nodes)
                {
                    node->flags().addFlag(FLAG_UPDATE);
                    node->update();
                    results.push_back(node.get());
                }
            }
            return  results;
        }

        void        gltfMesh2Mesh(tinygltf::Model& model,MeshsIndex& meshMap)
        {
            for (size_t i = 0; i < model.meshes.size(); ++i)
            {
                Meshs&  meshs   =   meshMap[int(i)];
                meshs.reserve(model.meshes[i].primitives.size());
                for (size_t j = 0; j < model.meshes[i].primitives.size(); ++j)
                {
                   auto&    primitive   =   model.meshes[i].primitives[j];

                    if (primitive.indices < 0)
                        continue;
                    /// 解析顶点 法线 UV和颜色数据
                    size_t      vertexCount = 0;
                    floats      positionData;
                    floats      normalData  ;
                    floats      uvData      ;
                    floats      uvData1     ;
                    floats      tangentData ;
                    uint8x4s    colorData   ;
                    Mesh    mesh    =   new FEMesh(_ctx);
                    auto    itr     =   primitive.attributes.find("POSITION");
                    assert(itr!= primitive.attributes.end());
                    if (itr ==  primitive.attributes.end())
                        continue;
              
                    const tinygltf::Accessor&   posAccessor     =   model.accessors[itr->second];
                    const tinygltf::BufferView& posView         =   model.bufferViews[posAccessor.bufferView];
                    const tinygltf::Buffer&     posBuffer       =   model.buffers[posView.buffer];

                    positionData    =   procData<3>(posAccessor,posView,posBuffer);
                    vertexCount     =   posAccessor.count;

                    auto&   buffer  =   mesh->getOrCreate({IS_VERTEX_POS,FMT_R32G32B32_FLOAT});
                    buffer.setBuffer(positionData.data(),positionData.size()* sizeof(positionData[0]));

                    /// 每个属性只做一次 find,后续直接复用迭代器
                    auto    norItr  =   primitive.attributes.find("NORMAL");
                    if (norItr != primitive.attributes.end())
                    {
                        const auto&     norAccessor     =   model.accessors[norItr->second];
                        const auto&     norView         =   model.bufferViews[norAccessor.bufferView];
                        const auto&     norBuffer       =   model.buffers[norView.buffer];

                        normalData  =   procData<3>(norAccessor,norView,norBuffer);
                        auto&           buffer1         =   mesh->getOrCreate({IS_VERTEX_NOR,FMT_R32G32B32_FLOAT});
                        buffer1.setBuffer(normalData.data(),normalData.size()* sizeof(normalData[0]));
                    }
                    auto    tanItr  =   primitive.attributes.find("TANGENT");
                    if (tanItr != primitive.attributes.end())
                    {
                        const auto&     tanAccessor     =   model.accessors[tanItr->second];
                        const auto&     tanView         =   model.bufferViews[tanAccessor.bufferView];
                        const auto&     tanBuffer       =   model.buffers[tanView.buffer];
                        tangentData =   procData<3>(tanAccessor,tanView,tanBuffer);

                        auto&           buffer1          =   mesh->getOrCreate({IS_VERTEX_TANGENT,FMT_R32G32B32_FLOAT});
                        buffer1.setBuffer(tangentData.data(),tangentData.size()* sizeof(tangentData[0]));
                    }
                    auto    uv0Itr  =   primitive.attributes.find("TEXCOORD_0");
                    if (uv0Itr != primitive.attributes.end())
                    {
                        const auto&     uvAccessor     =   model.accessors[uv0Itr->second];
                        const auto&     uvView         =   model.bufferViews[uvAccessor.bufferView];
                        const auto&     uvBuffer       =   model.buffers[uvView.buffer];
                        uvData      =   procData<2>(uvAccessor,uvView,uvBuffer);

                        auto&           buffer1         =   mesh->getOrCreate({IS_VERTEX_TEXCOORD0,FMT_R32G32_FLOAT});
                        buffer1.setBuffer(uvData.data(),uvData.size()* sizeof(uvData[0]));
                    }
                    auto    uv1Itr  =   primitive.attributes.find("TEXCOORD_1");
                    if (uv1Itr != primitive.attributes.end())
                    {
                        const auto&     uvAccessor     =   model.accessors[uv1Itr->second];
                        const auto&     uvView         =   model.bufferViews[uvAccessor.bufferView];
                        const auto&     uvBuffer       =   model.buffers[uvView.buffer];
                        uvData1     =   procData<2>(uvAccessor,uvView,uvBuffer);
                        auto&           buffer1     =   mesh->getOrCreate({IS_VERTEX_TEXCOORD1,FMT_R32G32_FLOAT});
                        buffer1.setBuffer(uvData1.data(),uvData1.size()* sizeof(uvData1[0]));
                    }
                    auto    colorItr    =   primitive.attributes.find("COLOR_0");
                    if (colorItr != primitive.attributes.end())
                    {
                        const auto&     colorAccessor  =   model.accessors[colorItr->second];
                        const auto&     colorView      =   model.bufferViews[colorAccessor.bufferView];
                        const auto&     colorBuffer    =   model.buffers[colorView.buffer];
                        if (colorAccessor.type == TINYGLTF_TYPE_VEC2)
                        {
                            colorData   =   procColor<2>(colorAccessor,colorView,colorBuffer);
                            auto&           buffer1     =   mesh->getOrCreate({IS_VERTEX_COLOR0,FMT_R8G8B8A8_UNORM});
                            buffer1.setBuffer(colorData.data(),colorData.size()* sizeof(colorData[0]));
                        }
                        else if (colorAccessor.type == TINYGLTF_TYPE_VEC3)
                        {
                            colorData   =   procColor<3>(colorAccessor,colorView,colorBuffer);
                            auto&           buffer1     =   mesh->getOrCreate({IS_VERTEX_COLOR0,FMT_R8G8B8A8_UNORM});
                            buffer1.setBuffer(colorData.data(),colorData.size()* sizeof(colorData[0]));
                        }
                            
                        else if (colorAccessor.type == TINYGLTF_TYPE_VEC4)
                        {
                            colorData   =   procColor<4>(colorAccessor,colorView,colorBuffer);
                            auto&           buffer1     =   mesh->getOrCreate({IS_VERTEX_COLOR0,FMT_R8G8B8A8_UNORM});
                            buffer1.setBuffer(colorData.data(),colorData.size()* sizeof(colorData[0]));
                        }
                    }
                    /// 解析面的点索引数据，图元信息
                    Primitive   pri     =   parsePrimitive(primitive, model);
                    if(pri == nullptr)
                        continue;
                    mesh->setPrimitives({pri});
                    mesh->updateAabb();
                    meshs.emplace_back(mesh);
                }
            }
        }

        void        gltfMat2Material(tinygltf::Model& model,MaterialIndex& matMap)
        {
            for (size_t i = 0; i < model.materials.size(); ++i)
            {
                Material    pMat    =   parseMaterialPBR(model,int(i));
                matMap[int(i)]      =   pMat;
            }
        }
        /// <summary>
        /// 解析根级 KHR_materials_variants 扩展:
        ///   变体名表(extensions.KHR_materials_variants.variants)与
        ///   每个材质的 mappings([{material:目标材质索引, variants:[变体索引,...]}])
        /// </summary>
        void        parseVariants(tinygltf::Model& model)
        {
            _variantNames.clear();
            _variantMappings.clear();
            auto    extItr  =   model.extensions.find("KHR_materials_variants");
            if (extItr == model.extensions.end())
                return;
            const auto& obj     =   extItr->second.Get<tinygltf::Value::Object>();
            auto    varItr      =   obj.find("variants");
            if (varItr == obj.end() || !varItr->second.IsArray())
                return;
            for (const auto& v : varItr->second.Get<tinygltf::Value::Array>())
            {
                /// 变体条目形如 {"name": "Carmine Candy"},也可能是纯字符串
                if (v.IsString())
                    _variantNames.push_back(v.Get<std::string>());
                else if (v.IsObject())
                {
                    const auto& vObj    =   v.Get<tinygltf::Value::Object>();
                    auto    nameItr     =   vObj.find("name");
                    if (nameItr != vObj.end() && nameItr->second.IsString())
                        _variantNames.push_back(nameItr->second.Get<std::string>());
                    else
                        _variantNames.push_back("variant" + std::to_string(_variantNames.size()));
                }
            }
            for (size_t m = 0; m < model.materials.size(); ++m)
            {
                auto    mitr    =   model.materials[m].extensions.find("KHR_materials_variants");
                if (mitr == model.materials[m].extensions.end())
                    continue;
                const auto& mobj    =   mitr->second.Get<tinygltf::Value::Object>();
                auto    mapItr      =   mobj.find("mappings");
                if (mapItr == mobj.end() || !mapItr->second.IsArray())
                    continue;
                auto&   mappings    =   _variantMappings[int(m)];
                for (const auto& mv : mapItr->second.Get<tinygltf::Value::Array>())
                {
                    if (!mv.IsObject())
                        continue;
                    const auto& mvObj   =   mv.Get<tinygltf::Value::Object>();
                    auto    matItr      =   mvObj.find("material");
                    auto    varItr2     =   mvObj.find("variants");
                    if (matItr == mvObj.end() || varItr2 == mvObj.end() || !varItr2->second.IsArray())
                        continue;
                    int     target      =   matItr->second.GetNumberAsInt();
                    for (const auto& vi : varItr2->second.Get<tinygltf::Value::Array>())
                        mappings.push_back({vi.GetNumberAsInt(),target});
                }
            }
            /// 规范位置:mappings 挂在每个 mesh primitive 的 extensions 上
            /// (上面的材质级解析作兼容保留);按源材质索引合并并去重
            /// (同一源材质可被多个 primitive 引用,映射相同)
            for (const auto& mesh : model.meshes)
            {
                for (const auto& pri : mesh.primitives)
                {
                    if (pri.material < 0)
                        continue;
                    auto    pitr    =   pri.extensions.find("KHR_materials_variants");
                    if (pitr == pri.extensions.end())
                        continue;
                    const auto& pobj    =   pitr->second.Get<tinygltf::Value::Object>();
                    auto    pmItr       =   pobj.find("mappings");
                    if (pmItr == pobj.end() || !pmItr->second.IsArray())
                        continue;
                    auto&   mappings    =   _variantMappings[pri.material];
                    for (const auto& mv : pmItr->second.Get<tinygltf::Value::Array>())
                    {
                        if (!mv.IsObject())
                            continue;
                        const auto& mvObj   =   mv.Get<tinygltf::Value::Object>();
                        auto    matItr      =   mvObj.find("material");
                        auto    varItr2     =   mvObj.find("variants");
                        if (matItr == mvObj.end() || varItr2 == mvObj.end() || !varItr2->second.IsArray())
                            continue;
                        int     target      =   matItr->second.GetNumberAsInt();
                        for (const auto& vi : varItr2->second.Get<tinygltf::Value::Array>())
                        {
                            int     varIdx  =   vi.GetNumberAsInt();
                            bool    exists  =   false;
                            for (auto& pr : mappings)
                            {
                                if (pr.first == varIdx && pr.second == target)
                                {
                                    exists  =   true;
                                    break;
                                }
                            }
                            if (!exists)
                                mappings.push_back({varIdx,target});
                        }
                    }
                }
            }
        }
        /// <summary>
        /// 切换 KHR_materials_variants 变体:
        /// 把引用了带映射材质的节点换成该变体的目标材质。
        /// 已进渲染工厂的节点需重新入组(先按旧材质摘除,再按新材质注册,
        /// 因为混合/双面材质可能属于不同管线的工厂)。
        /// </summary>
        bool        applyVariant(Nodes& nodes,int variantIndex)
        {
            UNUSED(nodes);
            if (variantIndex < 0 || variantIndex >= (int)_variantNames.size())
                return  false;
            FEScene*    scene   =   _ctx.scene();
            bool        applied =   false;
            for (auto& var : _variantNodes)
            {
                auto&   node    =   var.first;
                auto    mitr    =   _variantMappings.find(var.second);
                if (mitr == _variantMappings.end())
                    continue;
                /// 查找该变体的目标材质
                Material    newMat  =   nullptr;
                for (auto& mapping : mitr->second)
                {
                    if (mapping.first != variantIndex)
                        continue;
                    auto    mItr    =   _variantMaterials.find(mapping.second);
                    if (mItr != _variantMaterials.end())
                        newMat  =   mItr->second;
                    break;
                }
                if (newMat == nullptr || node->material() == newMat)
                    continue;
                /// 节点是否已注册到渲染工厂
                bool    inFactory   =   false;
                if (node->mesh())
                {
                    for (auto& pri : node->mesh()->primitives())
                    {
                        if (pri && pri->factory())
                        {
                            inFactory   =   true;
                            break;
                        }
                    }
                }
                if (scene != nullptr && inFactory)
                {
                    /// 拷贝工厂列表(FERef 的 const operator-> 返回 const 指针,无法调 removeNode)
                    RFactorys   factories  =   scene->factoryMgr().objects();
                    size_t      removed     =   0;
                    for (auto& factory : factories)
                        removed +=  factory->removeNode(node);
                    node->setMaterial(newMat.get());
                    auto    results =   FEFactoryRender::addNodesToFactory(_ctx,*scene,{node});
                    for (auto& ret : results)
                        scene->factoryMgr().addObject(ret);
                }
                else
                {
                    node->setMaterial(newMat.get());
                }
                applied =   true;
            }
            return  applied;
        }
        /// 变体名表(KHR_materials_variants),顺序即变体索引
        const   Strings&    variantNames() const
        {
            return  _variantNames;
        }
        /// <summary>
        /// 解码 glTF URI 中的百分号转义(%20 等),得到可直接访问的文件名
        /// </summary>
        static  String  decodeUri(const String& uri)
        {
            auto    hexValue    =   [](char ch)->int
            {
                if (ch >= '0' && ch <= '9') return  ch - '0';
                if (ch >= 'a' && ch <= 'f') return  ch - 'a' + 10;
                if (ch >= 'A' && ch <= 'F') return  ch - 'A' + 10;
                return  -1;
            };
            String  result;
            result.reserve(uri.size());
            for (size_t i = 0; i < uri.size(); ++i)
            {
                if (uri[i] == '%' && i + 2 < uri.size())
                {
                    const int   hi  =   hexValue(uri[i + 1]);
                    const int   lo  =   hexValue(uri[i + 2]);
                    if (hi >= 0 && lo >= 0)
                    {
                        result.push_back(char((hi << 4) | lo));
                        i += 2;
                        continue;
                    }
                }
                result.push_back(uri[i]);
            }
            return  result;
        }
        /// <summary>
        /// 是否绝对路径:以 / 或 \ 开头,或形如 C:/ (盘符)
        /// </summary>
        static  bool    isAbsolutePath(const String& path)
        {
            if (path.empty())
                return  false;
            if (path[0] == '/' || path[0] == '\\')
                return  true;
            return  path.size() > 1 && path[1] == ':';
        }
        /// <summary>
        /// 生成 glTF 图像的来源对象(纹理只引用它,不再克隆展开后的像素):
        ///   1. 外部 uri          → FEFileBuffer(直接读取原始压缩字节)
        ///   2. bufferView(glb)  → FEFileBuffer(截取内嵌的原始压缩字节)
        ///   3. data URI          → FEFileBuffer(base64 解出原始压缩字节)
        ///   4. 其它(解码像素可用) → FEImage(CPU 图像,体积大但保证不丢图)
        /// </summary>
        Object      createImageSource(const tinygltf::Model& model,const tinygltf::Image& image,int imageIndex)
        {
            const   String  name    =   image.name.empty() ? ("image_" + std::to_string(imageIndex)) : image.name;
            /// 1. 外部文件(uri 相对 glTF 文件目录)
            if (!image.uri.empty() && !tinygltf::IsDataURI(image.uri))
            {
                /// URI 可能是相对路径且带百分号转义(%20 等),解码后再拼目录
                String  fileName    =   decodeUri(image.uri);
                if (!_currentDir.empty() && !isAbsolutePath(fileName))
                    fileName    =   _currentDir + "/" + fileName;
                auto    buffer  =   FEFileBuffer::loadFile(_ctx,fileName.c_str());
                if (buffer != nullptr)
                    return  Object(buffer.get());
            }
            /// 2. .glb 内嵌在 bufferView 中的图片
            if (image.bufferView >= 0 && image.bufferView < (int)model.bufferViews.size())
            {
                const auto& bView   =   model.bufferViews[image.bufferView];
                if (bView.buffer >= 0 && bView.buffer < (int)model.buffers.size())
                {
                    const auto&     buffer  =   model.buffers[bView.buffer];
                    const size_t    offset  =   size_t(bView.byteOffset);
                    if (offset < buffer.data.size() && offset + size_t(bView.byteLength) <= buffer.data.size())
                        return  Object(FEFileBuffer::create(_ctx,buffer.data.data() + offset,size_t(bView.byteLength),name).get());
                }
            }
            /// 3. data URI(把 base64 里的原始压缩字节取出来)
            if (tinygltf::IsDataURI(image.uri))
            {
                std::vector<unsigned char>  bytes;
                std::string                 mimeType;
                if (tinygltf::DecodeDataURI(&bytes,mimeType,image.uri,0,false) && !bytes.empty())
                    return  Object(FEFileBuffer::create(_ctx,bytes.data(),bytes.size(),name).get());
            }
            /// 4. 回退:已解码像素转成 CPU 图像(RGB 补 alpha=255)
            if (image.width <= 0 || image.height <= 0 || (image.component != 3 && image.component != 4))
                return  nullptr;
            if (image.image.empty())
                return  nullptr;
            ImagePtr    cpuImage    =   new FEImage(_ctx);
            FEImage::CreateInfo     ci;
            ci._extent  =   uint3(uint32(image.width),uint32(image.height),1);
            ci._format  =   FMT_R8G8B8A8_UNORM;
            ci._layers  =   1;
            ci._levels  =   1;
            ci._align   =   4;
            if (!cpuImage->create(ci))
                return  nullptr;
            auto            plane   =   cpuImage->plane(0,0);
            const size_t    count   =   size_t(image.width) * size_t(image.height);
            const uint8*    pixels  =   image.image.data();
            for (size_t p = 0; p < count; ++p)
            {
                uint8*  dst =   plane.dataOffset<uint8>(uint32(p / uint32(image.width)),uint32(p % uint32(image.width)));
                if (image.component == 4)
                    memcpy(dst,pixels + p * 4,4);
                else
                {
                    dst[0]  =   pixels[p * 3 + 0];
                    dst[1]  =   pixels[p * 3 + 1];
                    dst[2]  =   pixels[p * 3 + 2];
                    dst[3]  =   255;
                }
            }
            return  Object(cpuImage.get());
        }
        /// <summary>
        /// 由 glTF 图像创建纹理:保存原始压缩字节(FEFileBuffer)作为来源,
        /// 并据此解码上传(含 mip 链);来源不可用时回退到解码后的 CPU 图像。
        /// </summary>
        TexPtr      createTextureFromImage(const tinygltf::Model& model,int imageIndex,
                                           FEFilter magFilter,FEFilter minFilter,
                                           FEAddressMode addressMode,uint mips)
        {
            if (imageIndex < 0 || imageIndex >= (int)model.images.size())
                return  nullptr;
            Object  source  =   createImageSource(model,model.images[imageIndex],imageIndex);
            if (source == nullptr)
                return  nullptr;
            return  FETextureHelper::createFromSource(_ctx,source,magFilter,minFilter,addressMode,mips);
        }
        /// <summary>
        /// 按 glTF texture 索引获取/创建纹理(带缓存,多张贴图共享同一 image 时只上传一次)
        /// 采样模式取自 model.samplers:magFilter/minFilter → FEFilter,wrapS → FEAddressMode;
        /// minFilter 显式声明为"非 mipmap"变体时不生成 mip 链,其余情况(含未指定)生成完整链
        /// </summary>
        TexPtr      getOrCreateTexture(const tinygltf::Model& model,int texIndex)
        {
            if (texIndex < 0 || texIndex >= (int)model.textures.size())
                return  nullptr;
            auto    itr =   _texCache.find(texIndex);
            if (itr != _texCache.end())
                return  itr->second;
            const auto& texture =   model.textures[texIndex];

            FEFilter        magFilter   =   FILTER_LINEAR;
            FEFilter        minFilter   =   FILTER_LINEAR;
            FEAddressMode   addressMode =   ADDRESS_REPEAT;
            /// 0 = 按来源生成完整 mip 链;1 = 不使用 mipmap
            uint            mips        =   0;
            if (texture.sampler >= 0 && texture.sampler < (int)model.samplers.size())
            {
                const auto& sampler =   model.samplers[texture.sampler];
                if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
                    magFilter   =   FILTER_NEAREST;
                switch (sampler.minFilter)
                {
                case TINYGLTF_TEXTURE_FILTER_NEAREST:
                case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
                case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
                    minFilter   =   FILTER_NEAREST;
                    break;
                default:
                    minFilter   =   FILTER_LINEAR;
                    break;
                }
                /// 显式指定不带 mipmap 的过滤(9728/9729)时关闭 mip 链;-1(未指定)保持默认完整链
                if  (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST
                     || sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR)
                    mips    =   1;
                switch (sampler.wrapS)
                {
                case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
                    addressMode =   ADDRESS_MIRRORED_REPEAT;
                    break;
                case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
                    addressMode =   ADDRESS_CLAMP_TO_EDGE;
                    break;
                default:
                    addressMode =   ADDRESS_REPEAT;
                    break;
                }
            }

            TexPtr      tex;
            if (texture.source >= 0 && texture.source < (int)model.images.size())
                tex =   createTextureFromImage(model,texture.source,magFilter,minFilter,addressMode,mips);
            _texCache[texIndex] =   tex;
            return  tex;
        }
        void        createNodeAnimation(tinygltf::Model& model,AnimatonIndex& animas,NodeIndex& nodeMap)
        {   
            using       MapTime     =   std::map<int,RealsObject>;

            MapTime     timeMap;
            auto&       gltfAnim    =   model.animations;
            for (auto& var : gltfAnim)
            {   
                Animation   anim        =   nullptr;
                /// clip 对应 多个chanel
                AnimClip    clip        =   nullptr; 
                Node        node        =   nullptr;
                for (auto& channel: var.channels)
                {
                    auto    itr    =   nodeMap.find(channel.target_node);
                    if (itr != nodeMap.end())
                        node    =   itr->second;
                    else 
                        continue;
                   
                    anim    =   animas[channel.target_node];
                    if (anim == nullptr)
                    {
                        anim    =   new FEAnimation(_ctx);
                        clip    =   new FEAnimClip(_ctx); 
                        anim->setClip(clip);
                        bool    result  =   node->addComponent(anim.get());
                        UNUSED(result);
                        assert(result);
                        animas[channel.target_node] =   anim;
                    }
                    const auto  samplerId       =   channel.sampler;
                    const auto& sampler         =   var.samplers[samplerId];

                    const auto& inputAccessor   =   model.accessors[sampler.input];
                    const auto& inputView       =   model.bufferViews[inputAccessor.bufferView];
                    const auto& inputBuffer     =   model.buffers[inputView.buffer];

                    const auto& outputAccessor  =   model.accessors[sampler.output];
                    const auto& outputView      =   model.bufferViews[outputAccessor.bufferView];
                    const auto& outputBuffer    =   model.buffers[outputView.buffer];
                    
                    RealsObject timeLine        =   nullptr;
                    auto        timeItr         =   timeMap.find(sampler.input);
                    if (timeItr == timeMap.end())
                    {
                        timeLine    =   createTimeLine(inputAccessor,inputView,inputBuffer);
                        if (timeLine == nullptr)
                            node->removeComponent(anim.get());
                        else
                            timeMap[sampler.input]  =   timeLine;
                    }
                    else
                    {
                        timeLine    =   timeItr->second;
                    }
                    if (timeLine == nullptr)
                        continue;
                    if (channel.target_path == "translation")
                        createTransTrack(outputAccessor,outputView,outputBuffer,clip,timeLine);
                    else if (channel.target_path == "scale")
                        createScaleTrack(outputAccessor,outputView,outputBuffer,clip,timeLine);
                    else if (channel.target_path == "rotation")
                        createQuatTrack(outputAccessor,outputView,outputBuffer,clip,timeLine);
                }
            }
        }
        /// <summary>
        /// 暂未实现
        /// </summary>
        /// <param name=""></param>
        /// <param name="channel"></param>
        void        createMaterialAnim(const tinygltf::AnimationSampler&,const tinygltf::AnimationChannel& channel)
        {
            for (auto& var : channel.target_extensions)
            {
                const tinygltf::Value& extValue = var.second;

                // 3. 尝试获取 "pointer" 字段
                if (extValue.Has("pointer")) 
                {
                    // 4. 检查字段类型并获取字符串值
                    const tinygltf::Value& pointerValue = extValue.Get("pointer");
                    if (pointerValue.IsString()) {
                        std::string pointer = pointerValue.Get<std::string>();
                        std::cout << "Found animation pointer: " << pointer << std::endl;
                        // 这里，你就可以用这个指针字符串去做进一步处理了
                    }
                }
            }
        }
        RealsObject createTimeLine(  const tinygltf::Accessor&      accessor
                                    ,const tinygltf::BufferView&    view
                                    ,const tinygltf::Buffer&        buffer)
        {
            RealsObject timeLine    =   nullptr;
            switch (accessor.type)
            {
            case TINYGLTF_TYPE_SCALAR:
                switch (accessor.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    {   
                        timeLine    =   new FERealsObject(_ctx);
                        readValues<real,float>(accessor,view,buffer,timeLine->values());
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                    {
                        timeLine    =   new FERealsObject(_ctx);
                        readValues<real,real>(accessor,view,buffer,timeLine->values());
                    }
                    break;
                default:
                    assert(0!=0 && "error!");
                    break;
                }
                break;
            }
            return  timeLine;
        }

        bool    createTransTrack(const tinygltf::Accessor&      accessor
                                ,const tinygltf::BufferView&    view
                                ,const tinygltf::Buffer&        buffer
                                ,AnimClip                       clip
                                ,RealsObject                    times)
        {
            switch (accessor.type)
            {
            case TINYGLTF_TYPE_SCALAR:  
                switch (accessor.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    {
                        KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                        RealsObject     values  =   new FERealsObject(_ctx);
                        track->setTimeObject(times);
                        track->setValueObject(values);
                        track->setPropertyIndex(PROP_TRANSFORM_X);
                        readValues<real,float>(accessor,view,buffer,values->values());
                        clip->addTrack(track);
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                    {
                        KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                        RealsObject     values  =   new FERealsObject(_ctx);
                        track->setTimeObject(times);
                        track->setValueObject(values);
                        track->setPropertyIndex(PROP_TRANSFORM_X);
                        readValues<real,real>(accessor,view,buffer,values->values());
                        clip->addTrack(track);
                    }
                    break;
                default:
                    assert(0!=0);
                    return  false;
                }
                break;
            case TINYGLTF_TYPE_VEC2:
                switch (accessor.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    {
                        {
                            KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                            RealsObject     values  =   new FERealsObject(_ctx);
                            track->setTimeObject(times);
                            track->setValueObject(values);
                            track->setPropertyIndex(PROP_TRANSFORM_X);
                            readValues<real,float>(accessor,view,buffer,values->values());
                            clip->addTrack(track);
                        }
                        {
                            KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                            RealsObject     values  =   new FERealsObject(_ctx);
                            track->setTimeObject(times);
                            track->setValueObject(values);
                            track->setPropertyIndex(PROP_TRANSFORM_Y);
                            readValues<real,float>(accessor,view,buffer,values->values());
                            clip->addTrack(track);
                        }
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                    {
                       {
                            KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                            RealsObject     values  =   new FERealsObject(_ctx);
                            track->setTimeObject(times);
                            track->setValueObject(values);
                            track->setPropertyIndex(PROP_TRANSFORM_X);
                            readValues<real,real>(accessor,view,buffer,values->values());
                            clip->addTrack(track);
                       }
                       {
                           KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                           RealsObject     values  =   new FERealsObject(_ctx);
                           track->setTimeObject(times);
                           track->setValueObject(values);
                           track->setPropertyIndex(PROP_TRANSFORM_Y);
                           readValues<real,real>(accessor,view,buffer,values->values());
                           clip->addTrack(track);
                       }
                    }
                    break;
                default:
                    assert(0!=0);
                    return  false;
                }
                break;
            case TINYGLTF_TYPE_VEC3:
                switch (accessor.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    {
                        KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                        Real3sObject    values  =   new FEReal3sObject(_ctx);
                        track->setTimeObject(times);
                        track->setValueObject(values);
                        track->setPropertyIndex(PROP_TRANSFORM_XYZ);
                        readValues<real3,float3>(accessor,view,buffer,values->values());
                        clip->addTrack(track);
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                    {
                        KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                        Real3sObject    values  =   new FEReal3sObject(_ctx);
                        track->setTimeObject(times);
                        track->setValueObject(values);
                        track->setPropertyIndex(PROP_TRANSFORM_XYZ);
                        readValues<real3,real3>(accessor,view,buffer,values->values());
                        clip->addTrack(track);
                    }
                    break;
                default:
                    assert(0!=0);
                    return  false;
                }
                break;
            default:
                assert(0!=0);
                return  false;
            }
            return  true;
        }
        bool    createScaleTrack(const tinygltf::Accessor&      accessor
                                ,const tinygltf::BufferView&    view
                                ,const tinygltf::Buffer&        buffer
                                ,AnimClip                       clip
                                ,RealsObject                    times)
        {
            switch (accessor.type)
            {
            case TINYGLTF_TYPE_SCALAR:  
                switch (accessor.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    {
                        KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                        FloatsObject    values  =   new FEFloatsObject(_ctx);
                        track->setTimeObject(times);
                        track->setValueObject(values);
                        track->setPropertyIndex(PROP_SCALE_X);
                        readValues<float,float>(accessor,view,buffer,values->values());
                        clip->addTrack(track);
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                    {
                        KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                        FloatsObject    values  =   new FEFloatsObject(_ctx);
                        track->setTimeObject(times);
                        track->setValueObject(values);
                        track->setPropertyIndex(PROP_SCALE_X);
                        readValues<float,real>(accessor,view,buffer,values->values());
                        clip->addTrack(track);
                    }
                    break;
                default:
                    assert(0!=0);
                    return  false;
                }
                break;
            case TINYGLTF_TYPE_VEC2:
                switch (accessor.componentType)
                {
                /// case TINYGLTF_COMPONENT_TYPE_FLOAT:
                ///     {
                ///         KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                ///         Float2sObject   values  =   new FEFloat2sObject(_ctx);
                ///         track->setTimeObject(times);
                ///         track->setValueObject(values);
                ///         track->setPropertyIndex(PROP_SCALE_XY);
                ///         readValues<float2,float2>(accessor,view,buffer,values->values());
                ///         clip->addTrack(track);
                ///     }
                ///     break;
                /// case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                ///     {
                ///         KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                ///         Float2sObject   values  =   new FEFloat2sObject(_ctx);
                ///         track->setTimeObject(times);
                ///         track->setValueObject(values);
                ///         track->setPropertyIndex(PROP_SCALE_XY);
                ///         readValues<float2,real2>(accessor,view,buffer,values->values());
                ///         clip->addTrack(track);
                ///     }
                ///     break;
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                    assert(0!=0);
                    return  false;
                }
                break;
            case TINYGLTF_TYPE_VEC3:
                switch (accessor.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    {
                        KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                        Float3sObject   values  =   new FEFloat3sObject(_ctx);
                        track->setTimeObject(times);
                        track->setValueObject(values);
                        track->setPropertyIndex(PROP_SCALE_XYZ);
                        readValues<float3,float3>(accessor,view,buffer,values->values());
                        clip->addTrack(track);
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                    {
                        KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                        Float3sObject   values  =   new FEFloat3sObject(_ctx);
                        track->setTimeObject(times);
                        track->setValueObject(values);
                        track->setPropertyIndex(PROP_SCALE_XYZ);
                        readValues<float3,real3>(accessor,view,buffer,values->values());
                        clip->addTrack(track);
                    }
                    break;
                default:
                    assert(0!=0);
                    return  false;
                }
                break;
            default:
                assert(0!=0);
                return  false;
            }
            return  true;
        }
        bool    createQuatTrack( const tinygltf::Accessor&      accessor
                                ,const tinygltf::BufferView&    view
                                ,const tinygltf::Buffer&        buffer
                                ,AnimClip                       clip
                                ,RealsObject                    times)
        {
            switch (accessor.type)
            {
            case TINYGLTF_TYPE_VEC4:
                switch (accessor.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    {
                        KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                        QuatfsObject    values  =   new FEQuatfsObject(_ctx);
                        track->setTimeObject(times);
                        track->setValueObject(values);
                        track->setPropertyIndex(PROP_QUAT);
                        readValues<quatf,quatf>(accessor,view,buffer,values->values());
                        clip->addTrack(track);
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                    {
                        KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                        QuatfsObject    values  =   new FEQuatfsObject(_ctx);
                        track->setTimeObject(times);
                        track->setValueObject(values);
                        track->setPropertyIndex(PROP_QUAT);
                        readValues<quatf,quatr>(accessor,view,buffer,values->values());
                        clip->addTrack(track);
                    }
                    break;
                default:
                    assert(0!=0);
                    return  false;
                }
                break;
            default:
                assert(0!=0);
                return  false;
            }
            return  true;
        }
        
        /// <summary>
        /// 读取数据
        /// </summary>
        /// <typeparam name="TValue">存储类型</typeparam>
        /// <typeparam name="SValue">数据源数据类型</typeparam>
        /// <param name="accessor">访问器</param>
        /// <param name="view">buffer view</param>
        /// <param name="buffer">数据缓冲区</param>
        /// <param name="values">结果数据</param>
        template<typename TValue,typename SValue>
        void    readValues(  const tinygltf::Accessor&      accessor
                            ,const tinygltf::BufferView&    view
                            ,const tinygltf::Buffer&        buffer
                            ,std::vector<TValue>&           values)
        {
            const auto      pBuffer     =   &buffer.data[view.byteOffset + accessor.byteOffset];
            const SValue*   pSource     =   (const SValue*)pBuffer;
            values.resize(accessor.count);
            for (size_t i = 0; i < accessor.count; ++i)
            {
                values[i]   =   (TValue)pSource[i];
            }
        }
        Material    getOrCreate(MaterialIndex& matMap,const tinygltf::Model& model, int matIndex,Mesh mesh)
        { 
            auto    itr =   matMap.find(matIndex);
            if (itr != matMap.end())
                return  itr->second;

            if (model.materials.empty())
            {
                auto    mat         =   new FEMaterialV3(_ctx); 
                matMap[matIndex]    =   mat;
                return  mat;
            }
            if (matIndex < 0 || matIndex >= (int)model.materials.size())
                return  nullptr;
            if (mesh->slotBits().hasFlag(IS_VERTEX_NOR))
            {
                auto    mat         =   parseMaterialPBR(model,matIndex);
                matMap[matIndex]    =   mat;
                return  mat;
            }
            if (mesh->slotBits().hasFlag(IS_VERTEX_COLOR0))
            {
                auto    mat         =   parseMaterialV3C4(model,matIndex);
                matMap[matIndex]    =   mat;
                return  mat;
            }
            {
                auto    mat         =   parseMaterialV3(model,matIndex);
                matMap[matIndex]    =   mat;
                return  mat;
            }
        }

        Material    parseMaterialV3(const tinygltf::Model& model, int matIndex)
        {
            UNUSED(model);
            UNUSED(matIndex);
            /// auto&       material    =   model.materials[matIndex];
            auto        sysMat      =   new FEMaterialV3C4(_ctx);
            return      sysMat;
        }
        Material    parseMaterialV3C4(const tinygltf::Model& model, int matIndex)
        {
            UNUSED(model);
            UNUSED(matIndex);
           ///  auto&       material    =   model.materials[matIndex];
            auto        sysMat      =   new FEMaterialV3(_ctx);
            return      sysMat;
        }

        Material    parseMaterialPBR(const tinygltf::Model& model, int matIndex)
        {
            auto        texTransform        =   [](const tinygltf::ExtensionMap& extensions,float2& vScale,float2& vTrans,float& radian)
            {
                /// 检查扩展是否存在
                auto extIt = extensions.find("KHR_texture_transform");
                if (extIt != extensions.end())
                {
                    // 3. 获取扩展数据对象
                    const auto& extension   =   extIt->second;
                    const auto& obj         =   extension.Get<tinygltf::Value::Object>();

                    // 4. 读取 scale 数组
                    auto itr = obj.find("scale");
                    if (itr != obj.end() && itr->second.IsArray())
                    {
                        const auto& datas = itr->second.Get<tinygltf::Value::Array>();
                        if (datas.size() >= 2)
                        {
                            double valU =   datas[0].GetNumberAsDouble();
                            double valV =   datas[1].GetNumberAsDouble();
                            vScale      =   float2(valU,valV);
                        }
                    }
                    itr = obj.find("offset");
                    if (itr != obj.end() && itr->second.IsArray())
                    {
                        const auto& datas = itr->second.Get<tinygltf::Value::Array>();
                        if (datas.size() >= 2)
                        {
                            double valU =   datas[0].GetNumberAsDouble();
                            double valV =   datas[1].GetNumberAsDouble();
                            vTrans      =   float2(valU,valV);
                        }
                    }
                    itr = obj.find("rotation");
                    if (itr != obj.end())
                    {
                        double valU =   itr->second.GetNumberAsDouble();
                        radian      =   float(valU);
                    }
                    return  true;
                }
                return  false;
            };
            auto        getEmissiveStrength =   [](const tinygltf::ExtensionMap& extensions)
            {
                auto extIt = extensions.find("KHR_materials_emissive_strength");
                if (extIt != extensions.end())
                {
                    // 3. 获取扩展数据对象
                    const auto& extension   =   extIt->second;
                    const auto& objs        =   extension.Get<tinygltf::Value::Object>();
                    auto itr = objs.find("emissiveStrength");
                    if (itr != objs.end())
                    {
                        double  val =   itr->second.GetNumberAsDouble();
                        return  float(val);
                    }

                }
                return  1.0f;
            };
            auto&       material    =   model.materials[matIndex];
            bool        bTest       =   material.alphaMode == "MASK";
            UNUSED(bTest);
            auto        alphaRef    =   (float)material.alphaCutoff;
            UNUSED(alphaRef);
            const auto& pbr         =   material.pbrMetallicRoughness;
            UNUSED(pbr);
            auto        pbrMat      =   new FEMaterialPBR(_ctx);
            
            pbrMat->data()._value._emissive     =   float4(1,1,1,1);
            pbrMat->data()._value._diffuse      =   float4(1,1,1,1);
            pbrMat->data()._value._spacular     =   float4(1,1,1,1); 
            pbrMat->data()._value._roughness    =   0.3f;
            pbrMat->data()._value._metallic     =   0.7f;

            for (size_t i = 0 ;i <  material.emissiveFactor.size(); ++ i)
                pbrMat->data()._value._emissive[(int)i]  =   (float)material.emissiveFactor[i];

            for (size_t i = 0 ;i < material.pbrMetallicRoughness.baseColorFactor.size(); ++ i)
                pbrMat->data()._value._diffuse[(int)i]   =   (float)material.pbrMetallicRoughness.baseColorFactor[i];

            pbrMat->data()._value._metallic     =   (float)material.pbrMetallicRoughness.metallicFactor;
            pbrMat->data()._value._roughness    =   (float)material.pbrMetallicRoughness.roughnessFactor;

            pbrMat->data().update();

            /// ===== 全套贴图(glTF 金属度-粗糙度工作流) =====
            /// baseColor / metallicRoughness / normal / occlusion / emissive
            /// KHR_texture_transform(UV 变换)逐贴图解析后通过 setUvTransform 下发,
            /// shader 侧 applyUvTr(slot, uv) 在采样前应用(槽序 = PBRTexSlot)
            auto        applyTexTr  =   [&](const tinygltf::ExtensionMap& extensions,FEMaterialPBR::PBRTexSlot slot)
            {
                float2  vScale(1.0f,1.0f), vTrans(0.0f,0.0f);
                float   radian  =   0.0f;
                if (texTransform(extensions,vScale,vTrans,radian))
                    pbrMat->setUvTransform(uint(slot),vScale,vTrans,radian);
            };
            if (pbr.baseColorTexture.index >= 0)
            {
                pbrMat->setBaseColorMap(getOrCreateTexture(model,pbr.baseColorTexture.index));
                applyTexTr(pbr.baseColorTexture.extensions,FEMaterialPBR::SLOT_BASECOLOR);
            }
            if (pbr.metallicRoughnessTexture.index >= 0)
            {
                pbrMat->setMetallicRoughnessMap(getOrCreateTexture(model,pbr.metallicRoughnessTexture.index));
                applyTexTr(pbr.metallicRoughnessTexture.extensions,FEMaterialPBR::SLOT_METALLICROUGH);
            }
            if (material.normalTexture.index >= 0)
            {
                pbrMat->setNormalMap(getOrCreateTexture(model,material.normalTexture.index));
                pbrMat->setNormalScale((float)material.normalTexture.scale);
                applyTexTr(material.normalTexture.extensions,FEMaterialPBR::SLOT_NORMAL);
            }
            if (material.occlusionTexture.index >= 0)
            {
                pbrMat->setOcclusionMap(getOrCreateTexture(model,material.occlusionTexture.index));
                pbrMat->setOcclusionStrength((float)material.occlusionTexture.strength);
                applyTexTr(material.occlusionTexture.extensions,FEMaterialPBR::SLOT_OCCLUSION);
            }
            if (material.emissiveTexture.index >= 0)
            {
                pbrMat->setEmissiveMap(getOrCreateTexture(model,material.emissiveTexture.index));
                applyTexTr(material.emissiveTexture.extensions,FEMaterialPBR::SLOT_EMISSIVE);
            }
            pbrMat->setEmissiveStrength(getEmissiveStrength(material.extensions));

            /// ===== KHR 材质扩展 =====
            auto    getExtFloat =   [](const tinygltf::ExtensionMap& extensions,const char* extName,const char* key,float defVal)
            {
                auto    extItr  =   extensions.find(extName);
                if (extItr == extensions.end())
                    return  defVal;
                const auto& obj =   extItr->second.Get<tinygltf::Value::Object>();
                auto    itr     =   obj.find(key);
                if (itr == obj.end())
                    return  defVal;
                return  float(itr->second.GetNumberAsDouble());
            };
            /// KHR_materials_clearcoat 清漆层
            float   ccStrength  =   getExtFloat(material.extensions,"KHR_materials_clearcoat","clearcoatFactor",0.0f);
            if (ccStrength > 0.0f)
            {
                float   ccRoughness =   getExtFloat(material.extensions,"KHR_materials_clearcoat","clearcoatRoughnessFactor",0.0f);
                pbrMat->setClearcoat(ccStrength,ccRoughness);
            }
            /// KHR_materials_transmission 透射(无折射 pass,以 alpha 混合近似)
            float   transmission    =   getExtFloat(material.extensions,"KHR_materials_transmission","transmissionFactor",0.0f);
            if (transmission > 0.0f)
                pbrMat->setTransmission(transmission);
            /// KHR_materials_volume 体积(厚度/Beer-Lambert 吸收;配合 transmission)
            /// thicknessFactor>0 才有视觉效果;attenuationColor 默认白(不吸收),
            /// attenuationDistance 规范默认 +∞,这里以 0 表示"无穷大=不吸收"
            auto    volExtItr   =   material.extensions.find("KHR_materials_volume");
            if (volExtItr != material.extensions.end() && volExtItr->second.IsObject())
            {
                const auto& volObj  =   volExtItr->second.Get<tinygltf::Value::Object>();
                float   volThickness=   getExtFloat(material.extensions,"KHR_materials_volume","thicknessFactor",0.0f);
                float   volAttDist  =   getExtFloat(material.extensions,"KHR_materials_volume","attenuationDistance",0.0f);
                float3  volAttColor(1.0f,1.0f,1.0f);
                auto    colItr      =   volObj.find("attenuationColor");
                if (colItr != volObj.end() && colItr->second.IsArray())
                {
                    const auto& arr =   colItr->second.Get<tinygltf::Value::Array>();
                    for (int c = 0; c < 3 && c < (int)arr.size(); ++c)
                    {
                        if (arr[c].IsNumber())
                            volAttColor[c]  =   float(arr[c].GetNumberAsDouble());
                    }
                }
                if (volThickness > 0.0f)
                    pbrMat->setVolume(volThickness,volAttColor,volAttDist);
            }
            /// KHR_materials_iridescence 虹彩(薄膜干涉)
            float   iriFactor   =   getExtFloat(material.extensions,"KHR_materials_iridescence","iridescenceFactor",0.0f);
            if (iriFactor > 0.0f)
            {
                float   iriIor      =   getExtFloat(material.extensions,"KHR_materials_iridescence","iridescenceIor",1.3f);
                float   iriTmin     =   getExtFloat(material.extensions,"KHR_materials_iridescence","iridescenceThicknessMinimum",100.0f);
                float   iriTmax     =   getExtFloat(material.extensions,"KHR_materials_iridescence","iridescenceThicknessMaximum",400.0f);
                pbrMat->setIridescence(iriFactor,iriIor,iriTmin,iriTmax);

                /// iridescenceThicknessTexture(嵌套 textureInfo:index/texCoord/extensions,
                /// G 通道在 [tmin,tmax] 插值;CarConcept Paint 1 Pearl 即带此贴图)
                auto    iriExtItr   =   material.extensions.find("KHR_materials_iridescence");
                if (iriExtItr != material.extensions.end() && iriExtItr->second.IsObject())
                {
                    const auto& iriObj  =   iriExtItr->second.Get<tinygltf::Value::Object>();
                    auto        texItr  =   iriObj.find("iridescenceThicknessTexture");
                    if (texItr != iriObj.end() && texItr->second.IsObject())
                    {
                        const auto& texObj  =   texItr->second.Get<tinygltf::Value::Object>();
                        auto        idxItr  =   texObj.find("index");
                        const int   texIndex=   (idxItr != texObj.end() && idxItr->second.IsNumber())
                                                ? idxItr->second.GetNumberAsInt() : -1;
                        if (texIndex >= 0)
                        {
                            pbrMat->setIriThicknessMap(getOrCreateTexture(model,texIndex));
                            /// KHR_texture_transform(嵌套于 textureInfo.extensions;
                            /// tinygltf 中 Value::Object 与 ExtensionMap 同为 map<string,Value>)
                            auto    extTItr =   texObj.find("extensions");
                            if (extTItr != texObj.end() && extTItr->second.IsObject())
                                applyTexTr(extTItr->second.Get<tinygltf::Value::Object>(),FEMaterialPBR::SLOT_IRITHICKNESS);
                        }
                    }
                }
            }
            /// AO 贴图使用 TEXCOORD_1(CarConcept 即如此)
            if (material.occlusionTexture.index >= 0 && material.occlusionTexture.texCoord == 1)
                pbrMat->setOcclusionTexCoord(1);

            /// ===== 管线变体(放在最后,贴图/参数设置完成后再切管线,避免多次重绑)=====
            /// BLEND 或 transmission>0 → 混合管线(pbrBlend 本身双面);否则 doubleSided → 双面管线(pbr2s)
            if (material.alphaMode == "BLEND" || transmission > 0.0f)
                pbrMat->setAlphaBlend(true);
            else if (material.doubleSided)
                pbrMat->setDoubleSided(true);

            return      pbrMat;

        }
        /// <summary>
        /// 进度通知
        /// </summary>
        /// <param name="val"></param>
        void        onProgress(real val)
        {
            printf("progress:%lf\n", val * 100.0);
        }
    protected:
        Nodes       parseNodes(tinygltf::Model& model,MeshsIndex& meshMap,MaterialIndex& matMap,NodeIndex& nodeMap)
        {
            Nodes   results;
            results.reserve(model.nodes.size());
            for (auto& scene : model.scenes)
            {
                for (size_t i = 0; i < scene.nodes.size(); ++i)
                {
                    auto    nodeId  =   scene.nodes[i] ;
                    assert( nodeId >= 0 && (nodeId < model.nodes.size()));

                    if(nodeId < 0 || nodeId > model.nodes.size())
                        continue;
                    auto&   gltfNode    =   model.nodes[nodeId];
                    auto    node        =   parseNode(model,gltfNode,nullptr,meshMap,matMap,nodeMap);
                    nodeMap[nodeId]     =   node;
                    results.push_back(node);
                }
            }
            return  results;
        }

        Node        parseNode(   tinygltf::Model& model
                                ,const tinygltf::Node& node
                                ,Node           parent
                                ,MeshsIndex&    meshMap
                                ,MaterialIndex& matMap
                                ,NodeIndex&     nodeMap)
        {
            Node    pNode   =   new FENode(_ctx);
            if (node.name.empty())
                pNode->setName("test");
            else
                pNode->setName(node.name);
            real3   trans   =   real3(0.0);
            if (node.translation.size() >= 3)
            {
                trans.x = static_cast<double>(node.translation[0]);
                trans.y = static_cast<double>(node.translation[1]);
                trans.z = static_cast<double>(node.translation[2]);
                pNode->setLocalTranslation(trans);
            }
            quatr rotation  =    quatr();
            if (node.rotation.size() == 4)
            {
                rotation.x = static_cast<double>(node.rotation[0]);
                rotation.y = static_cast<double>(node.rotation[1]);
                rotation.z = static_cast<double>(node.rotation[2]);
                rotation.w = static_cast<double>(node.rotation[3]);

                pNode->setLocalRotation(rotation);
            }
            real3 scale = real3(0.0);
            if (node.scale.size() >= 3)
            {
                scale.x = static_cast<double>(node.scale[0]);
                scale.y = static_cast<double>(node.scale[1]);
                scale.z = static_cast<double>(node.scale[2]);
                pNode->setLocalScaling(scale);
            }
            //有可能只有matrix，没有上面的三项
            if(node.matrix.size() == 16)
            {
                mat4r   mat;
                /// glTF node.matrix 为列主序:元素 e = col*4+row,引擎 mat4r 同为列主序,直接填充
                for(int i = 0; i < node.matrix.size(); ++i)
                {
                    mat[i/4][i%4] = node.matrix[i];
                }

                FE::decompose(mat, trans, scale, rotation);
                pNode->setLocalTranslation(trans);
                pNode->setLocalScaling(scale);
                pNode->setLocalRotation(rotation);
            }

            auto    itr     =   meshMap.find(node.mesh);
            if (itr != meshMap.end())
            {
                auto&   meshs       =   itr->second;
                auto&   gltfMesh    =   model.meshes[node.mesh];
                
                if (meshs.size() == 1)
                {
                    auto    mesh    =   meshs.front();
                    auto    mat     =   getOrCreate(matMap,model,gltfMesh.primitives[0].material,mesh);
                    /// 无材质时仍要保留网格与子节点,不能提前返回
                    pNode->setMesh(meshs.front());
                    if (mat != nullptr)
                    {
                        pNode->setMaterial(mat);
                        /// 变体:源材质带 KHR_materials_variants 映射时记录节点(供 applyVariant 切换)
                        if (_variantMappings.find(gltfMesh.primitives[0].material) != _variantMappings.end())
                            _variantNodes.push_back({pNode,gltfMesh.primitives[0].material});
                    }
                }
                else if (meshs.size() == gltfMesh.primitives.size() && !meshs.empty())
                {
                    for (size_t i = 0; i < gltfMesh.primitives.size(); ++i)
                    {
                        auto    mat =   getOrCreate(matMap,model,gltfMesh.primitives[i].material,meshs[i]);
                        Node        child   =   new FENode(_ctx);
                        child->setMesh(meshs[i]);
                        if (mat != nullptr)
                        {
                            child->setMaterial(mat);
                            /// 变体:同上,多 primitive 时记录到 child 节点
                            if (_variantMappings.find(gltfMesh.primitives[i].material) != _variantMappings.end())
                                _variantNodes.push_back({child,gltfMesh.primitives[i].material});
                        }

                        pNode->addChild(child.get());
                    }
                }
            }
            
            if (parent)
            {
                parent->addChild(pNode.get());
            }
            for (auto nodeId: node.children)
            {
                auto&   gltfNode    =   model.nodes[nodeId];
                auto    result      =   parseNode(model,gltfNode,pNode,meshMap,matMap,nodeMap);
                nodeMap[nodeId]     =   result;
            }
            return  pNode;
        }
        Primitive   parsePrimitive(const tinygltf::Primitive& primitive ,const tinygltf::Model& model)
        {
            FE::Primitive   pri             =   nullptr;
            const auto&     indexAccessor   =    model.accessors[primitive.indices];
            const auto&     indexBufferView =    model.bufferViews[indexAccessor.bufferView];
            const auto&     indexBuffer     =    model.buffers[indexBufferView.buffer];

            auto    getHostPrimitiveType     =   [](const tinygltf::Primitive& primitive)
            {
                switch (primitive.mode)
                {
                case TINYGLTF_MODE_POINTS:          return  PRI_POINTS;
                case TINYGLTF_MODE_LINE:            return  PRI_LINES;
                case TINYGLTF_MODE_LINE_LOOP:       return  PRI_LINE_STRIP;
                case TINYGLTF_MODE_LINE_STRIP:      return  PRI_LINE_STRIP;
                case TINYGLTF_MODE_TRIANGLES:       return  PRI_TRIANGLES;
                case TINYGLTF_MODE_TRIANGLE_STRIP:  return  PRI_TRIANGLE_STRIP;
                case TINYGLTF_MODE_TRIANGLE_FAN:    return  PRI_TRIANGLE_FAN;
                default:                            return  PRI_POINTS;
                }
            };

            if(indexAccessor.count <= 0)
                return nullptr;
            switch (indexAccessor.componentType)
            {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                {
                    auto    priObject   =   new FEDrawElementUint32(_ctx);
                    priObject->setPrimitive(getHostPrimitiveType(primitive));
                    auto    indices     =   reinterpret_cast<const uint32_t*> (&indexBuffer.data[indexAccessor.byteOffset + indexBufferView.byteOffset]);
                    uint    isLineLoop  =   TINYGLTF_MODE_LINE_LOOP == primitive.mode ? 1:0;
                    auto&   idxs        =   priObject->index();
                    idxs.resize(indexAccessor.count + isLineLoop);
                    for (size_t k = 0; k < indexAccessor.count; ++k)
                        idxs[k] = indices[k];
                    if (isLineLoop == 1)
                        idxs[indexAccessor.count]   =   indices[0];
                    return  priObject;
                }
                break;
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                {
                    auto    priObject   =   new FEDrawElementUint16(_ctx);
                    priObject->setPrimitive(getHostPrimitiveType(primitive));
                    auto    indices     =   reinterpret_cast<const uint16_t*> (&indexBuffer.data[indexAccessor.byteOffset + indexBufferView.byteOffset]);
                    uint    isLineLoop  =   TINYGLTF_MODE_LINE_LOOP == primitive.mode ? 1:0;
                    auto&   idxs        =   priObject->index();
                    idxs.resize(indexAccessor.count + isLineLoop);
                    for (size_t k = 0; k < indexAccessor.count; ++k)
                        idxs[k] = indices[k];
                    if (isLineLoop == 1)
                        idxs[indexAccessor.count]   =   indices[0];
                    return  priObject;
                }
                break;
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                {
                    auto    priObject   =   new FEDrawElementUint8(_ctx);
                    priObject->setPrimitive(getHostPrimitiveType(primitive));
                    auto    indices     =   reinterpret_cast<const uint8_t*> (&indexBuffer.data[indexAccessor.byteOffset + indexBufferView.byteOffset]);
                    uint    isLineLoop  =   TINYGLTF_MODE_LINE_LOOP == primitive.mode ? 1:0;
                    auto&   idxs        =   priObject->index();
                    idxs.resize(indexAccessor.count + isLineLoop);
                    for (size_t k = 0; k < indexAccessor.count; ++k)
                        idxs[k] = indices[k];
                    if (isLineLoop == 1)
                        idxs[indexAccessor.count]   =   indices[0];
                    return  priObject;
                }
                break;
            }
            return  nullptr;
        }
    protected:
        template<uint COMS>
        static  floats      procData(const tinygltf::Accessor&   posAccessor,const tinygltf::BufferView& posView,const tinygltf::Buffer&  posBuffer)
        {
            floats      result;
            const auto  dataPtr     =   &posBuffer.data[posView.byteOffset + posAccessor.byteOffset];
            size_t      byteStride  =   posAccessor.ByteStride(posView);

            result.reserve( posAccessor.count * COMS);

            for (size_t c = 0; c < posAccessor.count; ++c)
            {
                const unsigned char* address = dataPtr + (c * byteStride);
                switch (posAccessor.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_BYTE:
                    {
                        const int8_t* v = reinterpret_cast<const int8_t*>(address);
                        if (posAccessor.normalized)
                        {
                            for (size_t i = 0; i < COMS; i++)
                            {
                                float   x   =   (std::max)(v[i] * oneOver127F, -1.0f);
                                result.insert(result.end(),x);
                            }
                        }
                        else
                        {
                            for (size_t i = 0; i < COMS; i++)
                            {
                                result.insert(result.end(),v[i]);
                            }
                        }
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    {
                        const uint8_t* v = reinterpret_cast<const uint8_t*>(address);
                        if (posAccessor.normalized)
                        {
                            for (size_t i = 0; i < COMS; i++)
                            {
                                float   x   =   (v[i] * oneOver255F);
                                result.insert(result.end(),x);
                            }
                        }
                        else
                        {
                            for (size_t i = 0; i < COMS; i++)
                            {
                                result.insert(result.end(),v[i]);
                            }
                        }
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_SHORT:
                    {
                        const int16_t* v = reinterpret_cast<const int16_t*>(address);
                        if (posAccessor.normalized)
                        {
                            for (size_t i = 0; i < COMS; i++)
                            {
                                float   x   =   (std::max<float>)(v[i] * oneOver32767F, -1.0f);
                                result.insert(result.end(),x);
                            }
                        }
                        else
                        {
                            for (size_t i = 0; i < COMS; i++)
                            {
                                result.insert(result.end(),v[i]);
                            }
                        }
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    {
                        const uint16_t* v = reinterpret_cast<const uint16_t*>(address);
                        if (posAccessor.normalized)
                        {
                            for (size_t i = 0; i < COMS; i++)
                            {
                                float   x   =   (v[i] * oneOver65535F);
                                result.insert(result.end(),x);
                            }
                        }
                        else
                        {
                            for (size_t i = 0; i < COMS; i++)
                            {
                                result.insert(result.end(),v[i]);
                            }
                        }
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    {
                        const auto* v   =   reinterpret_cast<const float*>(address);
                        for (size_t i = 0; i < COMS; i++)
                        {
                            result.insert(result.end(),v[i]);
                        }
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                    {
                        const auto  v =     reinterpret_cast<const double*>(address);
                        for (size_t i = 0; i < COMS; i++)
                        {
                            result.insert(result.end(),float(v[i]));
                        }
                    }
                    break;
                default:
                    break;
                }
            }
            return  result;
        };

        template<uint COMS>
        static  uint8x4s    procColor(const tinygltf::Accessor&   posAccessor,const tinygltf::BufferView& posView,const tinygltf::Buffer& posBuffer)
        {
            static_assert(COMS <= 4);

            uint8x4s    result;
            const auto  dataPtr     =   &posBuffer.data[posView.byteOffset + posAccessor.byteOffset];
            size_t      byteStride  =   posAccessor.ByteStride(posView);

            result.reserve( posAccessor.count);

            static  constexpr   float   oneOver32767    =   (float)(1.0/32767.0); 
            static  constexpr   float   oneOver65535    =   (float)(1.0/65535.0);      //  1.0f/1024.f;

            for (size_t c = 0; c < posAccessor.count; ++c)
            {
                const unsigned char* address = dataPtr + (c * byteStride);
                switch (posAccessor.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_BYTE:
                    {
                        auto    v   =   reinterpret_cast<const int8_t*>(address);
                        uint8x4 data(255,255,255,255);
                        for (uint i = 0; i < COMS; i++)
                        {
                            data[i] =   uint8_t(v[i]  + 128);
                        }
                        result.push_back(data);
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    {
                        auto    v   =   reinterpret_cast<const uint8_t*>(address);
                        uint8x4 data(255,255,255,255);
                        for (uint i = 0; i < COMS; i++)
                        {
                            data[i] = uint8_t(v[i]);
                        }
                        result.push_back(data);
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_SHORT:
                    {
                        auto*   v = reinterpret_cast<const int16_t*>(address);
                        uint8x4 data(255,255,255,255);
                        for (uint i = 0; i < COMS; i++)
                        {
                            float   rf  =   (v[i] + 32768.0f) * oneOver255F;
                            data[i]     =   uint8_t(rf);
                        }
                        result.push_back(data);
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    {
                        auto    v   =   reinterpret_cast<const uint16_t*>(address);
                        uint8x4 data(255,255,255,255);
                        for (uint i = 0; i < COMS; i++)
                        {
                            float   rf  =   (v[i]) * oneOver255F;
                            data[i]     =   uint8_t(rf);
                        }
                        result.push_back(data);
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    {
                        auto    v   =   reinterpret_cast<const float*>(address);
                        uint8x4 data(255,255,255,255);
                        for (uint i = 0; i < COMS; i++)
                        {
                            float   rf  =   (v[i]) * 255.0f;
                            data[i]     =   uint8_t(rf);
                        }
                        result.push_back(data);
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                    {
                        auto    v   =     reinterpret_cast<const double*>(address);
                        uint8x4 data(255,255,255,255);
                        for (uint i = 0; i < COMS; i++)
                        {
                            float   rf  =   float(v[i]) * 255.0f;
                            data[i]     =   uint8_t(rf);
                        }
                        result.push_back(data);
                    }
                    break;
                default:
                    break;
                }
            }
            return  result;
        }
    public:
        static  Formats formatList()
        {
            return  FEGLTFFormat::formats(UUIDOF(FEFormatGLTFReader));
        }
    };
}
