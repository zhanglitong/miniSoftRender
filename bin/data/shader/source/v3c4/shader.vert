#version 450

#include "FEShaderDefine.h"

layout  (location = 0) in vec3  inPos;
layout  (location = 1) in vec4  inColor;

/// 平移整数部分
layout  (location = 2) in ivec3 intPart;
/// 平移小数部分
layout  (location = 3) in vec3  trans;
layout  (location = 4) in vec3  scale;
/// 旋转四元数
layout  (location = 5) in vec4  rot;
layout  (location = 6) in vec4  instanceColor;
layout  (location = 7) in uint  instanceFlagBits;


layout (binding = SB_Camera) uniform CameraBlock
{
    CameraData  _camera;
};

layout (location = 0)       out vec4    outColor;
layout (location = 1) flat  out uint    flagBits;

vec4   instColor()
{
    return (instanceFlagBits & RF_COLOR) == 0 ? vec4(1,1,1,1) : instanceColor;
}

void main()
{
    /// 大坐标处理:用 makeOffsetLocalMatrix(带偏移)构造 local 矩阵,
    /// 抵消相机整数偏移,减少大坐标下的浮点误差
    mat4    localOffset =   makeOffsetLocalMatrix(trans, scale, rot, intPart, _camera._offset.xyz);

    outColor            =   inColor  * instColor() ;
    flagBits            =   instanceFlagBits;
    gl_Position         =   _camera._offsetVp * localOffset * vec4(inPos.xyz, 1.0);
    ///
    gl_PointSize        =   1.0;
}
