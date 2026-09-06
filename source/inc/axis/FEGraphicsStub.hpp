#pragma     once

#include    "FEMath.hpp"
#include    <vector>

namespace   FE
{
    /// <summary>
    /// 图元类型
    /// </summary>
    enum    PrimitiveType
    {
        PT_POINTS           =   0,
        PT_LINES            =   1,
        PT_LINE_STRIP       =   2,
        PT_TRIANGLES        =   3,
        PT_TRIANGLE_STRIP   =   4,
        PT_TRIANGLE_FAN     =   5,
    };

    /// <summary>
    /// 裁剪模式
    /// </summary>
    enum    CullMode
    {
        CullNone    =   0,
        CullFront   =   1,
        CullBack    =   2,
        CM_NULL     =   CullNone,
    };

    /// <summary>
    /// 内存使用方式
    /// </summary>
    enum    MemoryUsage
    {
        HOST_COHERENT_BIT   =   1,
        DEVICE_LOCAL_BIT    =   2,
    };

    /// <summary>
    /// 索引类型
    /// </summary>
    enum    IndexType
    {
        IndexUint8  =   0,
        IndexUint16 =   1,
        IndexUint32 =   2,
    };

    /// <summary>
    /// 前向面方向
    /// </summary>
    enum    FrontFace
    {
        FF_CCW  =   0,
        FF_CW   =   1,
    };

    /// <summary>
    /// 多边形模式
    /// </summary>
    enum    PolygonMode
    {
        PM_POINT    =   0,
        PM_LINE     =   1,
        PM_FILL     =   2,
    };

    /// <summary>
    /// 着色器阶段
    /// </summary>
    enum    ShaderObject
    {
        SHADER_VERTEX   =   0,
        SHADER_FRAGMENT =   1,
    };

    /// <summary>
    /// 位置+法线顶点结构
    /// </summary>
    struct  V3N3
    {
        float   x, y, z;
        float   nx, ny, nz;
    };

    /// <summary>
    /// float3 数组
    /// </summary>
    using   ArrayFloat3 =   std::vector<float3>;

    /// <summary>
    /// 顶点缓冲区占位类
    /// </summary>
    class   VertexBuffer
    {
    public:
        bool    isValid() const { return _valid; }
        size_t  bufferSize() const { return _size; }
        void    update(size_t offset, size_t size, const void* data) { (void)offset; (void)size; (void)data; }
        void    setValid(bool v) { _valid = v; }
        void    setSize(size_t s) { _size = s; }
    private:
        bool    _valid  =   true;
        size_t  _size   =   0;
    };
    using   VertexBufPtr    =   std::shared_ptr<VertexBuffer>;

    /// <summary>
    /// 索引缓冲区占位类
    /// </summary>
    class   IndexBuffer
    {
    public:
        bool    isValid() const { return _valid; }
        size_t  bufferSize() const { return _size; }
        void    update(size_t offset, size_t size, const void* data) { (void)offset; (void)size; (void)data; }
        void    setValid(bool v) { _valid = v; }
        void    setSize(size_t s) { _size = s; }
    private:
        bool    _valid  =   true;
        size_t  _size   =   0;
    };
    using   IndexBufPtr =   std::shared_ptr<IndexBuffer>;

    /// <summary>
    /// 材质占位类
    /// </summary>
    class   Material
    {
    public:
    };
    using   MaterialPtr =   std::shared_ptr<Material>;

    /// <summary>
    /// VAO 占位类
    /// </summary>
    class   VertexArrayObject
    {
    public:
        void    update() {}
    };

    /// <summary>
    /// 图形管线占位类
    /// </summary>
    class   FEGraphicPL
    {
    public:
        bool    isValid() const { return false; }
        VertexArrayObject*  vao() { return &_vao; }
    private:
        VertexArrayObject   _vao;
    };
    using   GraphicPLPtr  =   std::shared_ptr<FEGraphicPL>;
}
