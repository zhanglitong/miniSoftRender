#version 450

#include "FEShaderDefine.h"

layout  (location = 0)  in  vec3    inPos;
layout  (location = 1)  in  vec3    inNor;
/// 平移整数部分
layout  (location = 2)  in  ivec3   intPart;
/// 平移小数部分
layout  (location = 3)  in  vec3    trans;
layout  (location = 4)  in  vec3    scale;
/// 旋转四元数
layout  (location = 5)  in  vec4    rot;

layout  (location = 6)  in  vec4    nodeColor;
layout  (location = 7)  in  uint    flagBits;

layout (binding = SB_Camera) uniform CameraBlock
{
    CameraData    _camera;
};

layout (location =  0)          out vec3    outPos;
layout (location =  1)          out vec3    outNor;
layout (location =  2)          out vec4    outColor;
layout (location =  3)  flat    out uint    outFlagBits;

void main()
{
    /// 真实世界坐标 local 矩阵:用于世界位置/法线(光照)
    mat4    matLocal    =   makeTransform(trans, scale, rot);
    /// 大坐标处理:带偏移 local 矩阵,用于裁剪位置,
    /// 抵消相机整数偏移,减少大坐标下的浮点误差
    mat4    localOffset =   makeOffsetLocalMatrix(trans, scale, rot, intPart, _camera._offset.xyz);

    gl_PointSize        =   1;
    vec4    tmp         =   matLocal * vec4(inPos.xyz, 1.0);
    outPos              =   tmp.xyz;
    outFlagBits         =   flagBits;
    outColor            =   nodeColor;
    outNor              =   normalize(mat3(matLocal) * inNor);
    gl_Position         =   _camera._offsetVp * localOffset * vec4(inPos.xyz, 1.0);
}
