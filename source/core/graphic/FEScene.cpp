
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
#include    "../inc/FEInputSystem.hpp"
#include    "../inc/FEFileFormatHelper.hpp"
#include    "../inc/fileFormat/fepk/FEFormatFepj.hpp"
#include    "../inc/graphic/FEFactoryAxisMove.hpp"
#include    "../inc/graphic/FEFactoryAxisRotate.hpp"
#include    "../inc/graphic/FEFactoryAxisScale.hpp"
#include    "../inc/graphic/FESceneBrowse.h"

namespace   FE
{
    FEScene::FEScene(FEContext& ctx)
        :FEObject(ctx)
        ,_nodeTree(ctx)
        ,_factorys(ctx)
        ,_viewerMgr(ctx)
        ,_updateList(ObjectLessFunc)
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
        ,_updateList(ObjectLessFunc)
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

    bool    FEScene::setup(App app,const FEUuid& rendererId)
    {
        FETimestamp     timestamp;
        LOG_INF("FE::FEAppHelper::create cost %lf",timestamp.milliSec());
        timestamp.update();
        _app        =   app;
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
        {
            _cmdPool    =   _device->graphicCmdPool();
        }
        _camera     =   new FECamera(_ctx,real3(0,0,0),real3(0,0,-2.5),real3(1,0,0));
        _camera->setUp(real3(0,1,0));
        _camera->perspective(DEG2RAD(45), float(app->cInfo()._width) / float(app->cInfo()._height), 0.05f, 256.0f);
        _camera->update();
        /// 
        auto    light   =   new FELightDir(_ctx);
        /// 
        _device->lights().add("main",light);
        /// 
        resize(MsgResize({{_app->cInfo()._width,_app->cInfo()._height}}));
        /// 
        initializeQueue();
        /// 
        loadPipelines();
        /// 注册动画系统
        {
            AnimSys animSys =   new FEAnimationSys(_ctx);
            _comSysMgr.addObject(animSys.get());
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
        /// 创建网格
        createGrid();
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

        _cmdPool    =   nullptr;
        _frame      =   nullptr;
        _imgDepth   =   nullptr;
        _depthView  =   nullptr;

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
   
    void    FEScene::dispatchNodesToSystem(const Nodes& nodeList,DispatchResult* result)
    {
        addNodesToFactory(nodeList,result);
        dispatchToSystem<FEAnimation>(_comSysMgr,nodeList);
        dispatchToSystem<FEInput>(_comSysMgr,nodeList);
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
        }

        for (auto var : _updateList.objects())
        {   
            auto    node    =   var->cast<FENode>();
            if (node)
            {
                node->update();
                node->fireChanged();
            }
        }
        _updateList.clearObjects();
        
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
        uint    width       =   _app->cInfo()._width;
        uint    height      =   _app->cInfo()._height;
        FECmdBuffer::RenderInfo  rsInfo  =   {};
        rsInfo._depth       =   _depthView;
        rsInfo._colors      =   {_frame->_imageViewer};
        rsInfo._clearColor  =   float4(0,0,0,1);
        rsInfo._rect.set(0,0,width,height);

        _frame->_cmd->beginRender(rsInfo);

        for (auto viewer : _viewerMgr.objects())
        {
            FEFramInfo  info    =  {0,_frame->_cmd};
            viewer->onMessage(MsgRender(info));
        }
        _frame->_cmd->endRender(rsInfo);
        
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
    }

    void    FEScene::addNodesToFactory(const Nodes& nodeList,DispatchResult* result)
    {
        auto    results =   FEFactoryRender::addNodesToFactory(_ctx,*this,nodeList);
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
            auto    pipelines   =   FEPipelineHelper::create(_ctx,*_device,nullptr,fullPath.c_str());

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

        auto    skyUBO      =   device.createUBO();
        skyUBO->create({sizeof(skyUBO),         MemoryUsage::DEVICE_DEFAULT_BIT});
        skyUBO->setObjectId(FEConstUuid::SkyUBOId);
        device.cacheObject(skyUBO.get());
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

        _imgDepth       =   nullptr;
        _depthView      =   nullptr;
        _imgDepth       =   _device->createGImage();
        _depthView      =   nullptr;
        {
            FEGImage::CreateInfo    info;
            {
                info._width     =   _app->cInfo()._width;
                info._height    =   _app->cInfo()._height;
                info._depth     =   1;
                info._usage     =   USAGE_DEPTH_STENCIL_ATTACHMENT | USAGE_SAMPLED;
                info._type      =   IT_2D;
                info._format    =   FMT_D32_S8_UNORM;
                info._aspect    =   ASPECT_DEPTH_BIT;
                info._layout    =   IL_UNDEFINED;
            }
            _imgDepth->create(info);
            _depthView  =   _imgDepth->createView();
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
       inputSys->addObject(facotryMov->inputComponent().get());
       inputSys->addObject(facotryRot->inputComponent().get());
       inputSys->addObject(facotryScl->inputComponent().get());
       /// 浏览工具
       SceneBrowse     browseTool  =   new FESceneBrowse(_ctx);
       inputSys->addObject(browseTool.get());
    }
}
