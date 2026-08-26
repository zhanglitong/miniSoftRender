
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

namespace   FE
{
    bool    FEScene::setup(App app)
    {
        FETimestamp     timestamp;
        LOG_INF("FE::FEAppHelper::create cost %lf",timestamp.milliSec());
        timestamp.update();
        _app        =   app;
        _renderSys  =   FERenderSystem::create(_ctx,RS_VULKAN);
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
        /// anchor
        /// anchor 增加通知
        _ctx.anchor().addNotify(this,[this](Object object)
        {
            UNUSED(object);
            if (_mousePoint)
            {
                _mousePoint->setLocalTranslation(_ctx.anchor().point());
                _mousePoint->update();
            }
        });
       
        return  true;
    }

    void    FEScene::test()
    {
        Material    material    =   new FEMaterialV3C4(_ctx);
        Material    matLine     =   new FEMaterialV3C4(_ctx);
        auto        nodes       =   loadNode(material);
        for (auto& node : nodes)
        {
            _nodeTree.addToplevelNode(node);
        }
        {
            auto    factorys    =   FEFactoryRender::addNodesToFactory(_ctx,*this,nodes);
            for (auto& var : factorys)
            {
                _factorys.addObject(var);
            }
        }
        {
            auto    node        =   createGrid(matLine);
            auto    factorys    =   FEFactoryRender::addNodesToFactory(_ctx,*this,{node});
            for (auto& var : factorys)
            {
                _factorys.addObject(var);
            }
        }
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
        if (_frame->_cmd == nullptr)
        {
            _frame->_cmd    =   _cmdPool->createCmd();
        }
        if (_frame->_cmd)
        {
            _frame->_cmd->reset();
            _frame->_cmd->begin();
            _frame->reset();
        }
        
    }
    void    FEScene::onFrameUpdate()
    {
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
        /// 所有组件系统更新
        for (auto& var : comSyss)
        {
            var->update(_ctx.deltaTime());
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
        rsInfo._clearColor  =   float4(1,1,1,1);
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
        if (_frame && _frame->_cmd )
        {
            _frame->_cmd->end();
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
            onClose();
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

    void    FEScene::onNodePropChanged(FENode* node)
    {
        auto    mesh    =   node->mesh();
        if (mesh)
        {
            auto&   pris    =   mesh->primitives();
            auto    slot    =   mesh->slotBits();
            for (auto& var : pris)
            {
                MeshKey mkey;
                mkey._drawType  =   var->type();
                mkey._primitive =   var->primitive();
                mkey._slotBits  =   slot;
                auto    key     =   mkey.key();
                /// 鏌ユ壘宸ュ巶瀵硅薄
                auto    factory =   _factorys.findObject(key);
                if (!factory)
                    continue;
                factory->nodePropChanged(node);
            }
        }
    }

    void    FEScene::onClose()
    {
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

        for (auto node: _nodeTree.topLevelNodes())
        {
            node->removeAllChildren();
        }
        _nodeTree.clear();

        if (_device)
        {
            _device->destroy();
        }
        _ctx._device    =   nullptr;
        _device         =   nullptr;
        _renderSys      =   nullptr;
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
                pData->_v             =   _camera->getView();
                pData->_p             =   _camera->getProject();
                pData->_position      =   float4(float3(_camera->getEye()),   0.0f);
                pData->_upDir         =   float4(float3(_camera->getUp()),    0.0f);  
                pData->_rightDir      =   float4(float3(_camera->getRight()), 0.0f); 
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

    Nodes   FEScene::loadNode(Material mat)
    {
        float3s     vertex   =   
        {
            {  1.0f,  1.0f, 0.0f },
            { -1.0f,  1.0f, 0.0f },
            {  0.0f, -1.0f, 0.0f },
        };
        Rgba8s     colors  =   
        {
            { 255,  0,      0,  255 } ,
            { 0,    255,    0,  255 } ,
            { 0,    0,      255,255 } ,
        };

        Node        root    =   new FENode(_ctx);
        Mesh        mesh    =   FEMeshBuilder::makeMesh(_ctx,vertex,colors);
        auto        aabb    =   mesh->updateAabb();

        for (size_t i = 0; i < 10; i++)
        {
            Node    child   =   new FENode(_ctx);
            child->setLocalTranslation(real3(0.1 * i,3,0.1 * i));
            child->setMaterial(mat);
            child->setMesh(mesh);
            root->addChild(child.get());
        }
        root->makeDirty();
        root->update();
        return  {root};
    }

    Node    FEScene::createGrid(Material mat)
    {
        Node    node    =   new FENode(_ctx);
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

        return  node;
    }
}
