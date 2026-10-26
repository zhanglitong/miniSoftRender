#version 450

#include "FEShaderDefine.h"

layout  (location = 0)  in  vec3    inDir;
layout  (location = 0)  out vec4    outColor;

/// cubemap 统一绑定到系统槽 SB_Material(4),
/// 与 C++ 侧 FEMaterialCube::bind(0,SB_Material,...) 以及
/// WGSL 版本(shader.wgls 的 @binding(4) uCube)保持一致。
layout  (binding = SB_Material) uniform samplerCube uCube;

void main()
{
    /// 以物体空间位置为方向采样立方体贴图
    outColor =   texture(uCube, normalize(inDir));
}
