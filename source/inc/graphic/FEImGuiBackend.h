#pragma     once

#include    "RSObject.h"
#include    "../FEFormat.hpp"

namespace   FE
{
    /// <summary>
    /// 单条 ImGui 绘制命令(来自 ImDrawCmd 的扁平化表示)。
    /// 裁剪矩形使用 ImGui 的显示坐标系(左上原点,单位为逻辑像素)。
    /// </summary>
    struct  FEImGuiDrawCmd
    {
        uint32  _indexCount     =   0;      ///< 索引数量
        uint32  _indexOffset    =   0;      ///< 索引起始位置(单位:索引)
        int32   _vertexOffset   =   0;      ///< 顶点基址(会加到每个索引上)
        float   _clipX          =   0.0f;
        float   _clipY          =   0.0f;
        float   _clipW          =   0.0f;
        float   _clipH          =   0.0f;
    };
    /// <summary>
    /// 一帧 ImGui 绘制数据。所有指针仅在 render() 调用期间有效。
    /// 顶点为 ImDrawVert(20 字节:pos 2*float, uv 2*float, col 1*uint),
    /// 索引为 16 位(ImDrawIdx 默认 unsigned short)。
    /// </summary>
    struct  FEImGuiDrawData
    {
        const void*             _vertexData     =   nullptr;
        uint32                  _vertexCount    =   0;
        const void*             _indexData      =   nullptr;
        uint32                  _indexCount     =   0;
        const FEImGuiDrawCmd*   _cmds           =   nullptr;
        uint32                  _cmdCount       =   0;
        float                   _displayPosX    =   0.0f;
        float                   _displayPosY    =   0.0f;
        float                   _displaySizeX   =   0.0f;
        float                   _displaySizeY   =   0.0f;
        float                   _scaleX         =   1.0f;
        float                   _scaleY         =   1.0f;
    };
    /// <summary>
    /// 字体图集纹理数据(RGBA8),由 ImGui 字体烘焙产生。
    /// </summary>
    struct  FEImGuiFontData
    {
        const void*             _pixels         =   nullptr;
        uint32                  _width          =   0;
        uint32                  _height         =   0;
    };

    class   FECmdBuffer;
    /// <summary>
    /// 调试 GUI(ImGui)渲染后端抽象接口。
    /// 具体实现位于各渲染后端(如 rs.vulkan 的 VKImGuiBackend)。
    /// 接口只使用与后端无关的扁平化数据,因此 core 不依赖 ImGui/Vulkan 头文件。
    /// 注意:本接口全部为内联/纯虚,故不加 FE_API(core 无导出实体,
    /// 加 FE_API 会让实现方产生 dllimport 的未解析符号)。
    /// </summary>
    class   FEImGuiBackend
    {
    public:
        virtual ~FEImGuiBackend()
        {}
        /// <summary>
        /// 初始化渲染资源(管线/描述符/采样器)。
        /// colorFormat 必须与渲染目标(交换链)颜色格式一致,否则动态渲染校验失败。
        /// </summary>
        virtual bool    init(FEFormat colorFormat)  =   0;
        /// <summary>
        /// 上传字体图集纹理(RGBA8)。
        /// </summary>
        virtual bool    uploadFont(const FEImGuiFontData& font) =   0;
        /// <summary>
        /// 把一帧绘制数据录制到命令缓冲(必须在 beginRender/endRender 之间调用)。
        /// </summary>
        virtual void    render(const FEImGuiDrawData& drawData,FECmdBuffer* cmd) =   0;
        /// <summary>
        /// 释放所有渲染资源。
        /// </summary>
        virtual void    destroy()   =   0;
    };
}
