#version 450

#include "FEShaderDefine.h"

layout  (location = 0) in vec3  inPos;
layout  (location = 1) in vec3  inNor;
layout  (location = 2) in vec4  inColor;

/// 平移整数部分
layout  (location = 3) in ivec3 intPart;
/// 平移小数部分
layout  (location = 4) in vec3  trans;
layout  (location = 5) in vec3  scale;
/// 旋转四元数
layout  (location = 6) in vec4  rot;
layout  (location = 7) in vec4  instanceColor;
layout  (location = 8) in uint  instanceFlagBits;

layout (binding = SB_Camera) uniform CameraBlock
{
    CameraData  _camera;
};

layout (location = 0)       out vec4    outColor;
layout (location = 1)       out vec3    outNor;
layout (location = 2) flat  out uint    flagBits;

vec4   instColor()
{
    return (instanceFlagBits & RF_COLOR) == 0 ? vec4(1,1,1,1) : instanceColor;
}

void main()
{
    mat4    localOffset =   makeOffsetLocalMatrix(trans, scale, rot, intPart, _camera._offset.xyz);

    /// 法线用逆转置矩阵:正确处理节点的非均匀缩放/镜像
    mat3    normalMat   =   mat3(transpose(inverse(localOffset)));

    outNor              =   normalize(normalMat * inNor);
    outColor            =   inColor * instColor();
    flagBits            =   instanceFlagBits;
    gl_Position         =   _camera._offsetVp * localOffset * vec4(inPos, 1.0);
    gl_PointSize        =   1.0;
}
