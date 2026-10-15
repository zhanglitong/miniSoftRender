// OIT Color Pass: 从链表读、按 depth 排序、alpha 合成
#version 450
#include    "FEShaderDefine.h"

#define UINT32_MAX 0xFFFFFFFFu
#define MAX_OIT_NODES_PER_PIXEL 256

// OIT 系统 binding（与 geometry pass 共用同一组 SSBO）
layout(set=0, binding=10) buffer OITCounterSBO
{
    uint oitCount;
    uint oitMaxCount;
    uint oitScreenW;
    uint oitScreenH;
};

// Head index buffer: 每个像素一个 uint32（链表头节点索引）
// 0xFFFFFFFF 表示空链表
layout(set=0, binding=11) buffer OITHeadIndex
{
    uint oitHeadIndex[];
};

// 链表节点池（OITNode 定义在 FEShaderDefine.h,与 geometry pass/C++ 侧布局一致）
layout(set=0, binding=12) buffer OITLinkedList
{
    OITNode oitNodes[];
};

layout(location = 0) out vec4 fragColor;

void main()
{
    uint pixelIdx = uint(gl_FragCoord.y) * oitScreenW + uint(gl_FragCoord.x);
    uint head     = oitHeadIndex[pixelIdx];

    if (head == UINT32_MAX)
    {
        discard;   // 该像素无任何透明 fragment
    }

    // 第一步：从链表提取所有节点到本地数组
    OITNode nodes[MAX_OIT_NODES_PER_PIXEL];
    uint count = 0;
    uint cur   = head;

    while (cur != UINT32_MAX && count < MAX_OIT_NODES_PER_PIXEL)
    {
        nodes[count] = oitNodes[cur];
        cur = nodes[count].next;
        count++;
    }

    if (count == 0)
    {
        discard;
    }

    // 第二步：按 depth 从小到大排序（插入排序，节点数少 → 快）
    // depth 越小越靠前（离相机越近，gl_FragCoord.z 范围 [0,1]）
    for (uint i = 1; i < count; i++)
    {
        OITNode key = nodes[i];
        int j = int(i) - 1;
        while (j >= 0 && nodes[j].depth > key.depth)
        {
            nodes[j + 1] = nodes[j];
            j--;
        }
        nodes[j + 1] = key;
    }

    // 第三步：从前往后 alpha 合成
    // front-to-back: result = result * (1 - alpha_i) + color_i * alpha_i
    vec3  accumRGB  = vec3(0.0);
    float accumAlpha = 0.0;

    for (uint i = 0; i < count; i++)
    {
        vec4  c = unpackColorHDR(uint2(nodes[i].colorRG, nodes[i].colorBA));
        accumRGB   += c.rgb * c.a * (1.0 - accumAlpha);
        accumAlpha += c.a * (1.0 - accumAlpha);
    }

    // 输出：alpha 表示该像素被透明层覆盖的程度
    fragColor = vec4(accumRGB, accumAlpha);
}
