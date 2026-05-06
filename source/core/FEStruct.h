#pragma     once
#include    <string>
#include    "../inc/FEObject.h"
#include    "../inc/FERect.hpp"

namespace   FE
{
    class   FERenderPass;
    class   FEFrameBuffer;
    class   FEPipeline;
    class   FEDescriptor;
    class   FEShader;
    class   FEProgram;

    using   String  =   std::string;

    enum    CallResult  :   uint32_t
    {
        CR_SUCCESS,
        CR_FAILED,
    };

    enum    StructureType:uint32_t
    {
    };

    enum    FEImageFormat : uint32_t
    {
        FMT_NONE    =   0,
        FMT_RGBA8       ,
        FMT_RGBAI16     ,
        FMT_RGBAI32     ,

        FMT_RGBAU16     ,
        FMT_RGBAU32     ,

        FMT_RGBAF16     ,
        FMT_RGBAF32     ,
    };

    struct  FEBufferInfor 
    {
        size_t  _size; 
    };

    struct  FECmdBufferInfo
    {
        FERenderPass*   renderPass  =   nullptr; 
        uint32_t        subpass     =   0;
        FEFrameBuffer*  framebuffer =   nullptr;
    } ;

    union   FEClearColorValue
    {
        float       float32[4];
        int32_t     int32[4];
        uint32_t    uint32[4];
    } ;

    struct  FEClearDepthStencilValue 
    {
        float       depth;
        uint32_t    stencil;
    };


    union   FEClearValue 
    {
        FEClearColorValue           color;
        FEClearDepthStencilValue    depthStencil;
    };


    struct  FERPBeginInfo 
    {
        FERenderPass*       renderPass      =   nullptr; 
        FEFrameBuffer*      framebuffer     =   nullptr;
        RectU16             renderArea;
        const FEClearValue* pClearValues    =   nullptr;
        uint32_t            clearValueCount =   0;
    };

    struct  FEPLBindInfo 
    {
        FEPipeline*         pipeline        =   nullptr; 
    };

    struct  FEBinding
    {
        uint32_t    _set        =   0;
        uint32_t    _binding    =   0;
        String      _name;
        Object      _objects;
    };

    using   FEBindings  =   std::vector<FEBinding>;

    struct  FEDescInfo
    {
        FEDescriptor*   _desc  =   nullptr;
    };

    enum    FEVertexInputRate :uint32_t
    {
        VERTEX_INPUT_VERTEX     =   0,
        VERTEX_INPUT_INSTANCE   =   1,
    } ;

    struct  FEVertexInputAttrDesc
    {
        uint32_t        binding;
        uint32_t        location;
        FEImageFormat   format;
        uint32_t        offset;
    };

    using   VertexAttrs =   std::vector<FEVertexInputAttrDesc>;

    struct  FEVertexInputBindDesc
    {
        uint32_t            binding;
        uint32_t            stride;
        FEVertexInputRate   inputRate;
        VertexAttrs         attrs;
    };

    using   VertexBinds =   std::vector<FEVertexInputBindDesc>;

    

    struct  FEPipelineInfo
    {
        FEProgram*  _program    =   nullptr;
        VertexBinds _binds;
    };

    struct  FEImageCreateInfo
    {
        /// <summary>
        /// _extent.x : 宽度
        /// _extent.y : 高度
        /// _extent.z : 深度
        /// </summary>
        uint3           _extent;
        /// <summary>
        /// mip级别
        /// </summary>
        uint32_t        _levels =   1;
        /// <summary>
        /// 描述数组大小
        /// </summary>
        uint32_t        _layers =   1;
        /// <summary>
        /// 数据格式
        /// </summary>
        FEImageFormat   _format =   FMT_RGBA8;
        /// <summary>
        /// 如果不为null,则使用
        /// </summary>
        size_t          _bufferSize =   0;
        void*           _buffer     =   nullptr;
        /// <summary>
        /// 标记_buffer管理权限(1,移交给内部释放)
        /// </summary>
        uint32_t        _flag       =   0;
    };
}