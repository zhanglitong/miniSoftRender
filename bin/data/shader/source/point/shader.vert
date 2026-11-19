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


layout (binding = SB_Camera) uniform CameraBlock
{
    CameraData  _camera;
};

layout(push_constant) uniform   PushConsts
{
    PointData   _point;
};

layout (location = 0) out vec4  outColor;

void main()
{
    /// 大坐标处理:用 makeOffsetLocalMatrix(带偏移)构造 local 矩阵,
    /// 抵消相机整数偏移,减少大坐标下的浮点误差
    mat4    localOffset =   makeOffsetLocalMatrix(trans, scale, rot, intPart, _camera._offset.xyz);

    outColor            =   inColor;
    float   pointMin    =   float((_point._point >> 16) & 0xFFu);
    float   pointMax    =   float((_point._point >> 8)  & 0xFFu);

    vec4    clip        =   _camera._offsetVp * localOffset * vec4(inPos.xyz, 1.0);
    /// 距离衰减:sizeScale>0 时按视空间深度缩放并钳制。
    /// 不用 clip.w 是因为正交投影下 w 恒为 1,衰减会失效;视空间深度透视/正交都有效
    float   pointSize   =   pointMin;
    if (_point._sizeScale > 0.0)
    {
        /// localOffset 之后是相对相机整数偏移的坐标,视空间深度需补回相机偏移
        vec4    worldPos    =   localOffset * vec4(inPos.xyz, 1.0) + vec4(vec3(_camera._offset.xyz), 0.0);
        vec4    viewPos     =   _camera._v * worldPos;
        float   depth       =   max(-viewPos.z, 1e-4);
        pointSize           =   clamp(_point._sizeScale / depth, pointMin, pointMax);
    }
    gl_PointSize        =   pointSize;
    gl_Position         =   clip;
}
