#version 450

#include "FEShaderDefine.h"

/// 距离场文本 shader:
///   顶点属性: 0=pos(屏幕像素坐标,左上原点,y 向下) 1=uv
///   实例属性: 6=节点颜色(IS_INSTANCE_COLOR)
///   不使用相机矩阵,直接把像素坐标变换到 NDC,作为屏幕空间叠加层绘制
///   push constant: 视口像素尺寸
layout  (location = 0)  in  vec3    inPos;
layout  (location = 1)  in  vec2    inUV;
layout  (location = 6)  in  vec4    nodeColor;

layout  (push_constant) uniform TextPush
{
    /// xy = 视口像素尺寸, zw = 1/视口尺寸
    vec4    _screen;
    /// x = 每个烘焙像素对应的 SDF 编码增量 = 1/(2*spread)(片元着色器使用)
    vec4    _sdf;
}   pc;

layout  (location = 0)  out vec2    outUV;
layout  (location = 1)  out vec4    outColor;

void main()
{
    vec2    ndc =   inPos.xy * pc._screen.zw;

    /// z = 0 且材质侧关闭深度测试:始终绘制在最前
    gl_Position =   vec4(ndc.x * 2.0 - 1.0,1.0 - ndc.y * 2.0,0.0,1.0);
    outUV       =   inUV;
    outColor    =   nodeColor;
}
