#version 450

#include "FEShaderDefine.h"

/// 距离场文本 shader:
///   顶点属性: 0=pos(像素坐标/像素偏移,左上原点,y 向下) 1=uv
///   实例属性: 2=平移整数部分 3=平移小数部分(布告板世界锚点) 6=节点颜色(IS_INSTANCE_COLOR)
///   模式(由 push constant _sdf.z 选择):
///     0 = 屏幕叠加层:不使用相机,直接把像素坐标变换到 NDC,作为屏幕空间叠加层绘制
///     1 = 三维布告板:实例平移为世界锚点经相机投影,像素偏移在裁剪空间乘 clip.w,
///         保持恒定屏幕像素尺寸且始终面向相机
///   push constant: 视口像素尺寸
layout  (location = 0)  in  vec3    inPos;
layout  (location = 1)  in  vec2    inUV;
/// 实例平移(布告板锚点):整数/小数拆分,与 v3c4 的大坐标方案一致
layout  (location = 2)  in  ivec3   intPart;
layout  (location = 3)  in  vec3    trans;
layout  (location = 6)  in  vec4    nodeColor;

layout  (binding = SB_Camera) uniform CameraBlock
{
    CameraData  _camera;
};

layout  (push_constant) uniform TextPush
{
    /// xy = 视口像素尺寸, zw = 1/视口尺寸
    vec4    _screen;
    /// x = 每个烘焙像素对应的 SDF 编码增量 = 1/(2*spread)(片元着色器使用)
    /// z = 绘制模式(0=屏幕叠加,1=三维布告板)
    vec4    _sdf;
    /// x = 描边宽度(屏幕像素,0=无描边); yzw = 描边颜色 RGB(归一化 0~1)
    /// 顶点阶段不使用,仅为与片元着色器保持 push constant 布局一致
    vec4    _stroke;
}   pc;

layout  (location = 0)  out vec2    outUV;
layout  (location = 1)  out vec4    outColor;

void main()
{
    if (pc._sdf.z > 0.5)
    {
        /// 三维布告板:世界锚点 = 小数部分 + 整数部分(抵消相机整数偏移,大坐标防抖)
        vec3    anchor  =   trans + vec3(intPart - _camera._offset.xyz);
        vec4    clip    =   _camera._offsetVp * vec4(anchor,1.0);
        /// 像素偏移 -> NDC 偏移(y 向下取反),乘 clip.w 抵消透视除法:
        /// 锚点再远字形也不缩小(屏幕像素尺寸恒定);偏移发生在屏幕平面内,
        /// 天然面向相机(布告板),无需读取相机基向量
        vec2    offset  =   inPos.xy * pc._screen.zw * 2.0 * vec2(1.0,-1.0);
        gl_Position =   vec4(clip.xy + offset * clip.w,clip.z,clip.w);
    }
    else
    {
        /// 屏幕叠加层:z = 0 且材质侧关闭深度测试:始终绘制在最前
        vec2    ndc =   inPos.xy * pc._screen.zw;
        gl_Position =   vec4(ndc.x * 2.0 - 1.0,1.0 - ndc.y * 2.0,0.0,1.0);
    }
    outUV       =   inUV;
    outColor    =   nodeColor;
}
