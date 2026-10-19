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
    /// x = SDF 编码增量 = 1/(2*spread);  y = 渲染模式(0=SDF,1=位图);
    /// z = 绘制模式(0=屏幕叠加,1=三维布告板)
    vec4    _sdf;
    /// x = 描边宽度(屏幕像素,0=无描边); yzw = 描边颜色 RGB(归一化 0~1)
    vec4    _stroke;
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
    vec3    color   =   inColor.rgb;

    if (pc._sdf.y > 0.5)
    {
        /// 位图模式(hinting 灰度):R 通道直接是覆盖率,不做阈值化,
        /// 描边在位图模式下为 no-op(距离场算法不适用)
        alpha   =   r;
    }
    else
    {
        /// SDF 模式 3x3 超采样:在像素中心的 3x3 邻域内取 9 个 SDF 样本,
        /// 各自阈值化后取平均,得到亚像素级覆盖率。
        /// 同时计算填充与描边两条覆盖率曲线:
        ///   fill  :SDF > 0.5(字形内部)
        ///   stroke:SDF > 0.5 - strokeWidth*sdfPerPixel(字形外侧描边带)
        float   sdfPerPx    =   texels * pc._sdf.x;            /// 每屏幕像素的 SDF 变化量
        float   strokeEdge  =   0.5 - pc._stroke.x * sdfPerPx;  /// 描边外缘对应的 SDF 值
        float   w           =   0.5 * max(fwidth(r),sdfPerPx); /// 抗锯齿半宽(SDF 单位)
        vec2    step        =   vec2(fwidth(inUV.x),fwidth(inUV.y)) / 5.0;
        float   fill        =   0.0;
        float   stroke      =   0.0;
        for (int y = -1; y <= 1; ++y)
            for (int x = -1; x <= 1; ++x)
            {
                float   d   =   texture(uFontAtlas,inUV + vec2(float(x) * step.x,
                                                                float(y) * step.y)).r;
                fill    +=  smoothstep(0.5 - w,0.5 + w,d);
                stroke  +=  smoothstep(strokeEdge - w,strokeEdge + w,d);
            }
        fill   /=   9.0;
        stroke /=   9.0;

        /// 最终覆盖率 = max(填充,描边):字形内部由填充覆盖,
        /// 外侧描边带由描边覆盖,两者叠加形成带描边的字形
        alpha           =   max(fill,stroke);
        /// 颜色在描边色与填充色之间按填充占比插值:
        ///   纯填充区 fill=1 -> 填充色;纯描边区 fill=0 -> 描边色;
        ///   边界处平滑过渡。strokeWidth=0 时 strokeEdge=0.5,stroke==fill,
        ///   color 恒为填充色,与未开启描边完全一致。
        float   t       =   fill / max(alpha,1e-5);
        color           =   mix(pc._stroke.yzw,inColor.rgb,t);

        /// 强化有覆盖率的区域:gamma < 1 把中低 alpha 拉高,
        /// 让字形内部与笔画中段更实,避免超采样平均后发虚
        alpha           =   pow(alpha,0.5);
    }

    fragColor   =   vec4(color,inColor.a * alpha);
}
