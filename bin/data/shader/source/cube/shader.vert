#version 450

#include "FEShaderDefine.h"

/// cube 材质 shader:立方体贴图(cubemap)采样测试
///   顶点属性: 0=pos 1=uv,实例属性: 2~7 与 texture/pbr 约定一致
///   binding 0: 相机 UBO(SB_Camera)
///   顶点输出: 0=dir(物体空间位置,作为 cubemap 采样方向)
layout  (location = 0)  in  vec3    inPos;
/// 纹理坐标(cubemap 采样不用,保留以复用 texture 的顶点布局)
layout  (location = 1)  in  vec2    inUV;
/// 平移整数部分
layout  (location = 2)  in  ivec3   intPart;
/// 平移小数部分
layout  (location = 3)  in  vec3    trans;
layout  (location = 4)  in  vec3    scale;
/// 旋转四元数
layout  (location = 5)  in  vec4    rot;
/// 节点颜色(cube 材质当前不使用,保持与 pbr 实例布局一致)
layout  (location = 6)  in  vec4    nodeColor;
layout  (location = 7)  in  uint    flagBits;

layout (binding = SB_Camera) uniform CameraBlock
{
    CameraData    _camera;
};

/// 传给片元的立方体贴图采样方向(物体空间位置,经插值后归一化)
layout (location =  0)          out vec3    outDir;

void main()
{
    /// 大坐标处理:带偏移 local 矩阵,
    /// 抵消相机整数偏移,减少大坐标下的浮点误差
    mat4    localOffset =   makeOffsetLocalMatrix(trans, scale, rot, intPart, _camera._offset.xyz);

    gl_PointSize        =   1;
    outDir              =   inPos;
    gl_Position         =   _camera._offsetVp * localOffset * vec4(inPos.xyz, 1.0);
}
