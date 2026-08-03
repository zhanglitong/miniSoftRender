#pragma     once

#include    "../FEInput.hpp"
#include    "../FECamera.hpp"
#include    "../FETimestamp.hpp"
#include    "../FEApp.hpp"
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

namespace   FE
{
    class   FEScene 
        :public FEObject
        ,public FEInput
    {
    public:
        friend  class   FEContext;
    public:
        struct  Frame
        {
            CMDPtr      _cmd;
            Fence       _fenceWait;
            Semaphore   _semRenderComplete;
            Semaphore   _semPresentComplete;
        };
        
        using   Frames          =   std::vector<Frame>;
    public:
        using   NotifyUpdate    =   std::function<void()>;   
        using   NotifyMap       =   std::map<const void*,NotifyUpdate>;
        using   FactoryRenderMgr=   FEKeyValues<uint64,FactoryRender>;
    public:
        FEScene(FEContext& ctx)
            :FEObject(ctx)
        {}
        FEScene(const FEScene& other)
            :FEObject(other)
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
        /// 获取当前帧索引
        /// </summary>
        /// <returns></returns>
        uint    currentFrame() const;
        /// <summary>
        /// 切换到下一帧
        /// </summary>
        /// <returns></returns>
        uint    nextFrame();
        /// <summary>
        /// 初始化内置的数据
        /// </summary>
        void    initializeBuildin(FEDevice& device);
        void    initializeQueue();
        void    resize(const uint2& size);

        Nodes   loadNode(Material mat);
        Node    createGrid(Material mat);
    protected:
        App                 _app;   
        /// <summary>
        /// 目前系统有两个队列，前台与后台
        /// </summary>
        uint                _curIndex   =   0;
        /// <summary>
        /// fbo的索引，fbo的数量可能多于对列数量
        /// </summary>
        uint                _curImgIdx  =   0;
        FEUpdateQueue       _updateQueue;
        Camera              _camera;
        RenderSys           _renderSys;
        Device              _device;
        Swapchain           _swapchain;
        RenderPass          _renderPass;
        CMDPool             _cmdPool;
        Frames              _frames;
        FrameBuffers        _frameBuffers;
        GImage              _imgDepth   =   nullptr;
        GImgView            _depthView  =   nullptr;
        /// <summary>
        /// 根节点
        /// </summary>
        Nodes               _roots;
        Node                _mousePoint;
        Node                _pickupNode;
        aabb3dr             _aabb;
        RFactoryMap         _factoryMap; 
        RFactorys           _factorys;
        real3               _pickPoint;
        int2                _ptMouse;
        bool                _lbtnDown   =   false;
        bool                _rbtnDown   =   false;
    };

    using   Scene =   SharedPtr<FEScene>;
}