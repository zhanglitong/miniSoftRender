// 进度条: 全屏矩形顶点着色器
// 无顶点输入,用 gl_VertexIndex 展开 6 个顶点(两个三角形),
// 矩形与进度、颜色全部由 push constant 提供,不需要 VBO/UBO/描述符集。
#version 450

layout(push_constant) uniform ProgressPush
{
    /// x0,y0,x1,y1(NDC,y 向上;y0 为矩形上边)
    vec4    _rect;
    vec4    _background;
    vec4    _fill;
    vec4    _border;
    /// 0~1
    float   _progress;
    /// 边框宽度(像素)
    float   _borderWidth;
    /// 矩形像素尺寸(w,h)
    vec2    _size;
} pc;

layout(location = 0) out vec2 vPx;

const vec2 corners[6] = vec2[6](
    vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0),
    vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

void main()
{
    vec2 c = corners[gl_VertexIndex];
    /// 点图元变体(using_defult 会为该管线生成 POINT_LIST):显式写 gl_PointSize,
    /// 否则 Vulkan 校验层报 "PointSize is not written but topology is POINT_LIST"
    gl_PointSize = 1.0;
    gl_Position  = vec4(mix(pc._rect.xy, pc._rect.zw, c), 0.0, 1.0);
    vPx          = c * pc._size;
}
