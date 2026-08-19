#include    "SWDevice.h"
#include    "SWRenderSystem.h"

#include    "SWCmdBuffer.h"
#include    "SWQueue.h"
#include    "SWFrameBuffer.h"
#include    "SWShader.h"
#include    "SWRenderPass.h"
#include    "SWGPipeline.h"
#include    "SWSwapchain.h"
#include    "SWGImage.h"
#include    "SWGImageView.h"
#include    "SWCmdPool.h"
#include    "SWDSet.h"
#include    "SWDSetLayout.h"
#include    "SWDSetPool.h"
#include    "SWFence.h"
#include    "SWSemaphore.h"
#include    "SWGPUBuffer.h"

namespace   FE
{
    SWDevice::~SWDevice()
    {
        if (_destroyNotify) _destroyNotify(*this);

        _graphicPool    =   nullptr;
        _computePool    =   nullptr;
        _transferPool   =   nullptr;

        _queueGraphic   =   nullptr;
        _queueCompute   =   nullptr;
        _queueTransfer  =   nullptr;
    }

    FEResult    SWDevice::create(const CreateInfo& cInfo)
    {
        _cInfo  =   cInfo;
        LOG_INF("SWDevice.create()");
        return  FEResult::ER_SUCCESS;
    }

    void    SWDevice::waitIdle()
    {
    }

    Fence   SWDevice::createFence()
    {
        LOG_DBG("SWDevice.createFence");
        return  new SWFence(_ctx);
    }

    Semaphore   SWDevice::createSemaphore()
    {
        LOG_DBG("SWDevice.createSemaphore");
        return  new SWSemaphore(_ctx);
    }

    CMDPool     SWDevice::createCmdPool()
    {
        LOG_DBG("SWDevice.createCmdPool");
        return  new SWCmdPool(_ctx);
    }

    FBOPtr      SWDevice::createFrameBuffer()
    {
        LOG_DBG("SWDevice.createFrameBuffer");
        return  new SWFrameBuffer(_ctx);
    }

    Shader      SWDevice::createShader()
    {
        LOG_DBG("SWDevice.createShader");
        return  new SWShader(_ctx);
    }

    RenderPass  SWDevice::createRenderPass()
    {
        LOG_DBG("SWDevice.createRenderPass");
        return  new SWRenderPass(_ctx);
    }

    GPipeline   SWDevice::createGPipeline()
    {
        LOG_DBG("SWDevice.createGPipeline");
        return  new SWGPipeline(_ctx);
    }

    Swapchain   SWDevice::createSwapchain()
    {
        LOG_DBG("SWDevice.createSwapchain");
        return  new SWSwapchain(_ctx);
    }

    GImage      SWDevice::createGImage()
    {
        LOG_DBG("SWDevice.createGImage");
        return  new SWGImage(_ctx);
    }

    DSetLayout  SWDevice::createDSLayout()
    {
        LOG_DBG("SWDevice.createDSLayout");
        return  new SWDSetLayout(_ctx);
    }

    DSetPool    SWDevice::createDSetPool()
    {
        LOG_DBG("SWDevice.createDSetPool");
        return  new SWDSetPool(_ctx);
    }

    DSet        SWDevice::createDSet()
    {
        LOG_DBG("SWDevice.createDSet");
        return  new SWDSet(_ctx);
    }

    VBO         SWDevice::createVBO()
    {
        LOG_DBG("SWDevice.createVBO");
        return  new SWGPUBuffer(_ctx,BufferUsage::VERTEX_BUFFER_BIT);
    }

    IBO         SWDevice::createIBO()
    {
        LOG_DBG("SWDevice.createIBO");
        return  new SWGPUBuffer(_ctx,BufferUsage::INDEX_BUFFER_BIT);
    }

    UBO         SWDevice::createUBO()
    {
        LOG_DBG("SWDevice.createUBO");
        return  new SWGPUBuffer(_ctx,BufferUsage::UNIFORM_BUFFER_BIT);
    }

    ITO         SWDevice::createITO()
    {
        LOG_DBG("SWDevice.createITO");
        return  new SWGPUBuffer(_ctx,BufferUsage::INDIRECT_BUFFER_BIT);
    }

    SBO         SWDevice::createSBO()
    {
        LOG_DBG("SWDevice.createSBO");
        return  new SWGPUBuffer(_ctx,BufferUsage::STORAGE_BUFFER_BIT);
    }

    GPUBuffer   SWDevice::createFromBuffer(const void* pData,uint64 length)
    {
        if (length == 0 || pData == nullptr)
            return  nullptr;
        LOG_DBG("SWDevice.createFromBuffer");
        auto    object  =  new SWGPUBuffer(_ctx,BufferUsage::TRANSFER_SRC_BIT);
        FEGPUBuffer::CreateInfo cInfo;
        cInfo._bufUsages    =   TRANSFER_SRC_BIT;
        cInfo._memUsages    =   HOST_VISIBLE_BIT;
        cInfo._length       =   length;
        if(object->create(cInfo))
        {
            object->update(pData,length,0);
            return  object;
        }
        return  nullptr;
    }

    Queue       SWDevice::queueGraphic()
    {
        if (_queueGraphic == nullptr)
        {
            _queueGraphic  =   new SWQueue(_ctx,nullptr);
        }
        return  _queueGraphic;
    }

    Queue       SWDevice::queueCompute()
    {
        if (_queueCompute == nullptr)
        {
            _queueCompute  =   new SWQueue(_ctx,nullptr);
        }
        return  _queueCompute;
    }

    Queue       SWDevice::queueTransfer()
    {
        if (_queueTransfer == nullptr)
        {
            _queueTransfer  =   new SWQueue(_ctx,nullptr);
        }
        return  _queueTransfer;
    }
}
