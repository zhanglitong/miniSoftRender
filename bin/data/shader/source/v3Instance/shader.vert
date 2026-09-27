#version 450

#include "FEShaderDefine.h"

layout  (location = 0) in vec3  inPos;
/// 平移整数部分
layout  (location = 2) in ivec3 intPart;
/// 平移小数部分
layout  (location = 3) in vec3  trans;
layout  (location = 4) in vec3  scale;
/// 旋转四元数
layout  (location = 5) in vec4  rot;


layout (binding = SB_Camera) uniform CameraBlock
{
    CameraData  _camera;
};
layout(push_constant) uniform   PushConsts
{
    mat4        _mvp;
    PointData   _point;
};


void main()
{
    /// 大坐标处理:用 makeOffsetLocalMatrix(带偏移)构造 local 矩阵,
    /// 抵消相机整数偏移,减少大坐标下的浮点误差
    mat4    localOffset =   makeOffsetLocalMatrix(trans, scale, rot, intPart, _camera._offset.xyz);

    float   pointMin    =   float((_point._point >> 16) & 0xFFu);
    float   pointMax    =   float((_point._point >> 8)  & 0xFFu);

    gl_PointSize        =   pointMin;
    gl_Position         =   _camera._offsetVp * localOffset * vec4(inPos.xyz, 1.0);
}
