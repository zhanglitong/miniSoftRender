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

// Head/层存储:每个像素一个 uint32（链表头）或 {lock, count}（K 槽模式）
layout(set=0, binding=11) buffer OITHeadIndex
{
#if OIT_SLOT_K > 0
    /// K 槽模式:每像素一个 uint —— 本帧预订到的层数
    uint oitPixelCount[];
#else
    uint oitHeadIndex[];
#endif
};

// 透明层存储（OITNode/OITSlot 定义在 FEShaderDefine.h,与 geometry pass/C++ 侧布局一致）
layout(set=0, binding=12) buffer OITLinkedList
{
#if OIT_SLOT_K > 0
    OITSlot oitSlots[];
#else
    OITNode oitNodes[];
#endif
};

layout(location = 0) out vec4 fragColor;

void main()
{
    uint pixelIdx = uint(gl_FragCoord.y) * oitScreenW + uint(gl_FragCoord.x);

#if OIT_SLOT_K > 0
    /// K 槽模式:几何趟按预订顺序落槽,槽内无序,这里统一按深度排序。
    /// 预订层数可能超过 K,只有前 K 层真正落槽
    uint count = min(oitPixelCount[pixelIdx], OIT_SLOT_K);
    if (count == 0u)
    {
        discard;   // 该像素无任何透明 fragment
    }

    uint    base = pixelIdx * OIT_SLOT_K;
    OITSlot slots[OIT_SLOT_K];
    for (uint i = 0; i < count; ++i)
    {
        slots[i] = oitSlots[base + i];
    }
    /// 插入排序:最多 8 个元素,比 geometry 趟做有序插入便宜
    for (uint i = 1; i < count; ++i)
    {
        OITSlot key = slots[i];
        int     j   = int(i) - 1;
        while (j >= 0 && slots[j].depth > key.depth)
        {
            slots[j + 1] = slots[j];
            j--;
        }
        slots[j + 1] = key;
    }

    vec3  accumRGB  = vec3(0.0);
    float accumAlpha = 0.0;

    /// 几何趟(pbr OIT 变体)写入的是预乘色 (rgb*a, a),
    /// 合成只按剩余透射率加权,不能再乘一次 alpha
    for (uint i = 0; i < count; ++i)
    {
        vec4  c = unpackColorHDR(uint2(slots[i].colorRG, slots[i].colorBA));
        accumRGB   += c.rgb * (1.0 - accumAlpha);
        accumAlpha += c.a   * (1.0 - accumAlpha);
    }

    fragColor = vec4(accumRGB, accumAlpha);
#else
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
        accumRGB   += c.rgb * (1.0 - accumAlpha);
        accumAlpha += c.a   * (1.0 - accumAlpha);
    }

    // 输出：alpha 表示该像素被透明层覆盖的程度
    fragColor = vec4(accumRGB, accumAlpha);
#endif
}
