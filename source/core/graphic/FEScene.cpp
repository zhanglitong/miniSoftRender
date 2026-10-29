
#include    "../inc/FELog.hpp"
#include    "../inc/FEPickup.hpp"
#include    "../inc/FEFileSystem.hpp"
#include    "../inc/FEImageUsage.hpp"
#include    "../inc/FEImageType.hpp"
#include    "../inc/geometry/FEGeometryGrid.hpp"
#include    "../inc/material/FEMaterialLibrary.hpp"
#include    "../inc/graphic/FEScene.h"
#include    "../inc/graphic/FEConstUuid.h"
#include    "../inc/graphic/FEShaderDefine.h"
#include    "../inc/graphic/FEPipelineHelper.h"
#include    "../inc/graphic/FELightMgr.h"
#include    "../inc/animation/FEAnimationSys.hpp"
#include    "../inc/constraint/FEConstraintSys.hpp"
#include    "../inc/constraint/FEConstraintTrans.hpp"
#include    "../inc/constraint/FEConstraintLookAt.hpp"
#include    "../inc/FEInputSystem.hpp"
#include    "../inc/FEFileFormatHelper.hpp"
#include    "../inc/fileFormat/fepk/FEFormatFepj.hpp"
#include    "../inc/graphic/FEFactoryAxisMove.hpp"
#include    "../inc/graphic/FEFactoryAxisRotate.hpp"
#include    "../inc/graphic/FEFactoryAxisScale.hpp"
#include    "../inc/graphic/FESceneBrowse.h"
#include    "../inc/component/FESkyComponent.hpp"
#include    "FEOffscreenSwapchain.h"

namespace   FE
{
    FEScene::FEScene(FEContext& ctx)
        :FEObject(ctx)
        ,_nodeTree(ctx)
        ,_factorys(ctx)
        ,_viewerMgr(ctx)
        ,_comSysMgr(ctx)
    {
        _nodeTree.eventsChangedNode()   +=  {this,[this](const FENode* node)
        {
            onNodePropChanged(node);
        }};
    }
    FEScene::FEScene(const FEScene& other)
        :FEObject(other)
        ,_nodeTree(other._nodeTree)
        ,_factorys(other._factorys)
        ,_viewerMgr(other._viewerMgr)
        ,_comSysMgr(other._comSysMgr)
    {}
    FEScene::~FEScene()
    {
        destroy();
    }

    /// <summary>
    /// 获取input system 
    /// </summary>
    /// <returns></returns>
    FEInputSystem*  FEScene::inputSystem() const
    {
        auto    sys =   _comSysMgr.query(UUIDOF(FEInputSystem));
        if (sys)
            return  sys->as<FEInputSystem>();
        else
            return  nullptr;
    }
    FEAnimationSys* FEScene::animationSystem() const
    {
        auto    sys =   _comSysMgr.query(UUIDOF(FEAnimationSys));
        if (sys)
            return  sys->as<FEAnimationSys>();
        else
            return  nullptr;
    }
    FEConstraintSys* FEScene::constraintSystem() const
    {
        auto    sys =   _comSysMgr.query(UUIDOF(FEConstraintSys));
        if (sys)
            return  sys->as<FEConstraintSys>();
        else
            return  nullptr;
    }
    FESkyComponent* FEScene::sky() const
    {
        /// as 的 const 重载返回 const 指针,而调用方需要调整天空参数,故移除 const
        return  (_sky != nullptr)
                ?   const_cast<FESkyComponent*>(_sky->as<FESkyComponent>())
                :   nullptr;
    }

    bool    FEScene::setup(App app,const FEUuid& rendererId)
    {
        _app    =   app;
        _size   =   uint2(uint(app->cInfo()._width),uint(app->cInfo()._height));
        return  initialize(rendererId);
    }
    bool    FEScene::setupHeadless(uint width,uint height,const FEUuid& rendererId)
    {
        _app    =   nullptr;
        _size   =   uint2(width,height);
        return  initialize(rendererId);
    }
    bool    FEScene::initialize(const FEUuid& rendererId)
    {
        FETimestamp     timestamp;
        LOG_INF("FE::FEAppHelper::create cost %lf",timestamp.milliSec());
        timestamp.update();
        _rendererId =   rendererId;
        _renderSys  =   FERenderSystem::create(_ctx,rendererId);
        assert(_renderSys != nullptr);
        if (_renderSys == nullptr)
            return  false;
        auto    result  =   _renderSys->create();
        auto    gpuList =   _renderSys->gpuList();

        for (auto& gpu : gpuList)
        {
            auto    gpuId   =   gpu.gpuId.toString();
            _ctx.log().infor("gpu.id    =   %s",gpuId.c_str());
            _ctx.log().infor("gpu.name  =   %s",gpu.name.c_str());
            _ctx.log().infor("gpu.type  =   %s",FERenderSystem::nameOf(gpu.type));
            _ctx.log().infor("gpu.api   =   %d",gpu.apiVersion);
        }
        if (gpuList.empty())
        {
            LOG_INF("systetm gpu not founded!");
            return  false;
        }
        _device         =   _renderSys->createDevice();
        assert(_device != nullptr);
        if (_device == nullptr)
        {
            LOG_INF("_renderSys->createDevice() failed!");
            return  false;
        }

        _ctx._device    =   _device;
        _ctx._scene     =   this;
        /// 
        _device->_createNotify  =   [&](FEDevice& device)
        {
            initializeBuildin(device);
        };
        {
            FEDevice::CreateInfo    infor   =   {};
            infor.deviceId  =   gpuList[0].gpuId;
            _device->create(infor);
        }
        /// 多采样能力钳制:必须在 resize/loadPipelines 之前确定,运行期不支持切换
        {
            if (_sampleCount != 1 && _sampleCount != 2 && _sampleCount != 4
                &&  _sampleCount != 8 && _sampleCount != 16)
                _sampleCount =   1;
            uint    maxCnt  =   _device->maxSampleCount();
            if (maxCnt < 1)
                maxCnt  =   1;
            if (_sampleCount > maxCnt)
                _sampleCount =   maxCnt;
            /// WebGPU 仅支持 1x/4x
            if (_rendererId == RS_WEBGPU && _sampleCount > 1)
                _sampleCount =   4;
        }
        {
            _cmdPool    =   _device->graphicCmdPool();
        }
        /// 默认相机: 位于 +Z 侧, 看向原点(-Z 方向), up=+Y, 保证右手坐标系 Z 向上
        _camera     =   new FECamera(_ctx,real3(0,0,0),real3(0,0,2.5),real3(1,0,0));
        _camera->setUp(real3(0,1,0));
        _camera->perspective(DEG2RAD(45), float(_size.x) / float(_size.y), 0.05f, 256.0f);
        _camera->update();
        /// 
        auto    light   =   new FELightDir(_ctx);
        /// 
        _device->lights().add("main",light);
        /// 
        resize(MsgResize(_size));
        /// 
        initializeQueue();
        /// 
        loadPipelines();
        /// 注册动画系统
        {
            AnimSys animSys =   new FEAnimationSys(_ctx);
            _comSysMgr.addObject(animSys.get());
            /// 注册约束系统(更新顺序位于动画之后,求解基于动画结果)
            ComponentSys    constSys  =   new FEConstraintSys(_ctx);
            _comSysMgr.addObject(constSys.get());
            /// 注册输入事件系统
            InputSys inputSys = new FEInputSystem(_ctx);
            _comSysMgr.addObject(inputSys.get());
        }

        LOG_EVT("start cost:%lf ms",timestamp.milliSec());

        {
            Node        node    =   new FENode(_ctx);
            Mesh        mesh    =   FEMeshBuilder::makePointMesh(_ctx,{float3(0,0,0)},{Rgba8(uint8x4(255,0,0,255))});
            node->setMesh(mesh);

            Material    mat     =   new FEMaterialPoint(_ctx);
            node->setMaterial(mat);
            _mousePoint         =   node;
            dispatchNodesToSystem({node});
        }
        {
            auto    viewer = new FEViewer(_ctx,_camera,nullptr,ViewerUsage::USAGE_Classic);
            _viewerMgr.addObject(viewer);
            _viewerMgr.setActiveViewer(viewer);
        }

        _frustCull  =   new FEFrustumCull(_ctx);
        /// anchor
        /// anchor 增加通知
        _ctx.anchor().notify()  +=  {[this](Object object)
        {
            UNUSED(object);
            if (_mousePoint)
            {
                _mousePoint->setLocalTranslation(_ctx.anchor().point());
                _mousePoint->update();
                _mousePoint->fireChanged();
            }
        }};
        /// 创建默认天空盒(默认启动:天空节点自动加入渲染工厂,太阳同步主方向光)
        createSky();
        /// 创建网格
        _gridNode   =   createGrid();
        /// 创建工具
        createTools();
        return  true;
    }
    void    FEScene::destroy()
    {
        LOG_INF("FEScene.onClose()");
        if (_device)
        {
            _device->waitIdle();
        }
        _swapchain  =   nullptr;
        _updateQueue.queue().clear();

        _mousePoint =   nullptr;
        _gridNode   =   nullptr;
        /// 默认天空:Node 持有 Component,而 Component::_owner 又强引用 Node,构成引用环;
        /// 先解绑组件并清空节点上的网格/材质,保证天空资源在 device 销毁前归还
        if  (_skyNode != nullptr)
            _skyNode->clearComponent();
        _skyNode    =   nullptr;
        _sky        =   nullptr;

        _cmdPool    =   nullptr;
        _frame      =   nullptr;
        _imgDepth   =   nullptr;
        _depthView  =   nullptr;
        _imgColorMSAA   =   nullptr;
        _msaaColorView  =   nullptr;
        _sceneTex   =   nullptr;
        _sceneTexView = nullptr;
        _sceneTexColorView = nullptr;

        for (auto  var : _factorys.objects())
        {
            var->destroy();
        }
        _factorys.clearObjects();

        _nodeTree.clear();

        if (_device)
        {
            _device->destroy();
        }
        _ctx._device    =   nullptr;
        _device         =   nullptr;
        _renderSys      =   nullptr;
    }

    void    FEScene::addNodesToTree(const Nodes& nodeList)
    {
        for (auto node : nodeList)
        {
            if (node->parent() != nullptr)
                continue;
            else
                _nodeTree.addToplevelNode(node);
        }
    }
   
    void    FEScene::removeNodesFromTree(const Nodes& nodeList)
    {
        for (auto node : nodeList)
        {
            /// 只摘顶层节点:带父节点的节点由父级一起移除,与 addNodesToTree 的规则对应
            if (node == nullptr || node->parent() != nullptr)
                continue;
            else
                _nodeTree.removeNode(node);
        }
    }
   
    void    FEScene::dispatchNodesToSystem(const Nodes& nodeList,DispatchResult* result)
    {
        addNodesToFactory(nodeList,result);
        dispatchToSystem<FEAnimation>(_comSysMgr,nodeList);
        dispatchToSystem<FEConstraint>(_comSysMgr,nodeList);
        dispatchToSystem<FEInput>(_comSysMgr,nodeList);
        /// transmission 场景折射纹理由 PBR 材质渲染时自取自用(见 FEMaterialPBR::refreshSceneTexture)
    }

    void    FEScene::removeNodesFromSystem(const Nodes& nodeList)
    {
        FEFactoryRender::removeNodesFromFactory(*this,nodeList);
        removeFromSystem<FEAnimation>(_comSysMgr,nodeList);
        removeFromSystem<FEConstraint>(_comSysMgr,nodeList);
        removeFromSystem<FEInput>(_comSysMgr,nodeList);
    }

    void    FEScene::onFrameStart()
    {
        if (_swapchain == nullptr)
            return;
        _frame  =   _swapchain->acquireNextFrame(UINT64_MAX);
        _ctx.setDeltaTime(_timestamp.second());
        _timestamp.update();

        if (_frame == nullptr)
            return;
        /// 帧序号递增:材质按帧节流查询场景折射纹理(见 FEMaterialPBR::refreshSceneTexture)
        _ctx.nextFrame();
        /// 无论 cmd 是否存在, 都要先等待并重置 fence,
        /// 防止上一帧的信号量/fence 处于 pending 状态导致后续 acquire 失败
        _frame->reset();
        if (_frame->_cmd == nullptr)
        {
            _frame->_cmd    =   _cmdPool->createCmd();
        }
        if (_frame->_cmd)
        {
            _frame->_cmd->reset();
            _frame->_cmd->begin();
        }

    }
    bool    FEScene::anyTransmissionMaterial()
    {
        for (auto& fac : _factorys.objects())
        {
            if (fac && fac->hasTransmissionMaterial())
                return  true;
        }
        return  false;
    }
    void    FEScene::onFrameUpdate()
    {
        if (_frame == nullptr || _frame->_cmd == nullptr)
            return;
        /// all componentSys for update
        /// 复制一份
        auto    comSyss     =   _comSysMgr.objects();
        /// 按照优先级排序
        std::sort(comSyss.begin(),comSyss.end(),[](const ComponentSys& left,const ComponentSys& right)
        {
            auto    prioLeft    =   left->priority(FEFactory::PT_Update);
            auto    prioRight   =   right->priority(FEFactory::PT_Update);
            if(prioLeft.priority() == prioRight.priority())
                return  prioLeft.order() < prioRight.order();
            else
                return  prioLeft.priority() <  prioRight.priority();
        });
        
        for (auto& var : comSyss)
        {
            var->update(_ctx.deltaTime());
            /// 组件更新过程中收集的脏节点由各系统自己显式刷新
            var->updateObjectList();
        }

        /// 渲染工厂
        auto    factorys    =   _factorys.objects();
        /// 按照优先级排序
        std::sort(factorys.begin(),factorys.end(),[](const FactoryRender& left,const FactoryRender& right)
        {
            auto    prioLeft    =   left->priority(FEFactory::PT_Update);
            auto    prioRight   =   right->priority(FEFactory::PT_Update);
            if(prioLeft.priority() == prioRight.priority())
                return  prioLeft.order() < prioRight.order();
            else
                return  prioLeft.priority() <  prioRight.priority();
        });
        /// 更新
        aabb3dr aabb;
        for (auto& var : factorys)
        {
            var->update(_frame->_cmd);
            aabb.merge(var->aabb());
        }
        auto    diff    =   _camera->getEye() - aabb.center();
        auto    vSize   =   (std::max)(aabb.getSize().x,aabb.getSize().y);
                vSize   =   (std::max)(vSize,aabb.getSize().z);
        auto    length  =   FE::length(diff) + vSize;
        _camera->setFar(length);
        _camera->update();

        _updateQueue.update(_frame->_cmd);

        for (auto viewer : _viewerMgr.objects())
        {
            FEFramInfo  info    =  {0,_frame->_cmd};
            viewer->onMessage(MsgUpdate(info));
        }

    }
    void    FEScene::onFrameRender()
    {
        if (_frame == nullptr)
            return;
        uint    width       =   _size.x;
        uint    height      =   _size.y;
        /// 多采样时渲染到 MSAA 颜色目标,结束时解析到交换链/离屏颜色目标
        GImgView    colorTarget =   _msaaColorView ? _msaaColorView : _frame->_imageViewer;
        FECmdBuffer::RenderInfo  rsInfo  =   {};
        rsInfo._depth       =   _depthView;
        rsInfo._colors      =   {colorTarget};
        if (_msaaColorView)
            rsInfo._resolveColors   =   {_frame->_imageViewer};
        rsInfo._clearColor  =   _clearColor;
        rsInfo._rect.set(0,0,width,height);

        auto    cmd = _frame->_cmd;

        if (_oitManager)
        {
            // ===== OIT: 三趟渲染 =====

            // 1. 每帧清零 OIT SSBO
            _oitManager->resetFrame(cmd);
            _oitManager->barrierToGeometry(cmd);

            // 2. 主趟：不透明物体（清 color + depth，写入不透明深度供几何趟遮挡测试）
            FEOITManager::setPass(FEOITManager::OITPass::MAIN);
            cmd->beginRender(rsInfo);
            for (auto viewer : _viewerMgr.objects())
            {
                FEFramInfo  info = {0, cmd};
                viewer->onMessage(MsgRender(info));
            }
            cmd->endRender(rsInfo);

            // 3. 几何趟：透明物体收集进链表（无 color 输出，depth LOAD 保留不透明深度）
            FEOITManager::setPass(FEOITManager::OITPass::LIST);
            FECmdBuffer::RenderInfo geoInfo = rsInfo;
            geoInfo._skipColor  =   true;
            geoInfo._loadDepth  =   true;   // depthTest ON / depthWrite OFF，测不透明深度
            cmd->beginRender(geoInfo);
            for (auto viewer : _viewerMgr.objects())
            {
                FEFramInfo  info = {0, cmd};
                viewer->onMessage(MsgRender(info));
            }
            cmd->endRender(geoInfo);
            FEOITManager::setPass(FEOITManager::OITPass::NONE);

            // 4. Barrier：Geometry Pass 写完 SSBO → Color Pass 读取
            _oitManager->barrierToColor(cmd);

            // 5. 合成趟：color LOAD（保留主趟画面），画全屏三角形从链表合成透明层
            FECmdBuffer::RenderInfo compInfo    =   rsInfo;
            compInfo._loadColor =   true;
            compInfo._skipDepth =   true;   // 合成不做深度测试
            cmd->beginRender(compInfo);

            auto colorPipeline = _oitManager->colorPipeline();
            auto colorDSets    = _oitManager->colorDSets();
            if (colorPipeline && !colorDSets.empty())
            {
                cmd->bindPipeline(colorPipeline);
                FECmdBuffer::DSetBind dBind;
                dBind.plBindPoint   = PL_GRAPIC;
                dBind.plLayout      = colorPipeline->nativeLayout();
                dBind.firstSet      = 0;
                dBind.dSets         = colorDSets;
                cmd->bindDescriptors(dBind);
                cmd->draw(0, 3, 0, 1);   // 全屏三角形（顶点在 VS 里展开）
            }

            cmd->endRender(compInfo);
        }
        else if (_sceneTexView && anyTransmissionMaterial())
        {
            // ===== transmission 场景折射:双趟渲染 =====
            // pass1: 仅不透明场景 → sceneTex(工厂 TP_OPAQUE 趟过滤掉透明组)
            FEFactoryRender::setTransPass(FEFactoryRender::TransPass::TP_OPAQUE);
            FECmdBuffer::RenderInfo p1Info  =   rsInfo;
            /// 多采样:深度也是多采样,pass1 必须同样渲染到 MSAA 目标后再解析到 sceneTex
            if (_msaaColorView)
                p1Info._resolveColors   =   {_sceneTexColorView ? _sceneTexColorView : _sceneTexView};
            else
                p1Info._colors          =   {_sceneTexColorView ? _sceneTexColorView : _sceneTexView};
            cmd->beginRender(p1Info);
            for (auto viewer : _viewerMgr.objects())
            {
                FEFramInfo  info = {0, cmd};
                viewer->onMessage(MsgRender(info));
            }
            /// endRender 把 sceneTex 从 COLOR_ATTACHMENT 转入 SHADER_READ_ONLY
            /// (VK 端按 usage 自动选择,见 VKCmdBuffer::endRender)
            cmd->endRender(p1Info);
            /// 生成 sceneTex mip 链(mip0 之外的级别,供粗糙透射按 lod 采样)
            if (_sceneTex && _sceneTex->mips() > 1)
                cmd->generateMips(_sceneTexView);

            // pass2: 全量渲染 → swapchain(透明材质在 shader 中采样 sceneTex 折射)
            FEFactoryRender::setTransPass(FEFactoryRender::TransPass::TP_ALL);
            cmd->beginRender(rsInfo);
            for (auto viewer : _viewerMgr.objects())
            {
                FEFramInfo  info    =  {0, cmd};
                viewer->onMessage(MsgRender(info));
            }
            cmd->endRender(rsInfo);
        }
        else
        {
            // ===== 默认单趟渲染 =====
            cmd->beginRender(rsInfo);

            for (auto viewer : _viewerMgr.objects())
            {
                FEFramInfo  info    =  {0, cmd};
                viewer->onMessage(MsgRender(info));
            }
            cmd->endRender(rsInfo);
        }
        
    }
    void    FEScene::onFrameEnd()
    {
        if (_frame == nullptr || _frame->_cmd == nullptr)
            return;
        if (_frame && _frame->_cmd )
            _frame->_cmd->end();
        /// 获取所有渲染工厂
        auto&   factorys    =   _factorys.objects();
        /// 清除所有标记
        /// 不能在工厂中清除,一个节点可能被多个工厂引用
        for (auto var : factorys)
        {
            var->clearFlagBits();
        }
        auto    queue   =   _device->queueGraphic();
        FEQueue::SubmitInfo smInfo;
        smInfo._frame   =   _frame;
        queue->submit(1,&smInfo);
        FESwapchain::PresentInfo    info    =   {};
        info._frame     =   _frame;
        info._queue     =   queue;
        _swapchain->queuePresent(info);
    }

    void    FEScene::renderFrame()
    {
        onFrameStart();
        onFrameUpdate();
        onFrameRender();
        onFrameEnd();
        /// 无头/离线场景:等待 GPU 完成,保证随后回读的像素有效
        if (_device)
            _device->waitIdle();
    }

    bool    FEScene::readPixels(void* dst,uint64 size)
    {
        if (_device == nullptr || _swapchain == nullptr)
            return  false;
        GImage  image   =   _swapchain->colorImage();
        if (image == nullptr)
            return  false;
        return  _device->readbackImage(image,dst,size);
    }

    void    FEScene::resize(uint width,uint height)
    {
        resize(MsgResize(uint2(width,height)));
    }

    void    FEScene::onMessage(const FEMessage& msgIn)
    {
        switch(msgIn.msgId())
        {
        case MSG_CLOSE  :
            destroy();
            break;
        case MSG_RESIZE :
            resize(static_cast<const MsgResize&>(msgIn));
            break;
        case MSG_LBUTTON_DOWN:
        case MSG_RBUTTON_DOWN:
        case MSG_LBUTTON_UP:
        case MSG_RBUTTON_UP:
        case MSG_MOUSE_WHEEL:
        case MSG_MOUSE_MOVE:
        case MSG_KEYDOWN:
        case MSG_KEYUP:
            if (_viewerMgr.activeViewer())
                _viewerMgr.activeViewer()->onMessage(msgIn);
            break;
        case MSG_UPDATE :
            onFrameStart();
            onFrameUpdate();
            break;
        case MSG_RENDER:
            onFrameRender();
            onFrameEnd();
            break;
        }
    }

    void    FEScene::onNodePropChanged(const FENode* node)
    {
        auto    mesh    =   node->mesh();
        if (mesh)
        {
            auto&   pris    =   mesh->primitives();
            for (auto& pri: pris)
            {
                auto    factory =   pri->factory();
                assert(factory != nullptr);
                if (factory == nullptr)
                    continue;
                Node    ptr         =   (FENode*)node;
                auto    rFactory    =   factory->as<FEFactoryRender>();
                rFactory->nodePropChanged(ptr);
            }
        }
    }
    bool    FEScene::open(const char* fepj)
    {
        if (fepj == nullptr)
        {
            LOG_ERR("open(nullptr).");
            return  false;
        }
        LOG_INF("FEScene.open(%s)",fepj);

        auto    fmtText =   FEFileFormat(".fepj","1.0.0.0","FE Buildin Format!");
        auto    reader  =   FEFileFormatHelper::queryReader(_ctx,fmtText);
        if (reader == nullptr)
        {
            LOG_ERR("No suitable file parser could be located to read the file.");
            return  false;
        }
        String  file    =   fepj;
        auto    objects =   reader->readFiles({file});

        if (objects.empty())
        {
            LOG_ERR("reader->readFiles(%s) return null.",fepj);
            return  false;
        }
        Nodes   nodes;
        for (auto var : objects)
        {   
            Node    node    =   var->cast<FENode>();
            if (node == nullptr)
                continue;
            node->flags().addFlag(FE::FLAG_UPDATE);
            node->update();
            nodes.push_back(node);
        }
        if (nodes.empty())
        {
            LOG_ERR("FEScene.open/nodes.empty().",fepj);
            return  false;
        }
        /// <summary>
        /// 打开工程语义为"替换"而非"追加":先清空当前场景再装载新内容。
        /// 文件读取与节点构建都在 clear() 之前完成,读取失败时不会破坏当前场景。
        /// 注意 clear() 会重置输入系统,内部已重新登记场景自带的浏览/编辑工具
        /// </summary>
        clear();
        dispatchNodesToSystem(nodes);
        addNodesToTree(nodes);

        _projectFileName    =   fepj;
        return  true;
    }

    bool    FEScene::save(const char* fepj)
    {
        String  fileName    =   fepj ? fepj : _projectFileName;
        if (fileName.empty())
            fileName    =   "unname.fepj";

        LOG_INF("FEScene.save(%s)",fileName.c_str());

        FEFileFormat    fmt(".fepj","1.0.0.0","FE Buildin Format!");
        fmt._type       =   FEFileFormat::DT_Model;
        fmt._mode       =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;
        auto    writer  =   FEFileFormatHelper::queryWriter(_ctx,fmt);
        if (writer == nullptr)
        {
            LOG_ERR("No suitable file parser could be located to write the file.");
            return  false;
        }
        auto    nodes   =   nodeTree().topLevelNodes();
        if (nodes.empty())
        {
            LOG_INF("FEScene.save/nodes.empty().",fepj);
            return  false;
        }
        Objects objects(nodes.size());
        for (size_t i = 0; i < nodes.size(); i++)
        {
            objects[i]  =   nodes[i].get();
        }
        if(writer->writeFile(objects,fileName))
            return  true;
        else
            return  false;
    }

    bool    FEScene::saveAs(const Objects& objects,const char* fileName)
    {
        if (fileName == nullptr)
            return  false;
        LOG_INF("FEScene.saveAs(%s)",fileName);

        /// TODO: 需要根据文件扩展名自动失识别writer
        FEFileFormat    fmt(".fepj","1.0.0.0","FE Buildin Format!");
        fmt._type       =   FEFileFormat::DT_Model;
        fmt._mode       =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;
        auto    writer  =   FEFileFormatHelper::queryWriter(_ctx,fmt);
        if (writer == nullptr)
        {
            LOG_ERR("No suitable file parser could be located to write the file.");
            return  false;
        }
        if(writer->writeFile(objects,fileName))
            return  true;
        else
            return  false;
    }
    void    FEScene::clear()
    {
        LOG_INF("FEScene.clear()");
        LOG_INF("_nodeTree.clear()");
        _nodeTree.clear();
        for (auto var : _comSysMgr.objects())
        {
            var->clear();
        }
        LOG_INF("_factorys.clear()");
        _factorys.clear();
        /// <summary>
        /// 上面清空输入系统时,会把场景自带的浏览/编辑工具(移动/旋转/缩放)一并移除,
        /// 它们不属于节点,不应随场景数据被清掉;这里重新登记,
        /// 否则重置后鼠标无法浏览场景
        /// </summary>
        registerInputTools();
    }

    void    FEScene::addNodesToFactory(const Nodes& nodeList,DispatchResult* result)
    {
        /// 通用分发路径复用同 key 的非常驻工厂,避免同一 mesh 重复建厂
        auto    results =   FEFactoryRender::addNodesToFactory(_ctx,*this,nodeList,true);
        for (auto& var : results)
        {
            _factorys.addObject(var);
        }
        if (result)
        {
            result->rFactorys   =   results;
        }
    }

    void    FEScene::loadPipelines()
    {
        /// 
        auto    plDir       =   _ctx.resourcePath() + "/data/pipeline/";
        auto    fileList    =   FEFileSystem::entryList(plDir,".xml");
        LOG_DBG("loadPipelines()");

        for (auto& file : fileList)
        {   
            auto    fullPath    =   plDir + file;
            auto    pipelines   =   FEPipelineHelper::create(_ctx,*_device,nullptr,fullPath.c_str(),_sampleCount);

            LOG_DBG("loading pipeline:%s",fullPath.c_str());

            for (auto& var : pipelines)
            {
                if (_device->pipelines().isExist(var->name().c_str()))
                {
                    LOG_DBG("pipeline:%s is exist",var->name().c_str());
                    continue;
                }
                _device->pipelines().add(var->name(),var);
            }
        }
    }


    void    FEScene::initializeBuildin(FEDevice& device)
    {
        auto    cameraUBO   =   device.createUBO();
        cameraUBO->create({sizeof(CameraData),      MemoryUsage::DEVICE_DEFAULT_BIT});
        cameraUBO->setObjectId(FEConstUuid::CameraUBOId);
        device.cacheObject(cameraUBO.get());
        /// 默锟较凤拷锟斤拷乒锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
        /// 锟斤拷系统锟狡癸拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?28,锟斤拷锟街凤拷锟斤拷
        auto    lightSBO    =   device.createSBO();
        lightSBO->create({sizeof(LightData) * 128,  MemoryUsage::DEVICE_DEFAULT_BIT});
        lightSBO->setObjectId(FEConstUuid::LightsId);
        device.cacheObject(lightSBO.get());

        auto    clipUBO     =   device.createUBO();
        clipUBO->create({sizeof(ClipData),       MemoryUsage::DEVICE_DEFAULT_BIT});
        clipUBO->setObjectId(FEConstUuid::ClipUBOId);
        device.cacheObject(clipUBO.get());

        /// 引擎全局 UBO:渲染状态(选中高亮)+ 天空环境,所有非 UI 材质共用
        auto    globalUBO   =   device.createUBO();
        globalUBO->create({sizeof(FEGlobalUBO),  MemoryUsage::DEVICE_DEFAULT_BIT});
        globalUBO->setObjectId(FEConstUuid::GlobalUBOId);
        {
            FEGlobalUBO global;
            globalUBO->update(&global,sizeof(FEGlobalUBO),0);
        }
        device.cacheObject(globalUBO.get());
    }

    void    FEScene::initializeQueue()
    {
        /// 
        updateQueue().addObject(_camera.get(),[&](CMDPtr cmd,FEUpdateObject& uData)
        {
            if (uData._gpu == nullptr)
            {
                uData._gpu  =   _device->queryCache(FEConstUuid::CameraUBOId)->as<FEGPUBuffer>();
            }
            if (uData._cpu == nullptr)
            {
                uData._cpu  =   _device->createUBO();
                uData._cpu->create({sizeof(CameraData),HOST_VISIBLE_BIT});
            }
            auto    pData   =   (CameraData*)uData._cpu->lock(sizeof(CameraData),0);
            if (pData)
            {
                mat4r       view        =   _camera->getView();
                mat4r       proj        =   _camera->getProject();
                mat4r       vp          =   proj * view;
                real3       eye         =   _camera->getEye();

                pData->_v               =   view;
                pData->_p               =   proj;
                pData->_vp              =   vp;
                /// 相机部分,整数部分
                pData->_offset.x        =   (int)eye.x;
                pData->_offset.y        =   (int)eye.y;
                pData->_offset.z        =   (int)eye.z;
                pData->_offset.w        =   0;
                /// 相机位置小数部分
                pData->_decimal.x       =   float(eye.x - pData->_offset.x);
                pData->_decimal.y       =   float(eye.y - pData->_offset.y);
                pData->_decimal.z       =   float(eye.z - pData->_offset.z);
                pData->_decimal.w       =   0;

                mat4r   matOffset       =   glm::translate(mat4r(1),real3(pData->_offset.x, pData->_offset.y, pData->_offset.z));
                mat4r   vpOffset        =   vp * matOffset;
                pData->_offsetVp        =   vpOffset;

                pData->_upDir           =   float4(float3(_camera->getUp()),    0.0f);
                pData->_rightDir        =   float4(float3(_camera->getRight()), 0.0f);
                uData._cpu->unlock();
            }
            if (uData._gpu && uData._cpu)
            {
                cmd->copyBuffer(uData._cpu,uData._gpu,sizeof(CameraData),0,0);
            }
        });

        updateQueue().addObject(&_device->lights(),[&](CMDPtr cmd,FEUpdateObject& uData)
        {

            if (uData._gpu == nullptr)
            {
                uData._gpu  =   _device->queryCache(FEConstUuid::LightsId)->as<FEGPUBuffer>();
            }
            if (uData._cpu == nullptr)
            {
                uData._cpu  =   _device->createSBO();
                uData._cpu->create({uData._gpu->cInfo()._length,HOST_VISIBLE_BIT});
            }

            LightMgr    pLight  =   uData._object->as<FELightMgr>();
            size_t      length  =   sizeof(LightData) * pLight->count();
            if(length > uData._gpu->cInfo()._length)
            {
                uData._gpu->resize(uData._gpu->cInfo()._length + sizeof(CameraData) * 16);
                uData._cpu->resize(uData._gpu->cInfo()._length + sizeof(CameraData) * 16);
                /// 
                uData._gpu->flags().addFlag(FLAG_UPDATE);
                uData._gpu->as<FENotify>()->fireNotify();
            }
            LightData*  pData   =   (LightData*)uData._cpu->lock(length,0);
            if (pData)
            {   
                auto&   lightData   =   pLight->data();
                uint    iIndex      =   0;
                for (auto& var : lightData)
                {
                    if (!var.second->flags().hasFlag(FLAG_VISIBLE))
                        continue;
                    switch(var.second->type())
                    {
                    case FELight::LT_Dir  :
                        {
                            auto    dirLight        =   var.second->as<FELightDir>();
                            pData[iIndex].lightType =   var.second->type();
                            pData[iIndex].r         =   dirLight->_color.r;
                            pData[iIndex].g         =   dirLight->_color.g;
                            pData[iIndex].b         =   dirLight->_color.b;

                            pData[iIndex].x         =   dirLight->_dir.x;
                            pData[iIndex].y         =   dirLight->_dir.y;
                            pData[iIndex].z         =   dirLight->_dir.z;
                        }
                        break;
                    case FELight::LT_Point:
                        {
                            auto    dirLight        =   var.second->as<FELightPoint>();
                            pData[iIndex].lightType =   var.second->type();
                            pData[iIndex].r         =   dirLight->_color.r;
                            pData[iIndex].g         =   dirLight->_color.g;
                            pData[iIndex].b         =   dirLight->_color.b;

                            pData[iIndex].x         =   dirLight->_pos.x;
                            pData[iIndex].y         =   dirLight->_pos.y;
                            pData[iIndex].z         =   dirLight->_pos.z;
                        }
                        break;
                    case FELight::LT_Spot :
                        break;
                    }
                    ++iIndex;
                }
                uData._cpu->unlock();
            }
            if (uData._gpu && uData._cpu)
            {
                cmd->copyBuffer(uData._cpu,uData._gpu,length,0,0);
            }
        });
    }

    void    FEScene::resize(const MsgResize& evt)
    {
        if (_device == nullptr)
            return;
        _device->waitIdle();

        uint    width   =   evt._info._size.x;
        uint    height  =   evt._info._size.y;
        _size   =   uint2(width,height);

        if (_app)
        {
            _swapchain      =   _device->createSwapchain();
            FESwapchain::CreateInfo infor   =   {};
            infor._width    =   width;
            infor._height   =   height;
            infor._appInst  =   _app ? _app->cInfo()._appInst : nullptr;
            infor._window   =   _app ? _app->cInfo()._window  : nullptr;
            _swapchain->create(infor);
        }
        else
        {
            /// 无头/离屏:不创建窗口交换链,渲染到内部颜色目标
            Swapchain   swapchain   =   new FEOffscreenSwapchain(_ctx);
            FESwapchain::CreateInfo infor   =   {};
            infor._width    =   width;
            infor._height   =   height;
            if (swapchain->create(infor))
                _swapchain  =   swapchain;
        }

        _imgDepth       =   nullptr;
        _depthView      =   nullptr;
        _imgDepth       =   _device->createGImage();
        _depthView      =   nullptr;
        {
            FEGImage::CreateInfo    info;
            {
                info._width     =   width;
                info._height    =   height;
                info._depth     =   1;
                /// 多采样深度仅作为 attachment 使用,不再声明 sampled 用途
                info._usage     =   (_sampleCount > 1)
                                    ? USAGE_DEPTH_STENCIL_ATTACHMENT
                                    : (USAGE_DEPTH_STENCIL_ATTACHMENT | USAGE_SAMPLED);
                info._type      =   IT_2D;
                info._format    =   FMT_D32_S8_UNORM;
                info._aspect    =   ASPECT_DEPTH_BIT;
                info._layout    =   IL_UNDEFINED;
                info._samples   =   _sampleCount;
            }
            _imgDepth->create(info);
            _depthView  =   _imgDepth->createView();
        }

        /// MSAA 颜色目标:渲染结束后解析(resolve)到交换链/离屏颜色目标
        _imgColorMSAA   =   nullptr;
        _msaaColorView  =   nullptr;
        if (_sampleCount > 1)
        {
            auto    colorImage  =   _device->createGImage();
            _imgColorMSAA       =   colorImage;
            FEGImage::CreateInfo    info;
            {
                info._width     =   width;
                info._height    =   height;
                info._depth     =   1;
                /// 多采样纹理只能作为 attachment 使用
                info._usage     =   USAGE_COLOR_ATTACHMENT;
                info._type      =   IT_2D;
                info._format    =   _swapchain ? _swapchain->colorFormat() : FMT_R8G8B8A8_UNORM;
                info._aspect    =   ASPECT_COLOR_BIT;
                info._layout    =   IL_UNDEFINED;
                info._samples   =   _sampleCount;
            }
            if (colorImage && colorImage->create(info))
                _msaaColorView  =   colorImage->createView();
            if (_msaaColorView == nullptr)
            {
                LOG_ERR("FEScene.resize: create MSAA color target failed, fallback to 1x");
                _imgColorMSAA   =   nullptr;
                _sampleCount    =   1;
            }
        }

        /// transmission 场景折射纹理:与窗口同尺寸,color attachment + sampled 双用途
        _sceneTex       =   nullptr;
        _sceneTexView   =   nullptr;
        _sceneTexColorView =   nullptr;
        {
            auto    sceneTex    =   _device->createTexture();
            FETexture::CreateInfo   tci;
            tci._width          =   width;
            tci._height         =   height;
            /// 必须与交换链同格式:pass1 用为 swapchain 格式创建的管线渲染到本纹理,
            /// 格式不一致时 wgpu 验证直接报 color attachment 不兼容
            /// (Rgba8Unorm vs Bgra8Unorm)并丢设备;Vulkan 同属不兼容 render pass
            tci._format         =   _swapchain ? _swapchain->colorFormat() : FMT_R8G8B8A8_UNORM;
            tci._magFilter      =   FILTER_LINEAR;
            tci._minFilter      =   FILTER_LINEAR;
            tci._addressMode    =   ADDRESS_CLAMP_TO_EDGE;
            tci._renderTarget   =   true;
            /// Vulkan 端建满 mip 链:pass1 渲 mip0 后由 VKCmdBuffer::generateMips
            /// 逐级 blit 生成,shader 按 roughness 选 lod 实现磨砂玻璃模糊折射
            /// (对标 glTF-Sample-Viewer);WebGPU 暂不处理,保持单 mip
            if (_rendererId == RS_VULKAN)
            {
                uint    maxDim  =   (std::max)(width,height);
                uint    mipCnt  =   1;
                while (maxDim > 1)
                {
                    maxDim  >>= 1;
                    ++mipCnt;
                }
                tci._mips       =   mipCnt;
            }
            if (sceneTex && sceneTex->create(tci) && sceneTex->imageView())
            {
                _sceneTex       =   sceneTex;
                _sceneTexView   =   sceneTex->imageView();
                /// 关键修复:pass1 只渲 mip0,单独建 levelCount=1 的 color attachment view
                /// 否则 VK 验证层要求 framebuffer 内完整 mip 链都在 COLOR_ATTACHMENT,
                /// 但 mip>=1 未被写入(保持 UNDEFINED)直接报 validation error
                _sceneTexColorView =   sceneTex->image()->createView(0, 1);
                /// 注册到 device cache,PBR 材质渲染时自取并自动检测重建后重绑
                sceneTex->setObjectId(FEConstUuid::SceneTexId);
                _device->cacheObject(sceneTex.get());
            }
        }

        assert(_camera != nullptr);

        if (_camera)
        {
            _camera->setViewSize(width,height);
            _camera->update();
        }
        for (auto viewer : _viewerMgr.objects())
        {
            viewer->onMessage(evt);
        }
        _device->waitIdle();
    }

    void    FEScene::createSky()
    {
        auto*   sky     =   new FESkyComponent(_ctx);
        _sky            =   sky;
        _skyNode        =   sky->createNode();
        _skyNode->addComponent(_sky);
        /// 与编辑网格一致:天空盒恒以相机为中心,不能参与剔除;
        /// resident 保证 clear() 时天空仍保留在工厂中(默认启动)
        auto    factorys    =   FEFactoryRender::addNodesToFactory(_ctx,*this,{_skyNode});
        for (auto& var : factorys)
        {
            var->setResident(true);
            var->setGPUCull(false);
            _factorys.addObject(var);
        }
    }

    Node    FEScene::createGrid()
    {
        Material    mat     =   new FEMaterialV3C4(_ctx);

        Node        node    =   new FENode(_ctx);
        FEGeometryGrid  geo(_ctx);

        geo.param()._size      =   100;
        geo.param()._divs      =   100;
        geo.param()._color1    =   Rgba8(212,  96, 112,255);
        geo.param()._color2    =   Rgba8(155,  209,79, 255);
        geo.param()._color3    =   Rgba8(0,    0,  255,255);
        geo.param()._color4    =   Rgba8(128,  128,128,255);

        auto    mesh    =   geo.triangular({{IS_VERTEX_POS,FMT_R32G32B32_FLOAT},{IS_VERTEX_COLOR0,FMT_R8G8B8A8_UNORM}});
        node->setMesh(mesh);
        node->setMaterial(mat);

        node->makeDirty();
        node->update();
        auto    factorys    =   FEFactoryRender::addNodesToFactory(_ctx,*this,{node});
        for (auto& var : factorys)
        {
            var->setResident(true);
            var->setGPUCull(false);
            _factorys.addObject(var);
        }
        return  node;
    }

    void    FEScene::showGrid(bool show)
    {
        if (_gridNode == nullptr)
            return;
        auto    mat =   _gridNode->material();
        if (mat == nullptr)
            return;
        /// 网格挂在独立工厂上,按材质反查所在工厂后切换其可见标志
        for (auto var : _factorys.objects())
        {
            if (!var->hasMaterialGroup(mat))
                continue;
            if (show)   var->flags().addFlag(FE::FLAG_VISIBLE);
            else        var->flags().removeFlag(FE::FLAG_VISIBLE);
        }
    }

    void    FEScene::showSky(bool show)
    {
        /// 先同步天空组件开关:关闭时把 SkyUBO 的天空开关置 0,
        /// 使 PBR 的解析天空 IBL(漫反射/镜面)一并停止计入
        auto*   skyComponent    =   sky();
        if  (skyComponent != nullptr)
            skyComponent->setSkyVisible(show);
        if (_skyNode == nullptr)
            return;
        auto    mat =   _skyNode->material();
        if (mat == nullptr)
            return;
        /// 天空挂在独立工厂上(不参与剔除),按材质反查所在工厂后切换其可见标志
        for (auto var : _factorys.objects())
        {
            if (!var->hasMaterialGroup(mat))
                continue;
            if (show)   var->flags().addFlag(FE::FLAG_VISIBLE);
            else        var->flags().removeFlag(FE::FLAG_VISIBLE);
        }
    }

    void    FEScene::createTools()
    {
        auto    inputSys    =   _comSysMgr.query(UUIDOF(FEInputSystem));
        if (inputSys == nullptr)
        {
            LOG_ERR("createTools/without an input module, you can't build any interactive features!");
            return;
        }
        auto    facotryMov  =   new FEFactoryAxisMove(_ctx);
        facotryMov->setResident(true);
        _factorys.addObject(facotryMov);

        auto    facotryRot  =   new FEFactoryAxisRotate(_ctx);
        facotryRot->setResident(true);
        _factorys.addObject(facotryRot);

       auto    facotryScl  =   new FEFactoryAxisScale(_ctx);
       facotryScl->setResident(true);
       _factorys.addObject(facotryScl);

       /// 默认隐藏掉
       facotryMov->inputComponent()->flags().removeFlag(FE::FLAG_VISIBLE);
       facotryRot->inputComponent()->flags().removeFlag(FE::FLAG_VISIBLE);
       facotryScl->inputComponent()->flags().removeFlag(FE::FLAG_VISIBLE);

       /// 修改编辑工具的Id(使用类型Id作为ObjectId) 保证对象id唯一不随机
       /// 就可以通过id查询到对象,
       facotryMov->inputComponent()->setObjectId(facotryMov->inputComponent()->classId());
       facotryRot->inputComponent()->setObjectId(facotryRot->inputComponent()->classId());
       facotryScl->inputComponent()->setObjectId(facotryScl->inputComponent()->classId());

       /// 加入到input system 接收 键盘鼠标等外设消息
       /// 记录场景自带的输入工具:clear() 会清空输入系统,届时要靠这份引用重新登记
       _inputTools.clear();
       _inputTools.push_back(facotryMov->inputComponent().get());
       _inputTools.push_back(facotryRot->inputComponent().get());
       _inputTools.push_back(facotryScl->inputComponent().get());
       /// 浏览工具
       _inputTools.push_back(new FESceneBrowse(_ctx));

       registerInputTools();
    }

    void    FEScene::registerInputTools()
    {
        auto    inputSys    =   _comSysMgr.query(UUIDOF(FEInputSystem));
        if (inputSys == nullptr)
        {
            LOG_ERR("registerInputTools/without an input module, you can't build any interactive features!");
            return;
        }
        for (auto& var : _inputTools)
        {
            if (var != nullptr)
                inputSys->addObject(var.get());
        }
    }
}
