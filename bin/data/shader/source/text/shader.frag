#version    450
#include    "FEShaderDefine.h"

/// 文本 shader:
///   binding 4(SB_Material): 字体图集(combined image sampler)
///   图集有两种编码:
///     SDF  :R 通道为距离场,0.5 为轮廓边界(大字号,无级缩放)
///     位图 :R 通道为 hinting 灰度覆盖率(小字号,原生像素)
///   通过 push constant 的 _sdf.y 选择模式(0=SDF,1=位图)
layout  (location = 0)          in  vec2    inUV;
layout  (location = 1)          in  vec4    inColor;

layout  (location = 0) out vec4 fragColor;

layout  (push_constant) uniform TextPush
{
    /// xy = 视口像素尺寸, zw = 1/视口尺寸
    vec4    _screen;
    /// x = SDF 编码增量 = 1/(2*spread);  y = 渲染模式(0=SDF,1=位图)
    vec4    _sdf;
}   pc;

/// 字体图集
layout  (binding = SB_Material) uniform sampler2D uFontAtlas;

void main()
{
    /// 每个屏幕像素在图集中覆盖的纹素数(文本四边形内恒定)
    vec2    texSize =   vec2(textureSize(uFontAtlas,0));
    float   texels  =   max(fwidth(inUV.x) * texSize.x,
                            fwidth(inUV.y) * texSize.y);
    float   r       =   texture(uFontAtlas,inUV).r;
    float   alpha;

    if (pc._sdf.y > 0.5)
    {
        /// 位图模式(hinting 灰度):R 通道直接是覆盖率,不做阈值化
        alpha   =   r;
    }
    else
    {
        /// SDF 模式 5x5 超采样:在像素中心的 5x5 邻域内取 25 个 SDF 样本,
        /// 各自阈值化后取平均,得到亚像素级覆盖率(比单样本更精确的抗锯齿)
        float   w       =   0.5 * max(fwidth(r),texels * pc._sdf.x);
        vec2    step    =   vec2(fwidth(inUV.x),fwidth(inUV.y)) / 5.0;
        alpha           =   0.0;
        for (int y = -1; y <= 1; ++y)
            for (int x = -1; x <= 1; ++x)
            {
                float   d   =   texture(uFontAtlas,inUV + vec2(float(x) * step.x,
                                                                float(y) * step.y)).r;
                alpha   +=  smoothstep(0.5 - w,0.5 + w,d);
            }
        alpha  /=   9.0;
        /// 强化有覆盖率的区域:gamma < 1 把中低 alpha 拉高,
        /// 让字形内部与笔画中段更实,避免超采样平均后发虚
        alpha   =   pow(alpha,0.5);
    }

    fragColor   =   vec4(inColor.rgb,inColor.a * alpha);
}
