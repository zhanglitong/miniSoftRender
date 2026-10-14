// OIT Geometry Pass: 透明物体 Fragment Shader
// 计算颜色 + alpha，然后写入 PPLL 链表节点
// 必须加 -DOIT_MODE 编译宏才能输出链表写入代码
#version 450
#include    "FEShaderDefine.h"

#ifndef OIT_MODE
#error "This shader must be compiled with -DOIT_MODE for OIT geometry pass"
#endif

#define UINT32_MAX 0xFFFFFFFFu

// ===== OIT 系统 SSBO（set=0, binding=10/11/12）=====
// Counter 必须是 buffer 才能用 atomicAdd
layout(set=0, binding=10) buffer OITCounterSBO
{
    uint oitCount;
    uint oitMaxCount;
    uint oitScreenW;
    uint oitScreenH;
};

layout(set=0, binding=11) buffer OITHeadIndex
{
    uint oitHeadIndex[];
};

// 链表节点池（与 color pass 布局必须完全一致）
struct OITNode
{
    vec4    color;
    float   depth;
    uint    next;
};

layout(set=0, binding=12) buffer OITLinkedList
{
    OITNode oitNodes[];
};

// ===== 顶点输入（VS 输出） =====
layout  (location = 0)          in vec3     inPos;
layout  (location = 1)          in vec3     inNor;
layout  (location = 2)          in vec4     inNodeColor;
layout  (location = 3)  flat    in uint     inFlagBits;

// ===== 材质/光照 UBO =====
layout (binding = SB_Material) uniform PBRBlock
{
    PBRData        _pbr;
};

layout(std140, binding = SB_Light) readonly buffer LightsBlock
{
    LightData    _lights[];
};

layout (binding = SB_EngineState) uniform EngineStateBlock
{
    EngineState  _engineState;
};

void main()
{
    // ===== 1. 正常计算颜色 + alpha =====
    vec4    color    = vec4(0,0,0,1);
    vec3    diff     = hasNodeColor(inFlagBits) ? inNodeColor.rgb : _pbr._diffuse.rgb;
    int     numLights = _lights.length();

    for(int i = 0; i < numLights; ++i)
    {
        vec3    lDir   = vec3(_lights[i].x,_lights[i].y,_lights[i].z);
        vec3    lColor = vec3(_lights[i].r,_lights[i].g,_lights[i].b);
        float   NdotL  = max(0.1, dot(normalize(inNor), lDir));
        vec3    diffuse = diff * (NdotL) + _pbr._emissive.rgb;
        color.xyz += diffuse;
    }

    // alpha 来自材质 diffuse 的 alpha 值（用户可以设置 _diffuse.a 控制透明度）
    color.a = _pbr._diffuse.a;

    if (isSelected(inFlagBits))
    {
        vec4    hl = unpackColor(_engineState.selectColor);
        color.rgb  = mix(color.rgb, hl.rgb, 0.5);
    }

    // ===== 2. 写入 PPLL 链表 =====
    uint pixelIdx = uint(gl_FragCoord.y) * oitScreenW + uint(gl_FragCoord.x);

    // 用 atomicAdd 分配一个节点索引
    uint nodeIdx = atomicAdd(oitCount, 1);
    if (nodeIdx < oitMaxCount)   // 溢出保护
    {
        oitNodes[nodeIdx].color = color;
        oitNodes[nodeIdx].depth = gl_FragCoord.z;

        // 挂到链表头（atomicExchange 返回旧的头节点索引）
        uint oldHead = atomicExchange(oitHeadIndex[pixelIdx], nodeIdx);
        oitNodes[nodeIdx].next = oldHead;
    }

    // depth write 保持 OFF（OIT pipeline 的 depthWriteEnable=false）
    // color attachment 数量为 0（Geometry Pass 无输出）
}
