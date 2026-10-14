// OIT Color Pass: 全屏三角形顶点着色器
// 无顶点输入，用 gl_VertexIndex 展开一个覆盖全屏的三角形
#version 450

void main()
{
    vec2 pos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}
