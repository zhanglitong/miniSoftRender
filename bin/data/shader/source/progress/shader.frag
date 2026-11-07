// 进度条: 圆角背景 + 圆角填充 + 圆角边框
// vPx 为矩形内像素坐标(左上角为 0),片元用圆角矩形 SDF 计算形状,
// 两端半径取高度一半 => 胶囊形;无需纹理,也不依赖任何描述符集。
#version 450

layout(push_constant) uniform ProgressPush
{
    vec4    _rect;
    vec4    _background;
    vec4    _fill;
    vec4    _border;
    float   _progress;
    float   _borderWidth;
    vec2    _size;
} pc;

layout(location = 0) in  vec2 vPx;
layout(location = 0) out vec4 outColor;

/// 圆角矩形有符号距离: p 为相对中心的坐标, r 为圆角半径
float   roundedBox(vec2 p,vec2 halfSize,float r)
{
    vec2    q   =   abs(p) - halfSize + vec2(r);
    return  length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - r;
}

void main()
{
    /// 边框透明时不占位,填充直接铺满外轮廓
    float   b       =   (pc._border.a > 0.0) ? max(pc._borderWidth, 0.0) : 0.0;
    vec2    halfSz  =   pc._size * 0.5;
    /// 两端圆角:半径 = 高度一半
    float   r       =   halfSz.y;

    /// 外轮廓 / 内轮廓覆盖率(1px 抗锯齿)
    float   d       =   roundedBox(vPx - halfSz, halfSz, r);
    float   outerA  =   clamp(0.5 - d, 0.0, 1.0);
    float   innerA  =   clamp(0.5 - d - b, 0.0, 1.0);
    float   borderA =   clamp(outerA - innerA, 0.0, 1.0);

    /// 内区填充(胶囊形,左右两端均圆)
    vec3    bodyRgb =   pc._background.rgb;
    float   bodyA   =   pc._background.a * innerA;
    float   innerW  =   max(pc._size.x - 2.0 * b, 1.0);
    float   innerH  =   max(pc._size.y - 2.0 * b, 1.0);
    float   fillW   =   innerW * clamp(pc._progress, 0.0, 1.0);
    if (fillW > 0.0)
    {
        vec2    fc      =   vec2(b + fillW * 0.5, pc._size.y * 0.5);
        vec2    fh      =   vec2(fillW * 0.5, innerH * 0.5);
        float   fr      =   min(fh.x, fh.y);
        float   fd      =   roundedBox(vPx - fc, fh, fr);
        float   fillA   =   clamp(0.5 - fd, 0.0, 1.0);

        bodyRgb =   mix(bodyRgb, pc._fill.rgb, fillA);
        bodyA   =   mix(bodyA, pc._fill.a * innerA, fillA);
    }

    /// 边框叠加到填充之上
    float   outA    =   borderA + bodyA * (1.0 - borderA);
    vec3    outRgb  =   (bodyRgb * bodyA * (1.0 - borderA) + pc._border.rgb * borderA)
                    /   max(outA, 1e-5);
    outColor    =   vec4(outRgb, outA);
}
