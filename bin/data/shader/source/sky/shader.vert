#version 450

#include "FEShaderDefine.h"

/// 天空盒(大气散射)顶点着色器:
///   立方体壳的局部坐标即"相机相对方向",天空盒始终以相机为中心。
///   做法:只取视图矩阵的旋转部分(把平移列清零),使盒体中心恒等于相机,
///   从而 inPos 直接就是从相机出发的世界方向,可直接作为采样方向传给片元;
///   天空保持世界朝向,相机平移/旋转时天空相对世界不动。
///   裁剪空间 z 强制到远平面(z=w),使天空永远最深处:
///   配合 depthCompareOp=LESS_OR_EQUAL,有几何处被遮挡,无几何处显示天空。
///   顶点属性: 0=pos 1=uv,实例属性: 2~7 与 texture/pbr 约定一致(天空盒不使用变换)
layout  (location = 0)  in  vec3    inPos;
layout  (location = 1)  in  vec2    inUV;
layout  (location = 2)  in  ivec3   intPart;
layout  (location = 3)  in  vec3    trans;
layout  (location = 4)  in  vec3    scale;
layout  (location = 5)  in  vec4    rot;
layout  (location = 6)  in  vec4    nodeColor;
layout  (location = 7)  in  uint    flagBits;

layout (binding = SB_Camera) uniform CameraBlock
{
    CameraData    _camera;
};

/// 传给片元的天空采样方向(相机相对,盒体归一化坐标)
layout (location =  0)          out vec3    outDir;

void main()
{
    gl_PointSize        =   1;
    outDir              =   inPos;
    /// 相机中心化:只取视图矩阵的旋转部分(平移列清零),盒体中心恒等于相机位置
    mat4    vRot        =   _camera._v;
    vRot[3]             =   vec4(0.0, 0.0, 0.0, 1.0);
    gl_Position         =   _camera._p * vRot * vec4(inPos.xyz, 1.0);
    /// 强制到远平面:z=w → NDC 深度=1(最大),天空不遮挡任何几何
    gl_Position.z       =   gl_Position.w;
}
