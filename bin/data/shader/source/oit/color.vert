// OIT Color Pass: 全屏三角形顶点着色器
// 无顶点输入，用 gl_VertexIndex 展开一个覆盖全屏的三角形
#version 450

void main()
{
    vec2 pos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    /// 点图元变体(using_defult 会为该管线生成 POINT_LIST):显式写 gl_PointSize,
    /// 否则 Vulkan 校验层报 "PointSize is not written but topology is POINT_LIST"
    gl_PointSize = 1.0;
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}
