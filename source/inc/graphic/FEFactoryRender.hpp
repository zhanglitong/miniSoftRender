#pragma     once

#include    "../FEPriority.hpp"
#include    "../FEKeyValues.hpp"
#include    "../FEFactory.hpp"
#include    "../node/FENode.hpp"
#include    "../FECamera.hpp"
#include    "FEDevice.h"
#include    "FEGraphicEnums.h"

namespace   FE
{
    class   FEScene;
    /// <summary>
    /// 一个工厂对应一种类型的属性
    /// 节点数据更新流程，当有节点更新，需要通知工厂，只标记工程需要更新，不做其他处理
    /// 工程根据标记进行检测更新
    /// 1. 如果是位置属性数据发生变化，节点组会循环检测所有节点标记，更新instance 数据
    /// 2. 如果是材质发生变化，外部需要移除/添加节点
    /// 3. 如果是mesh发生变化，如果是mesh数据，则更新vbo,ito
    /// 4. 如果是图元发生变化，更新索引，ito信息
    /// 没有按照材质 继续细分: 会有大量的mesh 重复
    /// mesh vbo 与工厂对应
    /// 工厂: 属性(mesh) + 绘制类型+ 图元类型
    /// </summary>
    

    /// <summary>
    /// 节点更新，局部更新最大值，如果同时要更新的对象超过了 MAX_LOCAL_UPDATE值，则全部更新
    /// </summary>
    constexpr   uint    MAX_LOCAL_UPDATE    =   64;
    class   FE_API  FEFactoryRender :public FEFactory
    {
    public:
        struct  NodesPrimitives
        {
            /// <summary>
            /// 记录工厂类型对应的节点数量
            /// </summary>
            Nodes       _nodes;
            /// <summary>
            /// 一个mesh有多个primitive,类型可能是一样，记录 factory 与 Primitives 对应关系
            /// </summary>
            Primitives  _pris;
        };
    public:
        using   RFactory            =   SharedPtr<FEFactoryRender>;
        using   RFactorys           =   std::vector<RFactory>;
        using   MeshKeyMap          =   std::map<uint64,NodesPrimitives>;
        using   MeshKeyCount        =   std::map<uint64,uint>;

        using   BufferCopy          =   FECmdBuffer::BufferCopy;
        using   BufferCopys         =   FECmdBuffer::BufferCopys;
        using   BufferCopyss        =   FECmdBuffer::BufferCopyss;
    public:
        struct  Counts
        {
            uint32  _vboBytes   =   0;
            uint32  _iboBytes   =   0;
            uint32  _instBytes  =   0;
            uint32  _itoBytes   =   0;
        };
    public:
        struct  GroupNode :public FEObject
        {
        public:
            friend  class   FEFactoryRender;
        public:
            struct  NodeUpdate
            {
                Node    node;
                uint    offset;
            };
            using   NodeUpdates     =   std::vector<NodeUpdate>;
            /// <summary>
            /// 预构建的绘制项:构建缓冲时一次性生成,
            /// WGSL 路径每帧直接线性绘制,避免遍历节点×图元并查偏移表
            /// </summary>
            struct  DrawItem
            {
                uint    _firstIndex     =   0;
                uint    _count          =   0;
                int     _baseVertex     =   0;
                uint    _primCount      =   0;
                uint    _instance       =   0;
            };
            using   DrawItems       =   std::vector<DrawItem>;
        public:
            GroupNode(FEContext& ctx)
                :FEObject(ctx)
            {
                _updates.reserve(MAX_LOCAL_UPDATE);
            }
        protected:
            Material    _mat;
            Nodes       _objects;
            aabb3dr     _aabb;
            /// <summary>
            /// 相对于大数组中的偏移量,一个工厂有多个 GroupNode
            /// _start 记录当前groupNode,在大数组中的其实位置
            /// </summary>
            uint32      _start      =   0;
            /// <summary>
            /// 命令偏移量,一个节点有可能会产生多个cmd,一个mesh 对应多个primative
            /// </summary>
            uint32      _cmdOffset  =   0;
            /// <summary>
            /// 全部命令的总数
            /// </summary>
            uint        _cmdCount   =   0;
            /// <summary>
            /// 需要被更新的节点,每一帧会检测该数组中是否有数据
            /// 如果有数据同步更新GPU 数据后，清空
            /// 该数组大小是固定的,如果超过最大值,则全部更新
            /// </summary>
            NodeUpdates _updates;
            /// <summary>
            /// 裁剪所需要的材质
            /// </summary>
            Material    _cullMat;
            /// <summary>
            /// 预构建的绘制项(仅 WGSL 路径使用)
            /// </summary>
            DrawItems   _drawItems;
            /// <summary>
            /// 裁剪后命令数缓存:updateImpl 每帧失效,同一帧多趟渲染只做一次 GPU→CPU 回读
            /// </summary>
            uint32      _cullCount      =   0;
            bool        _cullValid      =   false;
        protected:
            void    setMaterial(Material mat)   {   _mat    =   mat;    }
            void    setStart(uint32 start)      {   _start  =   start;  }
            uint32  start() const               {   return  _start;     }
            uint    count() const               {   return  uint(_objects.size());}
           
            size_t  addNode(Node node);
            size_t  addNodes(Nodes::iterator nBegin, Nodes::iterator eEnd);
            size_t  addNodes(Nodes&  nodes);
            size_t  removeNode(Node node);
            size_t  removeNodes(Nodes& nodes);
            /// <summary>
            /// 修改intance
            /// </summary>
            /// <param name="node"></param>
            /// <returns></returns>
            size_t  nodePropChanged(Node node);
            /// <summary>
            /// 清除更新标记
            /// </summary>
            void    resetFlags();
            void    clearUpdates();
        
            /// <summary>
            /// 绘制命令数量,裁剪过后的命令数量
            /// </summary>
            /// <returns></returns>
            uint32  cullCount();
            /// <summary>
            /// 命令的起始偏移量(对象于整个总命令缓冲区)
            /// </summary>
            /// <returns></returns>
            uint32  cmdOffset() const
            {
                return  _cmdOffset;
            }
            /// <summary>
            /// 全部可用命令，没有被裁剪之前的数据
            /// </summary>
            /// <returns></returns>
            uint32  cmdCount() const
            {
                return  _cmdCount;
            }
            void    setCmdCount(uint32 cnt)
            {
                _cmdCount   =   cnt;
            }
            void    setCmdOffset(uint32 offset)
            {
                _cmdOffset  =   offset;
            }
            /// <summary>
            /// 检测是否按局部更新.
            /// 如果 _objects 数据量较少,且全部都需要更新,则使用全局更新
            /// </summary>
            /// <returns></returns>
            bool    isLocalUpdate() const
            {
                return  _updates.size() <= MAX_LOCAL_UPDATE && _updates.size() < _objects.size();
            }
            void    addUpdateNode(Node node,size_t offset);
            /// <summary>
            /// 收集组内局部更新数据
            /// </summary>
            /// <param name="slots">需要更新的槽数据</param>
            /// <param name="stride">一个元素大小</param>
            /// <param name="regions">输出更新区域信息</param>
            /// <param name="pStart">缓冲区首地址,用来计算拷贝偏移量使用</param>
            /// <param name="pDst">数据输出缓冲区地址</param>
            /// <returns>返回新的地址 = pDst + 拷贝的数据长度</returns>
            uint8*  copyPartial(const uints& slots,uint stride,BufferCopys& regions,const uint8*pStart,uint8* pDst);
            uint8*  copyFull(const uints& slots,uint startInst,uint8* pDst,uint removeFlagBits = 0);
            void    invalidateCullCount()   {   _cullValid  =   false;  }
        };
        using   Group       =   SharedPtr<GroupNode>;
        /// <summary>
        /// 按照材质排序
        /// </summary>
        using   Groups      =   std::vector<Group>;
    public:
        struct  VBind
        {
            uint32          _binding    =   0;
            uint32          _slots      =   0;
            VBOs            _vbos;
            BufferCopyss    _regions;
        };
        using   VBinds    =   std::vector<VBind>;
    public:
        FEFactoryRender(FEContext& ctx);

        FEFactoryRender(const FEFactoryRender& other);

        ~FEFactoryRender();

    public:
        /// <summary>
        /// 工厂的类型Id,标识该工厂中的节点类型,例如 mesh + primitive + material
        /// </summary>
        /// <returns></returns>
        inline  auto    key()   const
        {
           return   _key;
        }
        inline  void    setKey(MeshKey value)
        {
            _key = value;
        }
        inline  void    setKey(uint64 value)
        {
            _key.setValue(value);
        }
        /// <summary>
        /// 所有node的数量
        /// </summary>
        /// <returns></returns>
        inline  size_t  nodeCount() const
        {
            size_t      count   =   0;
            for (auto& var : _groupNode)
            {
                count   +=  var->_objects.size();
            }
            return  count;
        }
        /// <summary>
        /// 获取包围盒
        /// </summary>
        /// <returns></returns>
        inline  aabb3dr aabb() const
        {
            aabb3dr aabb;
            for (auto grp : _groupNode)
                aabb.merge(grp->_aabb);
            return  aabb;
        }
        /// <summary>
        /// 主要是清除更新标记
        /// </summary>
        inline  void    resetFlags()
        {
            for (auto& grp: _groupNode)
            {
                grp->resetFlags();
            }
        }
        const   auto&   usages() const
        {
            return  _usages;
        }
        inline  auto&   usages()
        {
            return  _usages;
        }
        /// <summary>
        /// 标志clear函数，如果 _resident == true 不做任何操作
        /// </summary>
        /// <param name="flag"></param>
        inline  void    setResident(bool flag)
        {
            _resident   =   flag;
        }
        inline  bool    isResident() const
        {
            return  _resident;
        }
        /// <summary>
        /// 获取或者创建裁剪所需的材质
        /// 并用 camera 参数填充材质数据
        /// </summary>
        Materials       getOrCreateCullMaterials(Camera camera);
        bool            supportGPUCull() const;
        void            setGPUCull(bool bFlag)
        {
            _gpuCull    =   bFlag;
        }

        Groups&         groups() 
        {
            return  _groupNode;
        }
        const Groups&   groups() const
        {
            return  _groupNode;
        }

        
    public:
        /// <summary>
        /// 添加节点:递归收集该节点及其带 mesh 的子节点,调用方只传顶层节点
        /// </summary>
        virtual size_t  addNode(Node  node);
        /// <summary>
        /// 批量添加:与 addNode 相同,对每个节点递归收集子节点
        /// </summary>
        virtual size_t  addNodes(Nodes&  nodes);
    public:
        virtual size_t  removeNode(Node node);
        /// <summary>
        /// 批量移除:不修改入参顺序(内部复制后按材质排序)
        /// </summary>
        virtual size_t  removeNodes(const Nodes& nodes);
        virtual size_t  nodePropChanged(Node node);
    public:
        virtual void    update(CMDPtr cmd);
        virtual void    render(CMDPtr cmd);
        /// <summary>
        /// ===== transmission 场景折射双趟过滤(全局趟状态,FEScene 每趟渲染前切换)=====
        ///   TP_ALL:    正常渲染(不透明组 + 透明组都画)
        ///   TP_OPAQUE: 仅不透明组(场景折射纹理预渲染趟,透明组跳过,
        ///              否则玻璃会出现在自身折射背景里并自折射)
        /// (枚举值带 TP_ 前缀:OPAQUE 与 wingdi.h 宏冲突)
        /// </summary>
        enum class  TransPass : uint { TP_ALL = 0, TP_OPAQUE };
        static  void        setTransPass(TransPass p);
        static  TransPass   transPass();
        /// <summary>
        /// 工厂内含任意透明材质组(alpha 混合/transmission)时返回 true,
        /// 渲染层据此把工厂排到不透明工厂之后绘制
        /// </summary>
        virtual bool    isTransparent() const;
        /// <summary>
        /// 工厂内是否存在 transmission 材质组(KHR_materials_transmission 因子>0),
        /// FEScene 据此决定是否需要先渲染不透明场景到 sceneTex 再做折射双趟
        /// </summary>
        bool            hasTransmissionMaterial() const;
        /// <summary>
        /// 销毁 GPU 数据:先释放工厂自身资源,再调用派生类 onDestroy()
        /// </summary>
        void            destroy();
        virtual void    clearFlagBits();
    protected:
        /// <summary>
        /// 派生类自有资源的释放扩展点,由 destroy() 内部调用(派生类无需再调基类)
        /// </summary>
        virtual void    onDestroy() {}

        virtual size_t  addNodesImpl(Nodes&  nodes);
        /// <summary>
        /// 不做子节点递归,直接把给定列表并入工厂(调用方须已完成收集)
        /// </summary>
        size_t          addNodesFlat(Nodes::iterator nBegin,Nodes::iterator nEnd);
        /// <summary>
        /// 批量移除已按材质排序的节点列表(供跨工厂移除时只排序一次)
        /// </summary>
        size_t          removeNodesSorted(const Nodes& nodes);
        /// <summary>
        /// 更新函数
        /// </summary>
        /// <param name="cmd"></param>
        virtual void    updateImpl(CMDPtr cmd);
        /// <summary>
        /// 节点关联的几何体mesh数据生成vbo /  vertex 非instance数据
        /// </summary>
        /// <returns></returns>
        virtual VBinds  buildVertexVBOs(MeshUSet& meshSet);
        virtual IBO     buildVertexIBO(MeshUSet& meshSet);
        /// <summary>
        /// 生成instance 数据
        /// </summary>
        /// <returns></returns>
        virtual VBinds  buildInstanceVBOs();
        virtual VBO     buildSphereVBO();
        /// <summary>
        /// 按需部分更新
        /// </summary>
        /// <param name="slots"></param>
        virtual void    updateInstanceLocal(uint slots);
        virtual void    updateInstanceBSphere();
        /// <summary>
        /// 统计待更新实例:局部更新数量 / 整组更新组数 / 整组实例数量
        /// </summary>
        bool    calcPendingInstances(uint32& localCnt,uint32& grpCnt,uint32& grpInstCnt);
        /// <summary>
        /// 把待更新实例写入暂存缓冲,并输出相对目标缓冲的拷贝区域
        /// </summary>
        void    fillInstanceUpdates(const uints& indexs,uint stride,uint localCnt,
                                   uint grpInstCnt,VBO staging,BufferCopys& regions,
                                   uint removeFlagBits);
        /// <summary>
        /// 新建暂存缓冲(不复用:实例更新频率低,缓存只会长期占用显存)
        /// </summary>
        VBO     acquireStaging(uint64 length);
        /// <summary>
        /// 收集并创建命令缓冲区
        /// </summary>
        /// <returns></returns>
        virtual ITO     buildIndirect(ITO&  gpuFull,ITO& gpuDraw);
        /// <summary>
        /// 根据材质获取节点所在的组
        /// </summary>
        /// <param name="mat"></param>
        /// <returns></returns>
        GroupNode*      queryGroup(Material mat)
        {
            auto    itr = std::lower_bound(_groupNode.begin(), _groupNode.end(), mat,[](const Group& left,const Material& right)
            {
                return  left->_mat.get() < right.get();
            });
            if (itr != _groupNode.end() && (*itr)->_mat == mat)
                return  *(itr);
            else
                return  nullptr;
        }
        /// <summary>
        /// 如果有，返回，没有插入
        /// </summary>
        /// <param name="mat"></param>
        /// <returns></returns>
        GroupNode*      queryOrCreateGroup(Material mat);
        Material        createCullMaterial(Group grp,Camera camera);
        void            updateCullMaterial(Group grp,Camera camera);
        /// <summary>
        /// 填充裁剪材质参数与绑定(创建/更新共用)
        /// </summary>
        void            fillCullParams(class FEMaterialCull* cull,Group grp,Camera camera);
        VBO     buildBuffer(MeshUSet& meshSet,FEInputSlot slot);
        VBO     buildBuffer(MeshUSet& meshSet,const InputDescs& inputs);
        /// <summary>
        /// CPU数据上传到GPU上 
        /// </summary>
        /// <param name="cpuBinds"></param>
        /// <param name="gpuBinds"></param>
        void    copyVBinds(VBinds& cpuBinds,VBinds& gpuBinds);
        void    copyVBindsRegions(VBinds& cpuBinds,VBinds& gpuBinds);
        void    copyBuffer(VBO src,VBO dsts,const BufferCopys& );

        VBO     boundSphereVBO()const;
        VBO     flagBitsVBO() const;
    public:
        /// <summary>
        /// 索引统计结果
        /// </summary>
        struct  IndexCounts
        {
            /// <summary>
            /// 索引元素总数,即绘制时可用的最大索引数量
            /// </summary>
            uint32  _indices    =   0;
            /// <summary>
            /// IBO 字节数,含逐图元 4 字节对齐填充
            /// </summary>
            uint32  _bytes      =   0;
        };
        /// <summary>
        /// 统计指定图元类型的索引数量与 IBO 字节数
        /// </summary>
        /// <param name="meshSet">参与统计的 mesh 集合</param>
        /// <param name="srcPri">图元类型</param>
        /// <returns>索引数量与所需 IBO 字节数</returns>
        static  IndexCounts indexCount(const MeshUSet& meshSet,EPrimitive srcPri);
        /// <summary>
        /// 收集有多少个node有mesh
        /// </summary>
        /// <param name="nodes"></param>
        /// <returns></returns>
        static  uint    countNode(const Node&  node);
        static  void    collectNode(const Node&  node,Nodes& nodes);
        /// <summary>
        /// 收集mesh,目的是过滤掉一样的mesh，然后生成大的缓冲区
        /// </summary>
        /// <param name="meshSet"></param>
        /// <returns>返回有多少给node带有mesh信息</returns>
        static  uint        collectMesh(MeshUSet& meshSet,const Groups& grps);
        /// <summary>
        /// 把节点(含其带 mesh 的子节点)按 mesh key 分派到渲染工厂。
        /// reuseExisting 为 true 时优先复用场景中同 key 的非常驻工厂,
        /// 避免同一 mesh key 重复建厂;天空盒/网格等常驻工厂的创建方须传 false
        /// </summary>
        /// <param name="ctx">上下文</param>
        /// <param name="scene">节点所属场景</param>
        /// <param name="nodes">要添加的顶层节点</param>
        /// <param name="reuseExisting">是否复用已有的非常驻工厂</param>
        /// <returns>本次涉及到的工厂列表(含被复用的工厂)</returns>
        static  RFactorys   addNodesToFactory(FEContext& ctx,FEScene& scene,const Nodes& nodes,
                                              bool reuseExisting = false);
        /// <summary>
        /// 把节点(含其带 mesh 的子节点)从场景所有渲染工厂中移除,
        /// 与 addNodesToFactory 相对应。工厂对象本身保留(可能仍驻留其它节点),
        /// 随场景销毁统一释放
        /// </summary>
        /// <param name="scene">节点所属场景</param>
        /// <param name="nodes">要移除的顶层节点</param>
        /// <returns>实际移除的节点-工厂引用次数</returns>
        static  size_t      removeNodesFromFactory(FEScene& scene,const Nodes& nodes);
        /// <summary>
        /// 该工厂是否包含某材质的渲染组(供场景侧按材质定位工厂,如编辑网格的显示/隐藏)
        /// </summary>
        bool    hasMaterialGroup(Material mat)
        {
            return  queryGroup(mat) != nullptr;
        }
    protected:
        FEDevice&       _device;
        MeshKey         _key;
        /// <summary>
        /// 节点按照材质进行分组
        /// </summary>
        Groups          _groupNode;
        VBinds          _vboVertexs;
        VBinds          _vboInstances;
        /// <summary>
        /// 所有mesh
        /// </summary>
        IBO             _ibo;
        /// <summary>
        /// 本工厂 IBO 内各 primitive 的索引偏移表(key = primitive 裸指针)。
        /// 共享 mesh 的同一 primitive 会被多个工厂各自拷入 IBO,
        /// primitive 对象上的 _indexOffset 会被最后一次构建覆盖,
        /// 每帧直接取对象值渲染的路径(WGSL)必须查本表
        /// </summary>
        std::map<const void*,uint>  _priOffsets;
        /// <summary>
        /// 本工厂 VBO 大数组内各 primitive 的顶点基址表(key = primitive 裸指针)。
        /// 与 _priOffsets 同理:primitive 对象上的 _vertexOffset 会被最后一次
        /// 构建该 mesh 的工厂覆盖,每帧直接取对象值渲染的路径(WGSL)必须查本表
        /// </summary>
        std::map<const void*,int>   _priVertexOffsets;
        /// <summary>
        /// 绘制命令
        /// </summary>
        ITO             _indirectClip;
        /// <summary>
        /// 命令数据，计算裁剪使用,裁剪后的命令存储到_indirectClip
        /// </summary>
        ITO             _indirectFull;
        /// <summary>
        /// 包围球，用来计算裁剪
        /// </summary>
        VBO             _vboSphere;
        Counts          _counts;
        /// <summary>
        /// 工厂默认归属的场景层。
        /// 注意:不能默认为 USAGE_Classic —— 它是 Background|Scene|GUI|Overlay 的并集,
        /// 是给 viewer 声明“启用哪些层”用的;工厂若继承它,会被每个启用层各绘制一遍
        /// (单趟渲染下即 4× 过度绘制,且透明物体被重复混合)。
        /// 需要其它图层的工厂(如 ImGui 工厂)应显式指定自己的 usages。
        /// </summary>
        ViewerUsages    _usages     =   ViewerUsage::USAGE_Scene;
        bool            _gpuCull    =   true;
        bool            _resident   =   false;
        /// <summary>
        /// transmission 场景折射双趟过滤全局状态(定义于 FEFactoryRender.cpp)
        /// </summary>
        static  TransPass   s_transPass;
    };
    using   RFactory            =   SharedPtr<FEFactoryRender>;
    using   RFactorys           =   std::vector<RFactory>;
    using   FactoryRender       =   RFactory;
    using   FactoryRenders      =   RFactorys;
    using   RFactoryMap         =   std::map<uint64,RFactory>;
}
