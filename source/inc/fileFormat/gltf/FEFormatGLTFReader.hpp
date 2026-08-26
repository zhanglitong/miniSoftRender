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
#include    "FEFormatGLTF.hpp"


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
        using   MeshsIndex      =   std::map<int,Meshs>;
        using   MaterialIndex   =   std::map<int,Material>;
        using   NodeIndex       =   std::map<int,Node>;
        using   AnimatonIndex   =   std::map<int,Animation>;
    public:
        IMPLEMENT_CLASS_REFLECT(FEFormatGLTFReader)
    public:
        FEFormatGLTFReader(FEContext& ctx)
            :FEStreamReader(ctx)
        {}
        FEFormatGLTFReader(const FEFormatGLTFReader& other)
            :FEStreamReader(other)
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
        /// 接口调用加载文件
        /// </summary>
        /// <param name="file"></param>
        /// <returns></returns>
        virtual Objects readFiles(const Strings& files) override
        {
            FESerializeCtx    xsCtx;
            ObjectUMap      objectMap;
            Objects         result;
            FEReader*       pReader =   nullptr;
            uint64          offset  =   0;
            xsCtx.query     =   [&](const OBJId& id,FEObject* object,FESerializeCtx::Option option)->FESerializeCtx::QResult
            {
                switch(option)
                {
                case FESerializeCtx::O_Query:
                    {
                        auto    itr =   objectMap.find(id);
                        if (itr != objectMap.end())
                            return  {itr->second.get(),0};
                        else
                            return  {};
                    }
                    break;
                case FESerializeCtx::O_Cache:
                    objectMap[id]   =   object;
                    return  {object,0};
                case FESerializeCtx::O_Progress:
                    return  {(FEProgress*)this,0};
                case FESerializeCtx::O_UpdateProgress:
                    {
                        FEReader*   reader  =   object ? object->as<FEReader>() : nullptr;
                        if (reader != nullptr && xsCtx.change)
                        {
                            offset          +=  pReader ? pReader->length() : 0;
                            pReader         =   reader;
                            xsCtx.change    =   false;
                        }
                        if (reader)
                            xsCtx.doByte    =  offset   +   reader->tell();
                        auto    per     =   double(xsCtx.doByte)/(std::max)(1.0,double(xsCtx.allByte));
                        setProgress(per);
                    }
                    return  {(FEProgress*)this,0};
                default:
                    return  {};
                }
            };
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
            /// 查询函数
            newCtx.query        =   [&](const OBJId& id,FEObject* object,FESerializeCtx::Option option)->FESerializeCtx::QResult
            {
                switch(option)
                {
                case FESerializeCtx::O_Query:
                    {
                        auto    itr =   objectMap.find(id);
                        if (itr != objectMap.end())
                            return  {itr->second.get(),0};
                        else
                            return  {};
                    }
                    break;
                case FESerializeCtx::O_Cache:
                    objectMap[id]   =   object;
                    return  {object,0};
                case FESerializeCtx::O_Progress:
                    return  {(FEProgress*)this,0};
                case FESerializeCtx::O_UpdateProgress:
                    {
                        FEReader*   reader  =   object ? object->as<FEReader>() : nullptr;
                        if (reader != nullptr && pCtx->change)
                        {
                            offset          +=  pReader ? pReader->length() : 0;
                            pReader         =   reader;
                            pCtx->change    =   false;
                        }
                        auto    per     =   double(pCtx->doByte)/(std::max)(1.0,double(pCtx->allByte));
                        setProgress(per);
                    }
                    return  {(FEProgress*)this,0};
                default:
                    return  {};
                }
            };
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
                String              error;
                String              warning;
                String              dir =   FEFileSystem::filePath(fileName.c_str());
                if(!gltfContext.LoadASCIIFromString(&gltfModel,&error,&warning,var->dataAs<char>(),(uint)var->length(),dir))
                    continue;
                MeshsIndex          meshMap;
                MaterialIndex       matMap;
                AnimatonIndex       animations;
                NodeIndex           nodeMap;
                gltfMesh2Mesh(gltfModel,meshMap);
                gltfMat2Material(gltfModel,matMap);
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

                    if (primitive.attributes.find("NORMAL")  != primitive.attributes.end())
                    {
                        const auto&     norAccessor     =   model.accessors[primitive.attributes.find("NORMAL")->second];
                        const auto&     norView         =   model.bufferViews[norAccessor.bufferView];
                        const auto&     norBuffer       =   model.buffers[norView.buffer];

                        normalData  =   procData<3>(norAccessor,norView,norBuffer);
                        auto&           buffer1         =   mesh->getOrCreate({IS_VERTEX_NOR,FMT_R32G32B32_FLOAT});
                        buffer1.setBuffer(normalData.data(),normalData.size()* sizeof(normalData[0]));
                    }
                    if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
                    {
                        const auto&     tanAccessor     =   model.accessors[primitive.attributes.find("TANGENT")->second];
                        const auto&     tanView         =   model.bufferViews[tanAccessor.bufferView];
                        const auto&     tanBuffer       =   model.buffers[tanView.buffer];
                        tangentData =   procData<3>(tanAccessor,tanView,tanBuffer);

                        auto&           buffer1          =   mesh->getOrCreate({IS_VERTEX_TANGENT,FMT_R32G32B32_FLOAT});
                        buffer1.setBuffer(tangentData.data(),tangentData.size()* sizeof(tangentData[0]));
                    }
                    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
                    {
                        const auto&     uvAccessor     =   model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                        const auto&     uvView         =   model.bufferViews[uvAccessor.bufferView];
                        const auto&     uvBuffer       =   model.buffers[uvView.buffer];
                        uvData      =   procData<2>(uvAccessor,uvView,uvBuffer);

                        auto&           buffer1         =   mesh->getOrCreate({IS_VERTEX_TEXCOORD0,FMT_R32G32_FLOAT});
                        buffer1.setBuffer(uvData.data(),uvData.size()* sizeof(uvData[0]));
                    }
                    if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
                    {
                        const auto&     uvAccessor     =   model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
                        const auto&     uvView         =   model.bufferViews[uvAccessor.bufferView];
                        const auto&     uvBuffer       =   model.buffers[uvView.buffer];
                        uvData1     =   procData<2>(uvAccessor,uvView,uvBuffer);
                        auto&           buffer1     =   mesh->getOrCreate({IS_VERTEX_TEXCOORD1,FMT_R32G32_FLOAT});
                        buffer1.setBuffer(uvData1.data(),uvData1.size()* sizeof(uvData1[0]));
                    }
                    if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
                    {
                        const auto&     colorAccessor  =   model.accessors[primitive.attributes.find("COLOR_0")->second];
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
                        timeLine    =   new TValueArray<real>(_ctx);
                        readValues<real,float>(accessor,view,buffer,timeLine->values());
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                    {
                        timeLine    =   new TValueArray<real>(_ctx);
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
                        RealsObject     values  =   new TValueArray<real>(_ctx);
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
                        RealsObject     values  =   new TValueArray<real>(_ctx);
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
                        KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                        Real2sObject    values  =   new TValueArray<real2>(_ctx);
                        track->setTimeObject(times);
                        track->setValueObject(values);
                        track->setPropertyIndex(PROP_TRANSFORM_XY);
                        readValues<real2,float2>(accessor,view,buffer,values->values());
                        clip->addTrack(track);
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                    {
                        KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                        Real2sObject    values  =   new TValueArray<real2>(_ctx);
                        track->setTimeObject(times);
                        track->setValueObject(values);
                        track->setPropertyIndex(PROP_TRANSFORM_XY);
                        readValues<real2,real2>(accessor,view,buffer,values->values());
                        clip->addTrack(track);
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
                        Real3sObject    values  =   new TValueArray<real3>(_ctx);
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
                        Real3sObject    values  =   new TValueArray<real3>(_ctx);
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
                        FloatsObject    values  =   new TValueArray<float>(_ctx);
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
                        FloatsObject    values  =   new TValueArray<float>(_ctx);
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
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    {
                        KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                        Float2sObject   values  =   new TValueArray<float2>(_ctx);
                        track->setTimeObject(times);
                        track->setValueObject(values);
                        track->setPropertyIndex(PROP_SCALE_XY);
                        readValues<float2,float2>(accessor,view,buffer,values->values());
                        clip->addTrack(track);
                    }
                    break;
                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                    {
                        KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx);
                        Float2sObject   values  =   new TValueArray<float2>(_ctx);
                        track->setTimeObject(times);
                        track->setValueObject(values);
                        track->setPropertyIndex(PROP_SCALE_XY);
                        readValues<float2,real2>(accessor,view,buffer,values->values());
                        clip->addTrack(track);
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
                        Float3sObject   values  =   new TValueArray<float3>(_ctx);
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
                        Float3sObject   values  =   new TValueArray<float3>(_ctx);
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
                        QuatfsObject    values  =   new TValueArray<quatf>(_ctx);
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
                        QuatfsObject    values  =   new TValueArray<quatf>(_ctx);
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
            (void)model;
            (void)matIndex;
            /// auto&       material    =   model.materials[matIndex];
            auto        sysMat      =   new FEMaterialV3C4(_ctx);
            return      sysMat;
        }
        Material    parseMaterialV3C4(const tinygltf::Model& model, int matIndex)
        {
            (void)model;
            (void)matIndex;
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
            (void)bTest;
            auto        alphaRef    =   (float)material.alphaCutoff;
            (void)alphaRef;
            const auto& pbr         =   material.pbrMetallicRoughness;
            (void)pbr;
            auto        pbrMat      =   new FEMaterialPBR(_ctx);
            
            pbrMat->data()._value._emissive     =   float4(1,1,1,1);
            pbrMat->data()._value._diffuse      =   float4(1,1,1,1);
            pbrMat->data()._value._spacular     =   float4(1,1,1,1); 
            pbrMat->data()._value._roughness    =   0.3f;
            pbrMat->data()._value._metallic     =   0.7f;

            for (size_t i = 0 ;i <  material.emissiveFactor.size(); ++ i)
                pbrMat->data()._value._emissive[i]  =   (float)material.emissiveFactor[i];

            for (size_t i = 0 ;i < material.pbrMetallicRoughness.baseColorFactor.size(); ++ i)
                pbrMat->data()._value._diffuse[i]   =   (float)material.pbrMetallicRoughness.baseColorFactor[i];

            pbrMat->data()._value._metallic     =   (float)material.pbrMetallicRoughness.metallicFactor;
            pbrMat->data()._value._roughness    =   (float)material.pbrMetallicRoughness.roughnessFactor;

            pbrMat->data().update();

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
                    if (mat == nullptr)
                        return  pNode;
                    pNode->setMesh(meshs.front());
                    pNode->setMaterial(mat);
                }
                else if (meshs.size() == gltfMesh.primitives.size() && !meshs.empty())
                {
                    for (size_t i = 0; i < gltfMesh.primitives.size(); ++i)
                    {
                        auto    mat =   getOrCreate(matMap,model,gltfMesh.primitives[i].material,meshs[i]);
                        if (mat == nullptr)
                            continue;
                        Node        child   =   new FENode(_ctx);
                        child->setMesh(meshs[i]);
                        child->setMaterial(mat);

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
