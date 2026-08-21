#pragma     once

#include    "../FEInput.hpp"
#include    "../FECamera.hpp"
#include    "../FETimestamp.hpp"
#include    "../FEApp.hpp"
#include    "../node/FENodeTree.hpp"
#include    "FEFactoryRender.hpp"
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
#include    "../animation/FEActionGroup.hpp"

namespace   FE
{
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
        FEScene(FEContext& ctx)
            :FEObject(ctx)
            ,_nodeTree(ctx)
            ,_factorys(ctx)
            ,_viewerMgr(ctx)
            ,_comSysMgr(ctx)
            ,_actionGrp(ctx)
        {
        }
        FEScene(const FEScene& other)
            :FEObject(other)
            ,_nodeTree(other._nodeTree)
            ,_factorys(other._factorys)
            ,_viewerMgr(other._viewerMgr)
            ,_comSysMgr(other._comSysMgr)
            ,_actionGrp(other._actionGrp)
        {}
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
        virtual bool    setup(App app);
        virtual void    test();
        /// <summary>
        /// 节点加入到系统下，只是挂在节点上，并不做渲染
        /// </summary>
        virtual void    addNodesToTree(const Nodes& nodeList);
        /// <summary>
        /// 节点加入到系统中,函数会把根绝规则把节点分配到不同的工厂中
        /// </summary>
        /// <param name="nodeList"></param>
        /// <returns></returns>
        RFactorys       addNodesToFactory(const Nodes& nodeList);
        /// <summary>
        /// 
        /// </summary>
        /// <param name="nodeList"></param>
        void            addNodesToSystem(const Nodes& nodeList);
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
        virtual void    onNodePropChanged(FENode* node);
    protected:
        virtual void    onClose();
    protected:
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
        Node    createGrid(Material mat);
    protected:
        App                 _app;  
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
        FEActionGroup       _actionGrp;
        FEUpdateQueue       _updateQueue;
        Camera              _camera;
        
        RenderSys           _renderSys;
        Device              _device;
        Swapchain           _swapchain;
        CMDPool             _cmdPool;
        GImage              _imgDepth   =   nullptr;
        GImgView            _depthView  =   nullptr;
        Frame               _frame;
        Node                _mousePoint;
        aabb3dr             _aabb;
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
        static  size_t  addToSystem(FEComponentSysMgr& sysMgr,const Nodes&  nodes)
        {
            uint        nCount  =   FEComponentSys::countObjects<TObject>(nodes);
            if (nCount == 0)
                return  0;
            Components  coms;
            coms.reserve(nCount);
            FEComponentSys::collectObjects<FEAnimation>(nodes,coms);

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
