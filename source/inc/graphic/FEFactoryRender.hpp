#pragma     once

#include    "../FEPriority.hpp"
#include    "../FEKeyValues.hpp"
#include    "../FEFactory.hpp"
#include    "../node/FENode.hpp"
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
    class   FEFactoryRender :public FEFactory
    {
    public:
        using   RFactory            =   SharedPtr<FEFactoryRender>;
        using   RFactorys           =   std::vector<RFactory>;
        using   MeshKeyMap          =   std::map<uint64,Nodes>;
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
            GroupNode(FEContext& ctx)
                :FEObject(ctx)
            {}
        public:
            Material    _mat;
            Nodes       _objects;
            aabb3dr     _aabb;
            uint32      _start  =   0;
        public:
            void    setMaterial(Material mat)   {   _mat    =   mat;    }
            void    setStart(uint32 start)      {   _start  =   start;  }
            uint32  start() const               {   return  _start;     }
            uint32  count()  const              {   return  uint32(_objects.size());}
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
            /// 修改顶点
            /// </summary>
            /// <param name="node"></param>
            /// <returns></returns>
            size_t  nodeMeshChanged(Node node);
            /// <summary>
            /// 清除更新标记
            /// </summary>
            void    resetFlags();
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
            VBOs            _vbos;
            BufferCopyss    _regions;
        };
        using   VBinds    =   std::vector<VBind>;
    public:
        FEFactoryRender(FEContext& ctx,FEDevice& device)
            :FEFactory(ctx)
            ,_device(device)
        {}
        FEFactoryRender(const FEFactoryRender& other)
            :FEFactory(other)
            ,_device(other._device)
        {}
    public:
        inline  uint64  key()   const
        {
           return   _key.key();
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
    public:
        virtual size_t  addNode(Node  node);
        virtual size_t  addNodes(Nodes&  nodes);
        virtual size_t  addNodes(Nodes::iterator nBegin,Nodes::iterator nEnd);
    public:
        virtual size_t  removeNode(Node node);
        virtual size_t  removeNodes(Nodes& nodes);
        virtual size_t  nodePropChanged(Node node);
    public:
        virtual void    update(CMDPtr cmd);
        virtual void    render(CMDPtr cmd);
        /// <summary>
        /// 销毁前一定调用该函数，解除数据对工厂的引用计数
        /// 才能正确的释放数据
        /// </summary>
        virtual void    destroy();
    protected:
        virtual size_t  addNodesImpl(Nodes&  nodes);
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
        virtual void    updateInstanceVBOs();
        /// <summary>
        /// 收集并创建命令缓冲区
        /// </summary>
        /// <returns></returns>
        virtual ITO     buildIndirect();
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
        virtual VBO     buildBuffer(MeshUSet& meshSet,FEInputSlot slot);
        virtual VBO     buildBuffer(MeshUSet& meshSet,const uints& indexs);
        /// <summary>
        /// CPU数据上传到GPU上 
        /// </summary>
        /// <param name="cpuBinds"></param>
        /// <param name="gpuBinds"></param>
        virtual void    copyVBinds(VBinds& cpuBinds,VBinds& gpuBinds);
        virtual void    copyVBindsRegions(VBinds& cpuBinds,VBinds& gpuBinds);
    public:
        static  uint2   indexCount(MeshUSet& meshSet,EPrimitive srcPri);
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
        static  RFactorys   addNodesToFactory(FEContext& ctx,FEScene& scene,const Nodes& nodes);
    protected:
        FEDevice&       _device;
        MeshKey         _key;
        /// <summary>
        /// 节点按照材质进行分组
        /// </summary>
        Groups          _groupNode;
        VBinds          _vboVertexs;
        VBinds          _vboInstances;
        IBO             _ibo;
        ITO             _indirect;
        Counts          _counts;
        ViewerUsages    _usages =   ViewerUsage::USAGE_Classic;
    };
    using   RFactory            =   SharedPtr<FEFactoryRender>;
    using   RFactorys           =   std::vector<RFactory>;
    using   FactoryRender       =   RFactory;
    using   FactoryRenders      =   RFactorys;
    using   RFactoryMap         =   std::map<uint64,RFactory>;

}
