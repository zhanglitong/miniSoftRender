#version 450

#include "FEShaderDefine.h"

layout  (location = 0)  in  vec3    aPos;
layout  (location = 1)  in  vec3    aNormal;
/// 平移整数部分
layout  (location = 2)  in  ivec3   intPart;
/// 平移小数部分
layout  (location = 3)  in  vec3    trans;
layout  (location = 4)  in  vec3    scale;
/// 旋转四元数
layout  (location = 5)  in  vec4    rot;
/// instance node color + flag bits (用于选中高亮)
layout  (location = 6)  in  vec4    nodeColor;
layout  (location = 7)  in  uint    flagBits;


layout (binding = SB_Camera) uniform CameraBlock
{
    CameraData  _camera;
};

layout (location = 0) out vec3 outWorld;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec4 outNodeColor;
layout (location = 3) flat out uint outFlagBits;

void main()
{
    /// 真实世界坐标 local 矩阵:用于世界位置/法线
    mat4    aModel      =   makeTransform(trans, scale, rot);
    /// 大坐标处理:带偏移 local 矩阵,用于裁剪位置
    mat4    localOffset =   makeOffsetLocalMatrix(trans, scale, rot, intPart, _camera._offset.xyz);

    vec4    world       =   aModel * vec4(aPos, 1.0);
    outNormal           =   normalize(mat3(aModel) * aNormal);
    outWorld            =   world.xyz;
    outNodeColor        =   nodeColor;
    outFlagBits         =   flagBits;
    gl_Position         =   _camera._offsetVp * localOffset * vec4(aPos, 1.0);
    ///
    gl_PointSize        =   1.0;
}
