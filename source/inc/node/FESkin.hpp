#pragma     once
#include    <unordered_map>
#include    <vector>
#include    "../FEContext.hpp"
#include    "../FEMath.hpp"
#include    "../FEComponent.hpp"
#include    "../graphic/FEGPUBuffer.h"
#include    "../mesh/FEMesh.hpp"
#include    "FENode.hpp"
namespace   FE
{
    /// <summary>
    /// 一根骨骼的绑定:关节节点(强引用,避免节点销毁后悬垂) + 逆绑定矩阵。
    /// 它不是独立的场景实体——关节仍然是 FENode;
    /// 这里只把"节点 + 矩阵"成对存放,让蒙皮热循环顺序访问一次即可拿到两者。
    /// </summary>
    struct  FEBone
    {
        Node    node;
        mat4r   inverseBind =   mat4r(1.0);
    };

    using   Joints          =   std::vector<FENode*>;
    using   InverseBinds    =   std::vector<mat4r>;
    using   Bones           =   std::vector<FEBone>;

    DEFINE_CLASS_UUID(FESkinData,"{A4D9E2C7-6F31-4B85-9E27-C8A1D3F5B604}");
    /// <summary>
    /// 一份 glTF skin 的共享骨骼数据(glTF skins[k] 的 joints / inverseBindMatrices):
    ///   _bones 与 JOINTS_0 中的索引一一对应,顺序即 joints[] 顺序。
    /// 同一个 glTF skin 被多个 mesh 节点引用时(多 primitive 拆出的子节点),
    /// 这些节点各自挂一个 FESkin 组件,但共享同一份 FESkinData;
    /// 蒙皮矩阵不能共享——skinMatrix 里的 inverse(meshGlobal) 每个节点不同。
    /// </summary>
    class   FE_API  FESkinData   :public  FEObject
    {
    public:
        IMPLEMENT_CLASS_REFLECT_MINI(FESkinData)
    public:
        FESkinData(FEContext& ctx)
            :FEObject(ctx)
        {}
        FESkinData(const FESkinData& other)
            :FEObject(other)
            ,_bones(other._bones)
            ,_worldRot(other._worldRot)
            ,_pendingJoints(other._pendingJoints)
        {}
        inline  void    setJoints(const Joints& joints,const InverseBinds& inverseBinds)
        {
            _bones.clear();
            _pendingJoints.clear();
            _bones.reserve(joints.size());
            for (size_t i = 0;i < joints.size();++i)
            {
                FEBone  bone;
                bone.node           =   Node(joints[i]);
                bone.inverseBind    =   (i < inverseBinds.size()) ? inverseBinds[i] : mat4r(1.0);
                _bones.emplace_back(bone);
            }
        }
        inline  size_t  jointCount() const
        {
            return  _bones.size();
        }
        inline  const   Bones&  bones() const
        {
            return  _bones;
        }
        inline  Bones&  bones()
        {
            return  _bones;
        }
        /// <summary>
        /// 蒙皮结果相对引擎坐标系的额外朝向修正(左乘到世界空间):
        ///   world = _worldRotation * jointGlobal * inverseBind * vertex
        /// 蒙皮时 mesh 节点的世界变换会被约掉,所以 Y-up 模型要在 Z-up 引擎中
        /// 站立,必须在蒙皮矩阵里左乘 Y-up -> Z-up 的旋转
        /// </summary>
        inline  void    setWorldRotation(const mat4r& rot)
        {
            _worldRot   =   rot;
        }
        inline  const   mat4r&  worldRotation() const
        {
            return  _worldRot;
        }
        /// <summary>
        /// 写入骨骼:关节对象 id + inverseBind 矩阵 + worldRotation。
        /// 关节引用的是 id,读回时节点可能尚未创建,由 resolvePending 统一回填
        /// </summary>
        virtual void    serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const override;
        virtual void    deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx) override;
        /// <summary>
        /// 用 id -> 节点 的映射回填反序列化时未解析的关节引用;
        /// 返回本次成功回填的关节数量
        /// </summary>
        size_t          resolveJoints(const std::unordered_map<OBJId,FENode*>& nodes);
        /// <summary>
        /// 等待回填的关节数量(0 表示引用已全部解析)
        /// </summary>
        size_t          pendingJointCount() const
        {
            size_t  count   =   0;
            for (auto& id : _pendingJoints)
            {
                if (id != FEUuid::zero())
                    ++count;
            }
            return  count;
        }
        /// <summary>
        /// 节点树读取完成后统一回填所有蒙皮组件的关节引用
        /// (关节节点作为节点树的普通节点保存,FESkinData 先于节点树被反序列化);
        /// 返回成功回填的关节数量
        /// </summary>
        static  size_t  resolvePending(const Nodes& roots);
    protected:
        Bones                   _bones;
        mat4r                   _worldRot   =   mat4r(1.0);
        /// <summary>
        /// 反序列化时尚未解析的关节对象 id(与 _bones 对齐,零 uuid 表示已解析)
        /// </summary>
        Uuids                   _pendingJoints;
    };

    DEFINE_CLASS_UUID(FESkin,"{3C7E1A92-5B4D-4F0E-8A61-9D2B4C6E7F10}");
    /// <summary>
    /// glTF 骨骼蒙皮组件(per mesh node):
    ///   作为 FEComponent 挂在 mesh 节点上,蒙皮作用的节点即组件宿主(attach 传入),
    ///   骨骼数据(joints / inverseBindMatrices)由 FESkinData 共享。
    /// 每帧由骨骼动画系统(FESkinSys)驱动:
    ///   GPU: compute shader 合成 skinning 矩阵写入 storage buffer(材质绑定 SB_Skin);
    ///        管线不可用时回退 CPU 合成 + 上传;
    ///   CPU: 把 LBS 结果写回 mesh 顶点属性并通知渲染工厂重建顶点缓冲。
    /// 顶点着色器按骨骼索引取矩阵:
    ///   skinMatrix = inverse(meshGlobal) * worldRotation * jointGlobal * inverseBindMatrix
    /// 之后仍由 instance 的世界变换把结果变换到世界空间。
    /// </summary>
    class   FE_API  FESkin  :public  FEComponent
    {
    public:
        using   Matrices    =   std::vector<mat4>;
    public:
        IMPLEMENT_CLASS_REFLECT(FESkin)
    public:
        FESkin(FEContext& ctx)
            :FEComponent(ctx)
        {
        }
        FESkin(const FESkin& other)
            :FEComponent(other)
            ,_data(other._data)
            ,_cpuSkin(other._cpuSkin)
        {}
        /// <summary>
        /// 共享骨骼数据:同一个 glTF skin 的多个 mesh 节点共享一份
        /// </summary>
        inline  void    setData(const SharedPtr<FESkinData>& data)
        {
            _data   =   data;
        }
        inline  SharedPtr<FESkinData>   data() const
        {
            return  _data;
        }
        /// <summary>
        /// 序列化依赖:共享骨骼数据(FESkinData)随组件一起写入 .fepj
        /// </summary>
        virtual size_t  queryDepends(ObjectUSet& uSet) const override;
        virtual void    serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const override;
        virtual void    deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx) override;
        /// <summary>
        /// 蒙皮所作用(mesh 所在)的节点:组件宿主,由 addComponent 内部 attach 绑定
        /// </summary>
        inline  FENode* meshNode() const
        {
            if (owner() == nullptr)
                return  nullptr;
            else
                return  owner()->as<FENode>();
        }
        /// <summary>
        /// joints 与 inverseBindMatrices 一一对应,顺序与 JOINTS_0 中的索引一致
        /// </summary>
        inline  void    setJoints(const Joints& joints,const InverseBinds& inverseBinds)
        {
            if (_data == nullptr)
                return;
            _data->setJoints(joints,inverseBinds);
            _matrices.resize(_data->jointCount(),mat4(1.0));
        }
        inline  size_t  jointCount() const
        {
            return  (_data == nullptr) ? 0 : _data->jointCount();
        }
        /// <summary>
        /// 朝向修正(左乘到世界空间),写入共享数据
        /// </summary>
        inline  void    setWorldRotation(const mat4r& rot)
        {
            if (_data != nullptr)
                _data->setWorldRotation(rot);
        }
        /// <summary>
        /// 纯 CPU 蒙皮开关:开启后骨骼动画系统(FESkinSys)每帧调用 skinMesh(),
        /// 把 LBS 结果写回 mesh 顶点属性并通知渲染工厂重建顶点缓冲;
        /// 此时材质应使用不做 GPU 蒙皮的普通管线(如 pbr)
        /// </summary>
        inline  void    setCpuSkin(bool on)
        {
            _cpuSkin    =   on;
        }
        inline  bool    cpuSkin() const
        {
            return  _cpuSkin;
        }
        inline  SBO&    buffer()
        {
            return  _buffer;
        }
        inline  const   SBO&    buffer() const
        {
            return  _buffer;
        }
        inline  const   Matrices&   matrices() const
        {
            return  _matrices;
        }
        /// <summary>
        /// 只重算蒙皮矩阵(mesh 局部空间),不触碰 GPU 资源;
        /// CPU 蒙皮与 GPU 上传都以此为准
        /// </summary>
        void    updateMatrices()
        {
            const   size_t  count   =   jointCount();
            FENode*         node    =   meshNode();
            if (_data == nullptr || node == nullptr || count == 0)
                return;
            _matrices.resize(count);
            const   mat4r   invMesh =   FE::inverse(node->globalTransform());
            const   mat4r   world   =   _data->worldRotation();
            auto&           bones   =   _data->bones();
            for (size_t i = 0;i < count;++i)
            {
                FEBone&         bone    =   bones[i];
                FENode*         joint   =   bone.node.get();
                if (joint == nullptr)
                {
                    _matrices[i]    =   mat4(1.0);
                    continue;
                }
                const   mat4r   skin    =   invMesh * world
                                            * joint->globalTransform()
                                            * bone.inverseBind;
                _matrices[i]            =   mat4(skin);
            }
        }
        /// <summary>
        /// 重算蒙皮矩阵并上传 GPU;由骨骼动画系统每帧调用。
        /// compute 不可用时的回退路径(CPU 合成 + 上传)。
        /// </summary>
        void    update()
        {
            updateMatrices();
            const   size_t  count   =   _matrices.size();
            if (meshNode() == nullptr || count == 0)
                return;
            const   uint64  length  =   uint64(count) * sizeof(mat4);
            if (!ensureStorage(_buffer,length))
                return;
            _buffer->update(_matrices.data(),length,0);
        }
        /// <summary>
        /// GPU compute 蒙皮的 uniform 参数(std140,与 skinCompute/shader.comp 的 SkinParamBuffer 一致)
        /// </summary>
        struct  ComputeParam
        {
            mat4    _invMeshWorld   =   mat4(1.0);
            uint    _count          =   0;
        };
        inline  Pipeline    computePipeline() const
        {
            return  _computePipe;
        }
        /// <summary>
        /// 准备 GPU compute 蒙皮:确保 compute 管线/描述符/缓冲可用,
        /// 并上传本帧输入(关节世界矩阵、逆绑定矩阵、per-skin 常量)。
        /// 返回 false 表示 compute 不可用(如后端不支持 compute),调用方回退 CPU 矩阵路径。
        /// 输出缓冲与顶点着色器的 SB_Skin 共用(_buffer),无需回读 CPU。
        /// 关节世界矩阵仍由 CPU 侧 FENode 层级每帧更新(约束/IK 仍可用),
        /// compute 只负责 invMesh * worldRot * jointGlobal * inverseBind 的矩阵合成。
        /// </summary>
        bool    prepareCompute()
        {
            if (_computeInvalid)
                return  false;
            const   size_t  count   =   jointCount();
            FENode*         node    =   meshNode();
            if (_data == nullptr || node == nullptr || count == 0)
                return  false;

            if (_computeSet == nullptr)
            {
                Pipeline    pl;
                if (!ctx().device().pipelines().query("skinCompute/default",pl) || pl == nullptr)
                {
                    _computeInvalid =   true;
                    return  false;
                }
                auto    dsets   =   pl->createDSets();
                if (dsets.empty())
                {
                    _computeInvalid =   true;
                    return  false;
                }
                _computePipe    =   pl;
                _computeSet     =   dsets.front();
            }

            const   uint64  length  =   uint64(count) * sizeof(mat4);
            if (!ensureStorage(_jointWorld,length)
                ||  !ensureStorage(_inverseBind,length)
                ||  !ensureStorage(_buffer,length))
                return  false;
            if (_computeParam == nullptr)
            {
                _computeParam   =   ctx().device().createUBO();
                if (_computeParam == nullptr
                    ||  !_computeParam->create({sizeof(ComputeParam),HOST_VISIBLE_BIT}))
                {
                    _computeParam   =   nullptr;
                    return  false;
                }
            }

            auto&   bones   =   _data->bones();
            _jointWorlds.resize(count);
            _inverseBinds.resize(count);
            for (size_t i = 0;i < count;++i)
            {
                FENode*     joint   =   bones[i].node.get();
                _jointWorlds[i]     =   (joint == nullptr) ? mat4(1.0) : mat4(joint->globalTransform());
                _inverseBinds[i]    =   mat4(bones[i].inverseBind);
            }
            _jointWorld->update(_jointWorlds.data(),length,0);
            _inverseBind->update(_inverseBinds.data(),length,0);

            ComputeParam    param;
            param._invMeshWorld =   mat4(FE::inverse(node->globalTransform()) * _data->worldRotation());
            param._count        =   uint(count);
            _computeParam->update(&param,sizeof(param),0);

            _computeSet->setBinding(0,{_jointWorld.get()});
            _computeSet->setBinding(1,{_inverseBind.get()});
            _computeSet->setBinding(2,{_buffer.get()});
            _computeSet->setBinding(3,{_computeParam.get()});
            _computeSet->update();
            return  true;
        }
        /// <summary>
        /// 在外部已 bindPipeline 的 compute 命令缓冲上绑定本 skin 的数据并派发;
        /// 调用前需先执行 prepareCompute()
        /// </summary>
        void    dispatchCompute(FECmdBuffer* cmd)
        {
            if (cmd == nullptr || _computePipe == nullptr || _computeSet == nullptr)
                return;
            FECmdBuffer::DSetBind   binds   =   {};
            binds.dSets         =   DSets{_computeSet};
            binds.firstSet      =   0;
            binds.offsetCount   =   0;
            binds.offsets       =   nullptr;
            binds.plBindPoint   =   PL_COMPUTE;
            binds.plLayout      =   _computePipe->nativeLayout();
            cmd->bindDescriptors(binds);

            const   uint    count   =   uint(jointCount());
            const   uint    groups  =   (count + uint(SKIN_GROUP_X) - 1) / uint(SKIN_GROUP_X);
            cmd->dispatch(groups,1,1);
        }
        /// <summary>
        /// 纯 CPU 端蒙皮(LBS):单个顶点位置
        ///   权重按总和归一化(与 pbrSkin 顶点着色器的语义一致),
        ///   权重和为 0 或骨骼索引越界时保持原值
        /// 调用前需先执行 updateMatrices()
        /// </summary>
        real3   skinPoint(const real3& pos,const ushort4& jointIdx,const float4& weights) const
        {
            const   float   wsum    =   weights.x + weights.y + weights.z + weights.w;
            if (wsum <= 0.0f)
                return  pos;
            const   float   invW    =   1.0f / wsum;
            float4          result  =   float4(0.0f);
            const   float4  src     =   float4(float(pos.x),float(pos.y),float(pos.z),1.0f);
            for (int k = 0;k < 4;++k)
            {
                const   float   w   =   weights[k] * invW;
                if (w <= 0.0f)
                    continue;
                const   size_t  j   =   jointIdx[k];
                if (j >= _matrices.size())
                    continue;
                result  +=  _matrices[j] * src * w;
            }
            return  real3(real(result.x),real(result.y),real(result.z));
        }
        /// <summary>
        /// 纯 CPU 端蒙皮(LBS):单个顶点法线,结果归一化;
        /// 权重和为 0 时保持原值。调用前需先执行 updateMatrices()
        /// </summary>
        float3  skinNormal(const float3& nor,const ushort4& jointIdx,const float4& weights) const
        {
            const   float   wsum    =   weights.x + weights.y + weights.z + weights.w;
            if (wsum <= 0.0f)
                return  nor;
            const   float   invW    =   1.0f / wsum;
            float3          result  =   float3(0.0f);
            for (int k = 0;k < 4;++k)
            {
                const   float   w   =   weights[k] * invW;
                if (w <= 0.0f)
                    continue;
                const   size_t  j   =   jointIdx[k];
                if (j >= _matrices.size())
                    continue;
                result  +=  mat3(_matrices[j]) * nor * w;
            }
            const   float   length  =   glm::length(result);
            return  (length > 0.0f) ? result / length : nor;
        }
        /// <summary>
        /// 纯 CPU 端蒙皮:把 mesh 的 POSITION/NORMAL 按骨骼权重计算后写回属性缓冲区。
        /// 绑定姿态数据在首次调用时缓存,之后每帧都基于绑定姿态重算(不会累乘);
        /// 写回后 mesh 局部包围盒随之更新,调用方(骨骼动画系统)还需通知渲染工厂
        /// 重建顶点缓冲(节点置 FLAG_PROP_MESH 并 fireChanged)。
        /// 要求:POSITION=R32G32B32_FLOAT,JOINTS_0=R16G16B16A16_UINT,
        ///       WEIGHTS_0=R32G32B32A32_FLOAT(glTF 解析器统一转换后的格式)
        /// 调用前需先执行 updateMatrices()
        /// </summary>
        bool    skinMesh(FEMesh& mesh)
        {
            if (_matrices.empty())
                return  false;
            FEAttribyteBuffer*  posBuf  =   mesh.get(IS_VERTEX_POS);
            FEAttribyteBuffer*  idxBuf  =   mesh.get(IS_VERTEX_BONE_INDEX);
            FEAttribyteBuffer*  wgtBuf  =   mesh.get(IS_VERTEX_BONE_WEIGHT);
            if (posBuf == nullptr || idxBuf == nullptr || wgtBuf == nullptr)
                return  false;
            if (posBuf->attr().format() != FMT_R32G32B32_FLOAT
                ||  idxBuf->attr().format() != FMT_R16G16B16A16_UINT
                ||  wgtBuf->attr().format() != FMT_R32G32B32A32_FLOAT)
                return  false;

            const   uint    count   =   posBuf->count();
            if (count == 0 || idxBuf->count() < count || wgtBuf->count() < count)
                return  false;

            CpuMeshData*    cache   =   queryCpuMesh(mesh);
            if (cache->positions.size() != count)
            {
                /// 缓存绑定姿态:之后的每帧都从这里重算
                const   float3* src =   reinterpret_cast<const float3*>(posBuf->data());
                cache->positions.assign(src,src + count);
                cache->normals.clear();
                FEAttribyteBuffer*  nrmBuf  =   mesh.get(IS_VERTEX_NOR);
                if (nrmBuf != nullptr
                    &&  nrmBuf->attr().format() == FMT_R32G32B32_FLOAT
                    &&  nrmBuf->count() >= count)
                {
                    const   float3* nsrc    =   reinterpret_cast<const float3*>(nrmBuf->data());
                    cache->normals.assign(nsrc,nsrc + count);
                }
            }

            const   ushort4*    joints  =   reinterpret_cast<const ushort4*>(idxBuf->data());
            const   float4*     weights =   reinterpret_cast<const float4*>(wgtBuf->data());

            float3s skinnedPos(count);
            for (uint i = 0;i < count;++i)
            {
                const   real3   p   =   skinPoint(real3(cache->positions[i]),joints[i],weights[i]);
                skinnedPos[i]       =   float3(float(p.x),float(p.y),float(p.z));
            }
            posBuf->setBuffer(skinnedPos.data(),size_t(count) * sizeof(float3));

            if (!cache->normals.empty())
            {
                FEAttribyteBuffer*  nrmBuf  =   mesh.get(IS_VERTEX_NOR);
                float3s             skinnedNor(count);
                for (uint i = 0;i < count;++i)
                    skinnedNor[i]   =   skinNormal(cache->normals[i],joints[i],weights[i]);
                nrmBuf->setBuffer(skinnedNor.data(),size_t(count) * sizeof(float3));
            }
            mesh.updateAabb();
            return  true;
        }
    protected:
        /// <summary>
        /// 确保 host visible storage buffer 存在且容量足够。
        /// 注意: VKGPUBuffer::update 按单个内存标志分支处理,
        /// 传组合标志(HOST_VISIBLE|HOST_COHERENT)会导致更新直接失败,
        /// 这里只传 HOST_VISIBLE_BIT,非一致内存由 flush 负责刷新
        /// </summary>
        bool    ensureStorage(SBO& buffer,const uint64 length)
        {
            if (buffer == nullptr)
            {
                buffer  =   ctx().device().createSBO();
                if (buffer == nullptr || !buffer->create({length,HOST_VISIBLE_BIT}))
                {
                    buffer  =   nullptr;
                    return  false;
                }
            }
            else if (buffer->cInfo()._length < length)
            {
                buffer->resize(length);
            }
            return  buffer != nullptr;
        }
        /// <summary>
        /// CPU 蒙皮缓存的绑定姿态数据(按 mesh 区分)
        /// </summary>
        struct  CpuMeshData
        {
            FEMesh*     mesh    =   nullptr;
            float3s     positions;
            float3s     normals;
        };
        using   CpuMeshes   =   std::vector<CpuMeshData>;
        CpuMeshData*    queryCpuMesh(FEMesh& mesh)
        {
            for (auto& var : _cpuMeshes)
            {
                if (var.mesh == &mesh)
                    return  &var;
            }
            _cpuMeshes.emplace_back();
            _cpuMeshes.back().mesh =   &mesh;
            return  &_cpuMeshes.back();
        }
    protected:
        /// <summary>
        /// 共享骨骼数据(同一个 glTF skin 的多个 mesh 节点共用)
        /// </summary>
        SharedPtr<FESkinData>   _data;
        bool                    _cpuSkin    =   false;
        Matrices                _matrices;
        SBO                     _buffer;
        /// <summary>
        /// GPU compute 蒙皮资源(惰性创建):
        ///   _jointWorld / _inverseBind 为 compute 输入,_buffer 为输出(与 VS 共用),
        ///   管线/描述符不可用时置 _computeInvalid,回退 CPU 矩阵上传
        /// </summary>
        Pipeline                _computePipe;
        DSet                    _computeSet;
        UBO                     _computeParam;
        SBO                     _jointWorld;
        SBO                     _inverseBind;
        std::vector<mat4>       _jointWorlds;
        std::vector<mat4>       _inverseBinds;
        bool                    _computeInvalid =   false;
        /// <summary>
        /// CPU 蒙皮的绑定姿态缓存
        /// </summary>
        CpuMeshes                   _cpuMeshes;
    };

    using   Skin    =   SharedPtr<FESkin>;
    /// <summary>
    /// 取节点上的蒙皮组件(未挂载返回 nullptr)。
    /// 放在 FESkin 一侧而不是 FENode 上,保持 FENode 不依赖蒙皮模块,
    /// 依赖方向始终是 FESkin -> FENode。
    /// </summary>
    inline  SharedPtr<FESkin>   skinOf(FENode* node)
    {
        if (node == nullptr)
            return  SharedPtr<FESkin>(nullptr);
        auto    skins   =   node->objects<FESkin>();
        return  skins.empty() ? SharedPtr<FESkin>(nullptr) : SharedPtr<FESkin>(skins.front());
    }
}
