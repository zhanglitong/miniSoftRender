#version 450

#include "FEShaderDefine.h"

/// 8 字节/点:word0 = x(int16)|y(int16)<<16, word1 = z(int16)|rgb565<<16
/// 位置为相对叶子盒中心的量化整数，实例缩放负责还原为世界单位
layout  (location = 0) in uvec2 inVertex;
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

    /// 解包 RGB565 到 [0,1] 颜色
    uint    color565    =   inVertex.y >> 16;
    float   r           =   float((color565 >> 11) & 0x1Fu) / 31.0;
    float   g           =   float((color565 >> 5)  & 0x3Fu) / 63.0;
    float   b           =   float(color565 & 0x1Fu) / 31.0;
    outColor            =   vec4(r, g, b, 1.0);

    /// 解包有符号 16 位量化坐标(左移后转 int 再算术右移完成符号扩展)
    vec3    qpos        =   vec3(float(int(inVertex.x << 16u) >> 16),
                                 float(int(inVertex.x) >> 16),
                                 float(int(inVertex.y << 16u) >> 16));

    float   pointMin    =   float((_point._point >> 16) & 0xFFu);
    float   pointMax    =   float((_point._point >> 8)  & 0xFFu);

    vec4    clip        =   _camera._offsetVp * localOffset * vec4(qpos, 1.0);
    /// 距离衰减:sizeScale>0 时按视空间深度缩放并钳制。
    /// 不用 clip.w 是因为正交投影下 w 恒为 1,衰减会失效;视空间深度透视/正交都有效
    float   pointSize   =   pointMin;
    if (_point._sizeScale > 0.0)
    {
        /// localOffset 之后是相对相机整数偏移的坐标,视空间深度需补回相机偏移
        vec4    worldPos    =   localOffset * vec4(qpos, 1.0) + vec4(vec3(_camera._offset.xyz), 0.0);
        vec4    viewPos     =   _camera._v * worldPos;
        float   depth       =   max(-viewPos.z, 1e-4);
        pointSize           =   clamp(_point._sizeScale / depth, pointMin, pointMax);
    }
    gl_PointSize        =   pointSize;
    gl_Position         =   clip;
}
