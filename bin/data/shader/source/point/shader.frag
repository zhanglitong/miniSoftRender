// ============================================
// 需要的特性
// ============================================
requires immediate_address_space;
 // 如果需要点精灵
requires points; 

// ============================================
// 结构体定义
// ============================================

// 顶点输入
struct VertexInput {
    @location(0) inPos: vec3f,
    @location(1) inColor: vec4f,
    @location(2) matLocalC0: vec4f,
    @location(3) matLocalC1: vec4f,
    @location(4) matLocalC2: vec4f,
    @location(5) matLocalC3: vec4f,
};

// 顶点输出 / 片元输入
struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) outColor: vec4f,
    @builtin(point_size) pointSize: f32,   // 顶点输出专用
    @builtin(point_coord) pointCoord: vec2f, // 片元输入专用（需要 points 特性）
};

// 片元输出
struct FragmentOutput {
    @location(0) fragColor: vec4f,
};

// Uniform Buffer
struct CameraData {
    _p: mat4x4f,
    _v: mat4x4f,
};

// Push Constants
struct PushConsts {
    _point: u32,
    _color: u32,
};

// ============================================
// 绑定资源
// ============================================

@group(0) @binding(0) var<uniform> _camera: CameraData;
var<immediate> _point: PushConsts;

// ============================================
// 工具函数
// ============================================

// 解包 8 位归一化颜色
fn unpackUnorm4x8(value: u32) -> vec4f {
    return vec4f(
        f32(value & 0xFFu) / 255.0,
        f32((value >> 8) & 0xFFu) / 255.0,
        f32((value >> 16) & 0xFFu) / 255.0,
        f32((value >> 24) & 0xFFu) / 255.0
    );
}

// ============================================
// 顶点着色器
// ============================================

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    // 构建局部矩阵
    let matLocal = mat4x4f(
        in.matLocalC0,
        in.matLocalC1,
        in.matLocalC2,
        in.matLocalC3
    );
    
    // 解包点数据
    let pointValue = _point._point;
    let pointMin = f32((pointValue >> 16) & 0xFFu);
    let pointMax = f32((pointValue >> 8) & 0xFFu);
    let overrideVal = f32((pointValue >> 24) & 0xFFu);
    
    // 输出
    var out: VertexOutput;
    out.outColor = in.inColor;
    out.pointSize = pointMin;  // 设置点大小
    out.position = _camera._p * _camera._v * matLocal * vec4f(in.inPos, 1.0);
    
    return out;
}

// ============================================
// 片元着色器
// ============================================

@fragment
fn fs_main(in: VertexOutput) -> FragmentOutput 
{
    // 解包颜色
    let color = unpackUnorm4x8(_point._color);
    
    // 解包点数据
    let pointValue = _point._point;
    let overrideVal = f32((pointValue >> 24) & 0xFFu);
    
    // 点精灵圆形裁剪
    let coord = in.pointCoord - vec2f(0.5, 0.5);
    let dist = length(coord);
    if (dist > 0.5) {
        discard;
    }
    
    // 计算最终颜色
    var out: FragmentOutput;
    out.fragColor = in.outColor * (1.0 - overrideVal) + color * color;
    
    return out;
}