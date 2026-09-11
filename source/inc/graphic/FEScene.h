#pragma     once

#include    "../FEInput.hpp"
#include    "../FECamera.hpp"
#include    "../FETimestamp.hpp"
#include    "../FEApp.hpp"
#include    "../node/FENodeTree.hpp"
#include    "FEFactoryRender.hpp"
#include    "FEFrustumCull.h"
#include    "FEUpdateQueue.hpp"
#include    "FERenderSystem.h"
#include    "FESwapchain.h"
#include    "FEFrameBuffer.h"
#include    "FERenderPass.h"
#include    "FECmdPool.h"
#include    "FECmdBuffer.h"
#include    "FEGPUBuffer.h"
#include    "FEFence.h"
#include    "FESemaphore.h"
#include    "FEDSet.h"
#include    "FEDSetPool.h"
#include    "FEDSetLayout.h"
#include    "FEFactoryMgr.hpp"
#include    "FEViewerMgr.hpp"
#include    "FEComponentSysMgr.hpp"
#include    "FEFrame.h"

namespace   FE
{
    struct  DispatchResult
    {
        RFactorys   rFactorys;
    };

    class   FEInputSystem;
    class   FEScene 
        :public FEObject
        ,public FEInput
    {
    public:
        friend  class   FEContext;
        using   Frames          =   std::vector<Frame>;
    public:
        using   NotifyUpdate    =   std::function<void()>;   
        using   NotifyMap       =   std::map<const void*,NotifyUpdate>;
        using   FactoryRenderMgr=   FEKeyValues<uint64,FactoryRender>;
    public:
        FEScene(FEContext& ctx);

        FEScene(const FEScene& other);

        virtual ~FEScene();

        inline  auto&   updateQueue()
        {
            return  _updateQueue;
        }
        const   auto&   updateQueue() const
        {
            return  _updateQueue;
        }
        inline  auto    device()
        {
            return  _device;
        }

        inline  auto    camera() const
        {
            return  _camera;
        }
        inline  auto    gpuFrustCull() const
        {
            return  _frustCull;
        }
        inline  auto&   nodeTree()
        {
            return  _nodeTree;
        }
        inline  auto&   nodeTree() const
        {
            return  _nodeTree;
        }
        inline  auto&   factoryMgr()
        {
            return  _factorys;
        }
        inline  auto&   factoryMgr() const
        {
            return  _factorys;
        }
        inline  auto&   viewerMgr()
        {
            return  _viewerMgr;
        }
        inline  auto&   viewerMgr() const
        {
            return  _viewerMgr;
        }
        inline  Frame   currentFrame() const
        {
            return  _frame;
        }
        /// <summary>
        /// 获取input system 
        /// </summary>
        /// <returns></returns>
        FEInputSystem*  inputSystem() const;

        virtual bool    setup(App app,const FEUuid& rendererId   =   RS_VULKAN);
        virtual void    destroy();
        /// <summary>
        /// 节点加入到系统下，只是挂在节点上，并不做渲染
        /// </summary>
        virtual void    addNodesToTree(const Nodes& nodeList);
        
        /// <summary>
        /// 函数会把节点数据以及组件数据分发到对应的系统中
        /// </summary>
        /// <param name="nodeList"></param>
        /// <param name="result"></param>
        void            dispatchNodesToSystem(const Nodes& nodeList,DispatchResult* result = nullptr);
    public:
        virtual void    onFrameStart();
        virtual void    onFrameUpdate();
        virtual void    onFrameRender();
        virtual void    onFrameEnd();
        /// <summary>
        /// 消息处理
        /// </summary>
        /// <param name="msg"></param>
        virtual void    onMessage(const FEMessage& msg) override;
        /// <summary>
        /// 节点基本属性数据发生变化,位置，旋转，平移，颜色等
        /// </summary>
        /// <param name="node"></param>
        virtual void    onNodePropChanged(const FENode* node);
        /// <summary>
        /// 打开工程
        /// </summary>
        /// <param name="fepj"></param>
        /// <returns></returns>
        virtual bool    open(const char* fepj);
        /// <summary>
        /// 保存,如果fepj != nullptr,则直接保存fepj文件中
        /// 如果fepj == nullptr, 则保存到open函数指定的文件中
        /// 如果没有open,则用默认值 "unname.fepj"
        /// </summary>
        virtual bool    save(const char* fepj = nullptr);
        /// <summary>
        /// 自定义保存
        /// </summary>
        /// <param name="objects"></param>
        /// <param name="fileName"></param>
        /// <returns></returns>
        virtual bool    saveAs(const Objects& objects,const char* fileName);
        /// <summary>
        /// 清空场景中的数据，恢复到初始化状态
        /// </summary>
        virtual void    clear();
    protected:
        virtual void    onClose();
    protected:
        /// <summary>
        /// 节点加入到系统中,函数会把根绝规则把节点分配到不同的工厂中
        /// </summary>
        /// <param name="nodeList"></param>
        /// <returns></returns>
        void    addNodesToFactory(const Nodes& nodeList,DispatchResult* result);
        /// <summary>
        /// 默认加载管线
        /// </summary>
        void    loadPipelines();
        /// <summary>
        /// 初始化内置的数据
        /// </summary>
        void    initializeBuildin(FEDevice& device);
        void    initializeQueue();
        void    resize(const MsgResize& evt);

        Nodes   loadNode(Material mat);
        /// <summary>
        /// 创建网格
        /// </summary>
        /// <returns></returns>
        Node    createGrid();
        /// <summary>
        /// 创建工具
        /// </summary>
        void    createTools();
    protected:
        App                 _app;  
        FETimestamp         _timestamp;
        /// <summary>
        /// 节点树，通过节点树，可以获取系统的数据
        /// </summary>
        FENodeTree          _nodeTree;
        /// <summary>
        /// 管理所有渲染工厂
        /// </summary>
        FEFactoryMgr        _factorys;
        /// <summary>
        /// 管理所有viewer
        /// </summary>
        FEViewerMgr         _viewerMgr;
        /// <summary>
        /// 管理所有系统，例如动画系统，物理系统，脚本系统,粒子系统
        /// </summary>
        FEComponentSysMgr   _comSysMgr;
        FEUpdateQueue       _updateQueue;
        Camera              _camera;
        FrustumCull         _frustCull;
        RenderSys           _renderSys;
        Device              _device;
        Swapchain           _swapchain;
        CMDPool             _cmdPool;
        GImage              _imgDepth   =   nullptr;
        GImgView            _depthView  =   nullptr;
        Frame               _frame;
        Node                _mousePoint;
        aabb3dr             _aabb;
        String              _projectFileName;
    public:
        /// <summary>
        /// 实现组件到系统见系统的分发功能
        /// 函数收集指定类型的组件，然后分发到对应的组件系统
        /// 典型应用场景: addToSystem<FEAnimation>(...);
        /// </summary>
        /// <typeparam name="TObject">类型，例如FEAnimation </typeparam>
        /// <param name="sysMgr">组件系统管理</param>
        /// <param name="nodes">节点对象集合</param>
        /// <returns>返回加到系统中的组件数</returns>
        template<typename TObject>
        static  size_t  dispatchToSystem(FEComponentSysMgr& sysMgr,const Nodes&  nodes)
        {
            uint        nCount  =   FEComponentSys::countObjects<TObject>(nodes);
            if (nCount == 0)
                return  0;
            Components  coms;
            coms.reserve(nCount);
            FEComponentSys::collectObjects<TObject>(nodes,coms);

            Component   com     =   coms.front();
            CLSVar      prop;
            bool        result  =   com->property().query(ComSysId,prop);
            assert(result);
            if (!result)
                return  0;
            assert(std::holds_alternative<FEUuid>(prop));

            if (!std::holds_alternative<FEUuid>(prop))
                return  0;
            FEUuid      comSysId    =   std::get<FEUuid>(prop);
            auto        comSys      =   sysMgr.query(comSysId);
            assert(comSys != nullptr);
            if (comSys == nullptr)
                return  0;
            else
                return  comSys->addObjects(coms);
        }
    };

    using   Scene =   SharedPtr<FEScene>;
}
