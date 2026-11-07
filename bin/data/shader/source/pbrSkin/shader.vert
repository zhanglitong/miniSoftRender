#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "FEShaderDefine.h"

layout(location = 0) in vec3  inPos;
layout(location = 1) in vec3  inNor;
layout(location = 8) in vec2  inUV;
layout(location = 9) in vec2  inUV1;
layout(location = 2) in ivec3 intPart;
layout(location = 3) in vec3  trans;
layout(location = 4) in vec3  scale;
layout(location = 5) in vec4  rot;
layout(location = 6) in vec4  nodeColor;
layout(location = 7) in uint  flagBits;

/// glTF JOINTS_0 / WEIGHTS_0(每顶点 4 骨骼影响)
layout(location = 10) in uvec4 inBoneIndex;
layout(location = 11) in vec4  inBoneWeight;

layout(set = 0, binding = 0) uniform CameraBlock {
    CameraData _camera;
};

/// 蒙皮矩阵(由 skinCompute 的 compute shader 每帧计算):
///   _bones[i] = inverse(meshGlobal) * worldRotation * jointGlobal * inverseBindMatrix
/// 计算好的矩阵直接写入该 storage buffer,由渲染工厂在绘制前绑定到该槽位
layout(set = 0, binding = 18) readonly buffer SkinBlock {
    mat4 _bones[];
};

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outNor;
layout(location = 2) out vec2 outUV;
layout(location = 3) out vec4 outColor;
layout(location = 4) flat out uint outFlagBits;
/// 第二套 UV(glTF TEXCOORD_1,AO 专用 UV 集等;网格未提供时引擎不绑定,读 0)
layout(location = 5) out vec2 outUV1;

vec3 qtransform(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

/// 线性混合蒙皮(LBS);权重和为 0 时退回绑定姿态
void skinVertex(inout vec3 pos,inout vec3 nor)
{
    float wsum  =   inBoneWeight.x + inBoneWeight.y + inBoneWeight.z + inBoneWeight.w;
    if (wsum <= 0.0)
        return;

    float   invW    =   1.0 / wsum;
    vec4    skinnedPos  =   vec4(0.0);
    vec3    skinnedNor  =   vec3(0.0);
    for (int i = 0; i < 4; ++i)
    {
        float   w   =   inBoneWeight[i] * invW;
        if (w <= 0.0)
            continue;
        mat4    m   =   _bones[inBoneIndex[i]];
        skinnedPos  +=  m * vec4(pos, 1.0) * w;
        skinnedNor  +=  mat3(m) * nor * w;
    }
    pos =   skinnedPos.xyz;
    nor =   skinnedNor;
}

void main()
{
    vec3    localPos    =   inPos;
    vec3    localNor    =   inNor;
    skinVertex(localPos, localNor);

    /// 实例变换:使用量化整数坐标,避免 float 精度丢失
    vec3 tPos        =   qtransform(rot, localPos * scale) + trans + intPart;
    mat3 matLocalN   =   mat3(qtransform(rot, vec3(1.0, 0.0, 0.0)),
                              qtransform(rot, vec3(0.0, 1.0, 0.0)),
                              qtransform(rot, vec3(0.0, 0.0, 1.0)));
    vec3 worldNormal =   matLocalN * localNor;

    /// 世界坐标(带大坐标量化偏移):用于 PBR 光照计算(V/L 方向)
    outPos           =   tPos;
    outNor           =   worldNormal;
    outUV            =   inUV;
    outUV1           =   inUV1;

    /// 节点/实例颜色:透传实例颜色属性,由片元着色器按 hasNodeColor 决定是否参与
    outColor         =   nodeColor;
    outFlagBits      =   flagBits;

    /// 点图元变体(using_defult 会为该管线生成 POINT_LIST):显式写 gl_PointSize,
    /// 否则 Vulkan 校验层报 "PointSize is not written but topology is POINT_LIST"
    gl_PointSize     =   1.0;

    /// 投影:使用 VP 矩阵
    gl_Position      =   _camera._vp * vec4(tPos, 1.0);
}
