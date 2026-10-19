#version 450

#include "FEShaderDefine.h"

layout  (location = 0)  in  vec2    inUV;
layout  (location = 0)  out vec4    outColor;

/// combined image sampler 统一绑定到系统槽 SB_Material(4),
/// 与 C++ 侧 FEMaterialTexture::bind(0,SB_Material,...) 以及
/// WGSL 版本(shader.wgls 的 @binding(4) uTex)保持一致。
/// (早期此处误写为 binding=1,导致 C++ 绑到 4 而 shader 读 1,
///  descriptor 从未被 vkUpdateDescriptorSets 写入)
layout  (binding = SB_Material) uniform sampler2D uTex;

void main()
{
    outColor =   texture(uTex, inUV);
}
